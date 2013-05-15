/*-------------------------------------------------------*/
/* user.c	( NTHU CS MapleBBS Ver 3.00 )		 */
/*-------------------------------------------------------*/
/* target : account / user routines		 	 */
/* create : 95/03/29				 	 */
/* update : 96/04/05				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern char *ufo_tbl[];


/* ----------------------------------------------------- */
/* �{�ҥΨ禡						 */
/* ----------------------------------------------------- */


void
justify_log(userid, justify)	/* itoc.010822: ���� .ACCT �� justify �o���A��O���b FN_JUSTIFY */
  char *userid;
  char *justify;	/* �{�Ҹ�� RPY:email-reply  KEY:�{�ҽX  POP:pop3�{��  REG:���U�� */
{
  char fpath[64];
  FILE *fp;

  usr_fpath(fpath, userid, FN_JUSTIFY);
  if (fp = fopen(fpath, "a"))		/* �Ϊ��[�ɮסA�i�H�O�s�����{�ҰO�� */
  {
    fprintf(fp, "%s\n", justify);
    fclose(fp);
  }
}


static int
ban_addr(addr)
  char *addr;
{
  char *host;
  char foo[128];	/* SoC: ��m���ˬd�� email address */

  /* Thor.991112: �O���Ψӻ{�Ҫ�email */
  sprintf(foo, "%s # %s (%s)\n", addr, cuser.userid, Now());
  f_cat(FN_RUN_EMAILREG, foo);

  /* SoC: �O���� email ���j�p�g */
  str_lower(foo, addr);

  /* check for acl (lower case filter) */

  host = (char *) strchr(foo, '@');
  *host = '\0';

  /* *.bbs@xx.yy.zz�B*.brd@xx.yy.zz �@�ߤ����� */
  if (host > foo + 4 && (!str_cmp(host - 4, ".bbs") || !str_cmp(host - 4, ".brd")))
    return 1;

  /* ���b�զW��W�Φb�¦W��W */
  return (!acl_has(TRUST_ACLFILE, foo, host + 1) ||
           acl_has(UNTRUST_ACLFILE, foo, host + 1) > 0);
}


/* ----------------------------------------------------- */
/* POP3 �{��						 */
/* ----------------------------------------------------- */


#ifdef HAVE_POP3_CHECK

static int		/* >=0:socket -1:�s�u���� */
Get_Socket(site)	/* site for hostname */
  char *site;
{
  int sock;
  struct sockaddr_in sin;
  struct hostent *host;

  sock = 110;

  /* Getting remote-site data */

  memset((char *) &sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(sock);

  if (!(host = gethostbyname(site)))
    sin.sin_addr.s_addr = inet_addr(site);
  else
    memcpy(&sin.sin_addr.s_addr, host->h_addr, host->h_length);

  /* Getting a socket */

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    return -1;
  }

  /* perform connecting */

  if (connect(sock, (struct sockaddr *) & sin, sizeof(sin)) < 0)
  {
    close(sock);
    return -1;
  }

  return sock;
}


static int		/* 0:���\ */
POP3_Check(sock, account, passwd)
  int sock;
  char *account, *passwd;
{
  FILE *fsock;
  char buf[512];

  if (!(fsock = fdopen(sock, "r+")))
  {
    outs("\n�Ǧ^���~�ȡA�Э��մX���ݬ�\n");
    return -1;
  }

  sock = 1;

  while (1)
  {
    switch (sock)
    {
    case 1:		/* Welcome Message */
      if (!fgets(buf, sizeof(buf), fsock))
        strcpy(buf, "Connection failed.");
      break;

    case 2:		/* Verify Account */
      fprintf(fsock, "user %s\r\n", account);
      fflush(fsock);
      if (!fgets(buf, sizeof(buf), fsock))
        strcpy(buf, "Connection failed.");
      break;

    case 3:		/* Verify Password */
      fprintf(fsock, "pass %s\r\n", passwd);
      fflush(fsock);
      if (!fgets(buf, sizeof(buf), fsock))
        strcpy(buf, "Connection failed.");
      sock = -1;
      break;

    default:		/* 0:Successful -1:Failure  */
      fprintf(fsock, "quit\r\n");
      fclose(fsock);
      return sock;
    }

    if (!strncmp(buf, "+OK", 3))
    {
      sock++;
    }
    else
    {
      outs("\n���ݨt�ζǦ^���~�T���p�U�G\n");
      prints("%s\n", buf);
      sock = -1;
    }
  }
}


