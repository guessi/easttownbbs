/*-------------------------------------------------------*/
/* menu.c    ( NTHU CS MapleBBS Ver 3.00 )               */
/*-------------------------------------------------------*/
/* target : menu/help/movie routines                     */
/* create : 95/03/29                                     */
/* update : 97/03/29                                     */
/*-------------------------------------------------------*/

#include "bbs.h"

extern UCACHE *ushm;
extern FCACHE *fshm;

#ifndef ENHANCED_VISIT
extern time_t brd_visit[];
#endif


/* ----------------------------------------------------- */
/* ���} BBS ��                                           */
/* ----------------------------------------------------- */

#define    FN_RUN_NOTE_PAD    "run/note.pad"
#define    FN_RUN_NOTE_TMP    "run/note.tmp"

typedef struct
{
  time_t tpad;
  char msg[400];
}    Pad;


int
pad_view()
{
  int fd, count;
  Pad *pad;

  if ((fd = open(FN_RUN_NOTE_PAD, O_RDONLY)) < 0)
    return XEASY;

  count = 0;
  mgets(-1);

  for (;;)
  {
    pad = mread(fd, sizeof(Pad));
    if (!pad)
    {
      vmsg(NULL);
      break;
    }
    else if (!(count % 5))    /* itoc.020122: �� pad �~�L */
    {
      clear();
      move(0, 23);
      prints("�i �� �� �W �� �d �� �O �j        �� %d ��\n\n", count / 5 + 1);
    }

    outs(pad->msg);
    count++;

    if (!(count % 5))
    {
      move(b_lines, 0);
      outs("�Ы� [SPACE] �~���[��A�Ϋ���L�䵲���G ");

      /* itoc.010127: �ץ��b�������k����ΤU�A������|�����G�h��檺���D */
      if (vkey() != ' ')
        break;
    }
  }

  close(fd);
  return 0;
}


static int
pad_draw()
{
  int i, cc, fdr, color;
  FILE *fpw;
  Pad pad;
  char *str, buf[3][71];

  /* itoc.����: ���Q�ΰ��m�סA�Ӫ� */
  static char pcolors[6] = {31, 32, 33, 34, 35, 36};

  /* itoc.010309: �d���O���Ѥ��P���C�� */
  color = vans("�߱��C�� 1) \033[41m  \033[m 2) \033[42m  \033[m 3) \033[43m  \033[m "
               "4) \033[44m  \033[m 5) \033[45m  \033[m 6) \033[46m  \033[m [Q] ");

  if (color < '1' || color > '6')
    return XEASY;
  else
    color -= '1';

  do
  {
    buf[0][0] = buf[1][0] = buf[2][0] = '\0';
    move(MENU_XPOS, 0);
    clrtobot();
    outs("\n�Яd�� (�ܦh�T��)�A��[Enter]����");
    for (i = 0; (i < 3) &&
        vget(16 + i, 0, "�G", buf[i], 71, DOECHO); i++);
    cc = vans("(S)�s���[�� (E)���s�ӹL (Q)��F�H[S] ");
    if (cc == 'q' || i == 0)
      return 0;
  } while (cc == 'e');

  time(&(pad.tpad));

  /* itoc.020812.����: �睊�����ɭԭn�`�N struct Pad.msg[] �O�_���j */
  str = pad.msg;
  sprintf(str, "�~�t\033[1;46m %s �� %s \033[m�u", cuser.userid, cuser.username);

  for (cc = strlen(str); cc < 60; cc += 2)
    strcpy(str + cc, "�w");
  if (cc == 60)
    str[cc++] = ' ';

  sprintf(str + cc,
      "\033[1;44m %s \033[m��\n"
      "�x  \033[1;%dm%-70s\033[m  �x\n"
      "�x  \033[1;%dm%-70s\033[m  �x\n"
      "��  \033[1;%dm%-70s\033[m  ��\n",
      Btime(&(pad.tpad)),
      pcolors[color], buf[0],
      pcolors[color], buf[1],
      pcolors[color], buf[2]);

  f_cat(FN_RUN_NOTE_ALL, str);

  if (!(fpw = fopen(FN_RUN_NOTE_TMP, "w")))
    return 0;

  fwrite(&pad, sizeof(pad), 1, fpw);

  if ((fdr = open(FN_RUN_NOTE_PAD, O_RDONLY)) >= 0)
  {
    Pad *pp;

    i = 0;
    cc = pad.tpad - NOTE_DUE * 60 * 60;
    mgets(-1);
    while (pp = mread(fdr, sizeof(Pad)))
    {
      fwrite(pp, sizeof(Pad), 1, fpw);
      if ((++i > NOTE_MAX) || (pp->tpad < cc))
        break;
    }
    close(fdr);
  }

  fclose(fpw);

  rename(FN_RUN_NOTE_TMP, FN_RUN_NOTE_PAD);
  pad_view();

  return 0;
}


