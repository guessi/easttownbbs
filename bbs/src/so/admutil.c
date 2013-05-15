/*-------------------------------------------------------*/
/* admutil.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : �������O					 */
/* create : 95/03/29					 */
/* update : 01/03/01					 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern BCACHE *bshm;
extern UCACHE *ushm;


/* ----------------------------------------------------- */
/* ���ȫ��O						 */
/* ----------------------------------------------------- */


int
a_user()
{
  int ans;
  ACCT acct;

  move(1, 0);
  clrtobot();

  while (ans = acct_get(msg_uid, &acct))
  {
    if (ans > 0)
      acct_setup(&acct, 1);
  }
  return 0;
}

int
a_changeRealName()
{
  int ans;
  ACCT acct;

  move(1, 0);
  clrtobot();
  
  while (ans = acct_get(msg_uid, &acct))
  {
    if (ans > 0)
      acct_changeRealName(&acct);
  }
  return 0;
}

int
a_show_from()
{
  int ans;
  ACCT acct;
  char fpath[64], buf[128], tmp[64], last[4];
  char reason[64], why[64];

  if (vans("�O�_�d�ߨϥΪ̤W���ӷ��H Y)�d�� N)���} [N] ") == 'y') /* guessi.060612 �n�D��J��] */
  {
    if (!vget(b_lines, 0, "�z��:", why, 64, DOECHO))
      return;
    sprintf(buf, "�d�ݨϥΪ̤W���ӷ� �z��: %s ", why);
    alog("�d�ߨӷ�", buf);
  }
  else
  {
    alog("�d�ߨӷ�", "");
    return; /* guessi.060612 ���X�ì������� */
  }

  move(1, 0);

  while (ans = acct_get(msg_uid, &acct))
  {
    if (ans > 0)
    {
      usr_fpath(fpath, acct.userid, FN_LOG);

      sprintf(reason, "�d�ߨϥΪ� %-13s�W���ӷ�", acct.userid);
      alog("�d�ߨӷ�", reason);
      
      if (vget(b_lines, 0, "���d�̫߳�X�����(��ܥ����Ы�Enter���~��) ", last, 4, DOECHO))
      {
        sprintf(tmp, "tmp/sf.%s", acct.userid);
        sprintf(buf, "cat %s | tail -%s > %s", fpath, last, tmp);
        system(buf);
        more(tmp, NULL);
	unlink(tmp);
      }
      else
        more(fpath, NULL);
      return 0;
    }
  }
  return;
}

int
a_search()	/* itoc.010902: �ɤO�j�M�ϥΪ� */
{
  ACCT acct;
  char c;
  char key[30];

  if (!vget(b_lines, 0, "�п�J����r(�m�W/�ʺ�/�ӷ�/�H�c)�G", key, 30, DOECHO))
    return XEASY;  

  alog("�j�M���", key);

  /* itoc.010929.����: �u�O�����ɤO :p �Ҽ{���� reaper ���X�@�� .PASSWDS �A�h�� */

  for (c = 'a'; c <= 'z'; c++)
  {
    char buf[64];
    struct dirent *de;
    DIR *dirp;

    sprintf(buf, "usr/%c", c);
    if (!(dirp = opendir(buf)))
      continue;

    while (de = readdir(dirp))
    {
      if (acct_load(&acct, de->d_name) < 0)
	continue;

      if (strstr(acct.realname, key) || strstr(acct.username, key) ||  
	strstr(acct.lasthost, key) || strstr(acct.email, key))
      {
	move(1, 0);
	acct_setup(&acct, 1);

	if (vans("�O�_�~��j�M�U�@���H[N] ") != 'y')
	{
	  closedir(dirp);
 	  goto end_search;
 	}
      }
    }
    closedir(dirp);
  }
end_search:
  vmsg("�j�M����");
  return 0;
}

