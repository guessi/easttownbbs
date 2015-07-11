/*-------------------------------------------------------*/
/* manage.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : �ݪO�޲z				 	 */
/* create : 95/03/29				 	 */
/* update : 96/04/05				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern BCACHE *bshm;


#ifdef HAVE_TERMINATOR
/* ----------------------------------------------------- */
/* �����\�� : �ط�������				 */
/* ----------------------------------------------------- */


extern char xo_pool[];


#define MSG_TERMINATOR	"�m�ط������١n"

int
post_terminator(xo)		/* Thor.980521: �׷��峹�R���j�k */
  XO *xo;
{
  int mode, type;
  HDR *hdr;
  char keyOwner[80], keyTitle[TTLEN + 1], buf[80];

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_FOOT;

  mode = vans(MSG_TERMINATOR "�R�� (1)����@�� (2)������D (3)�۩w�H[Q] ") - '0';

  if (mode == 1)
  {
    hdr = (HDR *) xo_pool + (xo->pos - xo->top);
    strcpy(keyOwner, hdr->owner);
  }
  else if (mode == 2)
  {
    hdr = (HDR *) xo_pool + (xo->pos - xo->top);
    strcpy(keyTitle, str_ttl(hdr->title));		/* ���� Re: */
  }
  else if (mode == 3)
  {
    if (!vget(b_lines, 0, "�@�̡G", keyOwner, 73, DOECHO))
      mode ^= 1;
    if (!vget(b_lines, 0, "���D�G", keyTitle, TTLEN + 1, DOECHO))
      mode ^= 2;
  }
  else
  {
    return XO_FOOT;
  }

  type = vans(MSG_TERMINATOR "�R�� (1)��H�O (2)�D��H�O (3)�Ҧ��ݪO�H[Q] ");
  if (type < '1' || type > '3')
    return XO_FOOT;

  sprintf(buf, "�R��%s�G%.35s ��%s�O�A�T�w��(Y/N)�H[N] ", 
    mode == 1 ? "�@��" : mode == 2 ? "���D" : "����", 
    mode == 1 ? keyOwner : mode == 2 ? keyTitle : "�۩w", 
    type == '1' ? "��H" : type == '2' ? "�D��H" : "�Ҧ���");

  if (vans(buf) == 'y')
  {
    BRD *bhdr, *head, *tail;
    char tmpboard[BNLEN + 1];

    /* Thor.980616: �O�U currboard�A�H�K�_�� */
    strcpy(tmpboard, currboard);

    head = bhdr = bshm->bcache;
    tail = bhdr + bshm->number;
    do				/* �ܤ֦� note �@�O */
    {
      int fdr, fsize, xmode;
      FILE *fpw;
      char fpath[64], fnew[64], fold[64];
      HDR *hdr;

      xmode = head->battr;
      if ((type == '1' && (xmode & BRD_NOTRAN)) || (type == '2' && !(xmode & BRD_NOTRAN)))
	continue;

      /* Thor.980616: ��� currboard�A�H cancel post */
      strcpy(currboard, head->brdname);

      sprintf(buf, MSG_TERMINATOR "�ݪO�G%s \033[5m...\033[m", currboard);
      outz(buf);
      refresh();

      brd_fpath(fpath, currboard, fn_dir);

      if ((fdr = open(fpath, O_RDONLY)) < 0)
	continue;

      if (!(fpw = f_new(fpath, fnew)))
      {
	close(fdr);
	continue;
      }

      fsize = 0;
      mgets(-1);
      while (hdr = mread(fdr, sizeof(HDR)))
      {
	xmode = hdr->xmode;

	if ((xmode & POST_MARKED) || 
	  ((mode & 1) && strcmp(keyOwner, hdr->owner)) ||
	  ((mode & 2) && strcmp(keyTitle, str_ttl(hdr->title))))
	{
	  if ((fwrite(hdr, sizeof(HDR), 1, fpw) != 1))
	  {
	    fclose(fpw);
	    close(fdr);
	    goto contWhileOuter;
	  }
	  fsize++;
	}
	else
	{
	  /* ���ós�u��H */

	  cancel_post(hdr);
	  hdr_fpath(fold, fpath, hdr);
	  unlink(fold);

          /* �Q��峹���̫�@�g�� ���I���D 061104 guessi */
          btime_update(brd_bno(currboard));
	}
      }
      close(fdr);
      fclose(fpw);

      sprintf(fold, "%s.o", fpath);
      rename(fpath, fold);
      if (fsize)
	rename(fnew, fpath);
      else
  contWhileOuter:
	unlink(fnew);

    } while (++head < tail);


    /* �٭� currboard */
    strcpy(currboard, tmpboard);
    return XO_LOAD;
  }

  return XO_FOOT;
}
#endif	/* HAVE_TERMINATOR */