static int
goodbye()
{
   /* guessi.091228 ��ֵe����ø �^��XO_NONE */
  if (vans("�z�T�w�n���}�i �F��p�� �j�ܡH Y)�A�� N)��^ [N] ") != 'y')
    return XO_NONE;

#ifdef LOG_BMW
  /* lkchu.981201: ���y�O���B�z */
  bmw_log();
#endif

  /* itoc.000407: �����e���@��²�� */
  if (!(cuser.ufo & UFO_MOTD))
  {
    clear();
    prints("�˷R�� \033[32m%s(%s)\033[m�A�O�ѤF�A�ץ��{�i %s �j\n�H�U�O�z�b���������U��ơG\n",
           cuser.userid, cuser.username, str_site);
    acct_show(&cuser, 0);
    vmsg(NULL);
  }

  u_exit("EXIT ");

  /* guessi.060610 �����M�ŵe�� */
  clear();
  refresh();

  exit(0);
}


/* ----------------------------------------------------- */
/* help & menu processring                               */
/* ----------------------------------------------------- */

void
vs_head(title, mid)
char *title, *mid;
{
  char buf[(T_COLS - 1) - 79 + 69 + 1];    /* d_cols �̤j�i��O(T_COLS -1) */
  char ttl[(T_COLS - 1) - 79 + 69 + 1];
  int spc, len;

  if (mid)    /* xxxx_head() ���O�� vs_head(title, str_site); */
  {
    clear();
  }
  else        /* menu() ���~�� vs_head(title, NULL); ��椤�L�� clear() */
  {
    move(0, 0);
    clrtoeol();
    mid = str_site;
  }

  /* guessi.101028 ��X�{���U�檺�ɭ� ����ܥ|�Ӧr(8��)
   * len: �����ٳѤU�h�����Ŷ� (65 - 8 = 57)
   */
  if (HAS_PERM(PERM_ALLREG) && rec_num(FN_RUN_RFORM, sizeof(RFORM)))
    len = d_cols + 57 - strlen(currboard);
  else
    len = d_cols + 65 - strlen(title) - strlen(currboard);

  /* guessi.070103 guest����ܶl�󴣥� */
  if (HAS_STATUS(STATUS_BIFF) && strcmp(cuser.userid, STR_GUEST))
  {
    mid = "\033[5;41m ���H�����ѵ��A�� \033[m";
    spc = 18;
  }
  else
  {
    /* �Ŷ������\�U�쥻�n�\�� mid�A�u�n�� mid �I�_ */
    if ((spc = strlen(mid)) > len)
    {
      spc = len;
      memcpy(ttl, mid, spc);
      mid = ttl;
      mid[spc] = '\0';
    }
  }

  /* �\�� mid �H��A�����٦� spc ��Ŷ��A�b mid ���k�U�� spc/2 �����ť� */
  spc = 2 + len - spc;
  len = 1 - spc & 1;
  memset(buf, ' ', spc >>= 1);
  buf[spc] = '\0';

/* guessi.101027 ���U�洣�ܿW����ܩ󥪤W�� */
#ifdef COLOR_HEADER
  spc = (time(0) % 7) + '1';
  if (HAS_PERM(PERM_ALLREG) && rec_num(FN_RUN_RFORM, sizeof(RFORM)))
    title = "�����U��";
  prints("\033[1;4%cm�i%s�j%s\033[33m%s\033[1;37;4%cm%s�ݪO�m%s�n\033[m\n",
       spc, title, buf, mid, spc, buf + len, currboard);
#else
  if (HAS_PERM(PERM_ALLREG) && rec_num(FN_RUN_RFORM, sizeof(RFORM)))
    title = "\033[5;36m�����U��\033[;1;44m";
  prints("\033[1;44m�i%s�j%s\033[33m%s\033[1;37;44m%s�ݪO�m%s�n\033[m\n",
       title, buf, mid, buf + len, currboard);
#endif
}


/* ------------------------------------- */
/* �ʵe�B�z                              */
/* ------------------------------------- */

/* itoc.010403: �� feeter �� status �W�ߥX�ӡA�w�ƨѨ�L function �s�� */
static char feeter[160];

