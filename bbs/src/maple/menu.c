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
/* 離開 BBS 站                                           */
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
    else if (!(count % 5))    /* itoc.020122: 有 pad 才印 */
    {
      clear();
      move(0, 23);
      prints("【 酸 甜 苦 辣 留 言 板 】        第 %d 頁\n\n", count / 5 + 1);
    }

    outs(pad->msg);
    count++;

    if (!(count % 5))
    {
      move(b_lines, 0);
      outs("請按 [SPACE] 繼續觀賞，或按其他鍵結束： ");

      /* itoc.010127: 修正在偵測左右鍵全形下，按左鍵會跳離二層選單的問題 */
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

  /* itoc.註解: 不想用高彩度，太花 */
  static char pcolors[6] = {31, 32, 33, 34, 35, 36};

  /* itoc.010309: 留言板提供不同的顏色 */
  color = vans("心情顏色 1) \033[41m  \033[m 2) \033[42m  \033[m 3) \033[43m  \033[m "
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
    outs("\n請留言 (至多三行)，按[Enter]結束");
    for (i = 0; (i < 3) &&
        vget(16 + i, 0, "：", buf[i], 71, DOECHO); i++);
    cc = vans("(S)存檔觀賞 (E)重新來過 (Q)算了？[S] ");
    if (cc == 'q' || i == 0)
      return 0;
  } while (cc == 'e');

  time(&(pad.tpad));

  /* itoc.020812.註解: 改版面的時候要注意 struct Pad.msg[] 是否夠大 */
  str = pad.msg;
  sprintf(str, "╭┤\033[1;46m %s － %s \033[m├", cuser.userid, cuser.username);

  for (cc = strlen(str); cc < 60; cc += 2)
    strcpy(str + cc, "─");
  if (cc == 60)
    str[cc++] = ' ';

  sprintf(str + cc,
      "\033[1;44m %s \033[m╮\n"
      "│  \033[1;%dm%-70s\033[m  │\n"
      "│  \033[1;%dm%-70s\033[m  │\n"
      "╰  \033[1;%dm%-70s\033[m  ╯\n",
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
   /* guessi.091228 減少畫面重繪 回傳XO_NONE */
  if (vans("您確定要離開【 東方小城 】嗎？ Y)再見 N)返回 [N] ") != 'y')
    return XO_NONE;

#ifdef LOG_BMW
  /* lkchu.981201: 水球記錄處理 */
  bmw_log();
#endif

  /* itoc.000407: 離站畫面一併簡化 */
  if (!(cuser.ufo & UFO_MOTD))
  {
    clear();
    prints("親愛的 \033[32m%s(%s)\033[m，別忘了再度光臨【 %s 】\n以下是您在站內的註冊資料：\n",
           cuser.userid, cuser.username, str_site);
    acct_show(&cuser, 0);
    vmsg(NULL);
  }

  u_exit("EXIT ");

  /* guessi.060610 離站清空畫面 */
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
  char buf[(T_COLS - 1) - 79 + 69 + 1];    /* d_cols 最大可能是(T_COLS -1) */
  char ttl[(T_COLS - 1) - 79 + 69 + 1];
  int spc, len;

  if (mid)    /* xxxx_head() 都是用 vs_head(title, str_site); */
  {
    clear();
  }
  else        /* menu() 中才用 vs_head(title, NULL); 選單中無需 clear() */
  {
    move(0, 0);
    clrtoeol();
    mid = str_site;
  }

  /* guessi.101028 當出現註冊單的時候 僅顯示四個字(8格)
   * len: 中間還剩下多長的空間 (65 - 8 = 57)
   */
  if (HAS_PERM(PERM_ALLREG) && rec_num(FN_RUN_RFORM, sizeof(RFORM)))
    len = d_cols + 57 - strlen(currboard);
  else
    len = d_cols + 65 - strlen(title) - strlen(currboard);

  /* guessi.070103 guest不顯示郵件提示 */
  if (HAS_STATUS(STATUS_BIFF) && strcmp(cuser.userid, STR_GUEST))
  {
    mid = "\033[5;41m 有人遞情書給你唷 \033[m";
    spc = 18;
  }
  else
  {
    /* 空間不夠擺下原本要擺的 mid，只好把 mid 截斷 */
    if ((spc = strlen(mid)) > len)
    {
      spc = len;
      memcpy(ttl, mid, spc);
      mid = ttl;
      mid[spc] = '\0';
    }
  }

  /* 擺完 mid 以後，中間還有 spc 格空間，在 mid 左右各放 spc/2 長的空白 */
  spc = 2 + len - spc;
  len = 1 - spc & 1;
  memset(buf, ' ', spc >>= 1);
  buf[spc] = '\0';

/* guessi.101027 註冊單提示獨立顯示於左上角 */
#ifdef COLOR_HEADER
  spc = (time(0) % 7) + '1';
  if (HAS_PERM(PERM_ALLREG) && rec_num(FN_RUN_RFORM, sizeof(RFORM)))
    title = "有註冊單";
  prints("\033[1;4%cm【%s】%s\033[33m%s\033[1;37;4%cm%s看板《%s》\033[m\n",
       spc, title, buf, mid, spc, buf + len, currboard);
#else
  if (HAS_PERM(PERM_ALLREG) && rec_num(FN_RUN_RFORM, sizeof(RFORM)))
    title = "\033[5;36m有註冊單\033[;1;44m";
  prints("\033[1;44m【%s】%s\033[33m%s\033[1;37;44m%s看板《%s》\033[m\n",
       title, buf, mid, buf + len, currboard);
#endif
}


/* ------------------------------------- */
/* 動畫處理                              */
/* ------------------------------------- */

/* itoc.010403: 把 feeter 的 status 獨立出來，預備供其他 function 叫用 */
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

  /* Thor: 同時 顯示 呼叫器 上站通知 隱身 */

#ifdef HAVE_ALOHA
  ufo &= UFO_PAGER | UFO_ALOHA | UFO_CLOAK | UFO_QUIET;
  if (orig_flag != ufo)
  {
    orig_flag = ufo;
    sprintf(flagmsg, "%s%s%s%s",
        (ufo & UFO_PAGER) ? "關" : "開",
        (ufo & UFO_ALOHA) ? "上" : "─",
        (ufo & UFO_QUIET) ? "靜" : "─",
        (ufo & UFO_CLOAK) ? "隱" : "─");
  }
#else
  ufo &= UFO_PAGER | UFO_CLOAK | UFO_QUIET;
  if (orig_flag != ufo)
  {
    orig_flag = ufo;
    sprintf(flagmsg, "%s%s%s  ",
        (ufo & UFO_PAGER) ? "關" : "開",
        (ufo & UFO_QUIET) ? "靜" : "─",
        (ufo & UFO_CLOAK) ? "隱" : "─");
  }
#endif

  if (now > uptime)    /* 過了子夜要更新生日旗標 */
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
    sprintf(coinmsg, "銀 %4d%c",
        (orig_money & 0x7FF00000) ? (orig_money >> 20) : (orig_money & 0x7FFFFC00) ? (orig_money >> 10) : orig_money,
        (orig_money & 0x7FF00000) ? 'M' : (orig_money & 0x7FFFFC00) ? 'K' : ' ');
  }

  /* Thor.980913: 最常見呼叫 status_foot() 的時機是每次更新 film，在 60 秒以上，
                  故不需針對 hh:mm 來特別作一字串儲存以加速 */

  ufo = (now - (uptime - 86400)) / 60;    /* 借用 ufo 來做時間(分) */

  /* itoc.010717: 改一下 feeter 使長度和 FEETER_XXX 一致 */
  sprintf(feeter, COLOR1 "[%12.12s %02d:%02d]" COLOR2 " 線上 %-4d人  我是%-12s %8s [呼叫器]%-9s ",
          fshm->today, ufo / 60, ufo % 60, total_user, cuser.userid, coinmsg, flagmsg);

  outf(feeter);
}


