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


extern int wordsnum;    /* itoc.010408: �p��峹�r�� */
extern int TagNum;
extern char xo_pool[];
extern char brd_bits[];


#ifdef HAVE_ANONYMOUS
extern char anonymousid[];  /* itoc.010717: �۩w�ΦW ID */
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

  /* ���F�T�w�s�y�X�Ӫ� stamp �]�O unique (���M�J���� chrono ����)�A
  �N���ͤ@�ӷs���ɮסA���ɮ��H�K link �Y�i�C
  �o�Ӧh���ͥX�Ӫ��U���|�b expire �Q sync �� (�]�����b .DIR ��) */
  hdr_stamp(folder, HDR_LINK | 'A', &buf, "etc/stamp");
  hdr->stamp = buf.chrono;
}

/* ----------------------------------------------------- */
/* ��} innbbsd ��X�H��B�s�u��H���B�z�{��             */
/* ----------------------------------------------------- */


void
btime_update(bno)
  int bno;
{
  if (bno >= 0)
    (bshm->bcache + bno)->btime = -1;  /* �� class_item() ��s�� */
}


#ifndef HAVE_NETTOOL
static       /* �� enews.c �� */
#endif
void
outgo_post(hdr, board)
  HDR *hdr;
  char *board;
{
  bntp_t bntp;

  memset(&bntp, 0, sizeof(bntp_t));

  if (board)    /* �s�H */
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
  if ((hdr->xmode & POST_OUTGO) &&           /* �~��H��     */
      (hdr->chrono > ap_start - 7 * 86400))  /* 7 �Ѥ������� */
  {
    outgo_post(hdr, NULL);
  }
}


