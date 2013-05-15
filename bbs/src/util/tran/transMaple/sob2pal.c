#if 0
  
  將原本sob2pal轉換好友獨立出來
  建議先做以下步驟
  cd ~/src/util/tran
  mv sob2pal.c sob2bpl.c 註: 將轉換看板好友獨立出來
  新增此檔案sob2pal.c
  以前的檔案因為同時轉換看板好友so...一次得要轉全部
  將看板好友/好友獨立分開轉換比較符合需求 :)
  若只要轉換好友 就不必浪費時間轉換看板好友囉
  
  使用方式:
  ./sob2pal 轉換全部
  ./sob2pal [userid] 轉換單一帳號
  
  轉換完記得請使用者重新上站 UTMP要更新 :)

#endif

#include "bbs.h"
#include "sob.h" /* 讀取oldbbs用 guessi */


#define OLD_BBSHOME 	"/home/oldbbs"	/* 2.36 */


/* ----------------------------------------------------- */
/* 3.02 functions                    			 */
/* ----------------------------------------------------- */


static int
acct_uno(userid)
  char *userid;
{
  int fd;
  int userno;
  char fpath[64];

  usr_fpath(fpath, userid, FN_ACCT);
  fd = open(fpath, O_RDONLY);
  if (fd >= 0)
  {
    read(fd, &userno, sizeof(userno));
    close(fd);
    return userno;
  }
  return -1;
}


/* tranfer function start */


static void
transfer_pal(userid)
  char *userid;
{
  ACCT acct;
  FILE *fp;
  int fd, friend_userno;
  char fpath[64], buf[64];
  char abuf[128], bbuf[128], cbuf[128];
  PAL pal;

  usr_fpath(fpath, userid, FN_PAL);         			/* 新的好友名單 */

  /* sob 的 usr 目錄有分大小寫，所以要先取得大小寫 */
  usr_fpath(buf, userid, FN_ACCT);
  if ((fd = open(buf, O_RDONLY)) >= 0)
  {
    read(fd, &acct, sizeof(ACCT));
    close(fd);
  }
  sprintf(buf, OLD_BBSHOME"/home/%s/overrides", acct.userid);	/* 舊的好友名單 */

  if (dashf(fpath))
    unlink(fpath);      /* 清掉重建 */

  strcpy(bbuf, "dadadasdasda");	/* 隨便亂給一個不會發生的 ID */

  if (!(fp = fopen(buf, "r")))
    return;

  while (1)
  {
//    fscanf(fp, "%s", abuf);

    fscanf(fp, "%s %s", abuf, cbuf);

    if (!strcmp(abuf, bbuf))
      break;
      
    strcpy(bbuf, abuf);

    if ((friend_userno = acct_uno(abuf)) >= 0)
    {
      memset(&pal, 0, sizeof(PAL));
      str_ncpy(pal.userid, abuf, sizeof(pal.userid));
      pal.ftype = 0;
      str_ncpy(pal.ship, cbuf, sizeof(pal.ship));
      pal.userno = friend_userno;
      rec_add(fpath, &pal, sizeof(PAL));
    }
  }

  fclose(fp);
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int fd;
  userec user;

  /* argc == 1 轉全部使用者 */
  /* argc == 2 轉某特定使用者 */

  if (argc > 2)
  {
    printf("Usage: %s [target_user]\n", argv[0]);
    exit(-1);
  }

  chdir(BBSHOME);

  if (!dashf(FN_PASSWD))
  {
    printf("ERROR! Can't open " FN_PASSWD "\n");
    exit(-1);
  }
  if (!dashd(OLD_BBSHOME "/home"))
  {
    printf("ERROR! Can't open " OLD_BBSHOME "/home\n");
    exit(-1);
  }

  if ((fd = open(FN_PASSWD, O_RDONLY)) >= 0)
  {
    while (read(fd, &user, sizeof(user)) == sizeof(user))
    {
      if (argc == 1)
      {
	transfer_pal(&user);
      }
      else if (!strcmp(user.userid, argv[1]))
      {
	transfer_pal(&user);
	exit(1);
      }
    }
    close(fd);
  }

  exit(0);
}