static void
status_foot()
{
  static int orig_flag = -1;
  static time_t uptime = -1;
  static int orig_money = -1;

  static char flagmsg[16];
  static char coinmsg[20];

  int ufo;
  time_t now;

  ufo = cuser.ufo;
  time(&now);

  /* Thor: �P�� ��� �I�s�� �W���q�� ���� */

#ifdef HAVE_ALOHA
  ufo &= UFO_PAGER | UFO_ALOHA | UFO_CLOAK | UFO_QUIET;
  if (orig_flag != ufo)
  {
    orig_flag = ufo;
    sprintf(flagmsg, "%s%s%s%s",
        (ufo & UFO_PAGER) ? "��" : "�}",
        (ufo & UFO_ALOHA) ? "�W" : "�w",
        (ufo & UFO_QUIET) ? "�R" : "�w",
        (ufo & UFO_CLOAK) ? "��" : "�w");
  }
#else
  ufo &= UFO_PAGER | UFO_CLOAK | UFO_QUIET;
  if (orig_flag != ufo)
  {
    orig_flag = ufo;
    sprintf(flagmsg, "%s%s%s  ",
        (ufo & UFO_PAGER) ? "��" : "�}",
        (ufo & UFO_QUIET) ? "�R" : "�w",
        (ufo & UFO_CLOAK) ? "��" : "�w");
  }
#endif

  if (now > uptime)    /* �L�F�l�]�n��s�ͤ�X�� */
  {
    struct tm *ptime;

    ptime = localtime(&now);

    if (cuser.day == ptime->tm_mday && cuser.month == ptime->tm_mon + 1)
      cutmp->status |= STATUS_BIRTHDAY;
    else
      cutmp->status &= ~STATUS_BIRTHDAY;

    uptime = now + 86400 - ptime->tm_hour * 3600 - ptime->tm_min * 60 - ptime->tm_sec;
  }


  if (cuser.money != orig_money)
  {
    orig_money = cuser.money;
    sprintf(coinmsg, "�� %4d%c",
        (orig_money & 0x7FF00000) ? (orig_money >> 20) : (orig_money & 0x7FFFFC00) ? (orig_money >> 10) : orig_money,
        (orig_money & 0x7FF00000) ? 'M' : (orig_money & 0x7FFFFC00) ? 'K' : ' ');
  }

  /* Thor.980913: �̱`���I�s status_foot() ���ɾ��O�C����s film�A�b 60 ��H�W�A
                  �G���ݰw�� hh:mm �ӯS�O�@�@�r���x�s�H�[�t */

  ufo = (now - (uptime - 86400)) / 60;    /* �ɥ� ufo �Ӱ��ɶ�(��) */

  /* itoc.010717: ��@�U feeter �Ϫ��שM FEETER_XXX �@�P */
  sprintf(feeter, COLOR1 "[%12.12s %02d:%02d]" COLOR2 " �u�W %-4d�H  �ڬO%-12s %8s [�I�s��]%-9s ",
          fshm->today, ufo / 60, ufo % 60, total_user, cuser.userid, coinmsg, flagmsg);

  outf(feeter);
}


void
movie()
{
  /* Thor: it is depend on which state */
  if ((bbsmode <= M_XMENU) && (cuser.ufo & UFO_MOVIE))
    film_out(FILM_MOVIE, MENU_XNOTE);

  /* itoc.010403: �� feeter �� status �W�ߥX�� */
  status_foot();
}


typedef struct
{
  void *func;
  /* int (*func) (); */
  usint level;
  int umode;
  char *desc;
}    MENU;


#define    MENU_LOAD    1
#define    MENU_DRAW    2
#define    MENU_FILM    4


#define    PERM_MENU    PERM_PURGE


static MENU menu_main[];


/* ----------------------------------------------------- */
/* administrator's maintain menu                         */
/* ----------------------------------------------------- */

static MENU menu_admin[];

static MENU menu_aboard[] =
{
  "bin/admutil.so:a_newbrd",      PERM_ALLBOARD,    - M_SYSTEM,
  "NewBoard    �� �s�W�ݪO ��",

  "bin/admutil.so:a_editbrd",     PERM_ALLBOARD,    - M_SYSTEM,
  "SetBoard    �� �]�w�ݪO ��",

  "bin/innbbs.so:a_innbbs",       PERM_ALLBOARD,    - M_SYSTEM,
  "InnBBS      �� ��H�]�w ��",

  menu_admin, PERM_MENU + Ctrl('A'), M_AMENU,
  "�ݪO���@"
};

static MENU menu_acct[] =
{
#ifdef HAVE_REGISTER_FORM
  "bin/admutil.so:a_register",       PERM_ALLREG,   - M_SYSTEM,
  "Register    �� �f���U�� ��",

  "bin/admutil.so:a_regmerge",       PERM_ALLREG,   - M_SYSTEM,
  "Merge       �� �_��f�� ��",

  "bin/admutil.so:a_verifyById",    PERM_ALLREG,   - M_SYSTEM,
  "CheckID     �� �� �� �� ��",

  "bin/admutil.so:a_changeRealName", PERM_ALLACCT,  - M_SYSTEM,
  "RealName    �� �ܧ�m�W ��",
#endif

  "bin/admutil.so:a_show_from",      PERM_ALLREG,   - M_SYSTEM,
  "ShowFrom    �� �d�ߨӷ� ��",

  menu_admin, PERM_MENU + Ctrl('A'), M_AMENU,
  "�b�����@"
};


