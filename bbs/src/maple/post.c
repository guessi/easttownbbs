/*-------------------------------------------------------*/
/* post.c       ( NTHU CS MapleBBS Ver 2.39 )            */
/*-------------------------------------------------------*/
/* target : bulletin boards' routines                    */
/* create : 95/03/29                                     */
/* update : 96/04/05                                     */
/*-------------------------------------------------------*/


#include "bbs.h"


extern BCACHE *bshm;
extern XZ xz[];


extern int wordsnum;    /* itoc.010408: 計算文章字數 */
extern int TagNum;
extern char xo_pool[];
extern char brd_bits[];


#ifdef HAVE_ANONYMOUS
extern char anonymousid[];  /* itoc.010717: 自定匿名 ID */
#endif


int
cmpchrono(hdr)
  HDR *hdr;
{
  return hdr->chrono == currchrono;
}

static void
change_stamp(folder, hdr)
  char *folder;
  HDR *hdr;
{
  HDR buf;

  /* 為了確定新造出來的 stamp 也是 unique (不和既有的 chrono 重覆)，
  就產生一個新的檔案，該檔案隨便 link 即可。
  這個多產生出來的垃圾會在 expire 被 sync 掉 (因為不在 .DIR 中) */
  hdr_stamp(folder, HDR_LINK | 'A', &buf, "etc/stamp");
  hdr->stamp = buf.chrono;
}

/* ----------------------------------------------------- */
/* 改良 innbbsd 轉出信件、連線砍信之處理程序             */
/* ----------------------------------------------------- */


void
btime_update(bno)
  int bno;
{
  if (bno >= 0)
    (bshm->bcache + bno)->btime = -1;  /* 讓 class_item() 更新用 */
}


#ifndef HAVE_NETTOOL
static       /* 給 enews.c 用 */
#endif
void
outgo_post(hdr, board)
  HDR *hdr;
  char *board;
{
  bntp_t bntp;

  memset(&bntp, 0, sizeof(bntp_t));

  if (board)    /* 新信 */
  {
    bntp.chrono = hdr->chrono;
  }
  else      /* cancel */
  {
    bntp.chrono = -1;
    board = currboard;
  }
  strcpy(bntp.board, board);
  strcpy(bntp.xname, hdr->xname);
  strcpy(bntp.owner, hdr->owner);
  strcpy(bntp.nick, hdr->nick);
  strcpy(bntp.title, hdr->title);
  rec_add("innd/out.bntp", &bntp, sizeof(bntp_t));
}


void
cancel_post(hdr)
  HDR *hdr;
{
  if ((hdr->xmode & POST_OUTGO) &&           /* 外轉信件     */
      (hdr->chrono > ap_start - 7 * 86400))  /* 7 天之內有效 */
  {
    outgo_post(hdr, NULL);
  }
}


static inline int
move_post(hdr, folder, by_bm)  /* 將 hdr 從 folder 搬到別的板 */
  HDR *hdr;
  char *folder;
  int by_bm;
{
  FILE *fp;
  HDR post;
  int xmode;
  char fpath[64], fnew[64], ffpath[64], *board;
  struct stat st;
  time_t now;
  struct tm *ptime;

  xmode = hdr->xmode;
  hdr_fpath(fpath, folder, hdr);

  if (!(xmode & POST_BOTTOM)) /* 置底文被砍不用 move_post */
  {
#ifdef HAVE_REFUSEMARK
    board = by_bm && !(xmode & POST_RESTRICT) ? BN_DELETED : BN_JUNK;  /* 加密文章丟去 junk */
#else
    board = by_bm ? BN_DELETED : BN_JUNK;
#endif

    if ((bshm->bcache + currbno)->readlevel != PERM_SYSOP &&
        (bshm->bcache + currbno)->readlevel != PERM_ALLBOARD)
    {
      brd_fpath(fnew, board, fn_dir);
      hdr_stamp(fnew, HDR_LINK | 'A', &post, fpath);


      /* guessi.120603 內文記載文章刪除記錄 */
      hdr_fpath(ffpath, (char *) fnew, &post);

      if (fp = fopen(ffpath, "a"))
      {
        time(&now);
        ptime = localtime(&now);

        if (!by_bm && !str_cmp(cuser.userid, hdr->owner))
        {
          fprintf(fp, "\033[1;30m※ 文章由作者自行刪除%-*s%02d/%02d %02d:%02d\033[m\n",
                      46, "", ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min);
        }
        else
        {
          fprintf(fp, "\033[1;30m※ 文章由管理員[%s]刪除%-*s%02d/%02d %02d:%02d\033[m\n",
                      cuser.userid, 46 - strlen(cuser.userid), "",
                      ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min);
        }

        fclose(fp);
      }

      /* 直接複製 trailing data：owner(含)以下所有欄位 */
      memcpy(post.owner, hdr->owner, sizeof(HDR) -
      (sizeof(post.chrono) + sizeof(post.xmode) + sizeof(post.xid) + sizeof(post.xname)));

      if (!by_bm && !stat(fpath, &st))
        by_bm = st.st_size;

      rec_bot(fnew, &post, sizeof(HDR));
      btime_update(brd_bno(board));
    }
    else
    {
      /* 秘密看板刪文不送至delete/junk */
      if (!stat(fpath, &st))
        by_bm = st.st_size;
    }
  }

  unlink(fpath);
  btime_update(currbno);
  cancel_post(hdr);

  return by_bm;
}

#ifdef HAVE_DETECT_CROSSPOST
/* ----------------------------------------------------- */
/* 改良 cross post 停權                                  */
/* ----------------------------------------------------- */

#define MAX_CHECKSUM_POST  20  /* 記錄最近 20 篇文章的 checksum */
#define MAX_CHECKSUM_LINE  6  /* 只取文章前 6 行來算 checksum */

typedef struct
{
  int sum;      /* 文章的 checksum */
  int total;      /* 此文章已發表幾篇 */
} CHECKSUM;

static CHECKSUM checksum[MAX_CHECKSUM_POST];
static int checknum = 0;

static inline int
checksum_add(str)    /* 回傳本列文字的 checksum */
  char *str;
{
  int sum, i, len;

  len = strlen(str);

  sum = len;    /* 當字數太少時，前四分之一很可能完全相同，所以將字數也加入 sum 值 */

  for (i = len >> 2; i > 0; i--)        /* 只算前四分之一字元的 sum 值 */
    sum += *str++;

  return sum;
}


static inline int    /* 1:是cross-post 0:不是cross-post */
checksum_put(sum)
  int sum;
{
  int i;

  if (sum)
  {
    for (i = 0; i < MAX_CHECKSUM_POST; i++)
    {
      if (checksum[i].sum == sum)
      {
        checksum[i].total++;

        if (checksum[i].total > MAX_CROSS_POST)
          return 1;
        return 0;  /* total <= MAX_CROSS_POST */
      }
    }

    if (++checknum >= MAX_CHECKSUM_POST)
      checknum = 0;

    checksum[checknum].sum = sum;
    checksum[checknum].total = 1;
  }
  return 0;
}

static inline void
checksum_rmv(sum)
  int sum;
{
  int i;

  if (sum)
  {
    for (i = 0; i < MAX_CHECKSUM_POST; i++)
    {
      if (checksum[i].sum == sum)
      {
        if (checksum[i].total > 0)
          checksum[i].total--;
        break;
      }
    }
  }
}

static int      /* return checksum */
checksum_count(fpath)
  char *fpath;
{
  int i, sum;
  char buf[ANSILINELEN];
  FILE *fp;

  sum = 0;
  if (fp = fopen(fpath, "r"))
  {
    for (i = -(LINE_HEADER + 1);;)  /* 前四列是檔頭 */
    {
      if (!fgets(buf, ANSILINELEN, fp))
        break;

      if (i < 0)  /* 跳過檔頭 */
      {
        i++;
        continue;
      }

      if (*buf == QUOTE_CHAR1 || *buf == '\n' || !strncmp(buf, "※", 2))   /* 跳過引言 */
        continue;

      sum += checksum_add(buf);

      if (++i >= MAX_CHECKSUM_LINE)
        break;
    }

    fclose(fp);
  }
  return sum;
}


static int
check_crosspost(fpath, bno)
  char *fpath;
  int bno;      /* 要轉去的看板 */
{
  int sum;
  char *blist, folder[64];
  ACCT acct;
  HDR hdr;

  if (HAS_PERM(PERM_ALLADMIN))
  return 0;

  /* 板主在自己管理的看板不列入跨貼檢查 */
  blist = (bshm->bcache + bno)->BM;

  if (HAS_PERM(PERM_BM) && blist[0] > ' ' && is_bm(blist, cuser.userid))
    return 0;

  sum = checksum_count(fpath);
  if (checksum_put(sum))
  {
    brd_fpath(folder, BN_CPRECORD, fn_dir);
    hdr_stamp(folder, HDR_COPY | 'A', &hdr, fpath);
    strcpy(hdr.owner, cuser.userid);
    strcpy(hdr.nick, cuser.username);
    sprintf(hdr.title, "[%s Cross-Post]", Now());
    rec_bot(folder, &hdr, sizeof(HDR));
    btime_update(brd_bno(BN_CPRECORD));

    bbstate &= ~STAT_POST;
    cuser.userlevel &= ~PERM_POST;
    cuser.userlevel |= PERM_DENYPOST;
    if (acct_load(&acct, cuser.userid) >= 0)
    {
      acct.tvalid = time(NULL) + CROSSPOST_DENY_DAY * 86400;
      acct_setperm(&acct, PERM_DENYPOST, PERM_POST);
    }
    board_main();
    mail_self(FN_ETC_CROSSPOST, str_sysop, "Cross-Post 停權", 0);
    vmsg("您因為過度跨貼而被停權");
    return 1;
  }
  return 0;
}
#endif  /* HAVE_DETECT_CROSSPOST */


/* ----------------------------------------------------- */
/* 發表、回應、編輯、轉錄文章                            */
/* ----------------------------------------------------- */


#ifdef HAVE_ANONYMOUS
static void
log_anonymous(fname)
  char *fname;
{
  char buf[512];

  sprintf(buf, "%s %-13s(%s)\n%-13s %s %s\n",
  Now(), cuser.userid, fromhost, currboard, fname, ve_title);
  f_cat(FN_RUN_ANONYMOUS, buf);
}
#endif


#ifdef HAVE_UNANONYMOUS_BOARD
static void
do_unanonymous(fpath)
  char *fpath;
{
  HDR hdr;
  char folder[64];

  brd_fpath(folder, BN_UNANONYMOUS, fn_dir);
  hdr_stamp(folder, HDR_LINK | 'A', &hdr, fpath);

  strcpy(hdr.owner, cuser.userid);
  strcpy(hdr.title, ve_title);

  rec_bot(folder, &hdr, sizeof(HDR));
  btime_update(brd_bno(BN_UNANONYMOUS));
}
#endif

