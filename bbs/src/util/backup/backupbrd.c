/*-------------------------------------------------------*/
/* util/backupbrd.c	( NTHU MapleBBS Ver 3.10 )       */
/*-------------------------------------------------------*/
/* target : 備份所有看板資料                             */
/* create : 01/10/19                                     */
/* update :   /  /                                       */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "bbs.h"


int
main()
{
  struct dirent *de;
  DIR *dirp;
  char *ptr;
  char bakpath[128], cmd[256];
  time_t now;
  struct tm *ptime;

  if (chdir(BBSHOME "/brd") || !(dirp = opendir(".")))
    exit(-1);

  /* 建立備份路徑目錄 */
  time(&now);
  ptime = localtime(&now);
  sprintf(bakpath, "%s/backup20%02d%02d/brd%02d%02d%02d", BAKPATH,
  ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday);
  /* ex: bak/backup200608/brd20060808 */
  mkdir(bakpath, 0755);

  /* 改變權限使 ftp 傳檔不會漏傳 */
  sprintf(cmd, "cp %s/%s %s/; chmod 644 %s/%s", BBSHOME, FN_BRD, bakpath, bakpath, FN_BRD);
  system(cmd);

  /* 把各看板分別壓縮成一個壓縮檔 */
  while (de = readdir(dirp))
  {
    ptr = de->d_name;

    if (ptr[0] > ' ' && ptr[0] != '.')
    {
      sprintf(cmd, "tar cfz %s/%s.tgz %s", bakpath, ptr, ptr);
      system(cmd);
    }
  }
  closedir(dirp);

  exit(0);
}