void
movie()
{
  /* Thor: it is depend on which state */
  if ((bbsmode <= M_XMENU) && (cuser.ufo & UFO_MOVIE))
    film_out(FILM_MOVIE, MENU_XNOTE);

  /* itoc.010403: 把 feeter 的 status 獨立出來 */
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
  "NewBoard    ◤ 新增看板 ◢",

  "bin/admutil.so:a_editbrd",     PERM_ALLBOARD,    - M_SYSTEM,
  "SetBoard    ◤ 設定看板 ◢",

  "bin/innbbs.so:a_innbbs",       PERM_ALLBOARD,    - M_SYSTEM,
  "InnBBS      ◤ 轉信設定 ◢",

  menu_admin, PERM_MENU + Ctrl('A'), M_AMENU,
  "看板維護"
};

static MENU menu_acct[] =
{
#ifdef HAVE_REGISTER_FORM
  "bin/admutil.so:a_register",       PERM_ALLREG,   - M_SYSTEM,
  "Register    ◤ 審註冊單 ◢",

  "bin/admutil.so:a_regmerge",       PERM_ALLREG,   - M_SYSTEM,
  "Merge       ◤ 復原審核 ◢",

  "bin/admutil.so:a_verifyById",    PERM_ALLREG,   - M_SYSTEM,
  "CheckID     ◤ 身 份 證 ◢",

  "bin/admutil.so:a_changeRealName", PERM_ALLACCT,  - M_SYSTEM,
  "RealName    ◤ 變更姓名 ◢",
#endif

  "bin/admutil.so:a_show_from",      PERM_ALLREG,   - M_SYSTEM,
  "ShowFrom    ◤ 查詢來源 ◢",

  menu_admin, PERM_MENU + Ctrl('A'), M_AMENU,
  "帳號維護"
};