static void
do_allpost(fpath, owner, nick, mode, chrono, bname)
  char *fpath;
  char *owner;
  char *nick;
  int mode;
  time_t chrono;
  char *bname;
{
  HDR hdr;
  char folder[64];
  char *brdname = BN_ALLPOST;       // 板名自定

  brd_fpath(folder, brdname, fn_dir);
  hdr_stamp(folder, HDR_LINK | 'A', &hdr, fpath);

  hdr.xmode = mode & ~POST_OUTGO;  /* 拿掉 POST_OUTGO */
  hdr.xid = chrono;
  strcpy(hdr.owner, owner);
  strcpy(hdr.nick, nick);
  if (!strncmp(ve_title, "Re:", 3)) /* 開頭為 Re: */
    sprintf(hdr.title, "%-*.*s .%s", 47 - strlen(bname), 47 - strlen(bname), ve_title, bname);
  else
    sprintf(hdr.title, "%-*.*s .%s", 43 - strlen(bname), 43 - strlen(bname), ve_title, bname);

  rec_bot(folder, &hdr, sizeof(HDR));

  btime_update(brd_bno(brdname));
}

/* 發文到看板 */
void
add_post(brdname, fpath, title)
  char *brdname;        /* 欲 post 的看板 */
  char *fpath;          /* 檔案路徑 */
  char *title;          /* 文章標題 */
{
  HDR hdr;
  char folder[64];

  brd_fpath(folder, brdname, fn_dir);
  hdr_stamp(folder, HDR_LINK | 'A', &hdr, fpath);
  strcpy(hdr.owner, cuser.userid);
  strcpy(hdr.nick, cuser.username);
  strcpy(hdr.title, title);
  rec_bot(folder, &hdr, sizeof(HDR));
  btime_update(brd_bno(brdname));
}

static int
find_class(folder, brdname, rc)
  char *folder;
  char *brdname;
  int rc;
{
  int xmode;
  char fpath[64];
  HDR hdr;
  FILE *fp;

  if (rc)           /* 已經找到 */
    return rc;

  if (fp = fopen(folder, "r"))
  {
    while (fread(&hdr, sizeof(HDR), 1, fp) == 1)
    {
      xmode = hdr.xmode & (GEM_BOARD | GEM_FOLDER);

      if (xmode == (GEM_BOARD | GEM_FOLDER))      /* 看板精華區捷徑 */
      {
        if (!strcmp(hdr.xname, brdname))
        {
          rc = 1;
          break;
        }
      }
      else if (xmode == GEM_FOLDER)               /* 卷宗 recursive 進去找 */
      {
        hdr_fpath(fpath, folder, &hdr);
        find_class(fpath, brdname, rc);
      }
    }
    fclose(fp);
  }
  return rc;
}

static int
depart_post(xo)
  XO *xo;
{
  char folder[64], fpath[64];
  HDR post, *hdr;
  BRD *brdp, *bend;
  int pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

  if ((hdr->xmode & POST_BOTTOM) ||    /* 不可轉錄置底文 跳過 */
      !HAS_PERM(PERM_ALLBOARD) ||      /* 若不是站務人員 跳過 */
      strcmp(currboard, BN_DEPARTALL)) /* 若不是指定看板 跳過 */
  return XO_NONE;

  if (hdr->xmode & POST_DIGEST)
  {
    vmsg("此文章已經轉錄過囉，若要重新轉錄請先解除標記");
    return XO_FOOT;
  }
  else if (vans("請確認轉錄至各系所看板？ Y)確定 N)取消 [N] ") != 'y')
  {
    return XO_FOOT;
  }

  hdr_fpath(fpath, xo->dir, hdr);

  hdr->xmode ^= POST_DIGEST; /* 給予自訂標記 供post_attr()顯示 */
  currchrono = hdr->chrono;
  rec_put(xo->dir, hdr, sizeof(HDR), pos, cmpchrono);

  brdp = bshm->bcache;
  bend = brdp + bshm->number;

  while (brdp < bend)
  {
    /* 轉錄至指定分類 但不包含本身 */
    if (find_class("gem/@/@DepartList", brdp->brdname, 0) && strcmp(brdp->brdname, BN_DEPARTALL))
    {
      brd_fpath(folder, brdp->brdname, fn_dir);
      hdr_stamp(folder, HDR_COPY | 'A', &post, fpath);
      strcpy(post.owner, hdr->owner);
      sprintf(post.title, "[轉錄] %s", hdr->title); /* 加註轉錄字樣 */
      rec_bot(folder, &post, sizeof(HDR));
      btime_update(brd_bno(brdp->brdname)); /* 看板閱讀紀錄(綠點)更新 */
    }
    brdp++;
  }

  vmsg("轉錄完成！");

  return XO_INIT; /* 刷新頁面 */
}

#define NUM_PREFIX 6

static int
do_post(xo, title)
  XO *xo;
  char *title;
{
  /* Thor.981105: 進入前需設好 curredit */
  HDR hdr;
  char fpath[64], *folder, *nick, *rcpt;
  int mode;
  time_t spendtime;
  BRD *brd;

  if (!(bbstate & STAT_POST))
  {
#ifdef NEWUSER_LIMIT
    if (cuser.lastlogin - cuser.firstlogin < 3 * 86400)
      vmsg("新手上路，三日後始可張貼文章");
    else
#endif
      vmsg("對不起，您沒有在此發表文章的權限");
    return XO_FOOT;
  }

  brd_fpath(fpath, currboard, FN_POSTLAW);

  if (more(fpath, (char *) -1) < 0)
    film_out(FILM_POST, 0);

  move(b_lines - 4, 0);
  clrtobot();

  prints("發表文章於【 %s 】看板", currboard);

#ifdef POST_PREFIX
  /* 借用 mode、rcpt、fpath */

  if (title)
  {
    rcpt = NULL;
  }
  else    /* itoc.020113: 新文章選擇標題分類 */
  {
    FILE *fp;
    char prefix[NUM_PREFIX][10];

    brd_fpath(fpath, currboard, "prefix");
    if (fp = fopen(fpath, "r"))
    {
      move(21, 0);
      outs("類別：");
      for (mode = 0; mode < NUM_PREFIX; mode++)
      {
        if (fscanf(fp, "%9s", fpath) != 1)
          break;
        strcpy(prefix[mode], fpath);
        prints("%d.%s ", mode + 1, fpath);
      }

      fclose(fp);
      mode = (vget(20, 0, "請選擇文章類別（按 Enter 跳過）：", fpath, 3, DOECHO) - '1');
      rcpt = (mode >= 0 && mode < NUM_PREFIX) ? prefix[mode] : NULL;
    }
    else
    {
      rcpt = NULL;
    }
  }
  if (!ve_subject(21, title, rcpt))
#else
  if (!ve_subject(21, title, NULL))
#endif
    return XO_HEAD;

  /* 未具備 Internet 權限者，只能在站內發表文章 */
  /* Thor.990111: 沒轉信出去的看板, 也只能在站內發表文章 */

  if (!HAS_PERM(PERM_INTERNET) || (currbattr & BRD_NOTRAN))
    curredit &= ~EDIT_OUTGO;

  utmp_mode(M_POST);
  fpath[0] = '\0';
  time(&spendtime);
  if (vedit(fpath, 1) < 0)
  {
    unlink(fpath);
    vmsg(msg_cancel);
    return XO_HEAD;
  }
  spendtime = time(0) - spendtime;  /* itoc.010712: 總共花的時間(秒數) */

  /* build filename */

  folder = xo->dir;
  hdr_stamp(folder, HDR_LINK | 'A', &hdr, fpath);

  /* set owner to anonymous for anonymous board */

#ifdef HAVE_ANONYMOUS
  /* Thor.980727: lkchu新增之[簡單的選擇性匿名功能] */
  if (curredit & EDIT_ANONYMOUS)
  {
    rcpt = anonymousid;  /* itoc.010717: 自定匿名 ID */
    nick = STR_ANONYMOUS;

    /* Thor.980727: lkchu patch: log anonymous post */
    /* Thor.980909: gc patch: log anonymous post filename */
    log_anonymous(hdr.xname);

#ifdef HAVE_UNANONYMOUS_BOARD
    do_unanonymous(fpath);
#endif
  }
  else
#endif
  {
    rcpt = cuser.userid;
    nick = cuser.username;
  }
  title = ve_title;
  mode = (curredit & EDIT_OUTGO) ? POST_OUTGO : 0;
#ifdef HAVE_REFUSEMARK
  if (curredit & EDIT_RESTRICT)
    mode |= POST_RESTRICT;
#endif

  /* qazq.031025: 公開板才做 all_post */
  brd = bshm->bcache + currbno;
  if ((brd->readlevel | brd->postlevel) < (PERM_VALID << 1))
    do_allpost(fpath, rcpt, nick, mode, hdr.chrono, currboard);

  hdr.xmode = mode;
  strcpy(hdr.owner, rcpt);
  strcpy(hdr.nick, nick);
  strcpy(hdr.title, title);

  rec_bot(folder, &hdr, sizeof(HDR));
  btime_update(currbno);

  if (mode & POST_OUTGO)
    outgo_post(&hdr, currboard);

  post_history(xo, &hdr);

  clear();
  outs("順利貼出文章，");

  if (currbattr & BRD_NOCOUNT || wordsnum < 30)
  {        /* itoc.010408: 以此減少灌水現象 */
    outs("文章不列入紀錄，敬請包涵。");
  }
  else
  {
    /* itoc.010408: 依文章長度/所費時間來決定要給多少錢；幣制才會有意義 */
    mode = BMIN(wordsnum, spendtime) / 10;  /* 每十字/秒 一元 */
    prints("這是您的第 %d 篇文章，得 %d 銀。", ++cuser.numposts, mode);
    addmoney(mode);
  }

  /* 回應到原作者信箱 */
  if (curredit & EDIT_BOTH)
  {
    rcpt = quote_user;

    if (strchr(rcpt, '@'))  /* 站外 */
      mode = bsmtp(fpath, title, rcpt, 0);
    else      /* 站內使用者 */
      mode = mail_him(fpath, rcpt, title, 0);

    outs(mode >= 0 ? "\n\n成功\回應至作者信箱" : "\n\n作者無法收信");
  }

  unlink(fpath);

  vmsg(NULL);

#ifdef HAVE_ANONYMOUS
  /* bugfix */
  if (curredit & EDIT_ANONYMOUS)
    curredit &= ~EDIT_ANONYMOUS;
#endif

  return XO_INIT;
}


int
do_reply(xo, hdr)
  XO *xo;
  HDR *hdr;
{
  curredit = 0;

  switch (vans("▲ 回應至 (F)看板 (M)作者信箱 (B)二者皆是 (Q)取消？[F] "))
  {
  case 'm':
    return do_mreply(hdr, 0);

  case 'q':
    /* Thor: 解決 Gao 發現的 bug，先假裝 reply 文章，卻按 q 取消，
  然後去編輯檔案，你就會發現跑出是否引用原文的選項了 */
    *quote_file = '\0';
    return XO_FOOT;

  case 'b':
    /* 若無寄信的權限，則只回看板 */
    if (HAS_PERM(strchr(hdr->owner, '@') ? PERM_INTERNET : PERM_LOCAL))
      curredit = EDIT_BOTH;
    break;
  }

  /* Thor.981105: 不論是轉進的, 或是要轉出的, 都是別站可看到的, 所以回信也都應該轉出 */
  if (hdr->xmode & (POST_INCOME | POST_OUTGO))
    curredit |= EDIT_OUTGO;

  strcpy(quote_user, hdr->owner);
  strcpy(quote_nick, hdr->nick);
  return do_post(xo, hdr->title);
}