static int		/* -1:���䴩 0:�K�X���~ 1:���\ */
do_pop3(addr)		/* itoc.010821: ��g�@�U :) */
  char *addr;
{
  int sock, i;
  char *ptr, *str, buf[80], username[80];
  char *alias[] = {"", "pop3.", "mail.", NULL};
  ACCT acct;

  strcpy(username, addr);
  *(ptr = strchr(username, '@')) = '\0';
  ptr++;

  clear();
  move(2, 0);
  prints("�D��: %s\n�b��: %s\n", ptr, username);
  outs("\033[1;5;36m�s�u���ݥD����...�еy��\033[m\n");
  refresh();

  for (i = 0; str = alias[i]; i++)
  {
    sprintf(buf, "%s%s", str, ptr);	/* itoc.020120: �D���W�٥[�W pop3. �ոլ� */
    if ((sock = Get_Socket(buf)) >= 0)	/* ���o�����B���䴩 POP3 */
      break;
  }

  if (sock < 0)
  {
    outs("�z���q�l�l��t�Τ��䴩 POP3 �{�ҡA�ϥλ{�ҫH�稭���T�{\n\n\033[1;36;5m�t�ΰe�H��...\033[m");
    return -1;
  }

  /* guessi.090916 �Y�P�D�����\�s�u �h�h��"�s�u��"���r�� �קK�~�| */
  sleep(1);
  move(4, 0);
  clrtoeol();
  prints("�w���\\�P�D���إ߳s�u�I");

  if (vget(15, 0, "�п�J�H�W�ҦC�X���u�@���b�����K�X�G", buf, 20, NOECHO))
  {
    move(17, 0);
    outs("\033[5;37m�����T�{��...�еy��\033[m\n");

    if (!POP3_Check(sock, username, buf))	/* POP3 �{�Ҧ��\ */
    {
      /* �����v�� */
      sprintf(buf, "POP: %s", addr);
      justify_log(cuser.userid, buf);
      strcpy(cuser.email, addr);
      if (acct_load(&acct, cuser.userid) >= 0)
      {
	time(&acct.tvalid);
	acct_setperm(&acct, PERM_VALID, 0);
      }

      /* �H�H�q���ϥΪ� */
      mail_self(FN_ETC_JUSTIFIED, str_sysop, msg_reg_valid, 0);
      cutmp->status |= STATUS_BIFF;
      vmsg(msg_reg_valid);

      /* guessi.060302 �q�L�{�Ҭ��� */
      rlog("POP3�{��", cuser.userid, addr);

      close(sock);
      return 1;
    }
  }

  close(sock);

  /* POP3 �{�ҥ��� */
  outs("�z���K�X�γ\\�����F�A�ϥλ{�ҫH�稭���T�{\n\n\033[1;36;5m�t�ΰe�H��...\033[m");
  return 0;
}
#endif


/* ----------------------------------------------------- */
/* �]�w E-mail address					 */
/* ----------------------------------------------------- */


