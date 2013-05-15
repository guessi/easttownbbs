/*-------------------------------------------------------*/
/* util/showHabit.c  ( NTHU CS MapleBBS Ver 3.00 )       */
/*-------------------------------------------------------*/
/* target : 觀察全站喜好設定使用情況                     */
/* create : 04/01/08                                     */
/* update :   /  /                                       */
/* author : qazq.bbs@bbs.cs.nchu.edu.tw                  */
/*-------------------------------------------------------*/

#define _ADMIN_C_

#include "bbs.h"

#define OUTFILE_HABIT   BBSHOME"/gem/@/@-habit"

static int habit[32] = {0};

static void
count_habit(flag)
 unsigned flag;
{
 int i;
 unsigned mask = 1 << 31;

 for (i = 31; i >= 0; i--)
 {
   if (flag & mask)
     habit[i]++;

     flag <<= 1;
 }
}


int
main()
{
  int i, totaluser = 0;
  char c, buf[30];
  FILE *fp;

  for (c = 'a'; c <= 'z'; c++)
  {
    char buf[64];
    struct dirent *de;
    DIR *dirp;

    sprintf(buf, BBSHOME "/usr/%c", c);
    chdir(buf);
    
    if (!(dirp = opendir(".")))
      continue;
      
    while (de = readdir(dirp))
    {
      ACCT acct;
      int fd;
      char *fname;
      fname = de->d_name;
      if (*fname <= ' ' || *fname == '.')
         continue;
	 
      sprintf(buf, "%s/.ACCT", fname);
      if ((fd = open(buf, O_RDONLY)) < 0)
         continue;
      read(fd, &acct, sizeof(ACCT));
      close(fd);
      count_habit(acct.ufo);
      totaluser++;
   }
   closedir(dirp);
 }
 if (!(fp = fopen(OUTFILE_HABIT, "w")))
   return;
 fprintf(fp, "全站人數：%d\n\n", totaluser);
 
 for (i = 0; i <= 31; i++)
 {
   sprintf(buf, "%s", ufo_tbl[i]);
   if (!strcmp(buf, "保留") || !ufo_tbl[i])
     continue;
   fprintf(fp, "[%02d]  %s  開啟人數：%-4d  關閉人數：%d\n",
   i, ufo_tbl[i], habit[i], totaluser - habit[i]);
 }
 fclose(fp);

 return 0;
}