int
a_newbrd()
{
  BRD newboard;

  memset(&newboard, 0, sizeof(BRD));

  /* itoc.010211: �s�ݪO�w�] postlevel = PERM_POST; battr = ����H */
  newboard.postlevel = PERM_POST;
  newboard.battr = BRD_NOTRAN;

  if (brd_new(&newboard) >= 0)
    vmsg("�s�O���ߡA�O�ۥ[�J�����s��");

  return 0;
}

int
a_editbrd()		/* itoc.010929: �ק�ݪO�ﶵ */
{
  int bno;
  BRD *brd;
  char bname[BNLEN + 1];

  if (brd = ask_board(bname, BRD_R_BIT, NULL))
  {
    bno = brd - bshm->bcache;
    brd_edit(bno);
  }
  else
  {
    vmsg(err_bid);
  }

  return 0;
}


int
a_xfile()		/* �]�w�t���ɮ� */
{
  static char *desc[] =
  {
    "�ݪO�峹����",

    "�q�L�{�ҫH��",
    "�����{�ҫH��",
    "��B�{�ҫH��",
    "���s�{�ҳq��",

#ifdef HAVE_DETECT_CROSSPOST
    "��K���v�q��",
#endif
    
    "�����W��",
    "���ȦW��",

    "�`��",

#ifdef HAVE_WHERE
    "�G�m IP",
    "�G�m FQDN",
#endif

#ifdef HAVE_TIP
    "�C��p���Z",
#endif

    "�i�����ؤ��i",

#ifdef HAVE_LOVELETTER
    "���Ѳ��;���w",
#endif

    "�{�ҥզW��",
    "�{�Ҷ¦W��",

    "���H�զW��",
    "���H�¦W��",

#ifdef HAVE_LOGIN_DENIED
    "�ڵ��s�u�W��",
#endif

    NULL
  };

  static char *path[] =
  {
    FN_ETC_EXPIRE,

    FN_ETC_VALID,
    FN_ETC_JUSTIFIED,
    FN_ETC_MJUSTIFIED,
    FN_ETC_REREG,

#ifdef HAVE_DETECT_CROSSPOST
    FN_ETC_CROSSPOST,
#endif
    
    FN_ETC_BADID,
    FN_ETC_SYSOP,

    FN_ETC_FEAST,

#ifdef HAVE_WHERE
    FN_ETC_HOST,
    FN_ETC_FQDN,
#endif

#ifdef HAVE_TIP
    FN_ETC_TIP,
#endif

    FN_ETC_ANNOUNCE,

#ifdef HAVE_LOVELETTER
    FN_ETC_LOVELETTER,
#endif

    TRUST_ACLFILE,
    UNTRUST_ACLFILE,

    MAIL_ACLFILE,
    UNMAIL_ACLFILE,

#ifdef HAVE_LOGIN_DENIED
    BBS_ACLFILE,
#endif

  };

  x_file(M_XFILES, desc, path);
  return 0;
}


int
a_resetsys()		/* ���m */
{
  switch (vans("�� �t�έ��] 1)�ʺA�ݪO 2)�����s�� 3)���W�ξ׫H 4)�����G[Q] "))
  {
  case '1':
    system("bin/camera");
    alog("���m�ʺA�ݪO", "");
    break;

  case '2':
    system("bin/account -nokeeplog");
    brh_save();
    board_main();
    alog("���m�����s��", "");
    break;

  case '3':
    system("kill -1 `cat run/bmta.pid`; kill -1 `cat run/bguard.pid`");
    alog("���m���W�ξ׫H", "");
    break;

  case '4':
    system("kill -1 `cat run/bmta.pid`; kill -1 `cat run/bguard.pid`; bin/account -nokeeplog; bin/camera");
    brh_save();
    board_main();
    alog("���m�t��", "");
    break;
  }
  return XEASY;
}


/* ----------------------------------------------------- */
/* �٭�ƥ���						 */
/* ----------------------------------------------------- */


