#include "sob.h"


static time_t
trans_hdr_chrono(filename)
  char *filename;
{
  char time_str[11];

  /* M.1087654321.A �� M.987654321.A */
  str_ncpy(time_str, filename + 2, filename[2] == '1' ? 11 : 10);

  return (time_t) atoi(time_str);
}


/* ----------------------------------------------------- */
/* �ഫ�ӤH��ذ�					 */
/* ----------------------------------------------------- */


static void
trans_man_stamp(folder, token, hdr, fpath, time)
  char *folder;
  int token;
  HDR *hdr;
  char *fpath;
  time_t time;
{
  char *fname, *family;
  int rc;

  fname = fpath;
  while (rc = *folder++)
  {
    *fname++ = rc;
    if (rc == '/')
      family = fname;
  }
  if (*family != '.')
  {
    fname = family;
    family -= 2;
  }
  else
  {
    fname = family + 1;
    *fname++ = '/';
  }

  *fname++ = token;

  *family = radix32[time & 31];
  archiv32(time, fname);

  if (rc = open(fpath, O_WRONLY | O_CREAT | O_EXCL, 0600))
  {
    memset(hdr, 0, sizeof(HDR));
    hdr->chrono = time;
    str_stamp(hdr->date, &hdr->chrono);
    strcpy(hdr->xname, --fname);
    close(rc);
  }
  return;
}


static void
transman(index, folder)
  char *index, *folder;
{
  static int count = 100;

  int fd;
  char *ptr, buf[256], fpath[64];
  fileheader fh;
  HDR hdr;
  time_t chrono;

  if ((fd = open(index, O_RDONLY)) >= 0)
  {
    while (read(fd, &fh, sizeof(fh)) == sizeof(fh))
    {
      strcpy(buf, index);
      ptr = strrchr(buf, '/') + 1;
      strcpy(ptr, fh.filename);

      if (*fh.filename == 'M' && dashf(buf))	/* �u�� M.xxxx.A �� D.xxxx.a */
      {
	/* �ഫ�峹 .DIR */
	memset(&hdr, 0, sizeof(HDR));
	chrono = trans_hdr_chrono(fh.filename);
	trans_man_stamp(folder, 'A', &hdr, fpath, chrono);
	hdr.xmode = 0;
	str_ncpy(hdr.owner, fh.owner, sizeof(hdr.owner));
	str_ncpy(hdr.title, fh.title + 3, sizeof(hdr.title));
	rec_add(folder, &hdr, sizeof(HDR));

	/* �����ɮ� */
	f_cp(buf, fpath, O_TRUNC);
      }
      else if (*fh.filename == 'D' && dashd(buf))
      {
	char sub_index[256];

	/* �ഫ�峹 .DIR */
	memset(&hdr, 0, sizeof(HDR));
	 chrono = ++count;		/* WD ���ؿ��R�W����_�ǡA�u�n�ۤv���Ʀr */
	trans_man_stamp(folder, 'F', &hdr, fpath, chrono);
	hdr.xmode = GEM_FOLDER;
	str_ncpy(hdr.owner, fh.owner, sizeof(hdr.owner));
	str_ncpy(hdr.title, fh.title + 3, sizeof(hdr.title));
	rec_add(folder, &hdr, sizeof(HDR));

	/* recursive �i�h�ഫ�l�ؿ� */
	strcpy(sub_index, buf);
	ptr = strrchr(sub_index, '/') + 1;
	sprintf(ptr, "%s/.DIR", fh.filename);
	transman(sub_index, fpath);
      }
    }
    close(fd);
  }
}


/* ----------------------------------------------------- */
/* �ഫ�D�{��						 */
/* ----------------------------------------------------- */


static void
transusr(user)
  userec *user;
{
  char buf[64];

  printf("�ഫ %s �ϥΪ�\n", user->userid);

  sprintf(buf, OLD_BBSHOME "/home/%s/man", user->userid);
  if (dashd(buf))
  {
    char index[64], folder[64];

    sprintf(index, "%s/.DIR", buf);
    usr_fpath(folder, user->userid, "gem/" FN_DIR);
    transman(index, folder);
  }
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
