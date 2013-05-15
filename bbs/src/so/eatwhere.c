/*-------------------------------------------------------*/
/* target : 要吃哪裡 (Where To Eat)                      */
/* create : 10.05.21                                     */
/* update : 11.08.21                                     */
/* author : guessi.bbs@bbs.ndhu.edu.tw                   */
/*-------------------------------------------------------*/
/* run/wtoeat/_/.DIR - control header                    */
/* run/wtoeat/_/@/@_ - description file                  */
/* run/wtoeat/_/@/G_ - log file                          */
/*-------------------------------------------------------*/

#include "bbs.h"

extern int TagNum;
extern XZ xz[];
extern char xo_pool[];

static int wte_add();
static int wte_body();
static int wte_head();

static int
cmpbtime(wte)
  WTE *wte;
{
  return wte->btime == currchrono;
}


static char
wte_attr(wte)
  WTE *wte;
{
  if (wte->mode & WTE_MARKED)
    return 'm';

  return ' ';
}


static int
wte_stamp(folder, wte, fpath)
  char *folder;
  WTE *wte;
  char *fpath;
{
  char *fname;
  char *family = NULL;
  int rc;
  int token;

  fname = fpath;
  while (rc = *folder++)
  {
    *fname++ = rc;
    if (rc == '/')
      family = fname;
  }

  fname = family;
  *family++ = '@';

  token = time(0);

  archiv32(token, family);

  rc = open(fpath, O_WRONLY | O_CREAT | O_EXCL, 0600);
  wte->btime = token;
  str_stamp(wte->date, &wte->btime);
  strcpy(wte->xname, fname);

  return rc;
}


static void
wte_fpath(fpath, folder, wte)
  char *fpath;
  char *folder;
  WTE *wte;
{
  char *str;
  int cc;

  while (cc = *folder++)
  {
    *fpath++ = cc;
    if (cc == '/')
      str = fpath;
  }
  strcpy(str, wte->xname);
}


static int
wte_init(xo)
  XO *xo;
{
  xo_load(xo, sizeof(WTE));
  return wte_head(xo);
}

static int
wte_load(xo)
  XO *xo;
{
  xo_load(xo, sizeof(WTE));
  return wte_body(xo);
}

int
tag_char(chrono)
  int chrono;
{
  return TagNum && !Tagger(chrono, 0, TAG_NIN) ? '*' : ' ';
}

static void
wte_item(num, wte)
  int num;
  WTE *wte;
{
  prints("%6d %s%c\033[m%c%-5s %-13s [\033[1;37m%s\033[m] %s\n",
         num, (wte->mode & WTE_MARKED) ? "\033[1;36m" : "", wte_attr(wte),
         tag_char(wte->btime), wte->date + 3, wte->owner, wte->class, wte->title);
}

static int
wte_body(xo)
  XO *xo;
{
  WTE *wte;
  int num, max, tail;

  max = xo->max;

  if (max <= 0)
  {
    if (vans("要新增美食資訊嗎(Y/N)？[N] ") == 'y')
      return wte_add(xo);
    return XO_QUIT;
  }

  wte = (WTE *) xo_pool;
  num = xo->top;
  tail = num + XO_TALL;

  if (max > tail)
    max = tail;

  move(3, 0);
  do
  {
    wte_item(++num, wte++);
  } while (num < max);
  clrtobot();

  return XO_FOOT;
}


static int
wte_head(xo)
  XO *xo;
{
  vs_head("美食天地", str_site);
  prints(NECKER_TOEAT, d_cols, "");
  return wte_body(xo);
}

static int
wte_tag(xo)
  XO *xo;
{
  WTE *wte;
  int tag, pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  wte = (WTE *) xo_pool + cur;

  if (tag = Tagger(wte->btime, pos, TAG_TOGGLE))
  {
    move(3 + cur, 0);
    wte_item(pos + 1, wte);
  }

  /* return XO_NONE; */
  return xo->pos + 1 + XO_MOVE; /* lkchu.981201: 跳至下一項 */
}