static void
show_availability(type)		/* �N BAKPATH �̭��Ҧ��i���^�ƥ����ؿ��L�X�� */
  char *type;
{
  int tlen, len, col;
  char *fname, fpath[64];
  struct dirent *de;
  DIR *dirp;
  FILE *fp;

  if (dirp = opendir(BAKPATH))
  {
    col = 0;
    tlen = strlen(type);

    sprintf(fpath, "tmp/restore.%s", cuser.userid);
    fp = fopen(fpath, "w");
    fputs("�� �i�Ѩ��^���ƥ����G\n\n", fp);

    while (de = readdir(dirp))
    {
      fname = de->d_name;
      if (!strncmp(fname, type, tlen))
      {
	len = strlen(fname) + 2;
	if (SCR_WIDTH - col < len)
	{
	  fputc('\n', fp);
	  col = 0;
	}
	else
	{
	  col += len;
	}
	fprintf(fp, "%s  ", fname);
      }
    }

    fputc('\n', fp);
    fclose(fp);
    closedir(dirp);

    
    more(fpath, (char *) -1);
    unlink(fpath);
  }
}


int
a_restore()
{
  int ch;
  char *type, *ptr;
  char *tpool[3] = {"brd", "gem", "usr"};
  char date[20], brdname[BNLEN + 1], src[64], cmd[256];
  ACCT acct;
  BPAL *bpal;

  ch = vans("�� �٭�ƥ� 1)�ݪO 2)��ذ� 3)�ϥΪ̡G[Q] ") - '1';
  if (ch < 0 || ch >= 3)
    return XEASY;

  type = tpool[ch];
  show_availability(type);

  if (vget(b_lines, 0, "�n���^���ƥ��ؿ��G", date, 20, DOECHO))
  {
    /* �קK�������F�@�Ӧs�b���ؿ��A���O�M type ���X */
    if (strncmp(date, type, strlen(type)))
      return 0;

    sprintf(src, BAKPATH"/%s", date);
    if (!dashd(src))
      return 0;
    ptr = strchr(src, '\0');

    clear();
    move(3, 0);
    outs("���٭�ƥ����ݪO/�ϥΪ̥����w�s�b�C\n"
      "�Y�ӬݪO/�ϥΪ̤w�R���A�Х����s�}�]/���U�@�ӦP�W���ݪO/�ϥΪ̡C\n"
      "�٭�ƥ��ɽнT�{�ӬݪO�L�H�ϥ�/�ϥΪ̤��b�u�W");

    if (ch == 0 || ch == 1)
    {
      if (!ask_board(brdname, BRD_L_BIT, NULL))
	return 0;
      sprintf(ptr, "/%s%s.tgz", ch == 0 ? "" : "brd/", brdname);
    }
    else /* if (ch == 2) */
    {
      if (acct_get(msg_uid, &acct) <= 0)
	return 0;
      type = acct.userid;
      str_lower(type, type);
      sprintf(ptr, "/%c/%s.tgz", *type, type);
    }

    if (!dashf(src))
    {
      /* �ɮפ��s�b�A�q�`�O�]���ƥ��I�ɸӬݪO/�ϥΪ̤w�Q�R���A�άO��ɮڥ��N�٨S���ӬݪO/�ϥΪ� */
      vmsg("�ƥ��ɮפ��s�b�A�иոը�L�ɶ��I���ƥ�");
      return 0;
    }

    if (vans("�٭�ƥ���A�ثe�Ҧ���Ƴ��|�y���A�аȥ��T�w(Y/N)�H[N] ") != 'y')
      return 0;

    alog("�٭�ƥ�", src);

    /* �����Y */
    if (ch == 0)
      ptr = "brd";
    else if (ch == 1)
      ptr = "gem/brd";
    else /* if (ch == 2) */
      sprintf(ptr = date, "usr/%c", *type);
    sprintf(cmd, "tar xfz %s -C %s/", src, ptr);
    /* system(cmd); */

#if 1	/* ��������ʰ��� */
    move(7, 0);
    outs("\n�ХH bbs �����n�J�u�@���A�é�\033[1;36m�a�ؿ�\033[m����\n\n\033[1;33m");
    outs(cmd);
    outs("\033[m\n\n");
#endif

    /* tar ���H��A�٭n������ */
    if (vans("�� ���O Y)�w���\\����H�W���O Q)������G[Q] ") == 'y')
    {
      if (ch == 0)	/* �٭�ݪO�ɡA�n��s�O�� */
      {
	if ((ch = brd_bno(brdname)) >= 0)
	{
	  brd_fpath(src, brdname, fn_pal);
	  bpal = bshm->pcache + ch;
	  bpal->pal_max = image_pal(src, bpal->pal_spool);
	}
      }
      else if (ch == 2)	/* �٭�ϥΪ̮ɡA���٭� userno */
      {
	ch = acct.userno;
	if (acct_load(&acct, type) >= 0)
	{
	  acct.userno = ch;
	  acct_save(&acct);
	}
      }
      vmsg("�٭�ƥ����\\�I");
      return 0;
    }
  }

  vmsg(msg_cancel);
  return 0;
}


