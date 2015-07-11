/*-------------------------------------------------------*/
/* util/transpal.c                   			 */
/*-------------------------------------------------------*/
/* target : SOB �� Maple 3.02 (�ݪO)�n�ͦW���ഫ         */
/* create : 01/09/08                     		 */
/* update :   /  /                   			 */
/* author : itoc.bbs@bbs.ee.nctu.edu.tw          	 */
/*-------------------------------------------------------*/
/* syntax : transpal                     		 */
/*-------------------------------------------------------*/


#if 0

   1. �]�w OLD_BBSHOME (sob config.h)
   2. �ק� struct FRIEND �M transfer_pal() transfer_brdpal()
   3. �ഫ(�ݪO)�n�ͦW�椧�e�A�z�������ഫ���ݪO�ΨϥΪ̡C

   ps. �ϥΫe�Х���ƥ��Ause on ur own risk. �{����H�Х]�[ :p
   ps. �P�� lkchu �� Maple 3.02 for FreeBSD

#endif


#include "bbs.h"


#define OLD_BBSHOME 	"/home/oldbbs"	/* 2.36 */

#undef TRAN_PAL /* �n���ഫ�W�ߨ� sob2pal.c ���ɮקאּ sob2bpl�u��ݪO�n�� */


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


/* ----------------------------------------------------- */

#ifdef TRAN_PAL
static void
transfer_pal(userid)
  char *userid;
{
  ACCT acct;
  FILE *fp;
  int fd, friend_userno;
  char fpath[64], buf[64];
  char abuf[80], bbuf[80];
  PAL pal;

  usr_fpath(fpath, userid, FN_PAL);         			/* �s���n�ͦW�� */

  /* sob �� usr �ؿ������j�p�g�A�ҥH�n�����o�j�p�g */
  usr_fpath(buf, userid, FN_ACCT);
  if ((fd = open(buf, O_RDONLY)) >= 0)
  {
    read(fd, &acct, sizeof(ACCT));
    close(fd);
  }
  sprintf(buf, OLD_BBSHOME"/home/%s/overrides", acct.userid);	/* �ª��n�ͦW�� */

  if (dashf(fpath))
    unlink(fpath);      /* �M������ */

  strcpy(bbuf, "dadadasdasda");	/* �H�K�õ��@�Ӥ��|�o�ͪ� ID */

  if (!(fp = fopen(buf, "r")))
    return;

  while (1)
  {
    fscanf(fp, "%s", abuf);

    if (!strcmp(abuf, bbuf))
      break;
    strcpy(bbuf, abuf);

    if ((friend_userno = acct_uno(abuf)) >= 0)
    {
      memset(&pal, 0, sizeof(PAL));
      str_ncpy(pal.userid, abuf, sizeof(pal.userid));
      pal.ftype = 0;
      str_ncpy(pal.ship, "", sizeof(pal.ship));
      pal.userno = friend_userno;
      rec_add(fpath, &pal, sizeof(PAL));
    }
  }

  fclose(fp);
}
#endif

static void
transfer_brdpal(userid)
  char *userid;
{
  FILE *fp;
  int friend_userno;
  char fpath[64], buf[64];
  char abuf[80], bbuf[80];
  PAL pal;

  brd_fpath(fpath, userid, FN_PAL);                     /* �s���ݪO�n�ͦW�� */
  sprintf(buf, OLD_BBSHOME"/boards/%s/visable", userid);     /* �ª��ݪO�n�ͦW�� */

  if (dashf(fpath))
    unlink(fpath);              /* �M������ */

  if (!(fp = fopen(buf, "r")))
    return;

  strcpy(bbuf, "daldjlsadadas");    /* �H�K�õ��@�Ӥ��|�o�ͪ� ID */

  while (1)
  {
    fscanf(fp, "%s", abuf);

    if (!strcmp(abuf, bbuf))
      break;
    strcpy(bbuf, abuf);

    if ((friend_userno = acct_uno(abuf)) >= 0)
    {
      memset(&pal, 0, sizeof(PAL));
      str_ncpy(pal.userid, abuf, sizeof(pal.userid));
      pal.ftype = 0;
      str_ncpy(pal.ship, "", sizeof(pal.ship));
      pal.userno = friend_userno;
      rec_add(fpath, &pal, sizeof(PAL));
    }
  }

  fclose(fp);
}


int  
main()
{
  char *str;
#ifdef TRAN_PAL
  char c;
  char buf[64];
#endif
  struct dirent *de;
  DIR *dirp;

  chdir(BBSHOME);

#ifdef TRAN_PAL
  /* �ഫ�ϥΪ̦n�ͦW�� */
  for (c = 'a'; c <= 'z'; c++)
  {
    sprintf(buf, "usr/%c", c);

    if (!(dirp = opendir(buf)))
      continue;

    while (de = readdir(dirp))
    {
      str = de->d_name;
      if (*str <= ' ' || *str == '.')
       continue;

      transfer_pal(str);
    }
    closedir(dirp);    
  }
#endif

  /* �ഫ�ݪO�n�ͦW�� */
  if (!(dirp = opendir("brd")))
    return 0;

  while (de = readdir(dirp))
  {
    str = de->d_name;
    if (*str <= ' ' || *str == '.')
      continue;

    transfer_brdpal(str);
  }
  closedir(dirp);  

  return 0;
}