static int
post_reply(xo)
  XO *xo;
{
  if (bbstate & STAT_POST)
  {
    HDR *hdr;

    hdr = (HDR *) xo_pool + (xo->pos - xo->top);

#ifdef HAVE_REFUSEMARK
    if ((hdr->xmode & POST_RESTRICT) &&
        strcmp(hdr->owner, cuser.userid) && !(bbstate & STAT_BM))
    return XO_NONE;
#endif

    hdr_fpath(quote_file, xo->dir, hdr);
    return do_reply(xo, hdr);
  }
  return XO_NONE;
}


static int
post_add(xo)
  XO *xo;
{
  curredit = EDIT_OUTGO;
  return do_post(xo, NULL);
}

/* ----------------------------------------------------- */
/* 印出 hdr 標題                                         */
/* ----------------------------------------------------- */

int
tag_char(chrono)
int chrono;
{
  return TagNum && !Tagger(chrono, 0, TAG_NIN) ? '*' : ' ';
}


#ifdef HAVE_DECLARE

#if 0
    蔡勒公式是一個推算哪一天是星期幾的公式.
    適用 2000/03/01 至 2099/12/31

    公式:
         c                y       26(m+1)
    W= [---] - 2c + y + [---] + [---------] + d - 1
         4                4         10
    W → 為所求日期的星期數. (星期日: 0  星期一: 1  ...  星期六: 6)
    c → 為已知公元年份的前兩位數字.
    y → 為已知公元年份的後兩位數字.
    m → 為月數
    d → 為日數
    [] → 表示只取該數的整數部分 (地板函數)
    ps.所求的月份如果是1月或2月,則應視為上一年的13月或14月.
    所以公式中m的取值範圍不是1到12,而是3到14
#endif

static inline int
cal_day(date)    /* itoc.010217: 計算星期幾 */
  char *date;
{
  int y, m, d;

  y = 10 * ((int) (date[0] - '0')) + ((int) (date[1] - '0'));
  d = 10 * ((int) (date[6] - '0')) + ((int) (date[7] - '0'));
  if (date[3] == '0' && (date[4] == '1' || date[4] == '2'))
  {
    y -= 1;
    m = 12 + (int) (date[4] - '0');
  }
  else
  {
    m = 10 * ((int) (date[3] - '0')) + ((int) (date[4] - '0'));
  }
  return (-1 + y + y / 4 + 26 * (m + 1) / 10 + d) % 7;
}
#endif


void
hdr_outs(hdr, cc)    /* print HDR's subject */
  HDR *hdr;
  int cc;      /* 印出最多 cc - 1 字的標題 */
{
  /* 回覆/原創/閱讀中的同主題回覆/閱讀中的同主題原創 */
  static char *type[4] = {"Re", "□", "\033[1;33m=>", "\033[1;32m□"};
  uschar *title, *mark;
  int ch, len;
#ifdef HAVE_DECLARE
  int square;
#endif
#ifdef CHECK_ONLINE
  UTMP *online;
#endif

  /* --------------------------------------------------- */
  /* 印出日期                                            */
  /* --------------------------------------------------- */

#ifdef HAVE_DECLARE
  if (cuser.ufo & UFO_NOCOLOR)
    prints("%s ", hdr->date + 3);
  else /* itoc.010217: 改用星期幾來上色 */
    prints("\033[1;3%dm%s\033[m ", cal_day(hdr->date) + 1, hdr->date + 3);
#else
  outs(hdr->date + 3);
  outc(' ');
#endif

  /* --------------------------------------------------- */
  /* 印出作者                                            */
  /* --------------------------------------------------- */

#ifdef CHECK_ONLINE

  /* guessi.060323 根據好友類別顯示不同顏色提示(僅處理在線使用者) */
  online = utmp_seek(hdr);

  if (online && strcmp(hdr->owner, cuser.userid))
  {
    if (is_mygood(acct_userno(hdr->owner)) && is_ogood(online))
      outs(COLOR_BOTHGOOD);
    else if (is_mygood(acct_userno(hdr->owner)))
      outs(COLOR_MYGOOD);
    else if (is_ogood(online))
      outs(COLOR_OGOOD);
    else
      outs(COLOR7);
  }
#endif

  mark = hdr->owner;
  len = IDLEN + 1;

  while (ch = *mark)
  {
    if ((--len <= 0) || (ch == '@'))  /* 站外的作者把 '@' 換成 '.' */
      ch = '.';
    outc(ch);

    if (ch == '.')
      break;

    mark++;
  }

  while (len--)
  {
    outc(' ');
  }

#ifdef CHECK_ONLINE
  if (online)
    outs(str_ransi);
#endif

  /* --------------------------------------------------- */
  /* 印出標題的種類                                      */
  /* --------------------------------------------------- */

#ifdef HAVE_REFUSEMARK
  if ((hdr->xmode & POST_RESTRICT) && strcmp(hdr->owner, cuser.userid) && !HAS_PERM(PERM_ALLADMIN) && !(bbstate & STAT_BM))
  {
    title = "<文章鎖定>";
    ch = 1;
  }
  else
#endif
  {

    title = str_ttl(mark = hdr->title);
    ch = title == mark;
    if (!strcmp(currtitle, title))
      ch += 2;
  }
  outs(type[ch]);
  outc(' ');

  /* --------------------------------------------------- */
  /* 印出標題                                            */
  /* --------------------------------------------------- */

  mark = title + cc;

#ifdef HAVE_DECLARE  /* Thor.980508: Declaration, 嘗試使某些title更明顯 */
  square = 0;    /* 0:不處理方括 1:要處理方括 */
  if (ch < 2)
  {
    if (*title == '[')
    {
      /* guessi.101117 公告才做標題上色, 使用strncmp比對前六個字元, 即為"[公告]" */
      if (!strncmp(title, "[公告]", 6))
      {
        outs("\033[1m");
        square = 1;
      }
    }
  }
#endif

  while ((cc = *title++) && (title < mark))
  {
#ifdef HAVE_DECLARE
    if (square)
    {
      if (square & 0x80 || cc & 0x80)  /* 中文字的第二碼若是 ']' 不算是方括 */
        square ^= 0x80;
      else if (cc == ']')
      {
        outs("]\033[m");
        square = 0;
        continue;
      }
    }
#endif

    outc(cc);
  }

#ifdef HAVE_DECLARE
  if (square || ch >= 2)  /* Thor.980508: 變色還原用 */
#else
  if (ch >= 2)
#endif
    outs("\033[m");

  outc('\n');
}


/* ----------------------------------------------------- */
/* 看板功能表                                            */
/* ----------------------------------------------------- */


static int post_body();
static int post_head();


static int
post_init(xo)
XO *xo;
{
  xo_load(xo, sizeof(HDR));
  return post_head(xo);
}


static int
post_load(xo)
XO *xo;
{
  xo_load(xo, sizeof(HDR));
  return post_body(xo);
}


static int
post_attr(fhdr)
HDR *fhdr;
{
  int mode, attr;

  mode = fhdr->xmode;

  /* 由於置底文沒有閱讀記錄，所以視為已讀 */
  if (cuser.ufo & UFO_READMD)  /* guessi.060825 喜好設定:推文未讀記號 */
    attr = !(mode & POST_BOTTOM) && brh_unread(fhdr->chrono) ? 0 : 0x20;
  else
    attr = !(mode & POST_BOTTOM) && brh_unread(BMAX(fhdr->chrono, fhdr->stamp)) ? 0 : 0x20;

#ifdef HAVE_REFUSEMARK
  if (mode & POST_RESTRICT)
    attr = '!';
  else
#endif

  if (mode & POST_DONE)
    attr |= 'S';
  else

  if (mode & POST_DIGEST)
    attr = (attr & 0x20) ? '#' : '*';
  else

#ifdef HAVE_LABELMARK
  if (mode & POST_DELETE)
    attr = 'D';
  else
#endif

  if (mode & POST_MARKED)
    attr |= 'M';
  else

  if (!attr)
  {
    /* guessi.090910 已讀過但有修改或有新推文 */
    /* 原文未讀:     +                        */
    /* 讀取後有更動: ~                        */
    /* 其他:         +                        */

    if (brh_unread(fhdr->chrono))
      attr = '+';
    else if (brh_unread(fhdr->stamp) && !(cuser.ufo & UFO_READMD))
      attr = '~';
    else
      attr = '+';
  }
  return attr;
}


static void
post_item(num, hdr)
  int num;
  HDR *hdr;
{
#ifdef HAVE_SCORE

  if (hdr->xmode & POST_BOTTOM)
    prints("  \033[1;32m重要\033[m");
  else
    prints("%6d", num);

  prints(" %s%c\033[m", (hdr->xmode & POST_MARKED) ? "\033[1;36m" :
                        (hdr->xmode & POST_DELETE) ? "\033[1;32m" :
                        (hdr->xmode & POST_DIGEST) ? "\033[1;35m" : "", post_attr(hdr));

  if (hdr->xmode & POST_SCORE && !(hdr->xmode & POST_BOTTOM)) /* 置底文不顯示評分 */
  {
    num = hdr->score;

    if (abs(num) >= 99)
      prints("\033[1;3%s\033[m", num > 0 ? "3m爆" : "0m爛" );
    else
      prints("\033[1;3%dm%02d\033[m", num >= 0 ? 1 : 2, abs(num));
  }
  else
    outs("  ");

  prints("%c", tag_char(hdr->chrono));

  hdr_outs(hdr, d_cols + 46);  /* 少一格來放分數 */
#else
  prints("%6d%c%c ", (hdr->xmode & POST_BOTTOM) ? "  重要" : num, tag_char(hdr->chrono), post_attr(hdr));
  hdr_outs(hdr, d_cols + 47);
#endif
}


static int
post_body(xo)
  XO *xo;
{
  HDR *fhdr;
  int num, max, tail;

  max = xo->max;
  if (max <= 0)
  {
    if (bbstate & STAT_POST)
    {
      if (vans("目前看板無文章，您要新增資料嗎？ Y)是 N)否 [N] ") == 'y')
      return post_add(xo);
    }
    else
    {
      vmsg("本看板尚無文章");
    }
    return XO_QUIT;
  }

  fhdr = (HDR *) xo_pool;
  num = xo->top;
  tail = num + XO_TALL;

  if (max > tail)
    max = tail;

  move(3, 0);
  do
  {
    post_item(++num, fhdr++);
  } while (num < max);
  clrtobot();

  return XO_FOOT;  /* itoc.010403: 把 b_lines 填上 feeter */
}


static int
post_head(xo)
  XO *xo;
{
  vs_head(currBM, xo->xyz);
  prints(NECKER_POST, d_cols, "", bshm->mantime[currbno]);

  return post_body(xo);
}


/* ----------------------------------------------------- */
/* 資料之瀏覽：browse / history                          */
/* ----------------------------------------------------- */