#ifdef HAVE_REGISTER_FORM

/* ----------------------------------------------------- */
/* �B�z Register Form					 */
/* ----------------------------------------------------- */


static void
biff_user(userno)
  int userno;
{
  UTMP *utmp, *uceil;

  utmp = ushm->uslot;
  uceil = (void *) utmp + ushm->offset;
  do
  {
    if (utmp->userno == userno)
      utmp->status |= STATUS_BIFF;
  } while (++utmp <= uceil);
}


static int
scan_register_form(fd)
  int fd;
{
  static char logfile[] = FN_RUN_RFORM_LOG;
  static char *reason[] = 
  {
    "�Э��s��g���U��� �ê��W���T��T",
    "�Э��s��g���U��� �z�ҿ�J����J��T�t���L�k���Ѫ��ýX",
    "�иԶ���U��T ���קK�ϥΤ������Ѥ�²�u�٩I",
    "�иԶ�m�W��T �гz�L�����m�ק�m�W�n��g���T��T",
    "�иԶ��}��� �Цܤ֥]�t: ��(��), �m����, ���X",
    "�иԶ��}��T �ФŶ�g�ն�a�}�αJ�٩и�",
    "�иԶ�s���q�� �ФŶ�g�ն�J�٤���Ǹ��X",
    "�иԶ�ͤ��T ����(�~) = �褸(�~) - 1911",
    "�иԶ�A�ȳ�� �]�t: �Ǯ�, ��t, �~��",
    "�иԶ�A�ȳ��",
    NULL
  };

  ACCT acct;
  RFORM rform;
  HDR fhdr;
  FILE *fout;

  int op, n;
  char buf[128], msg[256], *agent, *userid, *str;

  vs_bar("�f�ֵ��U���");
  agent = cuser.userid;

  while (read(fd, &rform, sizeof(RFORM)) == sizeof(RFORM))
  {
    userid = rform.userid;
    move(2, 0);
    prints("�ӽХN��: %s (�ӽЮɶ��G%s)\n", userid, Btime(&rform.rtime));
    prints("����ͤ�: %02s/%02s/%02s\n", rform.year, rform.month, rform.day);
    prints("�A�ȳ��: %s\n", rform.career);
    prints("�ثe��}: %s\n", rform.address);
    prints("�s���q��: %s\n%s\n", rform.phone, msg_seperator);
    clrtobot();

    if ((acct_load(&acct, userid) < 0) || (acct.userno != rform.userno))
    {
      vmsg("�d�L���H");
      op = 'd';
    }
    else
    {
      acct_show(&acct, 2);

#ifdef JUSTIFY_PERIODICAL
      if (acct.userlevel & PERM_VALID && acct.tvalid + VALID_PERIOD - INVALID_NOTICE_PERIOD >= acct.lastlogin)
#else
      if (acct.userlevel & PERM_VALID)
#endif
      {
	vmsg("���b���w�g�������U");
	op = 'd';
      }
      else if (acct.userlevel & PERM_ALLDENY)
      {
	/* itoc.050405: ���������v�̭��s�{�ҡA�]���|�ﱼ�L�� tvalid (���v����ɶ�) */
	vmsg("���b���ثe�Q���v��");
	op = 'd';
      }
      else
      {
	op = vans("�� �]�w�ﶵ: Y)�q�L N)�h�� Q)���X D)�R�� S)���L [S] ");
      }
    }

    switch (op)
    {
    case 'y':

      /* �����v�� */
      sprintf(msg, "REG: %s:%s:%s:by %s", rform.phone, rform.career, rform.address, agent);
      justify_log(acct.userid, msg);

      acct.year = atoi(rform.year);
      acct.month = atoi(rform.month);
      acct.day = atoi(rform.day);
      
      time(&(acct.tvalid));
      /* itoc.041025: �o�� acct_setperm() �èS�����b acct_load() �᭱�A�����j�F�@�� vans()�A
         �o�i��y������ acct �h�л\�s .ACCT �����D�C���L�]���O�����~�����v���A�ҥH�N����F */

      /* �O���ϥΪ̳q�L�{�� ��m��acct.tvalid���� �Ϩ�ɶ��۪� */

      sprintf(msg, "by %s", cuser.userid);  /* guessi.100407 ���[����(�ѽּf��) */
      rlog("���U��{��", acct.userid, msg); /* guessi.060302 ���ȼf�֬��� */

      acct_setperm(&acct, PERM_MVALID, 0);

      utmp_admset(acct.userno, STATUS_DATALOCK);

      /* �H�H�q���ϥΪ� */
      usr_fpath(buf, userid, fn_dir);
      hdr_stamp(buf, HDR_LINK, &fhdr, FN_ETC_MJUSTIFIED);
      strcpy(fhdr.title, MSG_REG_MVALID);
      strcpy(fhdr.owner, str_sysop);
      rec_add(buf, &fhdr, sizeof(fhdr));

      strcpy(rform.agent, agent);
      rec_add(logfile, &rform, sizeof(RFORM));

      biff_user(rform.userno);

      break;

    case 'q':			/* �Ӳ֤F�A������ */

      do
      {
	rec_add(FN_RUN_RFORM, &rform, sizeof(RFORM));
      } while (read(fd, &rform, sizeof(RFORM)) == sizeof(RFORM));

    case 'd':
      break;

    case 'n':

      move(9, 0);
      prints("�д��X�h�^�ӽЪ��]�A��J�ﶵ�Ϊ�����J��]�A�� <enter> ����\n\n");
      for (n = 0; str = reason[n]; n++)
	prints("%d) \033[1;3%dm%s\033[m\n", n, (n % 2) ? 7 : 0, str);
      clrtobot();

      if (op = vget(b_lines, 0, "�h�^��]�G", buf, 60, DOECHO))
      {
	int i;
	char folder[80], fpath[80];
	HDR fhdr;

	i = op - '0';
	if (i >= 0 && i < n)
	  strcpy(buf, reason[i]);

	usr_fpath(folder, acct.userid, fn_dir);
	if (fout = fdopen(hdr_stamp(folder, 0, &fhdr, fpath), "w"))
	{
	  fprintf(fout, "�z�n:\n\t�ѩ�z���Ѫ���Ƥ����Թ� �ڭ̵L�k�T�{����"
	    "\n\n\t�бz���s��g���U��� ����\n��]�p�U:\n\t\033[1;37m%s\033[m\n", buf);
	  fclose(fout);

	  strcpy(fhdr.owner, agent);
	  strcpy(fhdr.title, "[�h��] �бz���s��g���U���");
	  rec_add(folder, &fhdr, sizeof(fhdr));
	}

	strcpy(rform.reply, buf);	/* �z�� */
	strcpy(rform.agent, agent);
	rec_add(logfile, &rform, sizeof(RFORM));

	break;
      }

    default:			/* put back to regfile */

      rec_add(FN_RUN_RFORM, &rform, sizeof(RFORM));
    }
  }
}