static MENU menu_admin[] =
{
  "bin/admutil.so:a_user",           PERM_ALLACCT,  - M_SYSTEM,
  "User        �� �U�ȸ�� ��",

  "bin/admutil.so:a_search",         PERM_ALLACCT,  - M_SYSTEM,
  "Hunt        �� �j�M���� ��",

  menu_aboard, PERM_ALLBOARD,        M_AMENU,
  "Board       �� �ݪO���� ��",

  menu_acct, PERM_ALLREG,            M_AMENU,
  "Account     �� �b������ ��",

  "bin/admutil.so:a_xfile",          PERM_ALLADMIN, - M_XFILES,
  "Xfile       �� �t���ɮ� ��",

  /*
  "bin/admutil.so:a_restore",        PERM_SYSOP,    - M_SYSTEM,
  "QRestore    �� �٭�ƥ� ��",
  */

  "bin/admutil.so:a_resetsys",       PERM_ALLADMIN, - M_SYSTEM,
  "ResetBBS    �� ���m�t�� ��",

  menu_main, PERM_MENU + Ctrl('A'), M_AMENU,
  "�t�κ��@"
};


/* ----------------------------------------------------- */
/* mail menu                                             */
/* ----------------------------------------------------- */

static int
XoMbox()
{
  xover(XZ_MBOX);
  return 0;
}

static MENU menu_mail[] =
{
  XoMbox,       PERM_BASIC,    M_RMAIL,
  "Read       �u �\\Ū�H�� �t",

  m_send,       PERM_VALID,    M_SMAIL,
  "Mail       �u �����H�H �t",

#ifdef MULTI_MAIL
  /* Thor.981009: ����R�����B�H */
  m_list,       PERM_VALID,    M_SMAIL,
  "List       �u �s�ձH�H �t",
#endif

  m_internet,   PERM_INTERNET, M_SMAIL,
  "Internet   �u �H�̩f�� �t",

  /* guessi.060328 �ϥΪ̦ۭq�ӫH��H */
  m_setforward, PERM_VALID,    M_SMAIL,
  "SetForward �u �]�w��H �t",

#ifdef HAVE_SIGNED_MAIL
  m_verify,     0,             M_XMODE,
  "Verify     �u ���ҫH�� �t",
#endif

#ifdef HAVE_MAIL_ZIP
  m_zip,        PERM_INTERNET, M_SMAIL,
  "Zip        �u ���]��� �t",
#endif

  m_sysop,      0,             M_SMAIL,
  "Yes Sir!   �u ��ѯ��� �t",

  /* itoc.000512: �s�W m_bm */
  "bin/admutil.so:m_bm",  PERM_ALLADMIN, - M_SMAIL,
  "BM All     �u �O�D�q�i �t",

  /* itoc.000512: �s�W m_all */
  "bin/admutil.so:m_all", PERM_ALLADMIN, - M_SMAIL,
  "User All   �u �����q�i �t",

  menu_main, PERM_MENU + Ctrl('A'), M_MMENU,    /* itoc.020829: �� guest �S�ﶵ */
  "�q�l�l��"
};


/* ----------------------------------------------------- */
/* talk menu                                             */
/* ----------------------------------------------------- */

static int
XoUlist()
{
  xover(XZ_ULIST);
  return 0;
}

static MENU menu_talk[];

/* --------------------------------------------------- */
/* list menu                                           */
/* --------------------------------------------------- */

static MENU menu_list[] =
{
  t_pal,                  PERM_BASIC,      M_PAL,
  "Pal        �u �B�ͦW�� �t",

#ifdef HAVE_LIST
  t_list,                 PERM_BASIC,      M_PAL,
  "List       �u �S�O�W�� �t",
#endif

#ifdef HAVE_ALOHA
  "bin/aloha.so:t_aloha", PERM_PAGE,       - M_PAL,
  "Aloha      �u �W���q�� �t",
#endif

#ifdef LOGIN_NOTIFY
  t_loginNotify,          PERM_PAGE,       M_PAL,
  "Notify     �u �t�Ψ�M �t",
#endif

  menu_talk,              PERM_MENU + 'P', M_TMENU,
  "�U���W��"
};


static MENU menu_talk[] =
{
  XoUlist,        0,          M_LUSERS,
  "Users      �u �C�ȦW�� �t",

  menu_list,      PERM_BASIC, M_TMENU,
  "ListMenu   �u �]�w�W�� �t",

  t_pager,        PERM_BASIC, M_XMODE,
  "Pager      �u �����I�s �t",
  /*
  t_cloak,        PERM_CLOAK, M_XMODE,
  "Invis      �u �����K�k �t",
  */
  t_query,        0,          M_QUERY,
  "Query      �u �d�ߺ��� �t",

  t_talk,         PERM_PAGE,  M_PAGE,
  "Talk       �u ���ܺ��� �t",

  /* Thor.990220: ��ĥ~�� */
  "bin/chat.so:t_chat", PERM_CHAT, - M_CHAT,
  "ChatRoom   �u ���f��� �t",

  t_display,     PERM_BASIC,  M_BMW,
  "Display    �u �s�����y �t",

  t_bmw,         PERM_BASIC,  M_BMW,
  "Write      �u �^�U���y �t",

  menu_main, PERM_MENU + 'U', M_TMENU,
  "�𶢲��"
};


/* ----------------------------------------------------- */
/* user menu                                             */
/* ----------------------------------------------------- */

static MENU menu_user[];

