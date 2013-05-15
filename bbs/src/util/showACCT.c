/*-------------------------------------------------------*/
/* util/showACCT.c      ( NTHU CS MapleBBS Ver 3.10 )    */
/*-------------------------------------------------------*/
/* target : 顯示 ACCT 資料                               */
/* create : 01/07/16                                     */
/* update :                                              */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"

static char *
_bitmsg(str, level)
  char *str;
  int level;
{
  int cc;
  int num;
  static char msg[33];

  num = 0;
  while (cc = *str)
  {
    msg[num] = (level & 1) ? cc : '-';
    level >>= 1;
    str++;
    num++;
  }
  msg[num] = 0;

  return msg;
}


static inline void
showACCT(acct)
  ACCT *acct;
{
  char msg1[40], msg2[40], msg3[40], msg4[40], msg5[40], msg6[40];

  strcpy(msg1, _bitmsg(STR_PERM, acct->userlevel));
  strcpy(msg2, _bitmsg(STR_UFO, acct->ufo));
  strcpy(msg3, Btime(&(acct->firstlogin)));
  strcpy(msg4, Btime(&(acct->lastlogin)));
  strcpy(msg5, Btime(&(acct->tcheck)));
  strcpy(msg6, Btime(&(acct->tvalid)));

  printf("--\n"
    "編號: %-15d [ID]: %-15s 姓名: %-15s 暱稱: %-15s \n" 
    "權限: %-37s 設定: %-37s \n" 
    "簽名: %-37d 性別: %-15d \n" 
    "銀幣: %-15d 金幣: %-15d 生日: %02d/%02d/%02d \n" 
    "上站: %-15d 文章: %-15d 發信: %-15d \n" 
    "首次: %-37s 上次: %-30s \n" 
    "檢查: %-37s 通過: %-30s \n" 
    "登入: %-30s \n" 
    "掛站: %d時%02d分\n"
    "信箱: %-60s \n", 
    acct->userno, acct->userid, acct->realname, acct->username, 
    msg1, msg2,
    acct->signature, acct->sex,
    acct->money, acct->gold, acct->year, acct->month, acct->day, 
    acct->numlogins, acct->numposts, acct->numemails, 
    msg3, msg4, 
    msg5, msg6, 
    acct->lasthost, 
    (acct->staytime / 60) / 60, (acct-> staytime / 60) % 60,
    acct->email);
}


int
main(argc, argv)
  int argc;
  char **argv;
{
  int i, mode;

  if (( strcmp("-r", argv[1]) && argc < 2) ||
      (!strcmp("-r", argv[1]) && argc < 3))
  {
    printf("Usage: %s UserID [UserID] ...\n", argv[0]);
    printf("       %s -r UserID [UserID] ...\n", argv[0]);
    return -1;
  }

  if (strcmp("-r", argv[1]))
  {
    i = 1;
    mode = 0;
  }
  else
  {
    i = 2;
    mode = 1;
  }

  chdir(BBSHOME);

  for ( ; i < argc; i++)
  {
    ACCT acct;
    char fpath[64];

    if (!mode)
      usr_fpath(fpath, argv[i], FN_ACCT);
    else
      sprintf(fpath, "usr/@/%s/%s", argv[i], FN_ACCT);

    if (rec_get(fpath, &acct, sizeof(ACCT), 0) < 0)
    {
      printf("%s: read error (maybe no such id?)\n", argv[i]);
      continue;
    }

    showACCT(&acct);
  }
  printf("--\n");

  return 0;
}