int
verifyIdNumber(char *s)
{
   char  *p, *LEAD="ABCDEFGHJKLMNPQRSTUVXYWZIO";
   int x, i;

   if (strlen(s) != 10 || (p = strchr(LEAD, toupper(*s))) == NULL)
     return 0;

   x = p - LEAD;
   x = x / 10 + x % 10 * 9;
   p = s + 1;

   if (*p != '1' && *p != '2')
     return 0;

   for(i = 1; i < 9; i++)
   {
     if (isdigit(*p))
       x += (*p++ - '0') * (9 - i);
     else
       return 0;
   }
   x = 9 - x % 10;
   return  (x == *p - '0');
}


/*

	A=10 �x�_�� J=18 �s�˿� S=26 ������
	B=11 �x���� K=19 �]�߿� T=27 �̪F��
	C=12 �򶩥� L=20 �x���� U=28 �Ὤ��
	D=13 �x�n�� M=21 �n�뿤 V=29 �x�F��
	E=14 ������ N=22 ���ƿ� W=32 ������
	F=15 �x�_�� O=35 �s�˥� X=30 ���
	G=16 �y���� P=23 ���L�� Y=31 �����s
	H=17 ��鿤 Q=24 �Ÿq�� Z=33 �s����
	I=34 �Ÿq�� R=25 �x�n��

	X123456789 �N�Ĥ@�ӭ^��r�ഫ�����Ʀr
	abcdefghjklmnpqrstuvxywzio ���O���� 10 ~ 35
	�N�ഫ��ƭ� �Q��� = X1 �Ӧ�� = X2
	��l���E�X���O���� D1 ~ D9 �M�J�H�U�����oY

	Y = X1 + 9*X2 
	  + 8*D1 + 7*D2 + 6*D3 + 5*D4
	  + 4*D5 + 3*D6 + 2*D7 + 1*D8 + D9

	�YY�i�Q10�㰣 => ���T

*/