int
u_addr()
{
  char *msg, addr[64];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  /* itoc.050405: ���������v�̭��s�{�ҡA�]���|�ﱼ�L�� tvalid (���v����ɶ�) */
  if (HAS_PERM(PERM_ALLDENY))
  {
    vmsg("�z�ثe�Q���v���A�ȮɵL�k��H�c");
    return XEASY;
  }

  if(!HAS_PERM(PERM_MVALID))
  {
    vmsg("�Х���g���U��A�A�i�楻�ާ@");
    return XEASY;
  }

  film_out(FILM_EMAIL, 0);

  if (vget(b_lines - 2, 0, "E-mail �a�}�G", addr, sizeof(cuser.email), DOECHO))
  {
    if (not_addr(addr))
    {
      msg = err_email;
    }
    else if (strlen(addr) < 8)  /* �H�c�a�}���w�j��K�Ӧr�� ex: A@B.EDU */
    {
      msg = "�H�c�a�}���X�W�w �Э��s��J";
    }
    else if (ban_addr(addr))
    {
      msg = "�����������z���H�c�����{�Ҧa�}";
    }
    else
    {
#ifdef EMAIL_JUSTIFY
      if (vans("�ק� E-mail �n���s�{�ҡA�T�w�n�ק��(Y/N)�H[Y] ") == 'n')
	return 0;

#  ifdef HAVE_POP3_CHECK
      if (vans("�O�_�ϥ� POP3 �{��(Y/N)�H[N] ") == 'y')
      {
	if (do_pop3(addr) > 0)	/* �Y POP3 �{�Ҧ��\�A�h���}�A�_�h�H�{�ҫH�H�X */
	  return 0;
      }
#  endif

      if (bsmtp(NULL, NULL, addr, MQ_JUSTIFY) < 0)
      {
	msg = "�����{�ҫH��L�k�H�X�A�Х��T��g E-mail address";
      }
      else
      {
	ACCT acct;

	strcpy(cuser.email, addr);
	cuser.userlevel &= ~PERM_ALLVALID;
	if (acct_load(&acct, cuser.userid) >= 0)
	{
	  strcpy(acct.email, addr);
	  acct_setperm(&acct, 0, PERM_ALLVALID);
	}

	film_out(FILM_JUSTIFY, 0);
	prints("\n%s(%s)�z�n�A�ѩ�z��s E-mail address ���]�w�A\n\n"
	  "�бz���֨� \033[44m%s\033[m �Ҧb���u�@���^�Сy�����{�ҫH��z�C",
	  cuser.userid, cuser.username, addr);
	msg = NULL;
      }
#else
      msg = NULL;
#endif

    }
    vmsg(msg);
  }

  return 0;
}


/* ----------------------------------------------------- */
/* ��g���U��						 */
/* ----------------------------------------------------- */

#ifdef HAVE_REGISTER_FORM

/* guessi.111021 ���ҨϥΪ̦a�} */
int
valid_address(address)
  char *address;
{
  int count = 0;
  static int total = 30;
  static char* list[] = {
    /*  0 ~  4 */ "�L",      "���P",    "�ǰ|",    "�ǭb",    "�Ǯ�",
    /*  5 ~ 10 */ "�S��",    "�F��",    "�J��",    "����",    "�V��",
    /* 11 ~ 15 */ "�[�P",    "���s",    "�^��",    "���D",    "����",
    /* 16 ~ 20 */ "�Ӿǵ�",  "���i�D",  "�ئ��123��", "�j�Ǹ��G�q�@��", "�j�Ǹ��G�q1��",
    /* 21 ~ 25 */ "�j�Ǹ�2�q�@��", "�j�Ǹ�2�q1��", "���[�հ�", "���ڮհ�", "�ئ���@�G�T��",
    /* 26 ~ 30 */ "secret", "���K", "�涳", "�G��", "���f",
    NULL
  };

  do
  {
    if (strstr(address, list[count++]))
      return 1;
  } while (count < total);

  return 0;
}


static void
getfield(line, len, buf, desc, hint)
  int line, len;
  char *hint, *desc, *buf;
{
  move(line, 0);
  prints("%s%s", desc, hint);
  vget(line + 1, 0, desc, buf, len, GCARRY);
}


