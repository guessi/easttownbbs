/*-------------------------------------------------------*/
/* util/brdnouse.c        ( NTHU CS MapleBBS Ver 3.10 )  */
/*-------------------------------------------------------*/
/* target : �p��ݪO�L�s�峹�Ѽ�                         */
/* create : 04/02/07                                     */
/* update :   /  /                                       */
/* author : Efen.bbs@bbs.med.ncku.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"

#define OUTFILE_BRDEMPTY        "gem/@/@-brd_empty"
#define OUTFILE_BRDOVERDUE      "gem/@/@-brd_overdue"


static char nostat[MAXBOARD][BNLEN + 1]; /* ���C�J�Ʀ�]���ݪO */
static int nostat_num;                   /* nostat_num+1: �X�ӪO���C�J�έp */
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


static int      /* 1:���O���C�J�έp */
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
  int day;                      /* �X�ѨS��z��ذ� */
  char brdname[BNLEN + 1];      /* �O�W */
}       BRDDATA;


static int
int_cmp(a, b)
  BRDDATA *a, *b;
{
  return (b->day - a->day);     /* �Ѥj�ƨ�p */
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

  collect_nostat();     /* itoc.020127: �������C�J�Ʀ�]���ݪO */

  time(&now);
  ptime = localtime(&now);
  locus = 0;

  while (fread(&brd, sizeof(BRD), 1, fp) == 1)
  {
    brdname = brd.brdname;
    if (!*brdname)      /* ���O�w�Q�R�� */
      continue;

    if (is_nostat(brdname))
      continue;

    brd_fpath(folder, brdname, FN_DIR);

    strcpy(board[locus].brdname, brdname);
    board[locus].day = -1;
    if ((fd = open(folder, O_RDONLY)) >= 0 && !fstat(fd, &st))
    {
      fsize = st.st_size;
      while ((fsize -= sizeof(HDR)) >= 0)   /* ��̫�@�g���O�m���媺�ɶ� */
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
    "\033[1;33m �� �w�w�w�w�w�w�w�w�w�w�w�w�� \033[33;41m �ݪO�L��ݪO�O�� \033[40m ���w�w�w�w�w�w�w�w�w�w�w�w ��\033[m\n\n" \
    "            \033[1;42m �Ƨ� \033[44m      �ݪO�W�� \033[42m  �O            �D            \033[m\n");

  fprintf(fpo,
    "\033[1;33m �� �w�w�w�w�w�w�w�w�w�w�w�w�� \033[33;41m �ݪO�L�s�峹�O�� \033[40m ���w�w�w�w�w�w�w�w�w�w�w�w ��\033[m\n\n" \
    "          \033[1;42m �W�� \033[44m   �� �O �W ��    \033[42m " \
    "�ݪO�L�s�峹 \033[44m   �O     �D              \033[m\n");

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
      if (board[i].day > 90)  /* �u�έp�W�L�Q�ѵL�峹���ݪO */
      {
        fprintf(fpo, "            %3d  %12s          %5d       %-20.20s\n",
         n, board[i].brdname, board[i].day, whoisbm(board[i].brdname));
        n++;
      }
    }
  }

  fprintf(fpe, "\n\n������: 20%02d/%02d/%02d", ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday);
  fprintf(fpo, "\n\n������: 20%02d/%02d/%02d", ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday);
  
  fclose(fpe);
  fclose(fpo);

  return 0;
}