static MENU menu_admin[] =
{
  "bin/admutil.so:a_user",           PERM_ALLACCT,  - M_SYSTEM,
  "User        ◤ 顧客資料 ◢",

  "bin/admutil.so:a_search",         PERM_ALLACCT,  - M_SYSTEM,
  "Hunt        ◤ 搜尋引擎 ◢",

  menu_aboard, PERM_ALLBOARD,        M_AMENU,
  "Board       ◤ 看板相關 ◢",

  menu_acct, PERM_ALLREG,            M_AMENU,
  "Account     ◤ 帳號相關 ◢",

  "bin/admutil.so:a_xfile",          PERM_ALLADMIN, - M_XFILES,
  "Xfile       ◤ 系統檔案 ◢",

  /*
  "bin/admutil.so:a_restore",        PERM_SYSOP,    - M_SYSTEM,
  "QRestore    ◤ 還原備份 ◢",
  */

  "bin/admutil.so:a_resetsys",       PERM_ALLADMIN, - M_SYSTEM,
  "ResetBBS    ◤ 重置系統 ◢",

  menu_main, PERM_MENU + Ctrl('A'), M_AMENU,
  "系統維護"
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
  "Read       ├ 閱\讀信件 ┤",

  m_send,       PERM_VALID,    M_SMAIL,
  "Mail       ├ 站內寄信 ┤",

#ifdef MULTI_MAIL
  /* Thor.981009: 防止愛情幸運信 */
  m_list,       PERM_VALID,    M_SMAIL,
  "List       ├ 群組寄信 ┤",
#endif

  m_internet,   PERM_INTERNET, M_SMAIL,
  "Internet   ├ 寄依妹兒 ┤",

  /* guessi.060328 使用者自訂來信轉寄 */
  m_setforward, PERM_VALID,    M_SMAIL,
  "SetForward ├ 設定轉寄 ┤",

#ifdef HAVE_SIGNED_MAIL
  m_verify,     0,             M_XMODE,
  "Verify     ├ 驗證信件 ┤",
#endif

#ifdef HAVE_MAIL_ZIP
  m_zip,        PERM_INTERNET, M_SMAIL,
  "Zip        ├ 打包資料 ┤",
#endif

  m_sysop,      0,             M_SMAIL,
  "Yes Sir!   ├ 投書站長 ┤",

  /* itoc.000512: 新增 m_bm */
  "bin/admutil.so:m_bm",  PERM_ALLADMIN, - M_SMAIL,
  "BM All     ├ 板主通告 ┤",

  /* itoc.000512: 新增 m_all */
  "bin/admutil.so:m_all", PERM_ALLADMIN, - M_SMAIL,
  "User All   ├ 全站通告 ┤",

  menu_main, PERM_MENU + Ctrl('A'), M_MMENU,    /* itoc.020829: 怕 guest 沒選項 */
  "電子郵件"
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
  "Pal        ├ 朋友名單 ┤",

#ifdef HAVE_LIST
  t_list,                 PERM_BASIC,      M_PAL,
  "List       ├ 特別名單 ┤",
#endif

#ifdef HAVE_ALOHA
  "bin/aloha.so:t_aloha", PERM_PAGE,       - M_PAL,
  "Aloha      ├ 上站通知 ┤",
#endif

#ifdef LOGIN_NOTIFY
  t_loginNotify,          PERM_PAGE,       M_PAL,
  "Notify     ├ 系統協尋 ┤",
#endif

  menu_talk,              PERM_MENU + 'P', M_TMENU,
  "各類名單"
};


