/* �ഫñ�W�� �u��e���� ²��sob2usr.c�Ө�  */
/* �ѩ�u��ñ�W�� �W���� so... ����check ID */


#include "sob.h"

#define TRAN_SIGN /* ñ�W�� */
#define TRAN_PLAN /* �W���� */
/* �Y���ഫ�Nundef�o */

/* ----------------------------------------------------- */
/* �ഫñ�W�ɡB�p�e��					 */
/* ----------------------------------------------------- */

#ifdef TRAN_SIGN
static inline void
trans_sig(old)
  userec *old;
{
  int i;
  char buf[64], fpath[64], f_sig[20];

  for (i = 1; i <= 6; i++)
  {
    sprintf(buf, OLD_BBSHOME "/home/%s/sig.%d", old->userid, i);	/* �ª�ñ�W�� */
    if (dashf(buf))
    {
      sprintf(f_sig, "%s.%d", FN_SIGN, i);
      usr_fpath(fpath, old->userid, f_sig);
      f_cp(buf, fpath, O_TRUNC);
    }
  }

  return;
}
#endif
#ifdef TRAN_PLAN
static inline void
trans_plans(old)
  userec *old;
{
  char buf[64], fpath[64];

  sprintf(buf, OLD_BBSHOME "/home/%s/plans", old->userid);
  if (dashf(buf))
  {
    usr_fpath(fpath, old->userid, FN_PLANS);
    f_cp(buf, fpath, O_TRUNC);
  }
  return;
}

#endif

/* ----------------------------------------------------- */
/* �ഫ�D�{��						 */
/* ----------------------------------------------------- */


static void
transusr(user)
  userec *user;
{
#ifdef TRAN_SIGN
  trans_sig(user);
#endif
#ifdef TRAN_PLAN
  trans_plans(user);
#endif
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
	transusr(&user);
      }
      else if (!strcmp(user.userid, argv[1]))
      {
	transusr(&user);
	exit(1);
      }
    }
    close(fd);
  }

  exit(0);
}