static int
wte_memo_edit(xo)
  XO *xo;
{
  int mode;
  char fpath[64];

  if (!cuser.userlevel)
    return XO_NONE;

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  mode = vans("進板畫面 (D)刪除 (E)修改 (Q)取消？[E] ");

  if (mode != 'q')
  {
    sprintf(fpath, "run/wtoeat/%s", fn_note);

    if (mode == 'd')
      unlink(fpath);

    if (vedit(fpath, 0))  /* Thor.981020: 注意被talk的問題 */
      vmsg(msg_cancel);
  }
  return XO_HEAD;
}

static int
wte_memo(xo)
  XO *xo;
{
  char fpath[64];

  sprintf(fpath, "run/wtoeat/%s", fn_note);

  if (more(fpath, NULL) < 0)
  {
    vmsg("目前沒有進板畫面唷");
    return XO_FOOT;
  }

  return XO_HEAD;
}


static int
wte_mark(xo)
  XO *xo;
{
  WTE *wte;

  if (!cuser.userlevel)
    return XO_NONE;

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  wte = (WTE *) xo_pool + (xo->pos - xo->top);

  wte->mode ^= WTE_MARKED;
  currchrono = wte->btime;
  rec_put(xo->dir, wte, sizeof(WTE), xo->pos, cmpbtime);

  move(xo->pos - xo->top + 3, 0);
  wte_item(xo->pos + 1, wte);

  return XO_NONE;
}

#define NUM_PREFIX 7

static int
wte_add(xo)
  XO *xo;
{
  WTE wte;
  FILE *fp;
  int fd, ans;
  char *dir, fpath[64], path[64];
  char *class, *title, *address, *phone, *during, *description;
  char *prefix[NUM_PREFIX] = {"志學", "壽豐", "市區", "早點", "點心", "宵夜", "其他"};

  if (!cuser.userlevel)
    return XO_NONE;

  /* 沒有發文權限的使用者將無法使用本功能 */
  if (!HAS_PERM(PERM_POST))
    return XO_FOOT;

  memset(&wte, 0, sizeof(WTE));

  class = wte.class;
  title = wte.title;
  address = wte.address;
  phone = wte.phone;
  during = wte.during;
  description = wte.description;

  clear();
  film_out(FILM_POST, 0);

  move(b_lines - 5, 0);
  prints("美食資訊\n");
  outs("類別：");
  for (ans = 0; ans < NUM_PREFIX; ans++)
     prints("%d.[%s] ", ans + 1, prefix[ans]);
  ans = vget(b_lines - 3, 0, "請選擇美食分類（按 Enter 跳過）：", fpath, 3, DOECHO) - '1';

  if (ans < 0 || ans >= NUM_PREFIX)   /* 輸入數字選項 */
    ans = 0;

  strcpy(class, prefix[ans]);


  if (!vget(b_lines - 2, 0, "商家名稱：", title, sizeof(wte.title), DOECHO) &&
      !vget(b_lines - 1, 0, "商家地址：", address, sizeof(wte.address), DOECHO) &&
      !vget(b_lines, 0, "聯絡電話：", phone, sizeof(wte.phone), DOECHO) &&
      !vget(b_lines, 0, "營業時間：", during, sizeof(wte.during), DOECHO) &&
      !vget(b_lines, 0, "商家簡述：", description, sizeof(wte.description), DOECHO))
  {
    vmsg(msg_cancel);
    return wte_head(xo);
  }

  sprintf(path, "tmp/%s.wte", cuser.userid);  /* 連署原因的暫存檔案 */
  if (fd = vedit(path, 0))
  {
    unlink(path);
    vmsg(msg_cancel);
    return wte_head(xo);
  }

  dir = xo->dir;
  if ((fd = wte_stamp(dir, &wte, fpath)) < 0)
    return wte_head(xo);
  close(fd);

  strcpy(wte.owner, cuser.userid);

  fp = fopen(fpath, "a");
  /* 檔頭文字 格式固定 */
  fprintf(fp, "作者: %s (%s) 站內: 美食推薦\n", cuser.userid, cuser.username);
  fprintf(fp, "標題: %s\n", wte.title);
  fprintf(fp, "時間: %s\n\n", Now());
  /* 內容文字 格式自訂 */
  fprintf(fp, "美食分類：%s\n", wte.class);
  fprintf(fp, "商家名稱：%s\n", wte.title);
  fprintf(fp, "地    址：%s\n", wte.address);
  fprintf(fp, "聯絡電話：%s\n", wte.phone);
  fprintf(fp, "營業時間：%s\n", wte.during);
  fprintf(fp, "商家簡述：%s\n", wte.description);
  fprintf(fp, "張貼日期：%s\n", wte.date);
  fprintf(fp, "--\n");
  fprintf(fp, "詳細描述：\n");
  f_suck(fp, path);
  unlink(path);
  fclose(fp);

  rec_add(dir, &wte, sizeof(WTE));

  move(0, 0);
  clrtobot();
  outs("順利貼出文章，謝謝你的分享！");
  vmsg(NULL);

  /* 新增後，跳到該項目(最後一項) */
  xo->pos = xo->max;

  return XO_INIT;
}


