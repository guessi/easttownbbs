/*-------------------------------------------------------*/
/* util/delog.c         ( NTHU CS MapleBBS Ver 3.10 )    */
/*-------------------------------------------------------*/
/* target : 清除過舊的上站來源記錄                       */
/* create : 05/02/14                                     */
/* update :   /  /                                       */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/
/* syntax : delog [userid]                               */
/*-------------------------------------------------------*/


#include "bbs.h"


#define QUICK_DELOG


#ifdef QUICK_DELOG
#define KEEP_BYTES      40960   /* 只保留最後 40960 bytes 的上站來源記錄 */
#else
#define KEEP_DAYS       180     /* 只保留最近 180 天的上站來源記錄 */
static int now;
#endif


#ifdef QUICK_DELOG
static void
delog(fpath)
  char *fpath;
{
  int fd, fsize;
  char *fimage, *fend, *ptr;

  if (fimage = f_img(fpath, &fsize))
  {
    if (fsize > KEEP_BYTES)
    {
      fend = fimage + fsize;

      for (ptr = fimage + fsize - KEEP_BYTES;
        *ptr != '\n' && ptr < fend; ptr++);
      ptr++;

      if (ptr < fend)
      {
        if ((fd = open(fpath, O_WRONLY | O_CREAT | O_TRUNC, 0600)) >= 0)
        {
          write(fd, ptr, fend - ptr);
          close(fd);
        }
      }
    }
    free(fimage);
  }
  else
  {
    unlink(fpath);
  }
}
#else
static int
decode_Btime(str)
  char *str;
{
  /* 依 Btime() 格式不同而變 */

  /* 為簡單起見，不考慮閏年及大小月，所以實際保留的日期
     會和 KEEP_DAYS 略有不同 */

  if (*str == '2')
    return atoi(str) * 365 + atoi(str + 5) * 31 + atoi(str + 8);
  else                  /* 舊格式: 03/12/01 15:33:08 */
    return atoi(str) * 365 + atoi(str + 3) * 31 + atoi(str + 6);
}


static void
delog(fpath)
  char *fpath;
{
  int keep;
  char fnew[64], buf[256];
  FILE *fpr, *fpw;

  if (fpr = fopen(fpath, "r"))
  {
    sprintf(fnew, "%s.new", fpath);
    if (fpw = fopen(fnew, "w"))
    {
      keep = 0;
      while (fgets(buf, sizeof(buf), fpr))
      {
        if (!keep)
        {
          if (decode_Btime(buf) >= now)
            keep = 1;
        }

        if (keep)
          fputs(buf, fpw);
      }

      fclose(fpw);
    }
    fclose(fpr);

    unlink(fpath);
    rename(fnew, fpath);
  }
}
#endif


int
main(argc, argv)
  int argc;
  char *argv[];
{
  char c, *str, fpath[64];
  struct dirent *de;
  DIR *dirp;

  if (argc > 2)
  {
    printf("Usage: %s [userid]\n", argv[0]);
    return -1;
  }

#ifndef QUICK_DELOG
  now = decode_Btime(Now()) - KEEP_DAYS;
#endif

  if (argc == 2)
  {
    chdir(BBSHOME);
    usr_fpath(fpath, argv[1], FN_LOG);
    delog(fpath);
    return 0;
  }

  for (c = 'a'; c <= 'z'; c++)
  {
    printf("正開始處理 %c 開頭的 ID\n", c);

    sprintf(fpath, BBSHOME "/usr/%c", c);
    chdir(fpath);

    if (!(dirp = opendir(".")))
      continue;

    while (de = readdir(dirp))
    {
      str = de->d_name;
      if (*str <= ' ' || *str == '.')
        continue;

      sprintf(fpath, "%s/" FN_LOG, str);
      delog(fpath);
    }

    closedir(dirp);
  }

  return 0;
}