/* --------------------------------------------------- */
/* register menu                                       */
/* --------------------------------------------------- */

static MENU menu_register[] =
{

#ifdef HAVE_REGISTER_FORM
  u_register, PERM_BASIC, M_UFILES,
  "Register   �u ����U�� �t\033[1;33m(�b���{�ҽХ���g���U��)\033[m",
#endif

  u_addr,     PERM_BASIC, M_XMODE,
  "Address    �u �H�c�{�� �t(���ۧ���EMAIL�T�{ �~�|1)",

#ifdef HAVE_REGKEY_CHECK
  u_verify,   PERM_BASIC, M_UFILES,
  "Verify     �u �����ҽX �t(���ۧ���EMAIL�T�{ �~�|2)",
#endif

  u_deny,     PERM_BASIC, M_XMODE,
  "Perm       �u ��_�v�� �t",

  u_realname, PERM_BASIC, M_UFILES,
  "ChangeName �u �ӽЧ�W �t",

  menu_user,  PERM_MENU + 'A', M_UMENU,
  "���U���"
};


static MENU menu_user[] =
{
  u_info,        PERM_BASIC, M_XMODE,
  "Info       �u �ӤH��� �t",

  u_setup,       0,          M_UFILES,
  "Habit      �u �ߦn�Ҧ� �t",

  menu_register, PERM_BASIC, M_UMENU,
  "Register   �u ���U���� �t",

  u_lock,        PERM_BASIC, M_IDLE,
  "Lock       �u ��w�ù� �t",

  u_xfile,       PERM_BASIC, M_UFILES,
  "Xfile      �u �ӤH�ɮ� �t",

#ifdef HAVE_CLASSTABLE
  "bin/classtable.so:main_classtable", PERM_BASIC, - M_XMODE,
  "ClassTable �u �ӤH�Ҫ� �t",
#endif

  /* ��GUEST�]��ϥάd�߱K�X�\\�� */
  "bin/xyz.so:x_password", 0, - M_XMODE,
  "Password   �u �ѰO�K�X �t",

  u_log,          PERM_BASIC, M_UFILES,
  "ViewLog    �u �W���O�� �t",

  menu_main, PERM_MENU + Ctrl('A'), M_UMENU,
  "�ӤH�]�w"
};



#ifdef HAVE_EXTERNAL

/* ----------------------------------------------------- */
/* tool menu                                             */
/* ----------------------------------------------------- */

static MENU menu_tool[];


#ifdef HAVE_SONG
/* ----------------------------------------------------- */
/* song menu                                             */
/* ----------------------------------------------------- */

static MENU menu_song[] =
{
  "bin/song.so:XoSongLog",  0, - M_XMODE,
  "KTV        �u �I�q���� �t",

  "bin/song.so:XoSongMain", 0, - M_XMODE,
  "Book       �u �۩ұ��� �t",

  "bin/song.so:XoSongSub",  0, - M_XMODE,
  "Note       �u �q����Z �t",

  menu_tool, PERM_MENU + 'K', M_XMENU,
  "���I�q��"
};
#endif

#ifdef HAVE_GAME

/* --------------------------------------------------- */
/* game menu                                           */
/* --------------------------------------------------- */

static MENU menu_game[];

static MENU menu_game1[] =
{
  "bin/liteon.so:main_liteon", 0, - M_GAME,
  "0LightOn   �� �ж��}�O ��",

  "bin/guessnum.so:guessNum",  0, - M_GAME,
  "1GuessNum  �� ���q�Ʀr ��",

  "bin/guessnum.so:fightNum",  0, - M_GAME,
  "2FightNum  �� ���q�Ʀr ��",

  "bin/km.so:main_km",         0, - M_GAME,
  "3KongMing  �� �թ����� ��",

  "bin/recall.so:main_recall", 0, - M_GAME,
  "4Recall    �� �^�Ф��Z ��",

  "bin/mine.so:main_mine",     0, - M_GAME,
  "5Mine      �� �ý�a�p ��",

  "bin/fantan.so:main_fantan", 0, - M_GAME,
  "6Fantan    �� �f�u���s ��",

  "bin/dragon.so:main_dragon", 0, - M_GAME,
  "7Dragon    �� ���s�C�� ��",

  "bin/nine.so:main_nine",     0, - M_GAME,
  "8Nine      �� �Ѧa�E�E ��",

  menu_game, PERM_MENU + '0', M_XMENU,
  "�q���Ŷ�"
};