int
u_register()
{
  FILE *fn;
  int ans, try;
  RFORM rform;
  time_t now;
  struct tm *ptime;

  time(&now);
  ptime = localtime(&now);

#ifdef JUSTIFY_PERIODICAL
  if (HAS_PERM(PERM_VALID) && cuser.tvalid + VALID_PERIOD - INVALID_NOTICE_PERIOD >= ap_start)
#else
  if (HAS_PERM(PERM_VALID))
#endif
  {
    zmsg("�z�������T�{�w�g�����A���ݶ�g�ӽЪ�");
    return XEASY;
  }

  if (HAS_PERM(PERM_MVALID))
  {
    zmsg("�z�w�q�L��B�{�ҡA�б��۶i��q�l�l��a�}�{��");
    return XEASY;
  }

  if (fn = fopen(FN_RUN_RFORM, "rb"))
  {
    while (fread(&rform, sizeof(RFORM), 1, fn))
    {
      if ((rform.userno == cuser.userno) && !strcmp(rform.userid, cuser.userid))
      {
        fclose(fn);
        zmsg("�z�����U�ӽг�|�b�B�z���A�Э@�ߵ���");
        return XEASY;
      }
    }
    fclose(fn);
  }

  if (vans("�z�T�w�n��g���U���(Y/N)�H[N] ") != 'y')
    return XEASY;

  try = 0;
  ans = 'n';

  move(1, 0);
  clrtobot();
  prints("\n%s(%s) �z�n�A�оڹ��g�H�U����ơG\n(�� [Enter] ������l�]�w�A�ýФŨϥΥ��Τ�r)",
    cuser.userid, cuser.username);

  memset(&rform, 0, sizeof(RFORM));
  
  do
  {
    try++;

    if (try > 3)
    {
      vmsg("���~���զ��ƹL�h�A�Э��s�ӹL");
      return XO_BODY;
    }

    getfield( 5, 50, rform.career, "�A�ȳ��G", "�ǥͽЦܤ֥]�t�զW, �Ǩt, �~��");
    getfield( 7, 60, rform.address, "�ثe��}�G", "�����]�t���P���X�A�B���i���ն�αJ�٦a�}");
    getfield( 9, 20, rform.phone, "�s���q�ܡG", "�]�t�ϰ�X(�������X) �ФŶ�g�J�ٹ�q");
    getfield(11,  3, rform.year, "�ͤ�G", "�п�J����(�~) = �褸(�~) - 1911");
    getfield(13,  3, rform.month, "�ͤ�G", "��");
    getfield(15,  3, rform.day, "�ͤ�G", "��");

    /* guessi.111021 ���ҨϥΪ̦a�} */
    if (valid_address(rform.address))
    {
      ans = 'n';
      vmsg("�a�}��T���X�W�w");
      continue;
    }

    /* guessi.090915 �ˬd�~�� */
    if ((ptime->tm_year - atoi(rform.year) - 11) < 5)
    {
      vmsg("���X��p�B�ͷ|�ιq���H");
      ans = 'n';
      continue;
    }
    
    /* guessi.090915 �ˬd��� */
    if (atoi(rform.month) < 1 || atoi(rform.month) > 12)
    {
      vmsg("��T���~�A���G���");
      ans = 'n';
      continue;
    }
    
    /* guessi.090915 �ˬd��� 1. ���ˬd�O�_�W�L���`�d�� */
    if (atoi(rform.day) < 1 || atoi(rform.day) > 31)
    {
      vmsg("��T���~�A���G���");
      ans = 'n';
      continue;
    }
    
    /* guessi.090915 �ˬd��� 2. �ˬd�j�p��O�_�����`�� */
    if ((atoi(rform.month) < 7 && (atoi(rform.month) % 2 == 0) && atoi(rform.day) > 30) ||
	(atoi(rform.month) > 8 && (atoi(rform.month) % 2 == 1) && atoi(rform.day) > 30))
    {
      vmsg("�z��g������ܦh��30��A�d����~�A�Эץ��C");
      ans = 'n';
      continue;
    }
	
    /* guessi.090915 �ˬd��� 3. �ư��S���p�A�G��A�|��Ȥ��z�| */
    if ((atoi(rform.month) == 2) && (atoi(rform.day) > 29))
    {
      vmsg("�z��g������ܦh��30��A�d����~�A�Эץ��C");
      ans = 'n';
      continue;
    }
	
    /* guessi.090915 �N��֤F�ϰ츹�X �ܤ��������X */
    if (strlen(rform.phone) < 5)
    {
      vmsg("�p���q����즳�~�A�Эץ��C");
      ans = 'n';
      continue;
    }
    
    ans = vans("�H�W��g��T�O�_���T Y)�O N)�_ Q)���}�H[N] ");

    if (ans == 'q')
    {
      return 0;
    }
  } while (ans != 'y');

  /* guessi.110902 just in case */
  if (ans == 'y')
  {
    rform.userno = cuser.userno;
    strcpy(rform.userid, cuser.userid);
    time(&rform.rtime);
    rec_add(FN_RUN_RFORM, &rform, sizeof(RFORM));
  }

  return 0;
}
#endif


/* ----------------------------------------------------- */
/* ��g���{�X						 */
/* ----------------------------------------------------- */