static int
post_visit(xo)
  XO *xo;
{
  int ans, row, max;
  HDR *hdr;

  ans = vans("設定所有文章 (U)未讀 (V)已讀 (W)前已讀後未讀 (Q)取消？[Q] ");

  if (ans == 'v' || ans == 'u' || ans == 'w')
  {
    row = xo->top;
    max = xo->max - row + 3;
    if (max > b_lines)
      max = b_lines;

    hdr = (HDR *) xo_pool + (xo->pos - row);
    /* brh_visit(ans == 'w' ? hdr->chrono : ans == 'u'); */
    /* weiyu.041010: 在置底文上選 w 視為全部已讀 */
    brh_visit((ans == 'u') ? 1 : (ans == 'w' && !(hdr->xmode & POST_BOTTOM)) ? hdr->chrono : 0);

    hdr = (HDR *) xo_pool;

    return post_body(xo);
  }

  return XO_FOOT;
}

void
post_history(xo, hdr)          /* 將 hdr 這篇加入 brh */
  XO *xo;
  HDR *hdr;
{
  int fd;
  time_t prev, chrono, next, this;
  HDR buf;

  check_stamp:                   /* 設定檢查點                  */

  if (brh_unread(hdr->chrono)) /* 先檢查hdr->chrono 是否在brh */
    chrono = hdr->chrono;
  else                         /* 若已經在brh則檢查stamp      */
    chrono = BMAX(hdr->chrono, hdr->stamp);

  if (!brh_unread(chrono))     /* 如果已在 brh 中，就無需動作 */
    return;

  if ((fd = open(xo->dir, O_RDONLY)) >= 0)
  {
    prev = chrono + 1;
    next = chrono - 1;
    while (read(fd, &buf, sizeof(HDR)) == sizeof(HDR))
    {
      this = BMAX(buf.chrono, buf.stamp);
      if (chrono - this < chrono - prev)
        prev = this;
      else if (this - chrono < next - chrono)
        next = this;
    }
    close(fd);

    if (prev > chrono)      /* 沒有下一篇 */
      prev = chrono;
    if (next < chrono)      /* 沒有上一篇 */
      next = chrono;
    brh_add(prev, chrono, next);
  }

  chrono = BMAX(hdr->chrono, hdr->stamp); /* 若有stamp      */
  if (brh_unread(chrono)) /* 檢查BMAX()是否在brh 若無則返回 */
    goto check_stamp;
}


static int
find_board(fpath)
  char *fpath;
{
  FILE *fp;
  char buf[ANSILINELEN], *str, *ptr;

  if (fp = fopen(fpath, "r"))
  {
    ptr = fgets(buf, ANSILINELEN, fp);
    fclose(fp);

    if (ptr && ((str = strstr(buf, STR_POST1 " ")) ||
                (str = strstr(buf, STR_POST2 " "))
               )
       )
    {
      str += sizeof(STR_POST1);
      if (ptr = strchr(str, '\n'))
      {
        *ptr = '\0';
      }
      return brd_bno(str);
    }
  }
  return -1;
}

static void
allpost_history(xo, hdr)
  XO *xo;
  HDR *hdr;
{
  int fd, bno;
  BRD *brd;
  char folder[64], fpath[64];
  HDR old;

  if (strcmp(currboard, BN_ALLPOST))/* 在一般板讀完，也去 All_Post 標示已讀 */
  {
    brd_fpath(folder, BN_ALLPOST, fn_dir);
    if ((fd = open(folder, O_RDONLY)) >= 0)
    {
      lseek(fd, 0, SEEK_SET);
      while (read(fd, &old, sizeof(HDR)) == sizeof(HDR))
      {
        if (hdr->chrono == old.xid)
        {
          hdr_fpath(fpath, folder, &old);
          if ((bno = find_board(fpath)) >= 0)
          {
            brd = bshm->bcache + bno;
            if (!strcmp(currboard, brd->brdname))
            {
              bno = brd_bno(BN_ALLPOST);
              brd = bshm->bcache + bno;

              brh_get(brd->bstamp, bno);
              bno = hdr->chrono;
              brh_add(bno - 1, bno, bno + 1);

              /* 恢復原來看板 */
              brd = bshm->bcache + currbno;
              brh_get(brd->bstamp, currbno);
              break;
            }
          }
        }
      }
      close(fd);
    }
  }
  else                              /* 在 All_Post 讀完，也去別板標示已讀 */
  {
    hdr_fpath(fpath, xo->dir, hdr);
    if ((bno = find_board(fpath)) >= 0)
    {
      brd = bshm->bcache + bno;
      brh_get(brd->bstamp, bno);
      bno = hdr->xid;
      brh_add(bno - 1, bno, bno + 1);

      /* 恢復原來看板 */
      brd = bshm->bcache + currbno;
      brh_get(brd->bstamp, currbno);
    }
  }
}


static int
post_browse(xo)
  XO *xo;
{
  HDR *hdr;
  int xmode, pos, key;
  char *dir, fpath[64];

  dir = xo->dir;

  for (;;)
  {
    pos = xo->pos;
    hdr = (HDR *) xo_pool + (pos - xo->top);
    xmode = hdr->xmode;

#ifdef HAVE_REFUSEMARK
    if ((xmode & POST_RESTRICT) && !HAS_PERM(PERM_ALLADMIN) &&
        strcmp(hdr->owner, cuser.userid) && !(bbstate & STAT_BM))
    break;
#endif

    hdr_fpath(fpath, dir, hdr);

    /* Thor.990204: 為考慮more 傳回值 */
    if ((key = more(fpath, FOOTER_POST)) < 0)
      break;

    post_history(xo, hdr);
    allpost_history(xo, hdr);
    strcpy(currtitle, str_ttl(hdr->title));

re_key:

    switch (xo_getch(xo, key))
    {
    case XO_BODY:
      continue;

    case 'y':
    case 'r':
      if (bbstate & STAT_POST)
      {
        strcpy(quote_file, fpath);
        if (do_reply(xo, hdr) == XO_INIT)  /* 有成功地 post 出去了 */
          return post_init(xo);
      }
      break;

    case 'm':
      if ((bbstate & STAT_BOARD) && !(xmode & (POST_MARKED | POST_DELETE)))
      {
        /* hdr->xmode = xmode ^ POST_MARKED; */
        /* 在 post_browse 時看不到 m 記號，所以限制只能 mark */
        hdr->xmode = xmode | POST_MARKED;
        currchrono = hdr->chrono;
        rec_put(dir, hdr, sizeof(HDR), pos, cmpchrono);
      }
      break;

#ifdef HAVE_SCORE
    case 'X':
    case '%':
      post_score(xo);
      return post_init(xo);
#endif

    case '/':
    case 'n':
      if (vget(b_lines, 0, "搜尋：", hunt, sizeof(hunt), DOECHO))
      {
        more(fpath, FOOTER_POST);
        goto re_key;
      }
      continue;

    case 'E':
      return post_edit(xo);

    case 'C':  /* itoc.000515: post_browse 時可存入暫存檔 */
      {
        FILE *fp;
        if (fp = tbf_open())
        {
          f_suck(fp, fpath);
          fclose(fp);
        }
      }
      break;

    case 'h':
      xo_help("post");
      break;
    }
    break;
  }

  return post_head(xo);
}


/* ----------------------------------------------------- */
/* 精華區                                                */
/* ----------------------------------------------------- */

static int
post_gem(xo)
  XO *xo;
{
  int level;
  char fpath[64];

  strcpy(fpath, "gem/");
  strcpy(fpath + 4, xo->dir);

  level = 0;
  if (bbstate & STAT_BOARD)
    level ^= GEM_W_BIT;
  if (HAS_PERM(PERM_SYSOP))
    level ^= GEM_X_BIT;
  /* guessi.060411 站務可看見隱藏精華 */
  if (bbstate & STAT_BM || HAS_PERM(PERM_ALLBOARD))
    level ^= GEM_M_BIT;

  XoGem(fpath, "精華區", level);
  return post_init(xo);
}

static int
post_memo(xo)
  XO *xo;
{
  char fpath[64];

  brd_fpath(fpath, currboard, fn_note);

  if (more(fpath, NULL) < 0)
  {
    vmsg("本看板目前沒有進板畫面唷");
    return XO_FOOT;
  }

  return post_head(xo);
}


/* ----------------------------------------------------- */
/* 功能：tag / switch / cross / forward                  */
/* ----------------------------------------------------- */


static int
post_tag(xo)
  XO *xo;
{
  HDR *hdr;
  int tag, pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

  if (xo->key == XZ_XPOST)
    pos = hdr->xid;

  if (tag = Tagger(hdr->chrono, pos, TAG_TOGGLE))
  {
    move(3 + cur, 0);
    post_item(pos + 1, hdr);
  }

  return xo->pos + 1 + XO_MOVE; /* lkchu.981201: 跳至下一項 */
}


static int
post_switch(xo)
  XO *xo;
{
  int bno;
  BRD *brd;
  char bname[BNLEN + 1];

  if (brd = ask_board(bname, BRD_R_BIT, NULL))
  {
    if ((bno = brd - bshm->bcache) >= 0 && currbno != bno)
    {
      XoPost(bno);
      return XZ_POST;
    }
  }
  else
  {
    vmsg(err_bid);
  }
  return post_head(xo);
}