static MENU menu_talk[] =
{
  XoUlist,        0,          M_LUSERS,
  "Users      ├ 遊客名單 ┤",

  menu_list,      PERM_BASIC, M_TMENU,
  "ListMenu   ├ 設定名單 ┤",

  t_pager,        PERM_BASIC, M_XMODE,
  "Pager      ├ 切換呼叫 ┤",
  /*
  t_cloak,        PERM_CLOAK, M_XMODE,
  "Invis      ├ 隱身密法 ┤",
  */
  t_query,        0,          M_QUERY,
  "Query      ├ 查詢網友 ┤",

  t_talk,         PERM_PAGE,  M_PAGE,
  "Talk       ├ 情話綿綿 ┤",

  /* Thor.990220: 改採外掛 */
  "bin/chat.so:t_chat", PERM_CHAT, - M_CHAT,
  "ChatRoom   ├ 眾口鑠金 ┤",

  t_display,     PERM_BASIC,  M_BMW,
  "Display    ├ 瀏覽水球 ┤",

  t_bmw,         PERM_BASIC,  M_BMW,
  "Write      ├ 回顧水球 ┤",

  menu_main, PERM_MENU + 'U', M_TMENU,
  "休閒聊天"
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
  "Register   ├ 填註冊單 ┤\033[1;33m(帳號認證請先填寫註冊單)\033[m",
#endif

  u_addr,     PERM_BASIC, M_XMODE,
  "Address    ├ 信箱認證 ┤(接著完成EMAIL確認 途徑1)",

#ifdef HAVE_REGKEY_CHECK
  u_verify,   PERM_BASIC, M_UFILES,
  "Verify     ├ 填驗證碼 ┤(接著完成EMAIL確認 途徑2)",
#endif

  u_deny,     PERM_BASIC, M_XMODE,
  "Perm       ├ 恢復權限 ┤",

  u_realname, PERM_BASIC, M_UFILES,
  "ChangeName ├ 申請更名 ┤",

  menu_user,  PERM_MENU + 'A', M_UMENU,
  "註冊選單"
};