#ifdef HAVE_REGKEY_CHECK
int
u_verify()
{
  char buf[80], key[10];
  ACCT acct;

  if (HAS_PERM(PERM_VALID))
  {
    zmsg("�z�������T�{�w�g�����A���ݶ�g�{�ҽX");
	return XEASY;
  }

  /* �ˬd�O�_�q�L��B�{�� */
  if (!HAS_PERM(PERM_MVALID))
  {
    vmsg("�Х���g���U��A�A�i�楻�ާ@");
    return XEASY;
  }
  else
  {
    if (vget(b_lines, 0, "�п�J�{�ҽX�G", buf, 8, DOECHO))
    {
      archiv32(str_hash(cuser.email, cuser.tvalid), key);	/* itoc.010825: ���ζ}�ɤF�A������ tvalid �Ӥ�N�O�F */

      if (str_ncmp(key, buf, 7))
      {
	vmsg("��p�A�z���{�ҽX���~");
      }
      else
      {
	/* �����v�� */
	sprintf(buf, "KEY: %s", cuser.email);
	justify_log(cuser.userid, buf);
	if (acct_load(&acct, cuser.userid) >= 0)
	{
	  time(&acct.tvalid);
	  acct_setperm(&acct, PERM_VALID, 0);
	}

	/* �q�L�{�Ҭ��� */
	rlog("�{�ҽX�{��", cuser.userid, key);

	/* �H�H�q���ϥΪ� */
	mail_self(FN_ETC_JUSTIFIED, str_sysop, msg_reg_valid, 0);
	cutmp->status |= STATUS_BIFF;
	vmsg(msg_reg_valid);
      }
    }
  }

  return XEASY;
}
#endif


/* ----------------------------------------------------- */
/* ��_�v��						 */
/* ----------------------------------------------------- */


int
u_deny()
{
  ACCT acct;
  time_t diff;
  struct tm *ptime;
  char msg[80];

  if (!HAS_PERM(PERM_ALLDENY))
  {
    vmsg("�z�S�Q���v�A���ݴ_�v");
  }
  else
  {
    if ((diff = cuser.tvalid - time(0)) < 0)      /* ���v�ɶ���F */
    {
      if (acct_load(&acct, cuser.userid) >= 0)
      {
	time(&acct.tvalid);
#ifdef JUSTIFY_PERIODICAL
	/* xeon.050112: �b�{�ҧ֨���e�� Cross-Post�A�M�� tvalid �N�|�Q�]�w�쥼�Ӯɶ��A
	   ���_�v�ɶ���F�h�_�v�A�o�˴N�i�H�׹L���s�{�ҡA�ҥH�_�v��n���s�{�ҡC */
	acct_setperm(&acct, 0, PERM_ALLVALID | PERM_ALLDENY);
#else
	acct_setperm(&acct, 0, PERM_ALLDENY);
#endif
	vmsg("�U���ФŦA�ǡA�Э��s�W��");
      }
    }
    else
    {
      ptime = gmtime(&diff);
      sprintf(msg, "�z�٭n�� %d �~ %d �� %d �� %d �� %d ��~��ӽд_�v",
	ptime->tm_year - 70, ptime->tm_yday, ptime->tm_hour, ptime->tm_min, ptime->tm_sec);
      vmsg(msg);
    }
  }

  return XEASY;
}


/* ----------------------------------------------------- */
/* �ӽЧ��u��m�W					 */
/* ----------------------------------------------------- */