int
post_cross(xo)
  XO *xo;
{
  /* 來源看板 */
  char *dir, *ptr;
  HDR *hdr, xhdr;

  /* 欲轉去的看板 */
  int xbno;
  usint xbattr;
  char xboard[BNLEN + 1], xfolder[64];
  HDR xpost;

  int tag, rc, locus, finish;
  int method;    /* 0:原文轉載 1:從公開看板/精華區/信箱轉錄文章 2:從秘密看板轉錄文章 */
  usint tmpbattr;
  char tmpboard[BNLEN + 1];
  char fpath[64], buf[ANSILINELEN];
  FILE *fpr, *fpw, *fpo;

  if (!cuser.userlevel)  /* itoc.000213: 避免 guest 轉錄去 sysop 板 */
    return XO_NONE;

  tag = AskTag("轉錄");
  if (tag < 0)
    return XO_FOOT;

  dir = xo->dir;

  if (!ask_board(xboard, BRD_W_BIT, "\n\n\033[1;33m請挑選適當的看板，切勿轉錄超過四板。\033[m\n\n") ||
      (*dir == 'b' && !strcmp(xboard, currboard)))  /* 信箱、精華區中可以轉錄至currboard */
  return XO_HEAD;

  hdr = tag ? &xhdr : (HDR *) xo_pool + (xo->pos - xo->top);  /* lkchu.981201: 整批轉錄 */

  /* 原作者轉錄自己文章時，可以選擇「原文轉載」 */
  method = (HAS_PERM(PERM_ALLBOARD) || (!tag && !strcmp(hdr->owner, cuser.userid))) &&
  (vget(2, 0, "(1)原文轉載 (2)轉錄文章？[1] ", buf, 3, DOECHO) != '2') ? 0 : 1;

  if (!tag)  /* lkchu.981201: 整批轉錄就不要一一詢問 */
  {
    if (method && strncmp(hdr->title, "[轉錄]", 6))  /* 已有[轉錄]字樣就不要一直加了 */
      sprintf(ve_title, "[轉錄]%.66s", hdr->title);
    else
      strcpy(ve_title, hdr->title);

    if (!vget(2, 0, "標題：", ve_title, TTLEN + 1, GCARRY))
      return XO_HEAD;
  }

#ifdef HAVE_REFUSEMARK
  rc = vget(2, 0, "(S)存檔 (L)站內 (X)密封 (Q)取消？[Q] ", buf, 3, LCECHO);
  if (rc != 'l' && rc != 's' && rc != 'x')
    return XO_HEAD;
#else
  rc = vget(2, 0, "(S)存檔 (L)站內 (Q)取消？[Q] ", buf, 3, LCECHO);
  if (rc != 'l' && rc != 's')
    return XO_HEAD;
#endif

  if (method && *dir == 'b')  /* 從看板轉出，先檢查此看板是否為秘密板 */
  {
    /* 借用 tmpbattr */
    tmpbattr = (bshm->bcache + currbno)->readlevel;
    if (tmpbattr & PERM_SYSOP || tmpbattr & PERM_BOARD)
      method = 2;
  }

  xbno = brd_bno(xboard);
  xbattr = (bshm->bcache + xbno)->battr;

  /* Thor.990111: 在可以轉出前，要檢查有沒有轉出的權力? */
  if ((rc == 's') && (!HAS_PERM(PERM_INTERNET) || (xbattr & BRD_NOTRAN)))
  rc = 'l';

  /* 備份 currboard */
  if (method)
  {
    /* itoc.030325: 一般轉錄呼叫 ve_header，會使用到 currboard、currbattr，先備份起來 */
    strcpy(tmpboard, currboard);
    strcpy(currboard, xboard);
    tmpbattr = currbattr;
    currbattr = xbattr;
  }

  locus = 0;
  do  /* lkchu.981201: 整批轉錄 */
  {
    if (tag)
    {
      EnumTag(hdr, dir, locus, sizeof(HDR));

      if (method && strncmp(hdr->title, "[轉錄]", 6))     /* 已有[轉錄]字樣就不要一直加了 */
        sprintf(ve_title, "[轉錄]%.66s", hdr->title);
      else
        strcpy(ve_title, hdr->title);
    }

    if (hdr->xmode & GEM_FOLDER)  /* 非 plain text 不能轉 */
      continue;

    if (hdr->xmode & GEM_LINE) /* guessi.060612 GEM_LINE */
      continue;

#ifdef HAVE_REFUSEMARK
    if (hdr->xmode & POST_RESTRICT)
    continue;
#endif

    hdr_fpath(fpath, dir, hdr);

#ifdef HAVE_DETECT_CROSSPOST
    if (check_crosspost(fpath, xbno))
      break;
#endif

    brd_fpath(xfolder, xboard, fn_dir);

    if (method)    /* 一般轉錄 */
    {
      /* itoc.030325: 一般轉錄要重新加上 header */
      fpw = fdopen(hdr_stamp(xfolder, 'A', &xpost, buf), "w");
      ve_header(fpw);

      /* itoc.040228: 如果是從精華區轉錄出來的話，會顯示轉錄自 [currboard] 看板，
  然而 currboard 未必是該精華區的看板。不過不是很重要的問題，所以就不管了 :p */
      fprintf(fpw, "※ 本文轉錄自 [%s] %s\n\n",
      *dir == 'u' ? cuser.userid : method == 2 ? "秘密" : tmpboard,
      *dir == 'u' ? "信箱" : "看板");

      /* Kyo.051117: 若是從秘密看板轉出的文章，刪除文章第一行所記錄的看板名稱 */
      finish = 0;
      if ((method == 2) && (fpr = fopen(fpath, "r")))
      {
        if (fgets(buf, sizeof(buf), fpr) &&
            ((ptr = strstr(buf, str_post1)) || (ptr = strstr(buf, str_post2))) && (ptr > buf))
        {
          ptr[-1] = '\n';
          *ptr = '\0';

          do
          {
            fputs(buf, fpw);
          } while (fgets(buf, sizeof(buf), fpr));
          finish = 1;
        }
        fclose(fpr);
      }
      if (!finish)
        f_suck(fpw, fpath);

      ve_banner(fpw, 0); /* guessi.070317 留下站簽 記錄轉錄者資訊 */

      fclose(fpw);

      strcpy(xpost.owner, cuser.userid);
      strcpy(xpost.nick, cuser.username);
    }
    else    /* 原文轉錄 */
    {
      /* itoc.030325: 原文轉錄直接 copy 即可 */
      hdr_stamp(xfolder, HDR_COPY | 'A', &xpost, fpath);

      strcpy(xpost.owner, hdr->owner);
      strcpy(xpost.nick, hdr->nick);
      strcpy(xpost.date, hdr->date);  /* 原文轉載保留原日期 */
    }

    /* guessi.070317 轉錄文章時，原文張加註紀錄 (若對象為Admin站務看板則排除不做) */
    if (strcmp(xboard, "Admin") && (fpo = fopen(fpath, "a")))
    {
      char msg[64];
      usint checklevel;

      time_t now;
      struct tm *ptime;

      time(&now);
      ptime = localtime(&now);

      checklevel = (bshm->bcache + xbno)->readlevel;

      sprintf(msg, "本文已轉錄至 [%s] 看板", (checklevel == PERM_SYSOP || checklevel == PERM_BOARD) ? "秘密" : xboard);

      fprintf(fpo, "\033[1;30m轉 %s:%-*s %02d/%02d %02d:%02d\033[m\n", cuser.userid, 62 - strlen(cuser.userid), msg,
      ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min); /* 格式自訂 盡量與推文類似 */

      fclose(fpo);
    }

    strcpy(xpost.title, ve_title);

    if (rc == 's')
      xpost.xmode = POST_OUTGO;

#ifdef HAVE_REFUSEMARK
    else if (rc == 'x')
      xpost.xmode = POST_RESTRICT;
#endif

    rec_bot(xfolder, &xpost, sizeof(HDR));

    if (rc == 's')
      outgo_post(&xpost, xboard);
  } while (++locus < tag);

  btime_update(xbno);

  /* Thor.981205: check 被轉的板有沒有列入紀錄? */
  if (!(xbattr & BRD_NOCOUNT))
    cuser.numposts += tag ? tag : 1;  /* lkchu.981201: 要算 tag */

  /* 復原 currboard、currbattr */
  if (method)
  {
    strcpy(currboard, tmpboard);
    currbattr = tmpbattr;
  }

  vmsg("轉錄完成");
  return XO_HEAD;
}

/* ----------------------------------------------------- */
/* 板主功能：mark / delete / label                       */
/* ----------------------------------------------------- */

static int
post_mark(xo)
  XO *xo;
{
  if (bbstate & STAT_BOARD)
  {
    HDR *hdr;
    int pos, cur;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;

    if (hdr->xmode & (POST_BOTTOM | POST_DONE | POST_DIGEST))
      return XO_NONE;

#ifdef HAVE_LABELMARK
    if (hdr->xmode & POST_DELETE)  /* 待砍的文章不能 mark */
      return XO_NONE;
#endif

    hdr->xmode ^= POST_MARKED;
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(pos + 1, hdr);
  }
  return XO_NONE;
}

static int
post_digest(xo) /* 標記文摘 */
  XO *xo;
{
  if (bbstate & STAT_BOARD || HAS_PERM(PERM_ALLADMIN)) /* 檢查是否為板主 */
  {
    HDR *hdr;
    int pos, cur;
    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;

    if (hdr->xmode & (POST_BOTTOM | POST_MARKED | POST_DONE | POST_RESTRICT))
      return XO_NONE;

#ifdef HAVE_LABELMARK
    if (hdr->xmode & POST_DELETE)
      return XO_NONE;
#endif

    hdr->xmode ^= POST_DIGEST;
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(pos + 1, hdr);
  }
  return XO_NONE;
}

static int
post_done_mark(xo)
  XO *xo;
{
  if (HAS_PERM(PERM_ALLADMIN))
  {
    HDR *hdr;
    int pos, cur;
    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;

    if (hdr->xmode & (POST_MARKED | POST_BOTTOM | POST_DIGEST | POST_RESTRICT))
      return XO_NONE;

#ifdef HAVE_LABELMARK
    if (hdr->xmode & POST_DELETE)
      return XO_NONE;
#endif

    hdr->xmode ^= POST_DONE;
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(pos + 1, hdr);
  }
  return XO_NONE;
}

static int
post_bottom(xo)
  XO *xo;
{
  if (bbstate & STAT_BOARD)
  {
    HDR *hdr, post;
    char fpath[64];

    hdr = (HDR *) xo_pool + (xo->pos - xo->top);
    currchrono = hdr->chrono;

    if (vans(hdr->xmode & POST_BOTTOM ? "取消置底？ Y)是 N)否 [N] " : "加入置底？ Y)是 N)否 [N] ") != 'y')
      return XO_FOOT;

    /* itoc.限制置底文數量 */
    int fd, fsize;
    struct stat st;
    if (!(hdr->xmode & POST_BOTTOM))    /* guessi.060417 若為取消 則可執行 */
    {
      if ((fd = open(xo->dir, O_RDONLY)) >= 0)
      {
        if (!fstat(fd, &st))
        {
          fsize = st.st_size;
          while ((fsize -= sizeof(HDR)) >= 0)
          {
            lseek(fd, fsize, SEEK_SET);
            read(fd, &post, sizeof(HDR));
            if (!(post.xmode & POST_BOTTOM))
              break;
          }
        }

        close(fd);

        if ((st.st_size - fsize) / sizeof(HDR) > 5)
        {
          vmsg("置底文數量不得超過五篇");
          return XO_FOOT;
        }
      }
    }

    if (hdr->xmode & POST_BOTTOM)
    {
      rec_del(xo->dir, sizeof(HDR), xo->pos, cmpchrono);
      return post_load(xo);
    }
    else
    {
      /* guessi.060220 加入判斷是否為置底文 詢問取消or加入置底 */
      hdr_fpath(fpath, xo->dir, hdr);
      hdr_stamp(xo->dir, HDR_LINK | 'A', &post, fpath);

      /* guessi 待砍文章不可置底 */
#ifdef HAVE_LABELMARK
      if (hdr->xmode & POST_DELETE)
        return XO_NONE;
#endif

#ifdef HAVE_REFUSEMARK
      post.xmode = POST_BOTTOM | (hdr->xmode & POST_RESTRICT);
#else
      post.xmode = POST_BOTTOM;
#endif

      strcpy(post.date, hdr->date);   /* guessi 修正置底文日期與原文相同 */
      strcpy(post.owner, hdr->owner);
      strcpy(post.nick, hdr->nick);
      strcpy(post.title, hdr->title);

      rec_add(xo->dir, &post, sizeof(HDR));

      return post_load(xo);             /* 立刻顯示置底文章 */
    }
  }
  return XO_NONE;
}