static MENU menu_game2[] =
{
  "bin/dice.so:main_dice",       0, - M_GAME,
  "0Dice      �� �g�Y��l ��",

  "bin/gp.so:main_gp",           0, - M_GAME,
  "1GoldPoker �� ���P���J ��",

  "bin/bj.so:main_bj",           0, - M_GAME,
  "2BlackJack �� �G�Q�@�I ��",

  "bin/chessmj.so:main_chessmj", 0, - M_GAME,
  "3ChessMJ   �� �H�ѳ±N ��",

  "bin/seven.so:main_seven",     0, - M_GAME,
  "4Seven     �� �䫰�C�i ��",

  "bin/race.so:main_race",       0, - M_GAME,
  "5Race      �� �i�ɰ��� ��",

  "bin/bingo.so:main_bingo",     0, - M_GAME,
  "6Bingo     �� ���G�j�� ��",

  "bin/marie.so:main_marie",     0, - M_GAME,
  "7Marie     �� �j�p���� ��",

  "bin/bar.so:main_bar",         0, - M_GAME,
  "8Bar       �� �a�x���� ��",

  menu_game, PERM_MENU + '0', M_XMENU,
  "�C���ֶ�"
};

static MENU menu_game3[] =
{
  "bin/pip.so:main_pip", PERM_BASIC, - M_GAME,
  "0Chicken   �� �q�l�p�� ��",

  "bin/pushbox.so:main_pushbox", 0, - M_GAME,
  "1PushBox   �� �ܮw�f�f ��",

  "bin/tetris.so:main_tetris",   0, - M_GAME,
  "2Tetris    �� �Xù���� ��",

  "bin/gray.so:main_gray",       0, - M_GAME,
  "3Gray      �� �L�Ǥj�� ��",

  menu_game, PERM_MENU + '0', M_XMENU,
  "�Ϥ�S��"
};

static MENU menu_game[] =
{
  menu_game1, PERM_BASIC, M_XMENU,
  "1Game      �u �q���Ѱ� �t",

  menu_game2, PERM_BASIC, M_XMENU,
  "2Game      �u �C���ֶ� �t",

  menu_game3, PERM_BASIC, M_XMENU,
  "3Game      �u �Ϥ�S�� �t",

  menu_main, PERM_MENU + '1', M_XMENU,
  "�C���H��"
};

#endif


#ifdef HAVE_BUY
/* --------------------------------------------------- */
/* buy menu                                            */
/* --------------------------------------------------- */

static MENU menu_buy[] =
{
  "bin/bank.so:x_bank",  PERM_BASIC, - M_GAME,
  "Bank       �u �H�U�Ȧ� �t",
/*
  "bin/bank.so:b_invis", PERM_BASIC, - M_GAME,
  "Invis      �u ���β{�� �t",

  "bin/bank.so:b_cloak", PERM_BASIC, - M_GAME,
  "Cloak      �u �L������ �t",

  "bin/bank.so:b_mbox",  PERM_BASIC, - M_GAME,
  "Mbox       �u �H�c�L�� �t",
*/
  menu_tool, PERM_MENU + 'B', M_XMENU,
  "���ĥ���"
};
#endif


/* --------------------------------------------------- */
/* other tools menu                                    */
/* --------------------------------------------------- */

static MENU menu_other[] =
{
  pad_view,               0,                    M_READA,
  "Note       �u �[�ݯd�� �t",

  /* itoc.010309: ���������i�H�g�d���O */
  pad_draw,               PERM_POST,            M_POST,
  "Pad        �u �߱���~ �t",

  #ifdef HAVE_CALENDAR
  "bin/todo.so:main_todo", PERM_BASIC,         - M_XMODE,
  "Todo       �u �ӤH��{ �t",

  "bin/calendar.so:main_calendar", 0,          - M_XMODE,
  "XCalendar  �u �U�~��� �t",
  #endif

  #ifdef HAVE_CREDIT
  "bin/new_credit.so:credit_main", PERM_BASIC, - M_XMODE,
  "CreditNote �u �O�b�⥾ �t",
  #endif

  "bin/xyz.so:x_password", 0,                  - M_XMODE,
  "Password   �u �ѰO�K�X �t",

  #ifdef HAVE_TIP
  "bin/xyz.so:x_tip",      0,                  - M_READA,
  "HowTo      �u �оǺ��F �t",
  #endif

  /* itoc.020829: �� guest �S�ﶵ */
  menu_tool, PERM_MENU + Ctrl('A'), M_XMENU,
  "��L�\\��"
};
#endif

#ifdef HAVE_EXTERNAL

static int
menu_sysinfo()
{
  double load[3];
  time_t now;

  getloadavg(load, 3);
  time(&now);

  clear();
  vs_bar("�t�θ�T");

  move(5, 0);
  prints("�ثe��m: %s�E%s (%s) \n", SCHOOLNAME, BBSNAME, MYIPADDR);
  prints("�t�έt��: \033[1;3%s\033[m (%.2f %.2f %.2f)\n", load[0] > 20 ? "1m����": load[0] > 10 ? "3m����" : "2m�}�n", load[0], load[1], load[2]);
  prints("�u�W�H��: %d / %d (MAX)\n", ushm->count, MAXACTIVE);
  prints("�t�ήɶ�: %s", ctime(&now));

  vmsg(NULL);
  return 0;
}