void
u_realname_sub(acct)
  ACCT *acct;
{
  ACCT x;
  /* clear screen */
  move(1, 0);
  clrtobot();
  /* backup */
  memcpy(&x, acct, sizeof(ACCT));
  /* show user original realname */
  move(b_lines - 5, 0);
  prints("�z���u��m�W: \033[1;37m%s\033[m\n", x.realname);
  /* input */
  vget(b_lines - 3, 0, "�п�J�z���u��m�W�G", x.realname, RNLEN + 1, DOECHO);
  /* data display */
  move(b_lines - 2, 0);
  prints("�u��m�W�N�� \033[1;30m%s\033[m ��אּ \033[1;32m%s\033[m", acct->realname, x.realname);
  /* check again */
  if (vans("�нT�{�O�_�ק� Y)�T�w N)���� [N] ") == 'y')
  {
    /* check if any data changed? */
    if (!memcmp(acct, &x, sizeof(ACCT)))
    {
      vmsg("�ާ@�����A�θ�ƵL�ץ�");
      return;
    }
    else
    {
      /* administrators' realname can only change by using admin menu func. */
      if (HAS_PERM(PERM_ALLACCT))
      {
        vmsg("���ȤH������ܧ�гz�L���ȿ��ާ@");
        return;
      }
      /* modified from bbsd.c:alog() */
      char buf[512];
      sprintf(buf, "%s %s %-13s�ϥΪ�[%s] %s -> %s\n", Now(), "�m�W�ܧ�",
              cuser.userid, cuser.userid, acct->realname, x.realname);
      /* user application, another copy */
      f_cat(FN_RUN_CRN, buf);
      /* copy user data */
      memcpy(acct, &x, sizeof(ACCT));
      acct_save(acct);
      /* also update cuser data */
      strcpy(cuser.realname, acct->realname);
      /* reset user perm, remove PERM_MVALID also */
      acct_setperm(acct, 0, PERM_ALLVALID | PERM_MVALID);
      /* re-login is required, prompt hint on screen */
      /* kick user by system? ...maybe not a good idea */
      vmsg("�Э��s�W���í��s�ާ@���U�y�{");
      /* mail & biff notify */
      mail_self(FN_ETC_REREG, str_sysop, "���s�{�ҳq��", 0);
      cutmp->status |= STATUS_BIFF;
    }
  }
  return;
}

int
u_realname()
{
  ACCT acct;
  char pass[PSWDLEN + 1];
  /* clear screen */
  move(1, 0);
  clrtobot();
  /* inform user, need to re-validate */
  vmsg("�Ъ`�N�G���u��m�W�N�ݭn���s�{��");
  /* load user data from .ACCT */
  if (acct_load(&acct, cuser.userid) < 0)
    return 0;
  /* require user password */
  vget(b_lines - 5, 0, "�п�J�K�X�H�~�����G", pass, PSWDLEN + 1, NOECHO);
  /* check password first */
  if (chkpasswd(acct.passwd, pass))
    vmsg("�K�X���~�A�ާ@����");
  else /* perform sub-routine */
  {
    /* check if user is already valid his/her account */
    if (HAS_PERM(PERM_VALID))
      vmsg("�w�q�L�{�ҡA��Ƥ��i��ʡA�Y���ץ��Ь����ȤH��");
    else
      u_realname_sub(&acct);
  }

  return 0;
}

/* ----------------------------------------------------- */
/* �ӤH�u��						 */
/* ----------------------------------------------------- */


int
u_info()
{
  char *str = "", username[UNLEN + 1];
  char *ptr = "", feeling[FLLEN + 1];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  move(1, 0);
  strcpy(username, str = cuser.username);
  strcpy(feeling, ptr = cuser.feeling);
  acct_setup(&cuser, 0);

  if (strcmp(username, str))
    memcpy(cutmp->username, str, UNLEN + 1);
  if (strcmp(feeling, ptr))
    memcpy(cutmp->feeling, ptr, FLLEN + 1);

  return 0;
}


int
u_setup()
{
  usint ulevel;
  int len;

  /* itoc.000320: �W��حn��� len �j�p, �]�O�ѤF�� ufo.h ���X�� STR_UFO */

  ulevel = cuser.userlevel;
  if (!ulevel)
    len = NUMUFOS_GUEST;
  else if (ulevel & PERM_ALLADMIN)
    len = NUMUFOS;		/* ADMIN ���F�i�� acl�A�ٶ��K�]�i�H�������N */
  else if (ulevel & PERM_CLOAK)
    len = NUMUFOS - 2;		/* ����ε����Bacl */
  else
    len = NUMUFOS_USER;

  cuser.ufo = cutmp->ufo = bitset(cuser.ufo, len, len, MSG_USERUFO, ufo_tbl);

  return 0;
}