#ifdef HAVE_REFUSEMARK
static int
post_refuse(xo)    /* itoc.010602: 文章加密 */
  XO *xo;
{
  HDR *hdr;
  int pos, cur;

  if (!cuser.userlevel)  /* itoc.020114: guest 不能對其他 guest 的文章加密 */
  return XO_NONE;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

  /* guessi.100524 不可針對置底文作此操作 */
  if (hdr->xmode & POST_BOTTOM)
    return XO_NONE;

  /* guessi.061213 板主、站務 才可以使用本功能 */
  if (bbstate & STAT_BM || HAS_PERM(PERM_ALLADMIN))
  {
    hdr->xmode ^= POST_RESTRICT;
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(pos + 1, hdr);
  }

  return XO_NONE;
}
#endif


static int
post_remove_mark(xo)
  XO *xo;
{
  /* guessi.091204 若非看板站務... 退出 */
  if (!(bbstate & STAT_BOARD))
    return XO_FOOT;

  if (vans("請確認消除標記？ Y)移除 N)保留 [N] ") != 'y')
    return XO_FOOT;

  HDR *hdr;
  int pos, cur;
  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

  hdr->xmode = 0;

  currchrono = hdr->chrono;
  rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

  move(3 + cur, 0);
  post_item(pos + 1, hdr);

  return XO_FOOT;
}

static int
post_is_system_board()
{
  /* guessi.100618 系統看板不提供刪文功能 */
  return (!strcmp(currboard, BN_SECURITY) ||
          !strcmp(currboard, BN_JUNK) ||
          !strcmp(currboard, BN_RECORD) ||
          !strcmp(currboard, BN_BMTA) ||
          !strcmp(currboard, BN_CPRECORD) ||
          !strcmp(currboard, BN_SPAM_DEL) ||
          !strcmp(currboard, BN_DELETED) ||
          !strcmp(currboard, BN_UNANONYMOUS) ||
          !strcmp(currboard, BN_ALLPOST));
}

#ifdef HAVE_LABELMARK
static int
post_label(xo)
  XO *xo;
{
  if(post_is_system_board())
  {
    zmsg("此看板設定無法標記代砍");
    return XO_FOOT;
  }

  if (bbstate & STAT_BOARD)
  {
    HDR *hdr;
    int pos, cur;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;

    if (hdr->xmode & (POST_MARKED | POST_DONE | POST_RESTRICT | POST_BOTTOM | POST_DIGEST))  /* mark 或 加密的文章不能待砍 */
      return XO_NONE;

    hdr->xmode ^= POST_DELETE;
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(pos + 1, hdr);

    return pos + 1 + XO_MOVE;  /* 跳至下一項 */
  }

  return XO_NONE;
}

static int
post_delabel(xo)
  XO *xo;
{
  int fdr, fsize, xmode;
  char fnew[64], fold[64], *folder;
  HDR *hdr;
  FILE *fpw;

  if (post_is_system_board())
  {
    zmsg("此看板設定無法刪除文章");
    return XO_FOOT;
  }

  if (!(bbstate & STAT_BOARD))
  return XO_NONE;

  if (vans("確定要刪除待砍文章嗎？ Y)刪除 N)保留 [N] ") != 'y')
    return XO_FOOT;

  folder = xo->dir;
  if ((fdr = open(folder, O_RDONLY)) < 0)
    return XO_FOOT;

  if (!(fpw = f_new(folder, fnew)))
  {
    close(fdr);
    return XO_FOOT;
  }

  fsize = 0;
  mgets(-1);

  while (hdr = mread(fdr, sizeof(HDR)))
  {
    xmode = hdr->xmode;

    if (!(xmode & POST_DELETE))
    {
      if ((fwrite(hdr, sizeof(HDR), 1, fpw) != 1))
      {
        close(fdr);
        fclose(fpw);
        unlink(fnew);
        return XO_FOOT;
      }
      fsize++;
    }
  }
  close(fdr);
  fclose(fpw);

  sprintf(fold, "%s.o", folder);
  rename(folder, fold);

  if (fsize)
    rename(fnew, folder);
  else
    unlink(fnew);

  btime_update(currbno);

  return post_load(xo);
}
#endif


static int
post_delete(xo)
  XO *xo;
{
  int pos, cur, by_BM;
  HDR *hdr;
  char buf[80];
  char *blist;

  if (!cuser.userlevel)
    return XO_NONE;

  if(post_is_system_board())
  {
    zmsg("此看板設定無法刪除文章");
    return XO_FOOT;
  }

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

  if ((hdr->xmode & POST_MARKED) ||
      (hdr->xmode & POST_RESTRICT) ||
      (hdr->xmode & POST_DONE) ||
      (!(bbstate & STAT_BOARD) &&
        strcmp(hdr->owner, cuser.userid)))
  return XO_NONE;

  blist = (bshm->bcache + brd_bno(currboard))->BM; /* bm list */

  by_BM = (!(bbstate & STAT_BOARD)) ? 0 : 
          ((HAS_PERM(PERM_BM) && is_bm(blist, cuser.userid)) ||
           str_cmp(hdr->owner, cuser.userid)) ? 1 : 0;

  if (vans(msg_del_ny) == 'y')
  {
    currchrono = hdr->chrono;

    pos = move_post(hdr, xo->dir, by_BM);

    if (!rec_del(xo->dir, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono))
    {
      /* guessi.070305 置底文不扣文章數、金錢 */
      if (!by_BM && !(currbattr & BRD_NOCOUNT) && !(hdr->xmode & POST_BOTTOM))
      {
#ifdef HAVE_DETECT_CROSSPOST
        int sum;
        hdr_fpath(buf, xo->dir, hdr);
        sum = checksum_count(buf);
        checksum_rmv(sum);
#endif

        /* itoc.010711: 砍文章要扣錢，算檔案大小 */
        pos = pos >> 3;  /* 相對於 post 時 wordsnum / 10 */

        /* itoc.010830.註解: 漏洞: 若 multi-login 砍不到另一隻的錢 */
        if (cuser.money > pos)
          cuser.money -= pos;
        else
          cuser.money = 0;

        if (cuser.numposts > 0)
          cuser.numposts--;

        sprintf(buf, "%s，您的文章減為 %d 篇", MSG_DEL_OK, cuser.numposts);

        vmsg(buf);
      }

      if (xo->key == XZ_XPOST)
      {
        vmsg("原列表經刪除後混亂，請重進串接模式！");
        return XO_QUIT;
      }
      return XO_LOAD;
    }
  }
  return XO_FOOT;
}


static int
chkpost(hdr)
  HDR *hdr;
{
  return (hdr->xmode & POST_MARKED);
}

static int
vfypost(hdr, pos)
  HDR *hdr;
  int pos;
{
  return (Tagger(hdr->chrono, pos, TAG_NIN) || chkpost(hdr));
}


static void
delpost(xo, hdr)
  XO *xo;
  HDR *hdr;
{
  move_post(hdr, xo->dir, 1);
}


static int
post_rangedel(xo)
  XO *xo;
{
  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  if(post_is_system_board())
  {
    zmsg("此看板設定無法刪除文章");
    return XO_FOOT;
  }

  btime_update(currbno);

  return xo_rangedel(xo, sizeof(HDR), chkpost, delpost);
}


static int
post_prune(xo)
  XO *xo;
{
  int ret;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  if (post_is_system_board())
  {
    zmsg("此看板設定無法刪除文章");
    return XO_FOOT;
  }

  ret = xo_prune(xo, sizeof(HDR), vfypost, delpost);

  btime_update(currbno);

  if (xo->key == XZ_XPOST && ret == XO_LOAD)
  {
    vmsg("原列表經批次刪除後混亂，請重進串接模式！");
    return XO_QUIT;
  }

  return ret;
}

#ifdef HAVE_REFUSEMARK
static int
chkrestrict(hdr)
  HDR *hdr;
{
  return !(hdr->xmode & POST_RESTRICT) ||
  !strcmp(hdr->owner, cuser.userid) || (bbstate & STAT_BM);
}
#endif

static int
post_copy(xo)     /* itoc.010924: 取代 gem_gather */
  XO *xo;
{
  int tag;

  tag = AskTag("看板文章拷貝");

  if (tag < 0)
    return XO_FOOT;

#ifdef HAVE_REFUSEMARK
  gem_buffer(xo->dir, tag ? NULL : (HDR *) xo_pool + (xo->pos - xo->top), chkrestrict);
#else
  gem_buffer(xo->dir, tag ? NULL : (HDR *) xo_pool + (xo->pos - xo->top), NULL);
#endif

  if (bbstate & STAT_BOARD)
  {
#ifdef XZ_XPOST
    if (xo->key == XZ_XPOST)
    {
      zmsg("檔案標記完成。[注意] 您必須先離開串接模式才能進入精華區。");
      return XO_FOOT;
    }
    else
#endif
    {
      zmsg("拷貝完成。[注意] 貼上後才能刪除原文！");
      return post_gem(xo);  /* 拷貝完直接進精華區 */
    }
  }

  zmsg("檔案標記完成。[注意] 您只能在擔任(小)板主所在或個人精華區貼上。");
  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* 站長功能：edit / title                                */
/* ----------------------------------------------------- */


int
post_edit(xo)
  XO *xo;
{
  char fpath[64];
  HDR *hdr;
  FILE *fp;

  if (post_is_system_board())
  {
    vmsg("此看板設定無法修改文章");
    return XO_FOOT;
  }

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);

  hdr_fpath(fpath, xo->dir, hdr);

  strcpy(ve_title, hdr->title);

#ifdef HAVE_REFUSEMARK
  if ((hdr->xmode & POST_RESTRICT) && !(bbstate & STAT_BM) && strcmp(hdr->owner, cuser.userid))
    return XO_NONE;
#endif

  /* guessi.****** 即使是站務修改，也留下修改記錄 */
  if ((cuser.userlevel && !strcmp(hdr->owner, cuser.userid)) || HAS_PERM(PERM_SYSOP))  /* 原作者修改 */
  {
    if (!vedit(fpath, 0))  /* 若非取消則加上修改資訊 */
    {
      if (fp = fopen(fpath, "a"))
      {
        ve_banner(fp, 1);
        fclose(fp);
        change_stamp(xo->dir, hdr);
        currchrono = hdr->chrono;
        rec_put(xo->dir, hdr, sizeof(HDR), xo->pos, cmpchrono);
        post_history(xo, hdr);
        /* 文章修改不設為未讀
         * btime_update(currbno);
         */
      }
    }
  }
  else
    return XO_NONE;

  if (strcmp(ve_title, hdr->title))
  {
    strcpy(hdr->title, ve_title);
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : xo->pos, cmpchrono);
    return XO_INIT;
  }

  /* return post_head(xo); */
  return XO_HEAD;  /* itoc.021226: XZ_POST 和 XZ_XPOST 共用 post_edit() */
}