static int
wte_browse(xo)
  XO *xo;
{
  int key;
  WTE *wte;
  char fpath[80];

  /* itoc.010304: 為了讓閱讀到一半也可以加入連署，考慮 more 傳回值 */
  for (;;)
  {
    wte = (WTE *) xo_pool + (xo->pos - xo->top);
    wte_fpath(fpath, xo->dir, wte);

    if ((key = more(fpath, FOOTER_TOEAT)) < 0)
      break;

    if (!key)
      key = vkey();

    switch (key)
    {
      case KEY_UP:
      case KEY_PGUP:
      case '[':
      case 'j':
        key = xo->pos - 1;

        if (key < 0)
        break;

        xo->pos = key;

        if (key <= xo->top)
        {
          xo->top = (key / XO_TALL) * XO_TALL;
          wte_load(xo);
        }
        continue;

      case KEY_DOWN:
      case KEY_PGDN:
      case ']':
      case 'k':
      case ' ':
        key = xo->pos + 1;

        if (key >= xo->max)
          break;

        xo->pos = key;

        if (key >= xo->top + XO_TALL)
        {
          xo->top = (key / XO_TALL) * XO_TALL;
          wte_load(xo);
        }
        continue;

      case 'h':
        xo_help("wtoeat");
        break;
    }
    break;
  }

  return wte_head(xo);
}