static MENU menu_user[] =
{
  u_info,        PERM_BASIC, M_XMODE,
  "Info       ├ 個人資料 ┤",

  u_setup,       0,          M_UFILES,
  "Habit      ├ 喜好模式 ┤",

  menu_register, PERM_BASIC, M_UMENU,
  "Register   ├ 註冊相關 ┤",

  u_lock,        PERM_BASIC, M_IDLE,
  "Lock       ├ 鎖定螢幕 ┤",

  u_xfile,       PERM_BASIC, M_UFILES,
  "Xfile      ├ 個人檔案 ┤",

#ifdef HAVE_CLASSTABLE
  "bin/classtable.so:main_classtable", PERM_BASIC, - M_XMODE,
  "ClassTable ├ 個人課表 ┤",
#endif

  /* 讓GUEST也能使用查詢密碼功\能 */
  "bin/xyz.so:x_password", 0, - M_XMODE,
  "Password   ├ 忘記密碼 ┤",

  u_log,          PERM_BASIC, M_UFILES,
  "ViewLog    ├ 上站記錄 ┤",

  menu_main, PERM_MENU + Ctrl('A'), M_UMENU,
  "個人設定"
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
  "KTV        ├ 點歌紀錄 ┤",

  "bin/song.so:XoSongMain", 0, - M_XMODE,
  "Book       ├ 唱所欲言 ┤",

  "bin/song.so:XoSongSub",  0, - M_XMODE,
  "Note       ├ 歌本投稿 ┤",

  menu_tool, PERM_MENU + 'K', M_XMENU,
  "玩點歌機"
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
  "0LightOn   ♂ 房間開燈 ♀",

  "bin/guessnum.so:guessNum",  0, - M_GAME,
  "1GuessNum  ♂ 玩猜數字 ♀",

  "bin/guessnum.so:fightNum",  0, - M_GAME,
  "2FightNum  ♂ 互猜數字 ♀",

  "bin/km.so:main_km",         0, - M_GAME,
  "3KongMing  ♂ 孔明棋譜 ♀",

  "bin/recall.so:main_recall", 0, - M_GAME,
  "4Recall    ♂ 回憶之卵 ♀",

  "bin/mine.so:main_mine",     0, - M_GAME,
  "5Mine      ♂ 亂踩地雷 ♀",

  "bin/fantan.so:main_fantan", 0, - M_GAME,
  "6Fantan    ♂ 番攤接龍 ♀",

  "bin/dragon.so:main_dragon", 0, - M_GAME,
  "7Dragon    ♂ 接龍遊戲 ♀",

  "bin/nine.so:main_nine",     0, - M_GAME,
  "8Nine      ♂ 天地九九 ♀",

  menu_game, PERM_MENU + '0', M_XMENU,
  "益智空間"
};

static MENU menu_game2[] =
{
  "bin/dice.so:main_dice",       0, - M_GAME,
  "0Dice      ♂ 狂擲骰子 ♀",

  "bin/gp.so:main_gp",           0, - M_GAME,
  "1GoldPoker ♂ 金牌撲克 ♀",

  "bin/bj.so:main_bj",           0, - M_GAME,
  "2BlackJack ♂ 二十一點 ♀",

  "bin/chessmj.so:main_chessmj", 0, - M_GAME,
  "3ChessMJ   ♂ 象棋麻將 ♀",

  "bin/seven.so:main_seven",     0, - M_GAME,
  "4Seven     ♂ 賭城七張 ♀",

  "bin/race.so:main_race",       0, - M_GAME,
  "5Race      ♂ 進賽馬場 ♀",

  "bin/bingo.so:main_bingo",     0, - M_GAME,
  "6Bingo     ♂ 賓果大戰 ♀",

  "bin/marie.so:main_marie",     0, - M_GAME,
  "7Marie     ♂ 大小瑪莉 ♀",

  "bin/bar.so:main_bar",         0, - M_GAME,
  "8Bar       ♂ 吧台瑪莉 ♀",

  menu_game, PERM_MENU + '0', M_XMENU,
  "遊戲樂園"
};

static MENU menu_game3[] =
{
  "bin/pip.so:main_pip", PERM_BASIC, - M_GAME,
  "0Chicken   ♂ 電子小雞 ♀",

  "bin/pushbox.so:main_pushbox", 0, - M_GAME,
  "1PushBox   ♂ 倉庫番番 ♀",

  "bin/tetris.so:main_tetris",   0, - M_GAME,
  "2Tetris    ♂ 俄羅斯塊 ♀",

  "bin/gray.so:main_gray",       0, - M_GAME,
  "3Gray      ♂ 淺灰大戰 ♀",

  menu_game, PERM_MENU + '0', M_XMENU,
  "反斗特區"
};