void
header_replace(xo, fhdr)  /* itoc.010709: 修改文章標題順便修改內文的標題 */
  XO *xo;
  HDR *fhdr;
{
  FILE *fpr, *fpw;
  char srcfile[64], tmpfile[64], buf[ANSILINELEN];

  hdr_fpath(srcfile, xo->dir, fhdr);
  strcpy(tmpfile, "tmp/");
  strcat(tmpfile, fhdr->xname);
  f_cp(srcfile, tmpfile, O_TRUNC);

  if (!(fpr = fopen(tmpfile, "r")))
    return;

  if (!(fpw = fopen(srcfile, "w")))
  {
    fclose(fpr);
    return;
  }

  fgets(buf, sizeof(buf), fpr);    /* 加入作者 */
  fputs(buf, fpw);

  fgets(buf, sizeof(buf), fpr);    /* 加入標題 */
  if (!str_ncmp(buf, "標", 2))    /* 如果有 header 才改 */
  {
    strcpy(buf, buf[2] == ' ' ? "標  題: " : "標題: ");
    strcat(buf, fhdr->title);
    strcat(buf, "\n");
  }
  fputs(buf, fpw);

  while(fgets(buf, sizeof(buf), fpr))  /* 加入其他 */
  {
    fputs(buf, fpw);
  }
  fclose(fpr);
  fclose(fpw);
  f_rm(tmpfile);
}


static int
post_title(xo)
  XO *xo;
{
  HDR *fhdr, mhdr;
  int pos, cur;

  if (!cuser.userlevel)  /* itoc.000213: 避免 guest 在 sysop 板改標題 */
    return XO_NONE;

  if(post_is_system_board())
  {
    zmsg("此看板設定無法修改標題");
    return XO_FOOT;
  }

  pos = xo->pos;
  cur = pos - xo->top;
  fhdr = (HDR *) xo_pool + cur;

  if (fhdr->xmode & POST_BOTTOM)
  {
    vmsg("置底文無法修改標題 請取消置底後修改原文標題");
    return XO_FOOT;
  }

  memcpy(&mhdr, fhdr, sizeof(HDR));

  if (strcmp(cuser.userid, mhdr.owner) && !(bbstate & STAT_BOARD))
    return XO_NONE;

  vget(b_lines, 0, "標題：", mhdr.title, TTLEN + 1, GCARRY);

  if (HAS_PERM(PERM_ALLBOARD))  /* itoc.000213: 原作者只能改標題 */
  {
    vget(b_lines, 0, "作者：", mhdr.owner, 73 /* sizeof(mhdr.owner) */, GCARRY);
    /* Thor.980727: sizeof(mhdr.owner) = 80 會超過一行 */
    vget(b_lines, 0, "暱稱：", mhdr.nick, sizeof(mhdr.nick), GCARRY);
    vget(b_lines, 0, "日期：", mhdr.date, sizeof(mhdr.date), GCARRY);
  }

  if (memcmp(fhdr, &mhdr, sizeof(HDR)) && vans(msg_sure_ny) == 'y')
  {
    memcpy(fhdr, &mhdr, sizeof(HDR));
    currchrono = fhdr->chrono;
    rec_put(xo->dir, fhdr, sizeof(HDR), xo->key == XZ_XPOST ? fhdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(++pos, fhdr);

    /* itoc.010709: 修改文章標題順便修改內文的標題 */
    header_replace(xo, fhdr);
  }
  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* 額外功能：write / score                               */
/* ----------------------------------------------------- */

static int
post_show_config(xo)
  XO *xo;
{
  char *blist;
  int bno;

  bno = brd_bno(currboard);
  blist = (bshm->bcache + bno)->BM;

  grayout(GRAYOUT_DARK);

  move(b_lines - 10, 0);
  clrtobot();
  prints("\033[36m"); prints(MSG_SEPERATOR); prints("\033[m");
  prints("\n目前看板:[%s]  板主:[%s]\n", currboard, blist[0] > ' ' ? blist : "徵求中");
  prints("\n 是否轉信 - %s", (currbattr & BRD_NOTRAN) ? "站內看板" : "轉信看板 轉信看板預設為跨站貼文");
  prints("\n 可否推文 - %s", (currbattr & BRD_NOSCORE) ? "不可評分" : "開放推文");
  prints("\n 記錄篇數 - %s", (currbattr & BRD_NOCOUNT) ? "不做記錄" : "記錄");
  prints("\n 熱門話題 - %s", (currbattr & BRD_NOSTAT) ? "不做統計" : "統計資料記錄於熱門話題");
  prints("\n 投票結果 - %s", (currbattr & BRD_NOVOTE) ? "不做記錄" : "將記錄結果於[Record]看板");

  if (cuser.userlevel)
    prints("\n\n您目前%s此看板的管理者", ((cuser.userlevel & PERM_BM) && blist[0] > ' ' && is_bm(blist, cuser.userid)) ? "是" : "不是");
  else
    prints("\n\n您目前身分為guest訪客");
  vmsg(NULL);

  return XO_HEAD;
}

int
post_write(xo)      /* itoc.010328: 丟線上作者水球 */
  XO *xo;
{
  if (HAS_PERM(PERM_PAGE))
  {
    HDR *hdr;
    UTMP *up;

    hdr = (HDR *) xo_pool + (xo->pos - xo->top);

    if (!(hdr->xmode & POST_INCOME) && (up = utmp_seek(hdr)))
      do_write(up);
  }
  return XO_NONE;
}


#ifdef HAVE_SCORE

static int curraddscore;

static void
addscore(hdd, ram)
  HDR *hdd, *ram;
{
  if (curraddscore)
  {
    hdd->xmode |= POST_SCORE;
    hdd->score += curraddscore;
  }
  hdd->stamp = ram->stamp;
}

/* guessi.090910 重新撰寫推文程式post_score() */
int
post_score(xo)
  XO *xo;
{
  HDR *hdr;
  FILE *fp;
  int ans, maxlen;
  char *userid, *verb, fpath[64];
  char preview[32], preview2[32], reason[64], reason2[64];
#ifdef HAVE_ANONYMOUS
  char anony[IDLEN + 1];
#endif
  time_t now;
  struct tm *ptime;

  /* 推文間隔限制 */
  static time_t next = 0;

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);
  hdr_fpath(fpath, xo->dir, hdr);

  /* 評分限制同發文限制 */
  if ((currbattr & BRD_NOSCORE) || !cuser.userlevel || !(bbstate & STAT_POST))
    return XO_NONE;

#ifdef HAVE_REFUSEMARK
  /* 限制閱讀 */
  if ((hdr->xmode & POST_RESTRICT) && strcmp(hdr->owner, cuser.userid) && !(bbstate & STAT_BM))
    return XO_NONE;
#endif

#ifdef HAVE_ANONYMOUS
  /* 匿名推文 */
  if (currbattr & BRD_ANONYMOUS)
  {
    userid = uid;
    /* 若匿名則於匿名後方附加"."以示區隔 */
    if (!vget(b_lines, 0, "請輸入欲使用的匿名，或按[ENTER]跳過此步驟", anony, IDLEN, DOECHO))
      userid = cuser.userid;
    else
      strcat(userid, ".");
  }
  else
    userid = cuser.userid;
#else
  userid = cuser.userid;
#endif

  /* 計算推文中可輸入的文字數量(半形字) */
  maxlen = 63 - strlen(userid);

  move(b_lines - 3, 0);
  clrtoeol();
  prints(MSG_SEPERATOR);

  move(b_lines - 2, 0);
  clrtoeol();
  prints("\033[1;30m◎回應文章 (至多可同時送出二行連續無中斷文字，時間紀錄為實際送出時間)\033[m\n");

  move(b_lines - 1, 0);
  clrtoeol();

  move(b_lines, 0);
  clrtoeol();

  /* guessi.110514 推文間格時間限制 0~3秒內不可再作推文動作 4~5秒內改用"→" 5秒以上恢復正常 */
  if ((next - time(NULL)) <= 0)
  {
    move(b_lines - 1, 0);
    prints("\033[1;37m◎選擇評價 \033[31m1)值得推薦 \033[32m2)給個呸字 \033[37m3)普普通通？[3] \033[m");
    ans = vkey();
  }

  switch (ans)
  {
	case '1':
	  verb = "31m推";
	  sprintf(preview, "推 %s:", userid);
	  break;
	case '2':
	  verb = "32m呸";
	  sprintf(preview, "呸 %s:", userid);
	  break;
	default:
	  verb = "1;31m→";
	  sprintf(preview, "→ %s:", userid);
	  break;
  }
  sprintf(preview2, "→ %s:", userid);

  /* 請求使用者輸入推文內容(第一行) */
  if (!vget(b_lines - 1, 0, preview, reason, maxlen, DOECHO | CTECHO))
    return XO_HEAD;

  vget(b_lines, 54, "接續/送出/取消[c/y/N]:", preview, 2, LCECHO);

  /* 輸入完成後才取時間 */
  time(&now);
  ptime = localtime(&now);

  /* 清除 b_lines - 1 行內容 顯示輸出預覽 */
  move(b_lines - 1, 0);
  clrtoeol();
  prints("\033[%s\033[m \033[1m%s\033[;33m:%-*s \033[35m%02d/%02d %02d:%02d\033[m\n",
  verb, userid, 62 - strlen(userid), reason, ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min);

  /* 檢查使用者輸入是否如同預期 */
  if (*preview != 'c' && *preview != 'y')
    return XO_HEAD;

  /* 請求使用者輸入推文內容(第二行) */
  if (*preview == 'c')
  {
    /* 第二行文字可省略不輸入 故不做檢查 */
    vget(b_lines, 0, preview2, reason2, maxlen, DOECHO | CTECHO);
    /* 詢問使用者是否確認送出文字 */
    vget(b_lines, 68, "送出[y/N]:", preview2, 2, LCECHO);

    if (*preview2 != 'y')
	  return XO_HEAD;
  }

  if (fp = fopen(fpath, "a"))
  {
    /* 實際送出推文時再次取得確切時間 */
    time(&now);
    ptime = localtime(&now);

    fprintf(fp, "\033[%s\033[m \033[1m%s\033[;33m:%-*s \033[35m%02d/%02d %02d:%02d\033[m\n",
    verb, userid, 62 - strlen(userid), reason, ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min);

	/* 當存在有第二行文字輸入時 */
	if (*preview == 'c')
	{
	  if (strlen(reason2) > 0)
	  {
            fprintf(fp, "\033[%s\033[m \033[1m%s\033[;33m:%-*s \033[35m%02d/%02d %02d:%02d\033[m\n",
            "1;31m→", userid, 62 - strlen(userid), reason2, ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min);
	  }
	  else
	    vmsg("第二行推文無輸入文字，將不列入記錄");
    }

    fclose(fp);
  }

  /* 設定推文間隔限制 */
  next = time(NULL) + 5;

  /* 設定評分數值偏移量 */
  curraddscore = ((ans - '0') == 1 && hdr->score < 99) ? 1 : (((ans - '0') == 2 && hdr->score > -99) ? -1 : 0);

  /* 當評分數值有偏移時"或"選擇僅接話 更新user read history */
  if (curraddscore != 0 || (ans - '0') == 3)
  {
    change_stamp(xo->dir, hdr);
    currchrono = hdr->chrono;
    rec_ref(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : xo->pos, cmpchrono, addscore);
    post_history(xo, hdr);
    /*
      btime_update(currbno);
    */
    return XO_LOAD;
  }

  return XO_HEAD;
}

/* guessi.060323 重置評分數值 */
static int
post_resetscore(xo)
  XO *xo;
{
  /* 檢查是否擁有板主以上權限 */
  if (bbstate & STAT_BOARD)
  {
    FILE *fp;
    HDR *hdr;
    time_t now;
    struct tm *ptime;
    int ans, pos, cur, mp;
    char score[3], fpath[64];

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;
    hdr_fpath(fpath, xo->dir, hdr);

    ans = vans("◎評分設定 1)自訂分數 2)分數歸零 [Q] ");
    
    if (ans == '1')
    {
      mp = vans("◎ 1)正值 2)負值 [1] ");

      if (!vget(b_lines, 0, "請輸入數字(不含正負符號)：", score, 3, DOECHO))
        return XO_FOOT;

      /* guessi.111026 排除數值超出範圍的狀況 */
      if (atoi(score) < 0 || atoi(score) > 99)
      {
        vmsg("輸入數值區間不合法");
        return XO_FOOT;
      }

      /* guessi.060323 無論是否曾被評分 均給予POST_SCORE標記 */
      hdr->xmode |= POST_SCORE;
      hdr->score = mp == '2' ? (atoi(score) * -1) : atoi(score);
      currchrono = hdr->chrono;
    }
    else if (ans == '2')
    {
      /* guessi.060323 將分數歸零 並去除POST_SCORE標記 */
      hdr->xmode &= ~POST_SCORE;
      hdr->score = 0;
      currchrono = hdr->chrono;
    }
    else
    {
      return XO_FOOT;
    }

    /* guessi.111026 於文章末端附加重置分數資訊 */
    if (fp = fopen(fpath, "a"))
    {
      time(&now);
      ptime = localtime(&now);

      fprintf(fp, "\033[1;30m※ 系統紀錄: 文章評分由 %s 設定重置%-*s%02d/%02d %02d:%02d\033[m\n",
              cuser.userid, 34 - strlen(cuser.userid), "", ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min);

      fclose(fp);
    }

    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);
    move(3 + cur, 7);
    outc(post_attr(hdr));
  }
  return XO_BODY;
}