static int
wte_delete(xo)
  XO *xo;
{
  WTE *wte;
  char *fname, fpath[80];
  char *list = "@G";    /* itoc.註解: 清 newbrd file */

  wte = (WTE *) xo_pool + (xo->pos - xo->top);

  if (!cuser.userlevel)
    return XO_NONE;

  if (wte->mode & WTE_MARKED)
    return XO_NONE;

  if (strcmp(cuser.userid, wte->owner) && !HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  if (vans(msg_del_ny) != 'y')
    return XO_FOOT;

  wte_fpath(fpath, xo->dir, wte);
  fname = strrchr(fpath, '@');
  while (*fname = *list++)
  {
    unlink(fpath);  /* Thor: 確定名字就砍 */
  }

  currchrono = wte->btime;
  rec_del(xo->dir, sizeof(WTE), xo->pos, cmpbtime);
  return wte_init(xo);
}



void
update_content(xo, wte)  /* itoc.010709: 修改文章標題順便修改內文的標題 */
  XO *xo;
  WTE *wte;
{
  FILE *fpr, *fpw;
  char srcfile[64], tmpfile[64], buf[ANSILINELEN];

  wte_fpath(srcfile, xo->dir, wte);
  strcpy(tmpfile, "tmp/");
  strcat(tmpfile, wte->xname);

  f_cp(srcfile, tmpfile, O_TRUNC);

  if (!(fpr = fopen(tmpfile, "r")))
    return;

  if (!(fpw = fopen(srcfile, "w")))
  {
    fclose(fpr);
    return;
  }

  while(fgets(buf, sizeof(buf), fpr)) /* 加入其他 */
  {
    if (!str_ncmp(buf, "標題:", 5))
      sprintf(buf, "標題: [%s] %s\n", wte->class, wte->title);

    if (!str_ncmp(buf, "美食分類", 8))
      sprintf(buf, "美食分類：%s\n", wte->class);

    if (!str_ncmp(buf, "商家名稱", 8))
      sprintf(buf, "商家名稱：%s\n", wte->title);

    if (!str_ncmp(buf, "地    址", 8))
      sprintf(buf, "地    址：%s\n", wte->address);

    if (!str_ncmp(buf, "聯絡電話", 8))
      sprintf(buf, "聯絡電話：%s\n", wte->phone);

    if (!str_ncmp(buf, "營業時間", 8))
      sprintf(buf, "營業時間：%s\n", wte->during);

    if (!str_ncmp(buf, "商家簡述", 8))
      sprintf(buf, "商家簡述：%s\n", wte->description);

    fputs(buf, fpw);
  }
  fclose(fpr);
  fclose(fpw);
  f_rm(tmpfile);
}


static int
wte_edit(xo)
  XO *xo;
{
  WTE *wte, xwte;
  char ans[6];

  if (!cuser.userlevel)
    return XO_NONE;

  vs_bar("詳細設定");

  wte = (WTE *) xo_pool + (xo->pos - xo->top);
  memcpy(&xwte, wte, sizeof(WTE));

  prints("\n");
  prints("美食分類：%s\n", xwte.class);
  prints("商家名稱：%s\n", xwte.title);
  prints("地    址：%s\n", xwte.address);
  prints("聯絡電話：%s\n", xwte.phone);
  prints("營業時間：%s\n", xwte.during);
  prints("商家簡述：%s\n", xwte.description);

  if (vget(9, 0, "是否要修改這筆資料 (E)設定 (Q)取消？[Q] ", ans, 3, LCECHO) == 'e')
  {
    /* 最後修改的使用者變更為擁有者 */
    strcpy(xwte.owner, cuser.userid);
    vget(11, 0, "美食分類：", xwte.class, sizeof(xwte.class), GCARRY);
    vget(12, 0, "商家名稱：", xwte.title, sizeof(xwte.title), GCARRY);
    vget(13, 0, "地    址：", xwte.address, sizeof(xwte.address), GCARRY);
    vget(14, 0, "聯絡電話：", xwte.phone, sizeof(xwte.phone), GCARRY);
    vget(15, 0, "營業時間：", xwte.during, sizeof(xwte.during), GCARRY);
    vget(16, 0, "商家簡述：", xwte.description, sizeof(xwte.description), GCARRY);

    move(b_lines - 2, 0);
    prints("若欲商家詳細介紹，請按[\033[1;35m大寫S\033[m]設定\n");

    if (memcmp(&xwte, wte, sizeof(WTE)) &&
        vans("請確認是否修改？ (Y/N)[N] ") == 'y')
    {
      memcpy(wte, &xwte, sizeof(WTE));
      currchrono = wte->btime;
      rec_put(xo->dir, wte, sizeof(WTE), xo->pos, cmpbtime);
      update_content(xo, wte);
    }
  }

  return wte_init(xo);
}

static int
wte_setup(xo)
  XO *xo;
{
  char fpath[64];
  WTE *wte;

  wte = (WTE *) xo_pool + (xo->pos - xo->top);

  if (!cuser.userlevel)
    return XO_NONE;

  wte_fpath(fpath, xo->dir, wte);
  vedit(fpath, 0);

  strcpy(wte->owner, cuser.userid);
  currchrono = wte->btime;
  rec_put(xo->dir, wte, sizeof(WTE), xo->pos, cmpbtime);

  return wte_head(xo);
}

void
update_title(xo, wte)  /* itoc.010709: 修改文章標題順便修改內文的標題 */
  XO *xo;
  WTE *wte;
{
  FILE *fpr, *fpw;
  char srcfile[64], tmpfile[64], buf[ANSILINELEN];

  wte_fpath(srcfile, xo->dir, wte);
  strcpy(tmpfile, "tmp/");
  strcat(tmpfile, wte->xname);

  f_cp(srcfile, tmpfile, O_TRUNC);

  if (!(fpr = fopen(tmpfile, "r")))
    return;

  if (!(fpw = fopen(srcfile, "w")))
  {
    fclose(fpr);
    return;
  }

  fgets(buf, sizeof(buf), fpr);   /* 加入作者 */
  fputs(buf, fpw);

  fgets(buf, sizeof(buf), fpr);   /* 加入標題 */
  if (!str_ncmp(buf, "標", 2))    /* 如果有 header 才改 */
  {
    sprintf(buf, "標題: [%s] %s\n", wte->class, wte->title);
  }
  fputs(buf, fpw);

  while(fgets(buf, sizeof(buf), fpr)) /* 加入其他 */
    fputs(buf, fpw);

  fclose(fpr);
  fclose(fpw);
  f_rm(tmpfile);
}

static int
wte_title(xo)
  XO *xo;
{
  int ans;
  WTE *wte, xwte;
  wte = (WTE *) xo_pool + (xo->pos - xo->top);
  char *prefix[NUM_PREFIX] = {"美食", "志學", "壽豐", "市區", "早點", "宵夜", "其他"};

  if (!cuser.userlevel)
    return XO_NONE;

  memcpy(&xwte, wte, sizeof(WTE));

  move(b_lines - 2, 0);
  clrtobot();
  prints("\n");
  outs("類別：");
  for (ans = 0; ans < NUM_PREFIX; ans++)
    prints("%d.[%s] ", ans + 1, prefix[ans]);

  ans = vans("請選擇搜尋目標分類（按 Enter 跳過）： ") - '1';

  if (ans >= 0 && ans < NUM_PREFIX)   /* 輸入數字選項 */
    strcpy(xwte.class, prefix[ans]);
  else
    strcpy(xwte.class, prefix[0]);

  move(b_lines - 2, 0);
  clrtobot();

  if (!vget(b_lines, 0, "商家名稱：", xwte.title, sizeof(xwte.title), GCARRY))
    return XO_FOOT;

  if (memcmp(wte, &xwte, sizeof(WTE)) &&
      vans("請確認是否修改 (Y/N)[N] ") == 'y')
  {
    memcpy(wte, &xwte, sizeof(WTE));
    currchrono = wte->btime;
    rec_put(xo->dir, wte, sizeof(WTE), xo->pos, cmpbtime);
    update_title(xo, wte);
  }

  return wte_head(xo);
}

int
wte_recommend(xo)
  XO *xo;
{
  WTE *wte, *wtmp;
  int *list, i, ans, fsize, count;
  char type[BCLEN + 1], *fimage, *prefix[NUM_PREFIX] = {"美食", "志學", "壽豐", "市區", "早點", "宵夜", "其他"};

  /* 若在所有項目中有沒有指定的類別 就不找了 */
  fimage = f_map(xo->dir, &fsize);

  if (fimage == (char *) -1)
  {
    vmsg("目前無法開啟索引檔");
    return XO_FOOT;
  }

  count = 0;

  do {
    count = 0;

    move(b_lines - 2, 0);
    clrtobot();
    prints("\n");
    outs("類別：");
    for (ans = 0; ans < NUM_PREFIX; ans++)
      prints("%d.[%s] ", ans + 1, prefix[ans]);

    ans = vans("請選擇搜尋目標分類（按 Enter 跳過）： ") - '1';

    srand((unsigned) time(NULL));
    /* 若沒有指定分類 隨機挑一個印出 */
    if (ans > 0 && ans < NUM_PREFIX)
      strcpy(type, prefix[ans]);
    else
      strcpy(type, prefix[rand() % NUM_PREFIX]);

    /* 暫存分類搜尋結果 (至多有 xo->max 個) */
    list = (int *) malloc(sizeof(int) * xo->max);

    for (i = 0; i < xo->max; i++)
    {
      wtmp = (WTE *) fimage + i;
      if (!strcmp(wtmp->class, type))
        list[count++] = i;
    }

    if (count <= 0)
      vmsg("分類結果中無項目，請重新選擇分類");

  } while (count <= 0);

  do {
    /* 針對list紀錄的位置 隨機挑出一個 */
    srand((unsigned) time(NULL));
    xo->pos = list[rand() % count];
    xo_load(xo, sizeof(WTE));
    wte = (WTE *) xo_pool + (xo->pos - xo->top);

    /* 找到後將結果列印出 */
    move(b_lines - 8, 0);
    clrtobot();

    prints("\n┌─────────────────────────────────────┐\n" \
           "│美食分類：%s%*s│\n" \
           "│商家名稱：%s%*s│\n" \
           "│地    址：%s%*s│\n" \
           "│商家簡述：%s%*s│\n" \
           "└─────────────────────────────────────┘\n", \
           wte->class,       64 - strlen(wte->class),       "", \
           wte->title,       64 - strlen(wte->title),       "", \
           wte->address,     64 - strlen(wte->address),     "", \
           wte->description, 64 - strlen(wte->description), "");
  } while (vans("◎請選擇動作？ C)繼續 Q)退出 [C] ") != 'q');

  free(list);

  return XO_HEAD;
}


int
wte_write(xo)      /* itoc.010328: 丟線上作者水球 */
  XO *xo;
{
  if (HAS_PERM(PERM_PAGE))
  {
    WTE *wte;
    UTMP *up;

    wte = (WTE *) xo_pool + (xo->pos - xo->top);

    /* 能夠張貼的一定是站內使用者 */
    if (up = utmp_get(0, wte->owner))
      do_write(up);
  }
  return XO_NONE;
}

static int
wte_cmp(a, b)
  WTE *a;
  WTE *b;
{
  return str_cmp(a->class, b->class);
}

static int
wte_sort(xo)
  XO *xo;
{
  if (vans("◎是否重整排序？ Y)確認 N)取消 [N] ") != 'y')
    return XO_FOOT;

  rec_sync(xo->dir, sizeof(WTE), wte_cmp, NULL);

  return wte_load(xo);
}

static int
wte_uquery(xo)
  XO *xo;
{
  WTE *wte;

  wte = (WTE *) xo_pool + (xo->pos - xo->top);

  move(1, 0);
  clrtobot();
  my_query(wte->owner);
  return wte_head(xo);
}


static int
wte_help(xo)
  XO *xo;
{
  xo_help("wtoeat");
  return wte_head(xo);
}


static KeyFunc wte_cb[] =
{
  XO_INIT, wte_init,
  XO_LOAD, wte_load,
  XO_HEAD, wte_head,
  XO_BODY, wte_body,

  'r', wte_browse,
  'm', wte_mark,
  'd', wte_delete,
  'S', wte_setup,
  'E', wte_edit,
  'T', wte_title,
  'w', wte_write,
  't', wte_tag,
  'q', wte_recommend,
  'b', wte_memo,
  'B', wte_memo_edit,
  'a', wte_add,

  Ctrl('S'), wte_sort,
  Ctrl('P'), wte_add,
  Ctrl('Q'), wte_uquery,

  'h', wte_help
};


int
eat_where()
{
  XO *xo;
  char fpath[64];

  sprintf(fpath, "run/eatwhere/%s", fn_dir);
  xz[XZ_TOEAT - XO_ZONE].xo = xo = xo_new(fpath);
  xz[XZ_TOEAT - XO_ZONE].cb = wte_cb;
  xo->key = XZ_TOEAT;

  sprintf(fpath, "run/eatwhere/%s", fn_note);
  more(fpath, NULL);

  xover(XZ_TOEAT);
  free(xo);

  return 0;
}