static inline int
move_post(hdr, folder, by_bm)  /* �N hdr �q folder �h��O���O */
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

  if (!(xmode & POST_BOTTOM)) /* �m����Q�夣�� move_post */
  {
#ifdef HAVE_REFUSEMARK
    board = by_bm && !(xmode & POST_RESTRICT) ? BN_DELETED : BN_JUNK;  /* �[�K�峹��h junk */
#else
    board = by_bm ? BN_DELETED : BN_JUNK;
#endif

    if ((bshm->bcache + currbno)->readlevel != PERM_SYSOP &&
        (bshm->bcache + currbno)->readlevel != PERM_ALLBOARD)
    {
      brd_fpath(fnew, board, fn_dir);
      hdr_stamp(fnew, HDR_LINK | 'A', &post, fpath);


      /* guessi.120603 ����O���峹�R���O�� */
      hdr_fpath(ffpath, (char *) fnew, &post);

      if (fp = fopen(ffpath, "a"))
      {
        time(&now);
        ptime = localtime(&now);

        if (!by_bm && !str_cmp(cuser.userid, hdr->owner))
        {
          fprintf(fp, "\033[1;30m�� �峹�ѧ@�̦ۦ�R��%-*s%02d/%02d %02d:%02d\033[m\n",
                      46, "", ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min);
        }
        else
        {
          fprintf(fp, "\033[1;30m�� �峹�Ѻ޲z��[%s]�R��%-*s%02d/%02d %02d:%02d\033[m\n",
                      cuser.userid, 46 - strlen(cuser.userid), "",
                      ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min);
        }

        fclose(fp);
      }

      /* �����ƻs trailing data�Gowner(�t)�H�U�Ҧ���� */
      memcpy(post.owner, hdr->owner, sizeof(HDR) -
      (sizeof(post.chrono) + sizeof(post.xmode) + sizeof(post.xid) + sizeof(post.xname)));

      if (!by_bm && !stat(fpath, &st))
        by_bm = st.st_size;

      rec_bot(fnew, &post, sizeof(HDR));
      btime_update(brd_bno(board));
    }
    else
    {
      /* ���K�ݪO�R�夣�e��delete/junk */
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
/* ��} cross post ���v                                  */
/* ----------------------------------------------------- */

#define MAX_CHECKSUM_POST  20  /* �O���̪� 20 �g�峹�� checksum */
#define MAX_CHECKSUM_LINE  6  /* �u���峹�e 6 ��Ӻ� checksum */

typedef struct
{
  int sum;      /* �峹�� checksum */
  int total;      /* ���峹�w�o��X�g */
} CHECKSUM;

static CHECKSUM checksum[MAX_CHECKSUM_POST];
static int checknum = 0;

static inline int
checksum_add(str)    /* �^�ǥ��C��r�� checksum */
  char *str;
{
  int sum, i, len;

  len = strlen(str);

  sum = len;    /* ��r�ƤӤ֮ɡA�e�|�����@�ܥi�৹���ۦP�A�ҥH�N�r�Ƥ]�[�J sum �� */

  for (i = len >> 2; i > 0; i--)        /* �u��e�|�����@�r���� sum �� */
    sum += *str++;

  return sum;
}


static inline int    /* 1:�Ocross-post 0:���Ocross-post */
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
    for (i = -(LINE_HEADER + 1);;)  /* �e�|�C�O���Y */
    {
      if (!fgets(buf, ANSILINELEN, fp))
        break;

      if (i < 0)  /* ���L���Y */
      {
        i++;
        continue;
      }

      if (*buf == QUOTE_CHAR1 || *buf == '\n' || !strncmp(buf, "��", 2))   /* ���L�ި� */
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
  int bno;      /* �n��h���ݪO */
{
  int sum;
  char *blist, folder[64];
  ACCT acct;
  HDR hdr;

  if (HAS_PERM(PERM_ALLADMIN))
  return 0;

  /* �O�D�b�ۤv�޲z���ݪO���C�J��K�ˬd */
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
    mail_self(FN_ETC_CROSSPOST, str_sysop, "Cross-Post ���v", 0);
    vmsg("�z�]���L�׸�K�ӳQ���v");
    return 1;
  }
  return 0;
}
#endif  /* HAVE_DETECT_CROSSPOST */


/* ----------------------------------------------------- */
/* �o��B�^���B�s��B����峹                            */
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
  char *brdname = BN_ALLPOST;       // �O�W�۩w

  brd_fpath(folder, brdname, fn_dir);
  hdr_stamp(folder, HDR_LINK | 'A', &hdr, fpath);

  hdr.xmode = mode & ~POST_OUTGO;  /* ���� POST_OUTGO */
  hdr.xid = chrono;
  strcpy(hdr.owner, owner);
  strcpy(hdr.nick, nick);
  if (!strncmp(ve_title, "Re:", 3)) /* �}�Y�� Re: */
    sprintf(hdr.title, "%-*.*s .%s", 47 - strlen(bname), 47 - strlen(bname), ve_title, bname);
  else
    sprintf(hdr.title, "%-*.*s .%s", 43 - strlen(bname), 43 - strlen(bname), ve_title, bname);

  rec_bot(folder, &hdr, sizeof(HDR));

  btime_update(brd_bno(brdname));
}

/* �o���ݪO */
void
add_post(brdname, fpath, title)
  char *brdname;        /* �� post ���ݪO */
  char *fpath;          /* �ɮ׸��| */
  char *title;          /* �峹���D */
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

  if (rc)           /* �w�g��� */
    return rc;

  if (fp = fopen(folder, "r"))
  {
    while (fread(&hdr, sizeof(HDR), 1, fp) == 1)
    {
      xmode = hdr.xmode & (GEM_BOARD | GEM_FOLDER);

      if (xmode == (GEM_BOARD | GEM_FOLDER))      /* �ݪO��ذϱ��| */
      {
        if (!strcmp(hdr.xname, brdname))
        {
          rc = 1;
          break;
        }
      }
      else if (xmode == GEM_FOLDER)               /* ���v recursive �i�h�� */
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

  if ((hdr->xmode & POST_BOTTOM) ||    /* ���i����m���� ���L */
      !HAS_PERM(PERM_ALLBOARD) ||      /* �Y���O���ȤH�� ���L */
      strcmp(currboard, BN_DEPARTALL)) /* �Y���O���w�ݪO ���L */
  return XO_NONE;

  if (hdr->xmode & POST_DIGEST)
  {
    vmsg("���峹�w�g����L�o�A�Y�n���s����Х��Ѱ��аO");
    return XO_FOOT;
  }
  else if (vans("�нT�{����ܦU�t�ҬݪO�H Y)�T�w N)���� [N] ") != 'y')
  {
    return XO_FOOT;
  }

  hdr_fpath(fpath, xo->dir, hdr);

  hdr->xmode ^= POST_DIGEST; /* �����ۭq�аO ��post_attr()��� */
  currchrono = hdr->chrono;
  rec_put(xo->dir, hdr, sizeof(HDR), pos, cmpchrono);

  brdp = bshm->bcache;
  bend = brdp + bshm->number;

  while (brdp < bend)
  {
    /* ����ܫ��w���� �����]�t���� */
    if (find_class("gem/@/@DepartList", brdp->brdname, 0) && strcmp(brdp->brdname, BN_DEPARTALL))
    {
      brd_fpath(folder, brdp->brdname, fn_dir);
      hdr_stamp(folder, HDR_COPY | 'A', &post, fpath);
      strcpy(post.owner, hdr->owner);
      sprintf(post.title, "[���] %s", hdr->title); /* �[������r�� */
      rec_bot(folder, &post, sizeof(HDR));
      btime_update(brd_bno(brdp->brdname)); /* �ݪO�\Ū����(���I)��s */
    }
    brdp++;
  }

  vmsg("��������I");

  return XO_INIT; /* ��s���� */
}

#define NUM_PREFIX 6

static int
do_post(xo, title)
  XO *xo;
  char *title;
{
  /* Thor.981105: �i�J�e�ݳ]�n curredit */
  HDR hdr;
  char fpath[64], *folder, *nick, *rcpt;
  int mode;
  time_t spendtime;
  BRD *brd;

  if (!(bbstate & STAT_POST))
  {
#ifdef NEWUSER_LIMIT
    if (cuser.lastlogin - cuser.firstlogin < 3 * 86400)
      vmsg("�s��W���A�T���l�i�i�K�峹");
    else
#endif
      vmsg("�藍�_�A�z�S���b���o��峹���v��");
    return XO_FOOT;
  }

  brd_fpath(fpath, currboard, FN_POSTLAW);

  if (more(fpath, (char *) -1) < 0)
    film_out(FILM_POST, 0);

  move(b_lines - 4, 0);
  clrtobot();

  prints("�o��峹��i %s �j�ݪO", currboard);

#ifdef POST_PREFIX
  /* �ɥ� mode�Brcpt�Bfpath */

  if (title)
  {
    rcpt = NULL;
  }
  else    /* itoc.020113: �s�峹��ܼ��D���� */
  {
    FILE *fp;
    char prefix[NUM_PREFIX][10];

    brd_fpath(fpath, currboard, "prefix");
    if (fp = fopen(fpath, "r"))
    {
      move(21, 0);
      outs("���O�G");
      for (mode = 0; mode < NUM_PREFIX; mode++)
      {
        if (fscanf(fp, "%9s", fpath) != 1)
          break;
        strcpy(prefix[mode], fpath);
        prints("%d.%s ", mode + 1, fpath);
      }

      fclose(fp);
      mode = (vget(20, 0, "�п�ܤ峹���O�]�� Enter ���L�^�G", fpath, 3, DOECHO) - '1');
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

  /* ����� Internet �v���̡A�u��b�����o��峹 */
  /* Thor.990111: �S��H�X�h���ݪO, �]�u��b�����o��峹 */

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
  spendtime = time(0) - spendtime;  /* itoc.010712: �`�@�᪺�ɶ�(���) */

  /* build filename */

  folder = xo->dir;
  hdr_stamp(folder, HDR_LINK | 'A', &hdr, fpath);

  /* set owner to anonymous for anonymous board */

#ifdef HAVE_ANONYMOUS
  /* Thor.980727: lkchu�s�W��[²�檺��ܩʰΦW�\��] */
  if (curredit & EDIT_ANONYMOUS)
  {
    rcpt = anonymousid;  /* itoc.010717: �۩w�ΦW ID */
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

  /* qazq.031025: ���}�O�~�� all_post */
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
  outs("���Q�K�X�峹�A");

  if (currbattr & BRD_NOCOUNT || wordsnum < 30)
  {        /* itoc.010408: �H���������{�H */
    outs("�峹���C�J�����A�q�Х]�[�C");
  }
  else
  {
    /* itoc.010408: �̤峹����/�ҶO�ɶ��ӨM�w�n���h�ֿ��F����~�|���N�q */
    mode = BMIN(wordsnum, spendtime) / 10;  /* �C�Q�r/�� �@�� */
    prints("�o�O�z���� %d �g�峹�A�o %d �ȡC", ++cuser.numposts, mode);
    addmoney(mode);
  }

  /* �^�����@�̫H�c */
  if (curredit & EDIT_BOTH)
  {
    rcpt = quote_user;

    if (strchr(rcpt, '@'))  /* ���~ */
      mode = bsmtp(fpath, title, rcpt, 0);
    else      /* �����ϥΪ� */
      mode = mail_him(fpath, rcpt, title, 0);

    outs(mode >= 0 ? "\n\n���\\�^���ܧ@�̫H�c" : "\n\n�@�̵L�k���H");
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

  switch (vans("�� �^���� (F)�ݪO (M)�@�̫H�c (B)�G�̬ҬO (Q)�����H[F] "))
  {
  case 'm':
    return do_mreply(hdr, 0);

  case 'q':
    /* Thor: �ѨM Gao �o�{�� bug�A������ reply �峹�A�o�� q �����A
  �M��h�s���ɮסA�A�N�|�o�{�]�X�O�_�ޥέ�媺�ﶵ�F */
    *quote_file = '\0';
    return XO_FOOT;

  case 'b':
    /* �Y�L�H�H���v���A�h�u�^�ݪO */
    if (HAS_PERM(strchr(hdr->owner, '@') ? PERM_INTERNET : PERM_LOCAL))
      curredit = EDIT_BOTH;
    break;
  }

  /* Thor.981105: ���׬O��i��, �άO�n��X��, ���O�O���i�ݨ쪺, �ҥH�^�H�]��������X */
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
/* �L�X hdr ���D                                         */
/* ----------------------------------------------------- */

int
tag_char(chrono)
int chrono;
{
  return TagNum && !Tagger(chrono, 0, TAG_NIN) ? '*' : ' ';
}


#ifdef HAVE_DECLARE

#if 0
    ���Ǥ����O�@�ӱ�����@�ѬO�P���X������.
    �A�� 2000/03/01 �� 2099/12/31

    ����:
         c                y       26(m+1)
    W= [---] - 2c + y + [---] + [---------] + d - 1
         4                4         10
    W �� ���ҨD������P����. (�P����: 0  �P���@: 1  ...  �P����: 6)
    c �� ���w�������~�����e���Ʀr.
    y �� ���w�������~��������Ʀr.
    m �� �����
    d �� �����
    [] �� ��ܥu���Ӽƪ���Ƴ��� (�a�O���)
    ps.�ҨD������p�G�O1���2��,�h�������W�@�~��13���14��.
    �ҥH������m�����Ƚd�򤣬O1��12,�ӬO3��14
#endif

static inline int
cal_day(date)    /* itoc.010217: �p��P���X */
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
  int cc;      /* �L�X�̦h cc - 1 �r�����D */
{
  /* �^��/���/�\Ū�����P�D�D�^��/�\Ū�����P�D�D��� */
  static char *type[4] = {"Re", "��", "\033[1;33m=>", "\033[1;32m��"};
  uschar *title, *mark;
  int ch, len;
#ifdef HAVE_DECLARE
  int square;
#endif
#ifdef CHECK_ONLINE
  UTMP *online;
#endif

  /* --------------------------------------------------- */
  /* �L�X���                                            */
  /* --------------------------------------------------- */

#ifdef HAVE_DECLARE
  if (cuser.ufo & UFO_NOCOLOR)
    prints("%s ", hdr->date + 3);
  else /* itoc.010217: ��άP���X�ӤW�� */
    prints("\033[1;3%dm%s\033[m ", cal_day(hdr->date) + 1, hdr->date + 3);
#else
  outs(hdr->date + 3);
  outc(' ');
#endif

  /* --------------------------------------------------- */
  /* �L�X�@��                                            */
  /* --------------------------------------------------- */

#ifdef CHECK_ONLINE

  /* guessi.060323 �ھڦn�����O��ܤ��P�C�ⴣ��(�ȳB�z�b�u�ϥΪ�) */
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
    if ((--len <= 0) || (ch == '@'))  /* ���~���@�̧� '@' ���� '.' */
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
  /* �L�X���D������                                      */
  /* --------------------------------------------------- */

#ifdef HAVE_REFUSEMARK
  if ((hdr->xmode & POST_RESTRICT) && strcmp(hdr->owner, cuser.userid) && !HAS_PERM(PERM_ALLADMIN) && !(bbstate & STAT_BM))
  {
    title = "<�峹��w>";
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
  /* �L�X���D                                            */
  /* --------------------------------------------------- */

  mark = title + cc;

#ifdef HAVE_DECLARE  /* Thor.980508: Declaration, ���ըϬY��title����� */
  square = 0;    /* 0:���B�z��A 1:�n�B�z��A */
  if (ch < 2)
  {
    if (*title == '[')
    {
      /* guessi.101117 ���i�~�����D�W��, �ϥ�strncmp���e���Ӧr��, �Y��"[���i]" */
      if (!strncmp(title, "[���i]", 6))
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
      if (square & 0x80 || cc & 0x80)  /* ����r���ĤG�X�Y�O ']' ����O��A */
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
  if (square || ch >= 2)  /* Thor.980508: �ܦ��٭�� */
#else
  if (ch >= 2)
#endif
    outs("\033[m");

  outc('\n');
}


/* ----------------------------------------------------- */
/* �ݪO�\���                                            */
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

  /* �ѩ�m����S���\Ū�O���A�ҥH�����wŪ */
  if (cuser.ufo & UFO_READMD)  /* guessi.060825 �ߦn�]�w:���奼Ū�O�� */
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
    /* guessi.090910 �wŪ�L�����ק�Φ��s���� */
    /* ��奼Ū:     +                        */
    /* Ū���ᦳ���: ~                        */
    /* ��L:         +                        */

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
    prints("  \033[1;32m���n\033[m");
  else
    prints("%6d", num);

  prints(" %s%c\033[m", (hdr->xmode & POST_MARKED) ? "\033[1;36m" :
                        (hdr->xmode & POST_DELETE) ? "\033[1;32m" :
                        (hdr->xmode & POST_DIGEST) ? "\033[1;35m" : "", post_attr(hdr));

  if (hdr->xmode & POST_SCORE && !(hdr->xmode & POST_BOTTOM)) /* �m���夣��ܵ��� */
  {
    num = hdr->score;

    if (abs(num) >= 99)
      prints("\033[1;3%s\033[m", num > 0 ? "3m�z" : "0m��" );
    else
      prints("\033[1;3%dm%02d\033[m", num >= 0 ? 1 : 2, abs(num));
  }
  else
    outs("  ");

  prints("%c", tag_char(hdr->chrono));

  hdr_outs(hdr, d_cols + 46);  /* �֤@��ө���� */
#else
  prints("%6d%c%c ", (hdr->xmode & POST_BOTTOM) ? "  ���n" : num, tag_char(hdr->chrono), post_attr(hdr));
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
      if (vans("�ثe�ݪO�L�峹�A�z�n�s�W��ƶܡH Y)�O N)�_ [N] ") == 'y')
      return post_add(xo);
    }
    else
    {
      vmsg("���ݪO�|�L�峹");
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

  return XO_FOOT;  /* itoc.010403: �� b_lines ��W feeter */
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
/* ��Ƥ��s���Gbrowse / history                          */
/* ----------------------------------------------------- */


static int
post_visit(xo)
  XO *xo;
{
  int ans, row, max;
  HDR *hdr;

  ans = vans("�]�w�Ҧ��峹 (U)��Ū (V)�wŪ (W)�e�wŪ�᥼Ū (Q)�����H[Q] ");

  if (ans == 'v' || ans == 'u' || ans == 'w')
  {
    row = xo->top;
    max = xo->max - row + 3;
    if (max > b_lines)
      max = b_lines;

    hdr = (HDR *) xo_pool + (xo->pos - row);
    /* brh_visit(ans == 'w' ? hdr->chrono : ans == 'u'); */
    /* weiyu.041010: �b�m����W�� w ���������wŪ */
    brh_visit((ans == 'u') ? 1 : (ans == 'w' && !(hdr->xmode & POST_BOTTOM)) ? hdr->chrono : 0);

    hdr = (HDR *) xo_pool;

    return post_body(xo);
  }

  return XO_FOOT;
}

void
post_history(xo, hdr)          /* �N hdr �o�g�[�J brh */
  XO *xo;
  HDR *hdr;
{
  int fd;
  time_t prev, chrono, next, this;
  HDR buf;

  check_stamp:                   /* �]�w�ˬd�I                  */

  if (brh_unread(hdr->chrono)) /* ���ˬdhdr->chrono �O�_�bbrh */
    chrono = hdr->chrono;
  else                         /* �Y�w�g�bbrh�h�ˬdstamp      */
    chrono = BMAX(hdr->chrono, hdr->stamp);

  if (!brh_unread(chrono))     /* �p�G�w�b brh ���A�N�L�ݰʧ@ */
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

    if (prev > chrono)      /* �S���U�@�g */
      prev = chrono;
    if (next < chrono)      /* �S���W�@�g */
      next = chrono;
    brh_add(prev, chrono, next);
  }

  chrono = BMAX(hdr->chrono, hdr->stamp); /* �Y��stamp      */
  if (brh_unread(chrono)) /* �ˬdBMAX()�O�_�bbrh �Y�L�h��^ */
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

  if (strcmp(currboard, BN_ALLPOST))/* �b�@��OŪ���A�]�h All_Post �ХܤwŪ */
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

              /* ��_��ӬݪO */
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
  else                              /* �b All_Post Ū���A�]�h�O�O�ХܤwŪ */
  {
    hdr_fpath(fpath, xo->dir, hdr);
    if ((bno = find_board(fpath)) >= 0)
    {
      brd = bshm->bcache + bno;
      brh_get(brd->bstamp, bno);
      bno = hdr->xid;
      brh_add(bno - 1, bno, bno + 1);

      /* ��_��ӬݪO */
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

    /* Thor.990204: ���Ҽ{more �Ǧ^�� */
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
        if (do_reply(xo, hdr) == XO_INIT)  /* �����\�a post �X�h�F */
          return post_init(xo);
      }
      break;

    case 'm':
      if ((bbstate & STAT_BOARD) && !(xmode & (POST_MARKED | POST_DELETE)))
      {
        /* hdr->xmode = xmode ^ POST_MARKED; */
        /* �b post_browse �ɬݤ��� m �O���A�ҥH����u�� mark */
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
      if (vget(b_lines, 0, "�j�M�G", hunt, sizeof(hunt), DOECHO))
      {
        more(fpath, FOOTER_POST);
        goto re_key;
      }
      continue;

    case 'E':
      return post_edit(xo);

    case 'C':  /* itoc.000515: post_browse �ɥi�s�J�Ȧs�� */
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
/* ��ذ�                                                */
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
  /* guessi.060411 ���ȥi�ݨ����ú�� */
  if (bbstate & STAT_BM || HAS_PERM(PERM_ALLBOARD))
    level ^= GEM_M_BIT;

  XoGem(fpath, "��ذ�", level);
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
    vmsg("���ݪO�ثe�S���i�O�e����");
    return XO_FOOT;
  }

  return post_head(xo);
}


/* ----------------------------------------------------- */
/* �\��Gtag / switch / cross / forward                  */
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

  return xo->pos + 1 + XO_MOVE; /* lkchu.981201: ���ܤU�@�� */
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
  /* �ӷ��ݪO */
  char *dir, *ptr;
  HDR *hdr, xhdr;

  /* ����h���ݪO */
  int xbno;
  usint xbattr;
  char xboard[BNLEN + 1], xfolder[64];
  HDR xpost;

  int tag, rc, locus, finish;
  int method;    /* 0:������ 1:�q���}�ݪO/��ذ�/�H�c����峹 2:�q���K�ݪO����峹 */
  usint tmpbattr;
  char tmpboard[BNLEN + 1];
  char fpath[64], buf[ANSILINELEN];
  FILE *fpr, *fpw, *fpo;

  if (!cuser.userlevel)  /* itoc.000213: �קK guest ����h sysop �O */
    return XO_NONE;

  tag = AskTag("���");
  if (tag < 0)
    return XO_FOOT;

  dir = xo->dir;

  if (!ask_board(xboard, BRD_W_BIT, "\n\n\033[1;33m�ЬD��A���ݪO�A��������W�L�|�O�C\033[m\n\n") ||
      (*dir == 'b' && !strcmp(xboard, currboard)))  /* �H�c�B��ذϤ��i�H�����currboard */
  return XO_HEAD;

  hdr = tag ? &xhdr : (HDR *) xo_pool + (xo->pos - xo->top);  /* lkchu.981201: ������ */

  /* ��@������ۤv�峹�ɡA�i�H��ܡu�������v */
  method = (HAS_PERM(PERM_ALLBOARD) || (!tag && !strcmp(hdr->owner, cuser.userid))) &&
  (vget(2, 0, "(1)������ (2)����峹�H[1] ", buf, 3, DOECHO) != '2') ? 0 : 1;

  if (!tag)  /* lkchu.981201: �������N���n�@�@�߰� */
  {
    if (method && strncmp(hdr->title, "[���]", 6))  /* �w��[���]�r�˴N���n�@���[�F */
      sprintf(ve_title, "[���]%.66s", hdr->title);
    else
      strcpy(ve_title, hdr->title);

    if (!vget(2, 0, "���D�G", ve_title, TTLEN + 1, GCARRY))
      return XO_HEAD;
  }

#ifdef HAVE_REFUSEMARK
  rc = vget(2, 0, "(S)�s�� (L)���� (X)�K�� (Q)�����H[Q] ", buf, 3, LCECHO);
  if (rc != 'l' && rc != 's' && rc != 'x')
    return XO_HEAD;
#else
  rc = vget(2, 0, "(S)�s�� (L)���� (Q)�����H[Q] ", buf, 3, LCECHO);
  if (rc != 'l' && rc != 's')
    return XO_HEAD;
#endif

  if (method && *dir == 'b')  /* �q�ݪO��X�A���ˬd���ݪO�O�_�����K�O */
  {
    /* �ɥ� tmpbattr */
    tmpbattr = (bshm->bcache + currbno)->readlevel;
    if (tmpbattr & PERM_SYSOP || tmpbattr & PERM_BOARD)
      method = 2;
  }

  xbno = brd_bno(xboard);
  xbattr = (bshm->bcache + xbno)->battr;

  /* Thor.990111: �b�i�H��X�e�A�n�ˬd���S����X���v�O? */
  if ((rc == 's') && (!HAS_PERM(PERM_INTERNET) || (xbattr & BRD_NOTRAN)))
  rc = 'l';

  /* �ƥ� currboard */
  if (method)
  {
    /* itoc.030325: �@������I�s ve_header�A�|�ϥΨ� currboard�Bcurrbattr�A���ƥ��_�� */
    strcpy(tmpboard, currboard);
    strcpy(currboard, xboard);
    tmpbattr = currbattr;
    currbattr = xbattr;
  }

  locus = 0;
  do  /* lkchu.981201: ������ */
  {
    if (tag)
    {
      EnumTag(hdr, dir, locus, sizeof(HDR));

      if (method && strncmp(hdr->title, "[���]", 6))     /* �w��[���]�r�˴N���n�@���[�F */
        sprintf(ve_title, "[���]%.66s", hdr->title);
      else
        strcpy(ve_title, hdr->title);
    }

    if (hdr->xmode & GEM_FOLDER)  /* �D plain text ������ */
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

    if (method)    /* �@����� */
    {
      /* itoc.030325: �@������n���s�[�W header */
      fpw = fdopen(hdr_stamp(xfolder, 'A', &xpost, buf), "w");
      ve_header(fpw);

      /* itoc.040228: �p�G�O�q��ذ�����X�Ӫ��ܡA�|�������� [currboard] �ݪO�A
  �M�� currboard �����O�Ӻ�ذϪ��ݪO�C���L���O�ܭ��n�����D�A�ҥH�N���ޤF :p */
      fprintf(fpw, "�� ��������� [%s] %s\n\n",
      *dir == 'u' ? cuser.userid : method == 2 ? "���K" : tmpboard,
      *dir == 'u' ? "�H�c" : "�ݪO");

      /* Kyo.051117: �Y�O�q���K�ݪO��X���峹�A�R���峹�Ĥ@��ҰO�����ݪO�W�� */
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

      ve_banner(fpw, 0); /* guessi.070317 �d�U��ñ �O������̸�T */

      fclose(fpw);

      strcpy(xpost.owner, cuser.userid);
      strcpy(xpost.nick, cuser.username);
    }
    else    /* ������ */
    {
      /* itoc.030325: ���������� copy �Y�i */
      hdr_stamp(xfolder, HDR_COPY | 'A', &xpost, fpath);

      strcpy(xpost.owner, hdr->owner);
      strcpy(xpost.nick, hdr->nick);
      strcpy(xpost.date, hdr->date);  /* �������O�d���� */
    }

    /* guessi.070317 ����峹�ɡA���i�[������ (�Y��H��Admin���ȬݪO�h�ư�����) */
    if (strcmp(xboard, "Admin") && (fpo = fopen(fpath, "a")))
    {
      char msg[64];
      usint checklevel;

      time_t now;
      struct tm *ptime;

      time(&now);
      ptime = localtime(&now);

      checklevel = (bshm->bcache + xbno)->readlevel;

      sprintf(msg, "����w����� [%s] �ݪO", (checklevel == PERM_SYSOP || checklevel == PERM_BOARD) ? "���K" : xboard);

      fprintf(fpo, "\033[1;30m�� %s:%-*s %02d/%02d %02d:%02d\033[m\n", cuser.userid, 62 - strlen(cuser.userid), msg,
      ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min); /* �榡�ۭq �ɶq�P�������� */

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

  /* Thor.981205: check �Q�઺�O���S���C�J����? */
  if (!(xbattr & BRD_NOCOUNT))
    cuser.numposts += tag ? tag : 1;  /* lkchu.981201: �n�� tag */

  /* �_�� currboard�Bcurrbattr */
  if (method)
  {
    strcpy(currboard, tmpboard);
    currbattr = tmpbattr;
  }

  vmsg("�������");
  return XO_HEAD;
}

/* ----------------------------------------------------- */
/* �O�D�\��Gmark / delete / label                       */
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
    if (hdr->xmode & POST_DELETE)  /* �ݬ媺�峹���� mark */
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
post_digest(xo) /* �аO��K */
  XO *xo;
{
  if (bbstate & STAT_BOARD || HAS_PERM(PERM_ALLADMIN)) /* �ˬd�O�_���O�D */
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

    if (vans(hdr->xmode & POST_BOTTOM ? "�����m���H Y)�O N)�_ [N] " : "�[�J�m���H Y)�O N)�_ [N] ") != 'y')
      return XO_FOOT;

    /* itoc.����m����ƶq */
    int fd, fsize;
    struct stat st;
    if (!(hdr->xmode & POST_BOTTOM))    /* guessi.060417 �Y������ �h�i���� */
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
          vmsg("�m����ƶq���o�W�L���g");
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
      /* guessi.060220 �[�J�P�_�O�_���m���� �߰ݨ���or�[�J�m�� */
      hdr_fpath(fpath, xo->dir, hdr);
      hdr_stamp(xo->dir, HDR_LINK | 'A', &post, fpath);

      /* guessi �ݬ�峹���i�m�� */
#ifdef HAVE_LABELMARK
      if (hdr->xmode & POST_DELETE)
        return XO_NONE;
#endif

#ifdef HAVE_REFUSEMARK
      post.xmode = POST_BOTTOM | (hdr->xmode & POST_RESTRICT);
#else
      post.xmode = POST_BOTTOM;
#endif

      strcpy(post.date, hdr->date);   /* guessi �ץ��m�������P���ۦP */
      strcpy(post.owner, hdr->owner);
      strcpy(post.nick, hdr->nick);
      strcpy(post.title, hdr->title);

      rec_add(xo->dir, &post, sizeof(HDR));

      return post_load(xo);             /* �ߨ���ܸm���峹 */
    }
  }
  return XO_NONE;
}


#ifdef HAVE_REFUSEMARK
static int
post_refuse(xo)    /* itoc.010602: �峹�[�K */
  XO *xo;
{
  HDR *hdr;
  int pos, cur;

  if (!cuser.userlevel)  /* itoc.020114: guest ������L guest ���峹�[�K */
  return XO_NONE;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

  /* guessi.100524 ���i�w��m����@���ާ@ */
  if (hdr->xmode & POST_BOTTOM)
    return XO_NONE;

  /* guessi.061213 �O�D�B���� �~�i�H�ϥΥ��\�� */
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
  /* guessi.091204 �Y�D�ݪO����... �h�X */
  if (!(bbstate & STAT_BOARD))
    return XO_FOOT;

  if (vans("�нT�{�����аO�H Y)���� N)�O�d [N] ") != 'y')
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
  /* guessi.100618 �t�άݪO�����ѧR��\�� */
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
    zmsg("���ݪO�]�w�L�k�аO�N��");
    return XO_FOOT;
  }

  if (bbstate & STAT_BOARD)
  {
    HDR *hdr;
    int pos, cur;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;

    if (hdr->xmode & (POST_MARKED | POST_DONE | POST_RESTRICT | POST_BOTTOM | POST_DIGEST))  /* mark �� �[�K���峹����ݬ� */
      return XO_NONE;

    hdr->xmode ^= POST_DELETE;
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(pos + 1, hdr);

    return pos + 1 + XO_MOVE;  /* ���ܤU�@�� */
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
    zmsg("���ݪO�]�w�L�k�R���峹");
    return XO_FOOT;
  }

  if (!(bbstate & STAT_BOARD))
  return XO_NONE;

  if (vans("�T�w�n�R���ݬ�峹�ܡH Y)�R�� N)�O�d [N] ") != 'y')
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
    zmsg("���ݪO�]�w�L�k�R���峹");
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
      /* guessi.070305 �m���夣���峹�ơB���� */
      if (!by_BM && !(currbattr & BRD_NOCOUNT) && !(hdr->xmode & POST_BOTTOM))
      {
#ifdef HAVE_DETECT_CROSSPOST
        int sum;
        hdr_fpath(buf, xo->dir, hdr);
        sum = checksum_count(buf);
        checksum_rmv(sum);
#endif

        /* itoc.010711: ��峹�n�����A���ɮפj�p */
        pos = pos >> 3;  /* �۹�� post �� wordsnum / 10 */

        /* itoc.010830.����: �|�}: �Y multi-login �夣��t�@������ */
        if (cuser.money > pos)
          cuser.money -= pos;
        else
          cuser.money = 0;

        if (cuser.numposts > 0)
          cuser.numposts--;

        sprintf(buf, "%s�A�z���峹� %d �g", MSG_DEL_OK, cuser.numposts);

        vmsg(buf);
      }

      if (xo->key == XZ_XPOST)
      {
        vmsg("��C��g�R����V�áA�Э��i�걵�Ҧ��I");
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
    zmsg("���ݪO�]�w�L�k�R���峹");
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
    zmsg("���ݪO�]�w�L�k�R���峹");
    return XO_FOOT;
  }

  ret = xo_prune(xo, sizeof(HDR), vfypost, delpost);

  btime_update(currbno);

  if (xo->key == XZ_XPOST && ret == XO_LOAD)
  {
    vmsg("��C��g�妸�R����V�áA�Э��i�걵�Ҧ��I");
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
post_copy(xo)     /* itoc.010924: ���N gem_gather */
  XO *xo;
{
  int tag;

  tag = AskTag("�ݪO�峹����");

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
      zmsg("�ɮ׼аO�����C[�`�N] �z���������}�걵�Ҧ��~��i�J��ذϡC");
      return XO_FOOT;
    }
    else
#endif
    {
      zmsg("���������C[�`�N] �K�W��~��R�����I");
      return post_gem(xo);  /* �����������i��ذ� */
    }
  }

  zmsg("�ɮ׼аO�����C[�`�N] �z�u��b���(�p)�O�D�Ҧb�έӤH��ذ϶K�W�C");
  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* �����\��Gedit / title                                */
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
    vmsg("���ݪO�]�w�L�k�ק�峹");
    return XO_FOOT;
  }

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);

  hdr_fpath(fpath, xo->dir, hdr);

  strcpy(ve_title, hdr->title);

#ifdef HAVE_REFUSEMARK
  if ((hdr->xmode & POST_RESTRICT) && !(bbstate & STAT_BM) && strcmp(hdr->owner, cuser.userid))
    return XO_NONE;
#endif

  /* guessi.****** �Y�ϬO���ȭק�A�]�d�U�ק�O�� */
  if ((cuser.userlevel && !strcmp(hdr->owner, cuser.userid)) || HAS_PERM(PERM_SYSOP))  /* ��@�̭ק� */
  {
    if (!vedit(fpath, 0))  /* �Y�D�����h�[�W�ק��T */
    {
      if (fp = fopen(fpath, "a"))
      {
        ve_banner(fp, 1);
        fclose(fp);
        change_stamp(xo->dir, hdr);
        currchrono = hdr->chrono;
        rec_put(xo->dir, hdr, sizeof(HDR), xo->pos, cmpchrono);
        post_history(xo, hdr);
        /* �峹�ק藍�]����Ū
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
  return XO_HEAD;  /* itoc.021226: XZ_POST �M XZ_XPOST �@�� post_edit() */
}


void
header_replace(xo, fhdr)  /* itoc.010709: �ק�峹���D���K�ק鷺�媺���D */
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

  fgets(buf, sizeof(buf), fpr);    /* �[�J�@�� */
  fputs(buf, fpw);

  fgets(buf, sizeof(buf), fpr);    /* �[�J���D */
  if (!str_ncmp(buf, "��", 2))    /* �p�G�� header �~�� */
  {
    strcpy(buf, buf[2] == ' ' ? "��  �D: " : "���D: ");
    strcat(buf, fhdr->title);
    strcat(buf, "\n");
  }
  fputs(buf, fpw);

  while(fgets(buf, sizeof(buf), fpr))  /* �[�J��L */
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

  if (!cuser.userlevel)  /* itoc.000213: �קK guest �b sysop �O����D */
    return XO_NONE;

  if(post_is_system_board())
  {
    zmsg("���ݪO�]�w�L�k�ק���D");
    return XO_FOOT;
  }

  pos = xo->pos;
  cur = pos - xo->top;
  fhdr = (HDR *) xo_pool + cur;

  if (fhdr->xmode & POST_BOTTOM)
  {
    vmsg("�m����L�k�ק���D �Ш����m����ק�����D");
    return XO_FOOT;
  }

  memcpy(&mhdr, fhdr, sizeof(HDR));

  if (strcmp(cuser.userid, mhdr.owner) && !(bbstate & STAT_BOARD))
    return XO_NONE;

  vget(b_lines, 0, "���D�G", mhdr.title, TTLEN + 1, GCARRY);

  if (HAS_PERM(PERM_ALLBOARD))  /* itoc.000213: ��@�̥u�����D */
  {
    vget(b_lines, 0, "�@�̡G", mhdr.owner, 73 /* sizeof(mhdr.owner) */, GCARRY);
    /* Thor.980727: sizeof(mhdr.owner) = 80 �|�W�L�@�� */
    vget(b_lines, 0, "�ʺ١G", mhdr.nick, sizeof(mhdr.nick), GCARRY);
    vget(b_lines, 0, "����G", mhdr.date, sizeof(mhdr.date), GCARRY);
  }

  if (memcmp(fhdr, &mhdr, sizeof(HDR)) && vans(msg_sure_ny) == 'y')
  {
    memcpy(fhdr, &mhdr, sizeof(HDR));
    currchrono = fhdr->chrono;
    rec_put(xo->dir, fhdr, sizeof(HDR), xo->key == XZ_XPOST ? fhdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(++pos, fhdr);

    /* itoc.010709: �ק�峹���D���K�ק鷺�媺���D */
    header_replace(xo, fhdr);
  }
  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* �B�~�\��Gwrite / score                               */
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
  prints("\n�ثe�ݪO:[%s]  �O�D:[%s]\n", currboard, blist[0] > ' ' ? blist : "�x�D��");
  prints("\n �O�_��H - %s", (currbattr & BRD_NOTRAN) ? "�����ݪO" : "��H�ݪO ��H�ݪO�w�]���󯸶K��");
  prints("\n �i�_���� - %s", (currbattr & BRD_NOSCORE) ? "���i����" : "�}�����");
  prints("\n �O���g�� - %s", (currbattr & BRD_NOCOUNT) ? "�����O��" : "�O��");
  prints("\n �������D - %s", (currbattr & BRD_NOSTAT) ? "�����έp" : "�έp��ưO����������D");
  prints("\n �벼���G - %s", (currbattr & BRD_NOVOTE) ? "�����O��" : "�N�O�����G��[Record]�ݪO");

  if (cuser.userlevel)
    prints("\n\n�z�ثe%s���ݪO���޲z��", ((cuser.userlevel & PERM_BM) && blist[0] > ' ' && is_bm(blist, cuser.userid)) ? "�O" : "���O");
  else
    prints("\n\n�z�ثe������guest�X��");
  vmsg(NULL);

  return XO_HEAD;
}

int
post_write(xo)      /* itoc.010328: ��u�W�@�̤��y */
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

/* guessi.090910 ���s���g����{��post_score() */
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

  /* ���嶡�j���� */
  static time_t next = 0;

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);
  hdr_fpath(fpath, xo->dir, hdr);

  /* ��������P�o�孭�� */
  if ((currbattr & BRD_NOSCORE) || !cuser.userlevel || !(bbstate & STAT_POST))
    return XO_NONE;

#ifdef HAVE_REFUSEMARK
  /* ����\Ū */
  if ((hdr->xmode & POST_RESTRICT) && strcmp(hdr->owner, cuser.userid) && !(bbstate & STAT_BM))
    return XO_NONE;
#endif

#ifdef HAVE_ANONYMOUS
  /* �ΦW���� */
  if (currbattr & BRD_ANONYMOUS)
  {
    userid = uid;
    /* �Y�ΦW�h��ΦW�����["."�H�ܰϹj */
    if (!vget(b_lines, 0, "�п�J���ϥΪ��ΦW�A�Ϋ�[ENTER]���L���B�J", anony, IDLEN, DOECHO))
      userid = cuser.userid;
    else
      strcat(userid, ".");
  }
  else
    userid = cuser.userid;
#else
  userid = cuser.userid;
#endif

  /* �p����夤�i��J����r�ƶq(�b�Φr) */
  maxlen = 63 - strlen(userid);

  move(b_lines - 3, 0);
  clrtoeol();
  prints(MSG_SEPERATOR);

  move(b_lines - 2, 0);
  clrtoeol();
  prints("\033[1;30m���^���峹 (�ܦh�i�P�ɰe�X�G��s��L���_��r�A�ɶ���������ڰe�X�ɶ�)\033[m\n");

  move(b_lines - 1, 0);
  clrtoeol();

  move(b_lines, 0);
  clrtoeol();

  /* guessi.110514 ���嶡��ɶ����� 0~3�����i�A�@����ʧ@ 4~5�����"��" 5��H�W��_���` */
  if ((next - time(NULL)) <= 0)
  {
    move(b_lines - 1, 0);
    prints("\033[1;37m����ܵ��� \033[31m1)�ȱo���� \033[32m2)���өA�r \033[37m3)�����q�q�H[3] \033[m");
    ans = vkey();
  }

  switch (ans)
  {
	case '1':
	  verb = "31m��";
	  sprintf(preview, "�� %s:", userid);
	  break;
	case '2':
	  verb = "32m�A";
	  sprintf(preview, "�A %s:", userid);
	  break;
	default:
	  verb = "1;31m��";
	  sprintf(preview, "�� %s:", userid);
	  break;
  }
  sprintf(preview2, "�� %s:", userid);

  /* �ШD�ϥΪ̿�J���夺�e(�Ĥ@��) */
  if (!vget(b_lines - 1, 0, preview, reason, maxlen, DOECHO | CTECHO))
    return XO_HEAD;

  vget(b_lines, 54, "����/�e�X/����[c/y/N]:", preview, 2, LCECHO);

  /* ��J������~���ɶ� */
  time(&now);
  ptime = localtime(&now);

  /* �M�� b_lines - 1 �椺�e ��ܿ�X�w�� */
  move(b_lines - 1, 0);
  clrtoeol();
  prints("\033[%s\033[m \033[1m%s\033[;33m:%-*s \033[35m%02d/%02d %02d:%02d\033[m\n",
  verb, userid, 62 - strlen(userid), reason, ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min);

  /* �ˬd�ϥΪ̿�J�O�_�p�P�w�� */
  if (*preview != 'c' && *preview != 'y')
    return XO_HEAD;

  /* �ШD�ϥΪ̿�J���夺�e(�ĤG��) */
  if (*preview == 'c')
  {
    /* �ĤG���r�i�ٲ�����J �G�����ˬd */
    vget(b_lines, 0, preview2, reason2, maxlen, DOECHO | CTECHO);
    /* �߰ݨϥΪ̬O�_�T�{�e�X��r */
    vget(b_lines, 68, "�e�X[y/N]:", preview2, 2, LCECHO);

    if (*preview2 != 'y')
	  return XO_HEAD;
  }

  if (fp = fopen(fpath, "a"))
  {
    /* ��ڰe�X����ɦA�����o�T���ɶ� */
    time(&now);
    ptime = localtime(&now);

    fprintf(fp, "\033[%s\033[m \033[1m%s\033[;33m:%-*s \033[35m%02d/%02d %02d:%02d\033[m\n",
    verb, userid, 62 - strlen(userid), reason, ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min);

	/* ��s�b���ĤG���r��J�� */
	if (*preview == 'c')
	{
	  if (strlen(reason2) > 0)
	  {
            fprintf(fp, "\033[%s\033[m \033[1m%s\033[;33m:%-*s \033[35m%02d/%02d %02d:%02d\033[m\n",
            "1;31m��", userid, 62 - strlen(userid), reason2, ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min);
	  }
	  else
	    vmsg("�ĤG�����L��J��r�A�N���C�J�O��");
    }

    fclose(fp);
  }

  /* �]�w���嶡�j���� */
  next = time(NULL) + 5;

  /* �]�w�����ƭȰ����q */
  curraddscore = ((ans - '0') == 1 && hdr->score < 99) ? 1 : (((ans - '0') == 2 && hdr->score > -99) ? -1 : 0);

  /* ������ƭȦ�������"��"��ܶȱ��� ��suser read history */
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

/* guessi.060323 ���m�����ƭ� */
static int
post_resetscore(xo)
  XO *xo;
{
  /* �ˬd�O�_�֦��O�D�H�W�v�� */
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

    ans = vans("�������]�w 1)�ۭq���� 2)�����k�s [Q] ");
    
    if (ans == '1')
    {
      mp = vans("�� 1)���� 2)�t�� [1] ");

      if (!vget(b_lines, 0, "�п�J�Ʀr(���t���t�Ÿ�)�G", score, 3, DOECHO))
        return XO_FOOT;

      /* guessi.111026 �ư��ƭȶW�X�d�򪺪��p */
      if (atoi(score) < 0 || atoi(score) > 99)
      {
        vmsg("��J�ƭȰ϶����X�k");
        return XO_FOOT;
      }

      /* guessi.060323 �L�׬O�_���Q���� ������POST_SCORE�аO */
      hdr->xmode |= POST_SCORE;
      hdr->score = mp == '2' ? (atoi(score) * -1) : atoi(score);
      currchrono = hdr->chrono;
    }
    else if (ans == '2')
    {
      /* guessi.060323 �N�����k�s �åh��POST_SCORE�аO */
      hdr->xmode &= ~POST_SCORE;
      hdr->score = 0;
      currchrono = hdr->chrono;
    }
    else
    {
      return XO_FOOT;
    }

    /* guessi.111026 ��峹���ݪ��[���m���Ƹ�T */
    if (fp = fopen(fpath, "a"))
    {
      time(&now);
      ptime = localtime(&now);

      fprintf(fp, "\033[1;30m�� �t�ά���: �峹������ %s �]�w���m%-*s%02d/%02d %02d:%02d\033[m\n",
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


/* guessi.090610 ��ܤ峹�N�X(AID) */
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

  prints("�z�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�{\n");
  prints("�x�峹�N�X(AID): #%s (%s)%-*s�峹����: %-2d�x\n", hdr->xname, currboard, 35 - strlen(currboard), "", hdr->score);
  prints("�x�o��ɶ�: %.24s%-*s�x\n", ctime(&hdr->chrono), 65 - strlen(ctime(&hdr->chrono)), "");
#ifdef HAVE_PERMANENT_LINK
  /* guessi.120728 �Y�ݪO�����ëh����ܺ��}�s�� */
  if (!(readlevel & PERM_SYSOP || readlevel & PERM_BOARD))
  {
#   define FQDNLINK "http://" MYHOSTNAME "/plink?%s&%s"
    prints("�x�ä[�s��: " FQDNLINK "%-*s�x\n", currboard, hdr->xname, 34 - strlen(currboard) - strlen(hdr->xname), "");
  }
#endif
  prints("�|�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�}\n");

  if (cur > 15)
    prints("\033[1;37;40m%s\033[m\n", buf);

  clrtoeol();

  vmsg(NULL);

  return XO_HEAD;
}

/* guessi.090610 �ھڤ峹�N�X(AID)��M�峹 */
static int
post_aid_search(xo)
XO *xo;
{
  char *tag, *query, aid[9];
  int currpos, pos, max, match = 0;
  HDR *hdr;

  /* �O�s�ثe�Ҧb����m */
  currpos = xo->pos;
  /* �����ݪO�峹�`�ơF�s�K��AID�O���������i��b�䤤 */
  max = xo->max;

  /* �Y�S���峹�Ψ�L�ҥ~���p */
  if (max <= 0)
    return XO_FOOT;

  /* �ШD�ϥΪ̿�J�峹�N�X(AID) */
  if (!vget(b_lines, 0, "�п�J�峹�N�X(AID)�G #", aid, sizeof(aid), DOECHO))
    return XO_FOOT;
  query = aid;

  for (pos = 0; pos < max; pos++)
  {
    xo->pos = pos;
    xo_load(xo, sizeof(HDR));
    /* �]�wHDR��T */
    hdr = (HDR *) xo_pool + (xo->pos - xo->top);
    tag = hdr->xname;
    /* �Y���������峹�A�h�]�wmatch�ø��X */
    if (!strcmp(query, tag))
    {
      match = 1;
      break;
    }
  }

  /* �S���h��_xo->pos����ܴ��ܤ�r */
  if (!match)
  {
    zmsg("�䤣��峹�A�O���O����ݪO�F�O�H");
    xo->pos = currpos;  /* ��_xo->pos���� */
  }

  return post_load(xo);
}

static int
post_help(xo)
XO *xo;
{
  xo_help("post");
  /* return post_head(xo); */
  return XO_HEAD;    /* itoc.001029: �P xpost_help �@�� */
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
  'x', post_cross,    /* �b post/mbox �����O�p�g x ��ݪO�A�j�g Z ��ϥΪ� */
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


  '~', XoXselect,         /* itoc.001220: �j�M�@��/���D */
  'S', XoXsearch,         /* itoc.001220: �j�M�ۦP���D�峹 */
  'a', XoXauthor,         /* itoc.001220: �j�M�@�� */
  '/', XoXtitle,          /* itoc.001220: �j�M���D */
  'f', XoXfull,           /* itoc.030608: ����j�M */
  'G', XoXmark,           /* itoc.010325: �j�M mark �峹 */
  'U', XoXdigest,         /* �j�M��K */
  'L', XoXlocal,          /* itoc.010822: �j�M���a�峹 */

#ifdef HAVE_SCORE
  'Z', XoXscore,          /* guessi.120319 �j�M�����峹 */
#endif

#ifdef HAVE_XYNEWS
  'u', XoNews,            /* itoc.010822: �s�D�\Ū�Ҧ� */
#endif
  '#', post_aid_search,   /* guessi.090610 �H�峹�N�X(AID)�ֳt�M��*/
  'W', post_aid_query,    /* guessi.090610 �d�ߤ峹�N�X(AID) */

  'h', post_help
};


KeyFunc xpost_cb[] =
{
  XO_INIT, xpost_init,
  XO_LOAD, xpost_load,
  XO_HEAD, xpost_head,
  XO_BODY, post_body,     /* Thor.980911: �@�ΧY�i */

  'r', xpost_browse,
  'y', post_reply,
  't', post_tag,
  'x', post_cross,
  'c', post_copy,
  'g', gem_gather,
  'm', post_mark,
  'd', post_delete,     /* Thor.980911: ��K�O�D */
  'E', post_edit,       /* itoc.010716: ���� XPOST ���i�H�s����D�B�峹�A�[�K */
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
  'L', XoXlocal,     /* itoc.060206: �j�M��i�A�i���j�M */

#ifdef HAVE_SCORE
  'Z', XoXscore,          /* guessi.120319 �j�M�����峹 */
#endif

  'h', post_help     /* itoc.030511: �@�ΧY�i */
};


#ifdef HAVE_XYNEWS
KeyFunc news_cb[] =
{
  XO_INIT, news_init,
  XO_LOAD, news_load,
  XO_HEAD, news_head,
  XO_BODY, post_body,

  'r', XoXsearch,

  'h', post_help    /* itoc.030511: �@�ΧY�i */
};
#endif  /* HAVE_XYNEWS */