int
u_lock()
{
  char buf[PSWDLEN + 1];

  switch (vans("�O�_�i�J�ù���w���A�A�N����ǰe/�������y(Y�T�{/N����/C�ۭq)�H[N] "))
  {
  case 'c':		/* itoc.011226: �i�ۦ��J�o�b���z�� */
    if (vget(b_lines, 0, "�п�J�o�b���z�ѡG", cutmp->mateid, IDLEN + 1, DOECHO))
      break;

  case 'y':
    strcpy(cutmp->mateid, "����");
    break;

  default:
    return XEASY;
  }

  bbstate |= STAT_LOCK;		/* lkchu.990513: ��w�ɤ��i�^���y */
  cutmp->status |= STATUS_REJECT;	/* ��w�ɤ������y */

  clear();
  move(5, 20);
  prints("\033[1;33m" BBSNAME "    ���m/��w���A\033[m  [%s]", cuser.userid);

  do
  {
    vget(b_lines, 0, "�� �п�J�K�X�A�H�Ѱ��ù���w�G", buf, PSWDLEN + 1, NOECHO);
  } while (chkpasswd(cuser.passwd, buf));

  cutmp->status ^= STATUS_REJECT;
  bbstate ^= STAT_LOCK;

  return 0;
}


int
u_log()
{
  char fpath[64];

  usr_fpath(fpath, cuser.userid, FN_LOG);
  more(fpath, NULL);
  return 0;
}


/* ----------------------------------------------------- */
/* �]�w�ɮ�						 */
/* ----------------------------------------------------- */


/* static */			/* itoc.010110: �� a_xfile() �� */
void
x_file(mode, xlist, flist)
  int mode;			/* M_XFILES / M_UFILES */
  char *xlist[];		/* description list */
  char *flist[];		/* filename list */
{
  int n, i;
  char *fpath, *desc;
  char buf[64];

  move(MENU_XPOS, 0);
  clrtobot();
  n = 0;
  while (desc = xlist[n])
  {
    n++;
    if (n <= 9)			/* itoc.020123: ���G��A�@��E�� */
    {
      move(n + MENU_XPOS - 1, 0);
      clrtoeol();
      move(n + MENU_XPOS - 1, 2);
    }
    else
    {
      move(n + MENU_XPOS - 10, 40);
    }
    prints("(%d) %s", n, desc);

    if (mode == M_XFILES)	/* Thor.980806.����: �L�X�ɦW */
    {
      if (n <= 9)
	move(n + MENU_XPOS - 1, 22);
      else
	move(n + MENU_XPOS - 10, 62);
      outs(flist[n - 1]);
    }
  }

  vget(b_lines, 0, "�п���ɮ׽s���A�Ϋ� [0] �����G", buf, 3, DOECHO);
  i = atoi(buf);
  if (i <= 0 || i > n)
    return;

  n = vget(b_lines, 36, "(D)�R�� (E)�s�� [Q]�����H", buf, 3, LCECHO);
  if (n != 'd' && n != 'e')
    return;

  fpath = flist[--i];
  if (mode == M_UFILES)
    usr_fpath(buf, cuser.userid, fpath);
  else			/* M_XFILES */
    strcpy(buf, fpath);

  if (n == 'd')
  {
    if (vans(msg_sure_ny) == 'y')
      unlink(buf);
  }
  else
  {
    vmsg(vedit(buf, 0) ? "��ʤ���" : "��s����");	/* Thor.981020: �`�N�Qtalk�����D  */
  }
}


int
u_xfile()
{
  int i;

  static char *desc[] =
  {
    "�W���a�I�]�w��",
    "�W����",
    "ñ�W��.1",
    "ñ�W��.2",
    "ñ�W��.3",
    "ñ�W��.4",
    "ñ�W��.5",
    "ñ�W��.6",
    "�Ȧs��.1",
    "�Ȧs��.2",
    "�Ȧs��.3",
    "�Ȧs��.4",
    "�Ȧs��.5",
    NULL
  };

  static char *path[] =
  {
    "acl",
    "plans",
    FN_SIGN ".1",
    FN_SIGN ".2",
    FN_SIGN ".3",
    FN_SIGN ".4",
    FN_SIGN ".5",
    FN_SIGN ".6",
    "buf.1",
    "buf.2",
    "buf.3",
    "buf.4",
    "buf.5"
  };

  i = HAS_PERM(PERM_ALLADMIN) ? 0 : 1;
  x_file(M_UFILES, &desc[i], &path[i]);
  return 0;
}