int a_verifyById()
{
  char idnum[11], userid[IDLEN + 1], buf[128];
  ACCT acct;
  HDR fhdr;

  
  if (vans("�нT�{�O�_�ާ@���\\�� Y)�T�w N)���} [N] ") != 'y')
     return XEASY;
     
  if (!vget(b_lines, 0, "�п�J�ϥΪ̱b���G", userid, IDLEN + 1, DOECHO))
     return XEASY;

  if (acct_load(&acct, userid) < 0)
  {
     vmsg(err_uid);
     return XEASY;
  }

  if (acct.userlevel & PERM_VALID)
  {
    vmsg("�ӨϥΪ̤w�g�q�L�{��");
    return XEASY;
  }

  for (;;)
  {
    if (!vget(b_lines, 0, "�п�J�����Ҧr���G", idnum, 11, LCECHO))
      return XEASY; /* �����Ҧr�� LCECHO */

    idnum[0] = toupper(idnum[0]);

    if (verifyIdNumber(idnum))
    {
      time(&(acct.tvalid)); /* �����{�Үɶ� */

      acct.userlevel |= PERM_VALID;     /* ���� PERM_VALID �v�� */
      acct_save(&acct);

      /* �H�H���ϥΪ� */
      usr_fpath(buf, userid, fn_dir);
      hdr_stamp(buf, HDR_LINK, &fhdr, FN_ETC_JUSTIFIED);
      strcpy(fhdr.title, MSG_REG_VALID);
      strcpy(fhdr.owner, str_sysop);
      rec_add(buf, &fhdr, sizeof(fhdr));
      /* ����biff�q�� */
      biff_user(acct.userno);

      sprintf(buf, "%s by %s", idnum, cuser.userid);
      rlog("�����һ{��", userid, buf);

      sprintf(buf, "IDnum: %s by %s", idnum, cuser.userid);
      justify_log(acct.userid, buf); /* guessi.060320 �O��justify */
   
      sprintf(buf, "�ϥΪ� %s �q�L�����һ{��", userid);
      vmsg(buf);
      break;
    }
    vmsg("�����Ҧr�����~");
  }
}

