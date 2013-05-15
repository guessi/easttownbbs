/* 轉換簽名檔 只轉前六個 簡化sob2usr.c而來  */
/* 由於只轉簽名檔 名片檔 so... 不用check ID */


#include "sob.h"

#define TRAN_SIGN /* 簽名檔 */
#define TRAN_PLAN /* 名片檔 */
/* 若不轉換就undef囉 */

/* ----------------------------------------------------- */
/* 轉換簽名檔、計畫檔					 */
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
    sprintf(buf, OLD_BBSHOME "/home/%s/sig.%d", old->userid, i);	/* 舊的簽名檔 */
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
/* 轉換主程式						 */
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