/* ----------------------------------------------------- */
/* �O�D�\�� : �ק�O�W					 */
/* ----------------------------------------------------- */


static int
post_brdtitle(xo)
  XO *xo;
{
  BRD *oldbrd, newbrd;

  oldbrd = bshm->bcache + currbno;
  memcpy(&newbrd, oldbrd, sizeof(BRD));

  /* itoc.����: ���I�s brd_title(bno) �N�i�H�F�A�S�t�A�Z�F�@�U�n�F :p */

  vget(b_lines, 0, "�ݪO�D�D�G", newbrd.title, BTLEN + 1, GCARRY);

  if (memcmp(&newbrd, oldbrd, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
  {
    memcpy(oldbrd, &newbrd, sizeof(BRD));
    rec_put(FN_BRD, &newbrd, sizeof(BRD), currbno, NULL);
    brd_classchange("gem/@/@"CLASS_INIFILE, oldbrd->brdname, &newbrd);
  }

  return XO_HEAD;
}


/* ----------------------------------------------------- */
/* �O�D�\�� : �ק�i�O�e��				 */
/* ----------------------------------------------------- */


static int
post_memo_edit(xo)
  XO *xo;
{
  int mode;
  char fpath[64];

  mode = vans("�i�O�e�� (D)�R�� (E)�ק� (Q)�����H[E] ");

  if (mode != 'q')
  {
    brd_fpath(fpath, currboard, fn_note);

    if (mode == 'd')
    {
      unlink(fpath);
    }

    if (vedit(fpath, 0))	/* Thor.981020: �`�N�Qtalk�����D */
      vmsg(msg_cancel);
  }
  return XO_HEAD;
}

/* ----------------------------------------------------- */
/* �O�D�\�� : �ק�o�����                               */
/* ----------------------------------------------------- */

static int
post_postlaw_edit(xo)       /* �O�D�۩w�峹�o����� */
  XO *xo;
{
  int mode;
  char fpath[64];
  
  mode = vans("�峹�o����� (D)�R�� (E)�ק� (Q)�����H[E] ");
  
  if (mode != 'q')
  {
    brd_fpath(fpath, currboard, FN_POSTLAW);
    
    if (mode == 'd')
    {
      unlink(fpath);
      return XO_FOOT;
    }
    
    vmsg("�Ъ`�N�G���e�ФŶW�L19��I");

    if (vedit(fpath, 0))      /* Thor.981020: �`�N�Qtalk�����D */
      vmsg(msg_cancel);

    return XO_HEAD;
  }

  return XO_HEAD;
}

#ifdef POST_PREFIX
/* ----------------------------------------------------- */
/* �O�D�\�� : �ק�o�����O                               */
/* ----------------------------------------------------- */


static int
post_prefix_edit(xo)
  XO *xo;
{
#define NUM_PREFIX      6

  int i;
  FILE *fp;
  char fpath[64], buf[10], prefix[NUM_PREFIX][10], *menu[NUM_PREFIX + 3];
  char *prefix_def[NUM_PREFIX] =   /* �w�]�����O */
  {
    "[���i]", "[���n]", "[����]", "[���]", "[����]", "[���]"
  };

  if (!(bbstate & STAT_BOARD))
   return XO_NONE;

  i = vans("���O (D)�R�� (E)�ק� (Q)�����H[E] ");

  if (i == 'q')
    return XO_FOOT;

  brd_fpath(fpath, currboard, "prefix");

  if (i == 'd')
  {
    unlink(fpath);
    return XO_FOOT;
  }

  i = 0;

  if (fp = fopen(fpath, "r"))
  {
    for (; i < NUM_PREFIX; i++)
    {
      if (fscanf(fp, "%10s", buf) != 1)
        break;
      sprintf(prefix[i], "%d.%s", i + 1, buf);
    }
    fclose(fp);
  }

  /* �񺡦� NUM_PREFIX �� */
  for (; i < NUM_PREFIX; i++)
    sprintf(prefix[i], "%d.%s", i + 1, prefix_def[i]);

  menu[0] = "10";
  for (i = 1; i <= NUM_PREFIX; i++)
    menu[i] = prefix[i - 1];
  menu[NUM_PREFIX + 1] = "0.���}";
  menu[NUM_PREFIX + 2] = NULL;

  do
  {
    /* �b popupmenu �̭��� ���� ���} */
    i = pans(3, 20, "�峹���O", menu) - '0';
    if (i >= 1 && i <= NUM_PREFIX)
    {
      strcpy(buf, prefix[i - 1] + 2);
      if (vget(b_lines, 0, "���O�G", buf, 7, GCARRY))
        strcpy(prefix[i - 1] + 2, buf);
    }
  } while (i);

  if (fp = fopen(fpath, "w"))
  {
    for (i = 0; i < NUM_PREFIX; i++)
      fprintf(fp, "%s ", prefix[i] + 2);
    fclose(fp);
  }

  return XO_FOOT;
}
#endif      /* POST_PREFIX */

/* ----------------------------------------------------- */
/* �O�D�\�� : �ݪO�ݩ�					 */
/* ----------------------------------------------------- */


#ifdef HAVE_SCORE
static int
post_battr_noscore(xo)
  XO *xo;
{
  BRD *oldbrd, newbrd;

  oldbrd = bshm->bcache + currbno;
  memcpy(&newbrd, oldbrd, sizeof(BRD));

  switch (vans("�}����� (1)���\\ (2)���\\ (Q)�����H[Q] "))
  {
  case '1':
    newbrd.battr &= ~BRD_NOSCORE;
    break;
  case '2':
    newbrd.battr |= BRD_NOSCORE;
    break;
  default:
    return XO_FOOT;
  }

  if (memcmp(&newbrd, oldbrd, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
  {
    memcpy(oldbrd, &newbrd, sizeof(BRD));
    rec_put(FN_BRD, &newbrd, sizeof(BRD), currbno, NULL);
  }

  return XO_HEAD;
}
#endif	/* HAVE_SCORE */


/* ----------------------------------------------------- */
/* �O�D�\�� : �ק�O�D�W��				 */
/* ----------------------------------------------------- */


static int
post_changeBM(xo)
  XO *xo;
{
  char buf[80], userid[IDLEN + 2], clog[256], reason[64], *blist;
  BRD *oldbrd, newbrd;
  ACCT acct;
  int BMlen, len;

  oldbrd = bshm->bcache + currbno;

  blist = oldbrd->BM;

  /* guessi.090614 �u�����O�D�P���ȤH���i�]�w�O�D�W�� */
  if ((is_bm(blist, cuser.userid) != 1) && !HAS_PERM(PERM_ALLBOARD))
    return XO_HEAD;

  memcpy(&newbrd, oldbrd, sizeof(BRD));

  move(3, 0);
  clrtobot();

  move(8, 0);
  prints("�ثe�O�D�� %s\n�п�J�s���O�D�W��A�Ϋ� [Return] ����", oldbrd->BM);

  strcpy(buf, oldbrd->BM);
  BMlen = strlen(buf);

  while (vget(10, 0, "�п�JID�W��ƪO�D�A�����Ы� Enter�A�M���Ҧ��ƪO�D�Х��u�L�v�G", userid, IDLEN + 1, DOECHO))
  {
    /* �M���ƪO�D �h�u�O�d�O�D�ۤv��ID */
    if (!strcmp(userid, "�L"))
    {
      strcpy(buf, cuser.userid);
      BMlen = strlen(buf);
    }
    else if (is_bm(buf, userid))	/* �R���¦����O�D */
    {
      len = strlen(userid);
      /* guessi.090614 �ˬd�O�_�����O�D */
      if (BMlen == len || is_bm(buf, userid) == 1)
      {
        vmsg("���i�H�N���O�D�q�W�椤����");
        continue;
      }
      else if (!str_cmp(buf + BMlen - len, userid) &&	/* �W��W�̫�@��AID �᭱���� '/' */
                buf[BMlen - len - 1] == '/')
      {
	buf[BMlen - len - 1] = '\0';			/* �R�� ID �Ϋe���� '/' */
	len++;
      }
      else						/* ID �᭱�|�� '/' */
      {
	str_lower(userid, userid);
	strcat(userid, "/");
	len++;
	blist = str_str(buf, userid);
	strcpy(blist, blist + len);
      }
      BMlen -= len;
    }
    else if (acct_load(&acct, userid) >= 0 && !is_bm(buf, userid))	/* ��J�s�O�D */
    {
      len = strlen(userid);	/* '/' + userid */
  
      if (BMlen)	/* guessi.090614 �Ybrd->BM������ */
      {
	len++;
        if (BMlen + len > BMLEN)
        {
          vmsg("�O�D�W��L���A�L�k�N�o ID �]���O�D");
          continue;
        }
        sprintf(buf + BMlen, "/%s", acct.userid);
        BMlen += len;
      }
      else		/* guessi.090614 ���Ⱦާ@�� brd->BM�i��O�Ū� */
      {
        strcpy(buf, acct.userid);
	BMlen = len;
      }
      acct_setperm(&acct, PERM_BM, 0);
    }
    else
      continue;

    move(8, 0);
    prints("�ثe�O�D�� %s", buf);
    clrtoeol();
  }
  strcpy(newbrd.BM, buf);

  if (memcmp(&newbrd, oldbrd, sizeof(BRD)) &&
      vget(b_lines, 0, "�п�J�z��: ", reason, 64, DOECHO) &&
      vans(msg_sure_ny) == 'y')
  {
    if (strlen(reason) < 5)
    {
      vmsg("��J�L�u�A�Ш���y�z�󴫪O�D���z��");
      return XO_HEAD;
    }

    sprintf(clog, "�ݪO�W��: %s\n�O�D����: %s\n�� �� ��: %s\n�z    ��: %s\n\n", oldbrd->brdname, oldbrd->BM, newbrd.BM, reason);
    f_cat(FN_RUN_CBM, clog);

    memcpy(oldbrd, &newbrd, sizeof(BRD));
    rec_put(FN_BRD, &newbrd, sizeof(BRD), currbno, NULL);

    sprintf(currBM, "�O�D�G%s", newbrd.BM);
    
    return XO_HEAD;	/* �n��ø���Y���O�D */
  }

  return XO_HEAD;
}



#ifdef HAVE_MODERATED_BOARD
/* ----------------------------------------------------- */
/* �O�D�\�� : �ݪO�v��					 */
/* ----------------------------------------------------- */

#if 0
static int
post_brdlevel(xo)
  XO *xo;
{
  BRD *oldbrd, newbrd;

  oldbrd = bshm->bcache + currbno;
  memcpy(&newbrd, oldbrd, sizeof(BRD));

  switch (vans("1)���}�ݪO 2)���K�ݪO 3)�n�ͬݪO�H[Q] "))
  {
  case '1':				/* ���}�ݪO */
    newbrd.readlevel = 0;
    newbrd.postlevel = PERM_POST;
    newbrd.battr &= ~(BRD_NOSTAT | BRD_NOVOTE);
    break;

  case '2':				/* ���K�ݪO */
    newbrd.readlevel = PERM_SYSOP;
    newbrd.postlevel = 0;
    newbrd.battr |= (BRD_NOSTAT | BRD_NOVOTE);
    break;

  case '3':				/* �n�ͬݪO */
    newbrd.readlevel = PERM_BOARD;
    newbrd.postlevel = 0;
    newbrd.battr |= (BRD_NOSTAT | BRD_NOVOTE);
    break;

  default:
    return XO_HEAD;
  }

  if (memcmp(&newbrd, oldbrd, sizeof(BRD)) && vans(msg_sure_ny) == 'y')
  {
    memcpy(oldbrd, &newbrd, sizeof(BRD));
    rec_put(FN_BRD, &newbrd, sizeof(BRD), currbno, NULL);
  }

  return XO_HEAD;
}
#endif	/* HAVE_MODERATED_BOARD */

#endif // �p�����}�� 

#ifdef HAVE_MODERATED_BOARD
/* ----------------------------------------------------- */
/* �O�ͦW��Gmoderated board				 */
/* ----------------------------------------------------- */


static void
bpal_cache(fpath)
  char *fpath;
{
  BPAL *bpal;

  bpal = bshm->pcache + currbno;
  bpal->pal_max = image_pal(fpath, bpal->pal_spool);
}


extern XZ xz[];


static int
XoBM(xo)
  XO *xo;
{
  XO *xt;
  char fpath[64];

  brd_fpath(fpath, currboard, fn_pal);
  xz[XZ_PAL - XO_ZONE].xo = xt = xo_new(fpath);
  xt->key = PALTYPE_BPAL;
  xover(XZ_PAL);		/* Thor: �ixover�e, pal_xo �@�w�n ready */

  /* build userno image to speed up, maybe upgreade to shm */

  bpal_cache(fpath);

  free(xt);

  return XO_INIT;
}
#endif	/* HAVE_MODERATED_BOARD */


/* ----------------------------------------------------- */
/* �O�D���						 */
/* ----------------------------------------------------- */


int
post_manage(xo)
  XO *xo;
{
#ifdef POPUP_ANSWER
  char *menu[] = 
  {
    "BQ",
    "BTitle  �ק�ݪO�D�D",
    "WMemo   �s��i�O�e��",
    "PostLaw �s��o�����",
    "Manager �W��ƪO�D",

#ifdef POST_PREFIX
    "CPrefix �峹���O�s��",
#endif
 
#  ifdef HAVE_SCORE
    "Score   �]�w�i�_����",
#  endif


#  ifdef HAVE_MODERATED_BOARD
/*  �F��p�������Ѧ��A��
    "Level   ���}/�n��/���K",
*/
    "OPal    �O�ͦW��",
#  endif
    NULL
  };
#else
  char *menu = "�� �O�D��� (B)�D�D (W)�i�O (P)���� (M)�ƪO"

#ifdef POST_PREFIX
   " (C)���O"
#endif

#  ifdef HAVE_SCORE
   " (S)����"
#  endif

#  ifdef HAVE_MODERATED_BOARD
/*
 * " (L)�v��"
 */
  " (O)�O��"
#  endif
    "�H[Q] ";
#endif

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  grayout(GRAYOUT_DARK);

#ifdef POPUP_ANSWER
  switch (pans(3, 20, "�O�D���", menu))
#else
  switch (vans(menu))
#endif
  {
  case 'b':
    return post_brdtitle(xo);

  case 'w':
    return post_memo_edit(xo);

#ifdef POST_PREFIX
  case 'c':
    return post_prefix_edit(xo);
#endif

  case 'm':
    return post_changeBM(xo);

  case 'p':
    return post_postlaw_edit(xo);

#ifdef HAVE_SCORE
  case 's':
    return post_battr_noscore(xo);
#endif

#ifdef HAVE_MODERATED_BOARD
/*
  case 'l':
    return post_brdlevel(xo);
*/
  case 'o':
    return XoBM(xo);
#endif
  }

  return XO_HEAD;
}