static MENU menu_game[] =
{
  menu_game1, PERM_BASIC, M_XMENU,
  "1Game      ├ 益智天堂 ┤",

  menu_game2, PERM_BASIC, M_XMENU,
  "2Game      ├ 遊戲樂園 ┤",

  menu_game3, PERM_BASIC, M_XMENU,
  "3Game      ├ 反斗特區 ┤",

  menu_main, PERM_MENU + '1', M_XMENU,
  "遊戲人生"
};

#endif


#ifdef HAVE_BUY
/* --------------------------------------------------- */
/* buy menu                                            */
/* --------------------------------------------------- */

static MENU menu_buy[] =
{
  "bin/bank.so:x_bank",  PERM_BASIC, - M_GAME,
  "Bank       ├ 信託銀行 ┤",
/*
  "bin/bank.so:b_invis", PERM_BASIC, - M_GAME,
  "Invis      ├ 隱形現身 ┤",

  "bin/bank.so:b_cloak", PERM_BASIC, - M_GAME,
  "Cloak      ├ 無限隱形 ┤",

  "bin/bank.so:b_mbox",  PERM_BASIC, - M_GAME,
  "Mbox       ├ 信箱無限 ┤",
*/
  menu_tool, PERM_MENU + 'B', M_XMENU,
  "金融市場"
};
#endif


/* --------------------------------------------------- */
/* other tools menu                                    */
/* --------------------------------------------------- */

static MENU menu_other[] =
{
  pad_view,               0,                    M_READA,
  "Note       ├ 觀看留言 ┤",

  /* itoc.010309: 不必離站可以寫留言板 */
  pad_draw,               PERM_POST,            M_POST,
  "Pad        ├ 心情塗鴉 ┤",

  #ifdef HAVE_CALENDAR
  "bin/todo.so:main_todo", PERM_BASIC,         - M_XMODE,
  "Todo       ├ 個人行程 ┤",

  "bin/calendar.so:main_calendar", 0,          - M_XMODE,
  "XCalendar  ├ 萬年月曆 ┤",
  #endif

  #ifdef HAVE_CREDIT
  "bin/new_credit.so:credit_main", PERM_BASIC, - M_XMODE,
  "CreditNote ├ 記帳手札 ┤",
  #endif

  "bin/xyz.so:x_password", 0,                  - M_XMODE,
  "Password   ├ 忘記密碼 ┤",

  #ifdef HAVE_TIP
  "bin/xyz.so:x_tip",      0,                  - M_READA,
  "HowTo      ├ 教學精靈 ┤",
  #endif

  /* itoc.020829: 怕 guest 沒選項 */
  menu_tool, PERM_MENU + Ctrl('A'), M_XMENU,
  "其他功\能"
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
  vs_bar("系統資訊");

  move(5, 0);
  prints("目前位置: %s‧%s (%s) \n", SCHOOLNAME, BBSNAME, MYIPADDR);
  prints("系統負載: \033[1;3%s\033[m (%.2f %.2f %.2f)\n", load[0] > 20 ? "1m偏高": load[0] > 10 ? "3m中等" : "2m良好", load[0], load[1], load[2]);
  prints("線上人數: %d / %d (MAX)\n", ushm->count, MAXACTIVE);
  prints("系統時間: %s", ctime(&now));

  vmsg(NULL);
  return 0;
}


static MENU menu_tool[] =
{

#ifdef HAVE_BUY
  menu_buy,                    PERM_BASIC, M_XMENU,
  "Market     ├ 金融市場 ┤",
#endif

#ifdef HAVE_SONG
  menu_song,                   0,          M_XMENU,
  "KTV        ├ 真情點歌 ┤",
#endif

  "bin/vote.so:vote_all",      0,          - M_VOTE,
  "VoteALL    ├ 投票中心 ┤",

  menu_other,                  0,          M_XMENU,
  "Other      ├ 雜七雜八 ┤",

  "bin/eatwhere.so:eat_where",  0,          - M_XMODE,
  "Gourmet    ├ 美食天地 ┤",

  menu_sysinfo,		       0,          M_XMODE,
  "Xinfo      ├ 系統資訊 ┤",

  /* itoc.020829: 怕 guest 沒選項 */
  menu_main, PERM_MENU + Ctrl('A'), M_XMENU,
  "個人工具"
};

