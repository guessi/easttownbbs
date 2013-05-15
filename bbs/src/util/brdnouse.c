/*-------------------------------------------------------*/
/* util/brdnouse.c        ( NTHU CS MapleBBS Ver 3.10 )  */
/*-------------------------------------------------------*/
/* target : 計算看板無新文章天數                         */
/* create : 04/02/07                                     */
/* update :   /  /                                       */
/* author : Efen.bbs@bbs.med.ncku.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"

#define OUTFILE_BRDEMPTY        "gem/@/@-brd_empty"
#define OUTFILE_BRDOVERDUE      "gem/@/@-brd_overdue"


static char nostat[MAXBOARD][BNLEN + 1]; /* 不列入排行榜的看板 */
static int nostat_num;                   /* nostat_num+1: 幾個板不列入統計 */
static time_t now;


static void
collect_nostat()
{
  FILE *fp;
  BRD brd;

  nostat_num = -1;
  if (fp = fopen(FN_BRD, "r"))
  {
    while (fread(&brd, sizeof(BRD), 1, fp) == 1)
    {
      if ((brd.readlevel | brd.postlevel) >= (PERM_VALID << 1))
      /* (BASIC + ... + VALID) < (VALID << 1) */
      {
        nostat_num++;
        strcpy(nostat[nostat_num], brd.brdname);
      }
    }

    fclose(fp);
  }
}


static int      /* 1:本板不列入統計 */
is_nostat(brdname)
  char *brdname;
{
  int i;

  for (i = 0; i <= nostat_num; i++)
  {
    if (!strcmp(brdname, nostat[i]))
      return 1;
  }
  return 0;
}


static char *
whoisbm(brdname)
  char *brdname;
{
  int fd;
  BRD brd;
  static char BM[BMLEN + 1];

  BM[0] = '\0';

  if ((fd = open(FN_BRD, O_RDONLY)) >= 0)
  {
    while (read(fd, &brd, sizeof(BRD)) == sizeof(BRD))
    {
      if (!strcmp(brdname, brd.brdname))
      {
        strcpy(BM, brd.BM);
        break;
      }
    }
    close(fd);
  }

  return BM;
}


typedef struct
{
  int day;                      /* 幾天沒整理精華區 */
  char brdname[BNLEN + 1];      /* 板名 */
}       BRDDATA;


static int
int_cmp(a, b)
  BRDDATA *a, *b;
{
  return (b->day - a->day);     /* 由大排到小 */
}


int
main()
{
  BRD brd;
  struct stat st;
  struct tm *ptime;
  BRDDATA board[MAXBOARD];
  FILE *fpe, *fpo, *fp;
  int locus, i, m, n, fd, fsize;
  char folder[64], *brdname;
  HDR hdr;

  chdir(BBSHOME);

  if (!(fp = fopen(FN_BRD, "r")))
  return -1;

  collect_nostat();     /* itoc.020127: 收集不列入排行榜的看板 */

  time(&now);
  ptime = localtime(&now);
  locus = 0;

  while (fread(&brd, sizeof(BRD), 1, fp) == 1)
  {
    brdname = brd.brdname;
    if (!*brdname)      /* 此板已被刪除 */
      continue;

    if (is_nostat(brdname))
      continue;

    brd_fpath(folder, brdname, FN_DIR);

    strcpy(board[locus].brdname, brdname);
    board[locus].day = -1;
    if ((fd = open(folder, O_RDONLY)) >= 0 && !fstat(fd, &st))
    {
      fsize = st.st_size;
      while ((fsize -= sizeof(HDR)) >= 0)   /* 找最後一篇不是置底文的時間 */
      {
        lseek(fd, fsize, SEEK_SET);
        read(fd, &hdr, sizeof(HDR));
        if (!(hdr.xmode & POST_BOTTOM))
        {
          board[locus].day = (now - hdr.chrono) / 86400;
          break;
        }
      }
    }

    locus++;
  }
  fclose(fp);

  qsort(board, locus, sizeof(BRDDATA), int_cmp);

  fpe = fopen(OUTFILE_BRDEMPTY, "w");
  fpo = fopen(OUTFILE_BRDOVERDUE, "w");

  fprintf(fpe,
    "\033[1;33m ○ ────────────→ \033[33;41m 看板無文看板記錄 \033[40m ←──────────── ○\033[m\n\n" \
    "            \033[1;42m 排序 \033[44m      看板名稱 \033[42m  板            主            \033[m\n");

  fprintf(fpo,
    "\033[1;33m ○ ────────────→ \033[33;41m 看板無新文章記錄 \033[40m ←──────────── ○\033[m\n\n" \
    "          \033[1;42m 名次 \033[44m   看 板 名 稱    \033[42m " \
    "看板無新文章 \033[44m   板     主              \033[m\n");

  m = 1;
  n = 1;

  for (i = 0; i < locus; i++)
  {
    if (board[i].day < 0)
    {
      fprintf(fpe, "              %3d   %12s   %-20.20s\n",
        m, board[i].brdname, whoisbm(board[i].brdname));
      m++;
    }
    else
    {
      if (board[i].day > 90)  /* 只統計超過十天無文章之看板 */
      {
        fprintf(fpo, "            %3d  %12s          %5d       %-20.20s\n",
         n, board[i].brdname, board[i].day, whoisbm(board[i].brdname));
        n++;
      }
    }
  }

  fprintf(fpe, "\n\n結算日期: 20%02d/%02d/%02d", ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday);
  fprintf(fpo, "\n\n結算日期: 20%02d/%02d/%02d", ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday);
  
  fclose(fpe);
  fclose(fpo);

  return 0;
}