static MENU menu_tool[] =
{

#ifdef HAVE_BUY
  menu_buy,                    PERM_BASIC, M_XMENU,
  "Market     �u ���ĥ��� �t",
#endif

#ifdef HAVE_SONG
  menu_song,                   0,          M_XMENU,
  "KTV        �u �u���I�q �t",
#endif

  "bin/vote.so:vote_all",      0,          - M_VOTE,
  "VoteALL    �u �벼���� �t",

  menu_other,                  0,          M_XMENU,
  "Other      �u ���C���K �t",

  "bin/eatwhere.so:eat_where",  0,          - M_XMODE,
  "Gourmet    �u �����Ѧa �t",

  menu_sysinfo,		       0,          M_XMODE,
  "Xinfo      �u �t�θ�T �t",

  /* itoc.020829: �� guest �S�ﶵ */
  menu_main, PERM_MENU + Ctrl('A'), M_XMENU,
  "�ӤH�u��"
};

#endif    /* HAVE_EXTERNAL */


/* ----------------------------------------------------- */
/* main menu                                             */
/* ----------------------------------------------------- */

static int
Gem()
{
  /* itoc.001109: �ݪO�`�ަb (A)nnounce �U�� GEM_X_BIT�A��K�}�O */
  XoGem("gem/"FN_DIR, "��اG�i��", (HAS_PERM(PERM_ALLBOARD) ? (GEM_W_BIT | GEM_X_BIT | GEM_M_BIT) : 0));
  return 0;
}

static MENU menu_main[] =
{

  menu_admin, PERM_ALLADMIN, M_AMENU,
  "0Admin     �i \033[1;32m�t�κ��@��\033[m �j",

  Gem,        0,             M_GEM,
  "Announce   �i ��ؤ��G�� �j",

  Boards,     0,             M_BOARD,
  "Boards     �i �Ҧ��Q�תO �j",

  Class,      0,             M_BOARD,
  "Class      �i �����Q�׶� �j",

#ifdef MY_FAVORITE
  MyFavorite, PERM_BASIC,    M_MF,
  "Favorite   �i �̷R���ݪO �j",
#endif

  menu_mail,  0,             M_MMENU,
  "Mail       �i �H���Ƨ� �j",

  menu_talk,  0,             M_TMENU,
  "Talk       �i �𶢲�Ѧa �j",

  menu_user,  0,             M_UMENU,
  "User       �i �ӤH�u��� �j",

#ifdef HAVE_GAME
  menu_game, PERM_BASIC,     M_XMENU,
  "Play       �i �p���T�֩� �j",
#endif

#ifdef HAVE_EXTERNAL
  menu_tool,  0,             M_XMENU,
  "Xyz        �i �S��\\��� �j",
#endif

  goodbye,    0,             M_XMODE,
  "Goodbye     �A�|��F��p�� ",

  NULL, PERM_MENU + 'B', M_0MENU,
  "�D�\\���"
};