#endif    /* HAVE_EXTERNAL */


/* ----------------------------------------------------- */
/* main menu                                             */
/* ----------------------------------------------------- */

static int
Gem()
{
  /* itoc.001109: 看板總管在 (A)nnounce 下有 GEM_X_BIT，方便開板 */
  XoGem("gem/"FN_DIR, "精華佈告欄", (HAS_PERM(PERM_ALLBOARD) ? (GEM_W_BIT | GEM_X_BIT | GEM_M_BIT) : 0));
  return 0;
}

static MENU menu_main[] =
{

  menu_admin, PERM_ALLADMIN, M_AMENU,
  "0Admin     【 \033[1;32m系統維護區\033[m 】",

  Gem,        0,             M_GEM,
  "Announce   【 精華公佈欄 】",

  Boards,     0,             M_BOARD,
  "Boards     【 所有討論板 】",

  Class,      0,             M_BOARD,
  "Class      【 分類討論集 】",

#ifdef MY_FAVORITE
  MyFavorite, PERM_BASIC,    M_MF,
  "Favorite   【 最愛的看板 】",
#endif

  menu_mail,  0,             M_MMENU,
  "Mail       【 信件資料夾 】",

  menu_talk,  0,             M_TMENU,
  "Talk       【 休閒聊天地 】",

  menu_user,  0,             M_UMENU,
  "User       【 個人工具區 】",

#ifdef HAVE_GAME
  menu_game, PERM_BASIC,     M_XMENU,
  "Play       【 小城娛樂所 】",
#endif

#ifdef HAVE_EXTERNAL
  menu_tool,  0,             M_XMENU,
  "Xyz        【 特殊功\能區 】",
#endif

  goodbye,    0,             M_XMODE,
  "Goodbye     再會～東方小城 ",

  NULL, PERM_MENU + 'B', M_0MENU,
  "主功\能表"
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
          if (max < 0)        /* 找不到適合權限之功能，回上一層功能表 */
          {
            menu = (MENU *) menu->func;
            continue;
          }
#endif

          break;
        }

		/* 有該權限才秀出 */
        if (cc && !(cc & level))
          continue;

        table[++max] = menu;
      }

      if (mmx < max)
        mmx = max;

      /* 第一次上站若有新信，進入 Mail 選單 */
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

	/* itoc.020829: 預設選項第一個 */
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

	  /* 子目錄的 mode 要 <= M_XMENU */
      if (cmd <= M_XMENU)
      {
        menu->level = PERM_MENU + mptr->desc[0];
        menu = (MENU *) mptr->func;

        mode = MENU_LOAD | MENU_DRAW;
		/* itoc.010304: 進入子選單重撥 movie */
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

    /* itoc.010911: Select everywhere，不再限制是在 M_0MENU */
    case 's':
      utmp_mode(M_BOARD);
      Select();
      goto every_key;

#ifdef MY_FAVORITE
    /* itoc.010911: Favorite everywhere，不再限制是在 M_0MENU */
    case 'f':
    case Ctrl('F'):
      if (cuser.userlevel)    /* itoc.010407: 要檢查權限 */
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
	  /* 若不在 M_0MENU 中按 r 的話，要視為一般按鍵 */
      goto default_key;
      /* 特殊鍵處理結束 */

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
		/* itoc.010304: 退出子選單重撥 movie */
        /* mode = MENU_LOAD | MENU_DRAW | MENU_FILM; */
        depth--;
        continue;
      }
      cmd = 'G';

default_key:
    default:
	  /* 轉換為大寫 */
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

    /* 若游標移動位置 */
    if (cc != cx)
    {
#ifdef CURSOR_BAR
      /* 有 CURSOR_BAR */
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
      /* 沒有 CURSOR_BAR */
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
	  /* 若游標的位置沒有變 */
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