int
a_register()
{
  int num;
  char buf[80];

  num = rec_num(FN_RUN_RFORM, sizeof(RFORM));
  if (num <= 0)
  {
    zmsg("�ثe�õL�s���U���");
    return XEASY;
  }

  sprintf(buf, "�@�� %d ����ơA�}�l�f�ֶ� Y)�}�l�f�z N)���} [N] ", num);
  num = XEASY;

  if (vans(buf) == 'y')
  {
    sprintf(buf, "%s.tmp", FN_RUN_RFORM);
    if (dashf(buf))
    {
      vmsg("��L SYSOP �]�b�f�ֵ��U�ӽг�");
    }
    else
    {
      int fd;

      rename(FN_RUN_RFORM, buf);
      fd = open(buf, O_RDONLY);
      if (fd >= 0)
      {
	scan_register_form(fd);
	close(fd);
	unlink(buf);
	num = 0;
      }
      else
      {
	vmsg("�L�k�}�ҵ��U��Ƥu�@��");
      }
    }
  }
  return num;
}


int
a_regmerge()			/* itoc.000516: �_�u�ɵ��U��״_ */
{
  char fpath[64];
  FILE *fp;

  sprintf(fpath, "%s.tmp", FN_RUN_RFORM);
  if (dashf(fpath))
  {
    vmsg("�Х��T�w�w�L��L�����b�f�ֵ��U��A�H�K�o���Y���N�~�I");

    if (vans("�T�w�n�Ұʵ��U��״_�\\��(Y/N)�H[N] ") == 'y')
    {
      if (fp = fopen(FN_RUN_RFORM, "a"))
      {
	f_suck(fp, fpath);
	fclose(fp);
	unlink(fpath);
      }
      vmsg("�B�z�����A�H��Фp�ߡI");
    }
  }
  else
  {
    zmsg("�ثe�õL�״_���U�椧���n");
  }
  return XEASY;
}
#endif	/* HAVE_REGISTER_FORM */


/* ----------------------------------------------------- */
/* �H�H�������ϥΪ�/�O�D				 */
/* ----------------------------------------------------- */


static void
add_to_list(list, id)
  char *list;
  char *id;		/* ���� end with '\0' */
{
  char *i;

  /* ���ˬd���e�� list �̭��O�_�w�g���F�A�H�K���Х[�J */
  for (i = list; *i; i += IDLEN + 1)
  {
    if (!strncmp(i, id, IDLEN))
      return;
  }

  /* �Y���e�� list �S���A���򪽱����[�b list �̫� */
  str_ncpy(i, id, IDLEN + 1);
}


static void
make_bm_list(list)
  char *list;
{
  BRD *head, *tail;
  char *ptr, *str, buf[BMLEN + 1];

  /* �h bshm ����X�Ҧ� brd->BM */

  head = bshm->bcache;
  tail = head + bshm->number;
  do				/* �ܤ֦� note �@�O�A������ݪO���ˬd */
  {
    ptr = buf;
    strcpy(ptr, head->BM);

    while (*ptr)	/* �� brd->BM �� bm1/bm2/bm3/... �U�� bm ��X�� */
    {
      if (str = strchr(ptr, '/'))
	*str = '\0';
      add_to_list(list, ptr);
      if (!str)
	break;
      ptr = str + 1;
    }      
  } while (++head < tail);
}


static void
make_all_list(list)
  char *list;
{
  int fd;
  SCHEMA schema;

  if ((fd = open(FN_SCHEMA, O_RDONLY)) < 0)
    return;

  while (read(fd, &schema, sizeof(SCHEMA)) == sizeof(SCHEMA))
    add_to_list(list, schema.userid);

  close(fd);
}


