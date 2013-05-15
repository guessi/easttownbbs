#if 0
  
  �N�쥻sob2pal�ഫ�n�ͿW�ߥX��
  ��ĳ�����H�U�B�J
  cd ~/src/util/tran
  mv sob2pal.c sob2bpl.c ��: �N�ഫ�ݪO�n�ͿW�ߥX��
  �s�W���ɮ�sob2pal.c
  �H�e���ɮצ]���P���ഫ�ݪO�n��so...�@���o�n�����
  �N�ݪO�n��/�n�ͿW�ߤ��}�ഫ����ŦX�ݨD :)
  �Y�u�n�ഫ�n�� �N�������O�ɶ��ഫ�ݪO�n���o
  
  �ϥΤ覡:
  ./sob2pal �ഫ����
  ./sob2pal [userid] �ഫ��@�b��
  
  �ഫ���O�o�ШϥΪ̭��s�W�� UTMP�n��s :)

#endif

#include "bbs.h"
#include "sob.h" /* Ū��oldbbs�� guessi */


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

  /* argc == 1 ������ϥΪ� */
  /* argc == 2 ��Y�S�w�ϥΪ� */

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