#endif /* HAVE_SCORE */

static int
post_state(xo)
  XO *xo;
{
  HDR *hdr;
  char fpath[64], *dir;
  struct stat st;

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);
  dir = xo->dir;
  hdr_fpath(fpath, dir, hdr);

  grayout(GRAYOUT_DARK);

  move(b_lines - 7, 0);
  clrtobot();
  outs(msg_seperator);
  outs("\nDir : ");
  outs(dir);
  outs("\nName: ");
  outs(hdr->xname);
  outs("\nFile: ");
  outs(fpath);
  if (!stat(fpath, &st))
    prints("\nTime: %s\nSize: %d", Btime(&st.st_mtime), st.st_size);
  vmsg(NULL);

  return XO_HEAD;
}


/* guessi.090610 顯示文章代碼(AID) */
int
post_aid_query(xo)
  XO *xo;
{
  screenline slp[T_LINES];
  char buf[ANSILINELEN];
  HDR *hdr;
  int cur;
  usint readlevel;

  readlevel = (bshm->bcache + brd_bno(currboard))->readlevel;

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);
  cur = xo->pos - xo->top + 3;

  vs_save(slp);
  str_ansi(buf, slp[cur].data, slp[cur].width + 1);

  grayout(GRAYOUT_DARK);

  if (cur <= 15)
  {
    move(cur- 1, 0);
    clrtoeol();
    prints("\n\033[1;37;40m%s\033[m\n", buf);
    move(cur + 1, 0);
  }
  else
  {
    move(cur - 5, 0);
    clrtoeol();
    prints("\n");
  }

  prints("┌─────────────────────────────────────┐\n");
  prints("│文章代碼(AID): #%s (%s)%-*s文章評分: %-2d│\n", hdr->xname, currboard, 35 - strlen(currboard), "", hdr->score);
  prints("│發文時間: %.24s%-*s│\n", ctime(&hdr->chrono), 65 - strlen(ctime(&hdr->chrono)), "");
#ifdef HAVE_PERMANENT_LINK
  /* guessi.120728 若看板為隱藏則不顯示網址連結 */
  if (!(readlevel & PERM_SYSOP || readlevel & PERM_BOARD))
  {
#   define FQDNLINK "http://" MYHOSTNAME "/plink?%s&%s"
    prints("│永久連結: " FQDNLINK "%-*s│\n", currboard, hdr->xname, 34 - strlen(currboard) - strlen(hdr->xname), "");
  }
#endif
  prints("└─────────────────────────────────────┘\n");

  if (cur > 15)
    prints("\033[1;37;40m%s\033[m\n", buf);

  clrtoeol();

  vmsg(NULL);

  return XO_HEAD;
}

/* guessi.090610 根據文章代碼(AID)找尋文章 */
static int
post_aid_search(xo)
XO *xo;
{
  char *tag, *query, aid[9];
  int currpos, pos, max, match = 0;
  HDR *hdr;

  /* 保存目前所在的位置 */
  currpos = xo->pos;
  /* 紀錄看板文章總數；新貼文AID是未知的不可能在其中 */
  max = xo->max;

  /* 若沒有文章或其他例外狀況 */
  if (max <= 0)
    return XO_FOOT;

  /* 請求使用者輸入文章代碼(AID) */
  if (!vget(b_lines, 0, "請輸入文章代碼(AID)： #", aid, sizeof(aid), DOECHO))
    return XO_FOOT;
  query = aid;

  for (pos = 0; pos < max; pos++)
  {
    xo->pos = pos;
    xo_load(xo, sizeof(HDR));
    /* 設定HDR資訊 */
    hdr = (HDR *) xo_pool + (xo->pos - xo->top);
    tag = hdr->xname;
    /* 若找到對應的文章，則設定match並跳出 */
    if (!strcmp(query, tag))
    {
      match = 1;
      break;
    }
  }

  /* 沒找到則恢復xo->pos並顯示提示文字 */
  if (!match)
  {
    zmsg("找不到文章，是不是找錯看板了呢？");
    xo->pos = currpos;  /* 恢復xo->pos紀錄 */
  }

  return post_load(xo);
}

static int
post_help(xo)
XO *xo;
{
  xo_help("post");
  /* return post_head(xo); */
  return XO_HEAD;    /* itoc.001029: 與 xpost_help 共用 */
}


KeyFunc post_cb[] =
{
  XO_INIT, post_init,
  XO_LOAD, post_load,
  XO_HEAD, post_head,
  XO_BODY, post_body,

  'r', post_browse,
  's', post_switch,
  KEY_TAB, post_gem,
  'z', post_gem,

  'y', post_reply,
  'd', post_delete,
  'v', post_visit,
  'x', post_cross,    /* 在 post/mbox 中都是小寫 x 轉看板，大寫 Z 轉使用者 */
  't', post_tag,
  'E', post_edit,
  'T', post_title,
  'm', post_mark,
  '_', post_bottom,
  'D', post_rangedel,
  'b', post_memo,

#ifdef HAVE_SCORE
  'X', post_score,
  '%', post_score,
  'Y', post_resetscore,
#endif

  'I', post_show_config,
  'w', post_write,

  'c', post_copy,
  'g', gem_gather,

  'M', depart_post,

  Ctrl('P'), post_add,
  Ctrl('D'), post_prune,
  Ctrl('Q'), xo_uquery,
  Ctrl('B'), post_digest,
  Ctrl('S'), post_done_mark,
  Ctrl('E'), xo_usetup,
  Ctrl('F'), post_state,
  Ctrl('G'), post_remove_mark,

#ifdef HAVE_REFUSEMARK
  Ctrl('Y'), post_refuse,
#endif

#ifdef HAVE_LABELMARK
  'n', post_label,
  Ctrl('N'), post_delabel,
#endif

  'B' | XO_DL, (void *) "bin/manage.so:post_manage",
  'R' | XO_DL, (void *) "bin/vote.so:vote_result",
  'V' | XO_DL, (void *) "bin/vote.so:XoVote",

#ifdef HAVE_TERMINATOR
  Ctrl('X') | XO_DL, (void *) "bin/manage.so:post_terminator",
#endif


  '~', XoXselect,         /* itoc.001220: 搜尋作者/標題 */
  'S', XoXsearch,         /* itoc.001220: 搜尋相同標題文章 */
  'a', XoXauthor,         /* itoc.001220: 搜尋作者 */
  '/', XoXtitle,          /* itoc.001220: 搜尋標題 */
  'f', XoXfull,           /* itoc.030608: 全文搜尋 */
  'G', XoXmark,           /* itoc.010325: 搜尋 mark 文章 */
  'U', XoXdigest,         /* 搜尋文摘 */
  'L', XoXlocal,          /* itoc.010822: 搜尋本地文章 */

#ifdef HAVE_SCORE
  'Z', XoXscore,          /* guessi.120319 搜尋評分文章 */
#endif

#ifdef HAVE_XYNEWS
  'u', XoNews,            /* itoc.010822: 新聞閱讀模式 */
#endif
  '#', post_aid_search,   /* guessi.090610 以文章代碼(AID)快速尋文*/
  'W', post_aid_query,    /* guessi.090610 查詢文章代碼(AID) */

  'h', post_help
};


KeyFunc xpost_cb[] =
{
  XO_INIT, xpost_init,
  XO_LOAD, xpost_load,
  XO_HEAD, xpost_head,
  XO_BODY, post_body,     /* Thor.980911: 共用即可 */

  'r', xpost_browse,
  'y', post_reply,
  't', post_tag,
  'x', post_cross,
  'c', post_copy,
  'g', gem_gather,
  'm', post_mark,
  'd', post_delete,     /* Thor.980911: 方便板主 */
  'E', post_edit,       /* itoc.010716: 提供 XPOST 中可以編輯標題、文章，加密 */
  'T', post_title,
#ifdef HAVE_SCORE
  'X', post_score,
  '%', post_score,
  'Y', post_resetscore,
#endif
  'I', post_show_config,
  'w', post_write,
#ifdef HAVE_REFUSEMARK
  Ctrl('Y'), post_refuse,
#endif
#ifdef HAVE_LABELMARK
  'n', post_label,
#endif

  Ctrl('P'), post_add,
  Ctrl('D'), post_prune,
  Ctrl('Q'), xo_uquery,
  Ctrl('S'), post_done_mark,
  Ctrl('B'), post_digest,
  Ctrl('E'), xo_usetup,

  '~', XoXselect,
  'S', XoXsearch,
  'a', XoXauthor,
  '/', XoXtitle,
  'f', XoXfull,
  'G', XoXmark,
  'U', XoXdigest,
  'L', XoXlocal,     /* itoc.060206: 搜尋後可再進階搜尋 */

#ifdef HAVE_SCORE
  'Z', XoXscore,          /* guessi.120319 搜尋評分文章 */
#endif

  'h', post_help     /* itoc.030511: 共用即可 */
};


#ifdef HAVE_XYNEWS
KeyFunc news_cb[] =
{
  XO_INIT, news_init,
  XO_LOAD, news_load,
  XO_HEAD, news_head,
  XO_BODY, post_body,

  'r', XoXsearch,

  'h', post_help    /* itoc.030511: 共用即可 */
};
#endif  /* HAVE_XYNEWS */