static void
send_list(title, fpath, list)
  char *title;		/* �H�󪺼��D */
  char *fpath;		/* �H���ɮ� */
  char *list;		/* �H�H���W�� */
{
  char folder[64], *ptr;
  HDR mhdr;

  for (ptr = list; *ptr; ptr += IDLEN + 1)
  {
    usr_fpath(folder, ptr, fn_dir);
    if (hdr_stamp(folder, HDR_LINK, &mhdr, fpath) >= 0)
    {
      strcpy(mhdr.owner, str_sysop);
      strcpy(mhdr.title, title);
      mhdr.xmode = 0;
      rec_add(folder, &mhdr, sizeof(HDR));
    }
  }
}


static void
biff_bm()
{
  UTMP *utmp, *uceil;

  utmp = ushm->uslot;
  uceil = (void *) utmp + ushm->offset;
  do
  {
    if (utmp->pid && (utmp->userlevel & PERM_BM))
      utmp->status |= STATUS_BIFF;
  } while (++utmp <= uceil);
}


static void
biff_all()
{
  UTMP *utmp, *uceil;

  utmp = ushm->uslot;
  uceil = (void *) utmp + ushm->offset;
  do
  {
    if (utmp->pid)
      utmp->status |= STATUS_BIFF;
  } while (++utmp <= uceil);
}


int
m_bm()
{
  char *list, fpath[64];
  FILE *fp;
  int size;

  if (vans("�n�H�H�������Ҧ��O�D(Y/N)�H[N] ") != 'y')
    return XEASY;

  strcpy(ve_title, "[�O�D�q�i] ");
  if (!vget(1, 0, "���D�G", ve_title, TTLEN + 1, GCARRY))
    return 0;

  usr_fpath(fpath, cuser.userid, "sysmail");
  if (fp = fopen(fpath, "w"))
  {
    fprintf(fp, "�� [�O�D�q�i] �����q�i�A���H�H�G�U�O�D\n");
    fprintf(fp, "-------------------------------------------------------------------------\n");
    fclose(fp);
  }

  curredit = EDIT_MAIL;
  if (vedit(fpath, 1) >= 0)
  {
    vmsg("�ݭn�@�q�Z�����ɶ��A�Э@�ߵ���");

    size = (IDLEN + 1) * MAXBOARD * 4;	/* ���]�C�O�|�ӪO�D�w���� */
    if (list = (char *) malloc(size))
    {
      memset(list, 0, size);

      make_bm_list(list);
      send_list(ve_title, fpath, list);

      free(list);
      biff_bm();
    }
  }
  else
  {
    vmsg(msg_cancel);
  }

  unlink(fpath);

  return 0;
}


int
m_all()
{
  char *list, fpath[64];
  FILE *fp;
  int size;

  if (vans("�n�H�H�������ϥΪ�(Y/N)�H[N] ") != 'y')
    return XEASY;    

  strcpy(ve_title, "[�t�γq�i] ");
  if (!vget(1, 0, "���D�G", ve_title, TTLEN + 1, GCARRY))
    return 0;

  usr_fpath(fpath, cuser.userid, "sysmail");
  if (fp = fopen(fpath, "w"))
  {
    fprintf(fp, "�� [�t�γq�i] �����q�i�A���H�H�G�����ϥΪ�\n");
    fprintf(fp, "-------------------------------------------------------------------------\n");
    fclose(fp);
  }

  curredit = EDIT_MAIL;
  if (vedit(fpath, 1) >= 0)
  {
    vmsg("�ݭn�@�q�Z�����ɶ��A�Э@�ߵ���");

    size = (IDLEN + 1) * rec_num(FN_SCHEMA, sizeof(SCHEMA));
    if (list = (char *) malloc(size))
    {
      memset(list, 0, size);

      make_all_list(list);
      send_list(ve_title, fpath, list);

      free(list);
      biff_all();
    }
  }
  else
  {
    vmsg(msg_cancel);
  }

  unlink(fpath);

  return 0;
}