void
menu()
{
  MENU *menu, *mptr, *table[12];
  usint level, mode;
  int cc, cx;            /* current / previous cursor position */
  int max, mmx;          /* current / previous menu max */
  int cmd, depth;
  char *str;

  mode = MENU_LOAD | MENU_DRAW | MENU_FILM;
  menu = menu_main;
  level = cuser.userlevel;
  depth = mmx = 0;

  for (;;)
  {
    if (mode & MENU_LOAD)
    {
      for (max = -1;; menu++)
      {
        cc = menu->level;
        if (cc & PERM_MENU)
        {

#ifdef    MENU_VERBOSE
          if (max < 0)        /* �䤣��A�X�v�����\��A�^�W�@�h�\��� */
          {
            menu = (MENU *) menu->func;
            continue;
          }
#endif

          break;
        }

		/* �����v���~�q�X */
        if (cc && !(cc & level))
          continue;

        table[++max] = menu;
      }

      if (mmx < max)
        mmx = max;

      /* �Ĥ@���W���Y���s�H�A�i�J Mail ��� */
      if ((depth == 0) && HAS_STATUS(STATUS_BIFF))
        cmd = 'M';
      else
        cmd = cc ^ PERM_MENU;    /* default command */

      utmp_mode(menu->umode);
    }

    if (mode & MENU_DRAW)
    {
      if (mode & MENU_FILM)
      {
        clear();
        movie();
        cx = -1;
      }

      vs_head(menu->desc, NULL);

      mode = 0;
      do
      {
        move(MENU_XPOS + mode, MENU_YPOS + 2);
        if (mode <= max)
        {
          mptr = table[mode];
          str = mptr->desc;
          prints("(\033[1;36m%c\033[m)", *str++);
          outs(str);
        }
        clrtoeol();
      } while (++mode <= mmx);

      mmx = max;
      mode = 0;
    }

    switch (cmd)
    {
    case KEY_DOWN:
      cc = (cc == max) ? 0 : cc + 1;
      break;

    case KEY_UP:
      cc = (cc == 0) ? max : cc - 1;
      break;

	/* itoc.020829: �w�]�ﶵ�Ĥ@�� */
    case Ctrl('A'):
    case KEY_HOME:
      cc = 0;
      break;

    case KEY_END:
      cc = max;
      break;

    case KEY_PGUP:
      cc = (cc == 0) ? max : 0;
      break;

    case KEY_PGDN:
      cc = (cc == max) ? 0 : max;
      break;

    case '\n':
    case KEY_RIGHT:
      mptr = table[cc];
      cmd = mptr->umode;
#if 1
      /* Thor.990212: dynamic load , with negative umode */
      if (cmd < 0)
      {
        void *p = DL_get(mptr->func);
        if (!p)
          break;
        mptr->func = p;
        cmd = -cmd;
        mptr->umode = cmd;
      }
#endif
      utmp_mode(cmd);

	  /* �l�ؿ��� mode �n <= M_XMENU */
      if (cmd <= M_XMENU)
      {
        menu->level = PERM_MENU + mptr->desc[0];
        menu = (MENU *) mptr->func;

        mode = MENU_LOAD | MENU_DRAW;
		/* itoc.010304: �i�J�l��歫�� movie */
        /* mode = MENU_LOAD | MENU_DRAW | MENU_FILM; */

        depth++;
        continue;
      }

      {
        int (*func) ();

        func = mptr->func;
        mode = (*func) ();
      }

      utmp_mode(menu->umode);

      if (mode == XEASY)
      {
        outf(feeter);
        mode = 0;
      }
      else
      {
        mode = MENU_DRAW | MENU_FILM;
      }

      cmd = mptr->desc[0];
      continue;

#ifdef EVERY_Z
    case Ctrl('Z'):
      every_Z(0);
      goto every_key;

    case Ctrl('U'):
      every_U(0);
      goto every_key;
#endif

    /* itoc.010911: Select everywhere�A���A����O�b M_0MENU */
    case 's':
      utmp_mode(M_BOARD);
      Select();
      goto every_key;

#ifdef MY_FAVORITE
    /* itoc.010911: Favorite everywhere�A���A����O�b M_0MENU */
    case 'f':
    case Ctrl('F'):
      if (cuser.userlevel)    /* itoc.010407: �n�ˬd�v�� */
      {
        utmp_mode(M_MF);
        MyFavorite();
      }
      goto every_key;
#endif

    /* itoc.020301: Read currboard in M_0MENU */
    case 'r':
      if (bbsmode == M_0MENU)
      {
        if (currbno >= 0)
        {
          utmp_mode(M_BOARD);
          XoPost(currbno);
          xover(XZ_POST);
#ifndef ENHANCED_VISIT
          time(&brd_visit[currbno]);
#endif
        }
        goto every_key;
      }
	  /* �Y���b M_0MENU ���� r ���ܡA�n�����@����� */
      goto default_key;
      /* �S����B�z���� */

every_key:
      utmp_mode(menu->umode);
      mode = MENU_DRAW | MENU_FILM;
      cmd = table[cc]->desc[0];
      continue;

    case KEY_LEFT:
      if (depth > 0)
      {
        menu->level = PERM_MENU + table[cc]->desc[0];
        menu = (MENU *) menu->func;
        mode = MENU_LOAD | MENU_DRAW;
		/* itoc.010304: �h�X�l��歫�� movie */
        /* mode = MENU_LOAD | MENU_DRAW | MENU_FILM; */
        depth--;
        continue;
      }
      cmd = 'G';

default_key:
    default:
	  /* �ഫ���j�g */
      if (cmd >= 'a' && cmd <= 'z')
        cmd ^= 0x20;

      cc = 0;
      for (;;)
      {
        if (table[cc]->desc[0] == cmd)
          break;
        if (++cc > max)
        {
          cc = cx;
          goto menu_key;
        }
      }
    }

    /* �Y��в��ʦ�m */
    if (cc != cx)
    {
#ifdef CURSOR_BAR
      /* �� CURSOR_BAR */
      if (cx >= 0)
      {
        move(MENU_XPOS + cx, MENU_YPOS);
        if (cx <= max)
        {
          mptr = table[cx];
          str = mptr->desc;
          prints("  (\033[1;36m%c\033[m)%s ", *str, str + 1);
        }
        else
        {
          outs("  ");
        }
      }
      move(MENU_XPOS + cc, MENU_YPOS);
      mptr = table[cc];
      str = mptr->desc;
      prints(COLOR4 "> (%c)%s \033[m", *str, str + 1);
      cx = cc;
#else
      /* �S�� CURSOR_BAR */
      if (cx >= 0)
      {
        move(MENU_XPOS + cx, MENU_YPOS);
        outc(' ');
      }
      move(MENU_XPOS + cc, MENU_YPOS);
      outc('>');
      cx = cc;
#endif
    }
    else
    {
	  /* �Y��Ъ���m�S���� */
#ifdef CURSOR_BAR
      move(MENU_XPOS + cc, MENU_YPOS);
      mptr = table[cc];
      str = mptr->desc;
      prints(COLOR4 "> (%c)%s \033[m", *str, str + 1);
#else
      move(MENU_XPOS + cc, MENU_YPOS + 1);
#endif
    }

menu_key:

    cmd = vkey();
  }
}
