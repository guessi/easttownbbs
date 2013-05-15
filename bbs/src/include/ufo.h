/*-------------------------------------------------------*/
/* ufo.h	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : User Flag Option				 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


#ifndef	_UFO_H_
#define	_UFO_H_


/* ----------------------------------------------------- */
/* User Flag Option : flags in ACCT.ufo			 */
/* ----------------------------------------------------- */


#define BFLAG(n)	(1 << n)	/* 32 bit-wise flag */


#define UFO_NOUSE00	BFLAG(0)	/* �S�Ψ� */
#define UFO_MOVIE	BFLAG(1)	/* �ʺA�ݪO��� */
#define UFO_BRDPOST	BFLAG(2)	/* 1: �ݪO�C����ܽg��  0: �ݪO�C����ܸ��X itoc.010912: ���s�峹�Ҧ� */
#define UFO_BRDNAME	BFLAG(3)	/* itoc.010413: �ݪO�C��� 1:brdname 0:class+title �Ƨ� */
#define UFO_NOCOLOR	BFLAG(4)	/* �C����������C�� */
#define UFO_VEDIT	BFLAG(5)	/* ²�ƽs�边 */
#define UFO_MOTD	BFLAG(6)	/* ²�ƶi/�����e�� */

#define UFO_PAGER	BFLAG(7)	/* �����I�s�� */
#define UFO_RCVER	BFLAG(8)	/* itoc.010716: �ڦ��s�� */
#define UFO_QUIET	BFLAG(9)	/* ���f�b�H�ҡA�ӵL������ */
#define UFO_PAL		BFLAG(10)	/* �ϥΪ̦W��u��ܦn�� */
#define UFO_ALOHA	BFLAG(11)	/* �����W���q�� */
#define UFO_NOALOHA	BFLAG(12)	/* itoc.010716: �W�����q��/��M */

#define UFO_BMWDISPLAY	BFLAG(13)	/* itoc.010315: ���y�^�U���� */
#define UFO_NWLOG       BFLAG(14)	/* lkchu.990510: ���s��ܬ��� */
#define UFO_NTLOG       BFLAG(15)	/* lkchu.990510: ���s��Ѭ��� */

#define UFO_MAILBAD	BFLAG(16)	/* �ڦ��a�H�ӫH */
#define UFO_SHOWSIGN	BFLAG(17)	/* itoc.000319: �s�ɫe���ñ�W�� */

#define UFO_NOUSE18	BFLAG(18)
#define UFO_JUMPBRD	BFLAG(19)	/* itoc.020122: �۰ʸ��h�U�@�ӥ�Ū�ݪO */
#define UFO_HIDFROM	BFLAG(20)	/* guessi.060327 ���èӷ� */
#define UFO_NOINCOME	BFLAG(21)	/* �ڦ��~���H�� */

#define UFO_NOUSE22	BFLAG(22)
#define UFO_READMD	BFLAG(23)	/* guessi.060825 �峹��Ū�P�_ */
/***/
#define UFO_CLOAK	BFLAG(24)	/* 1: �i�J���� */
#define UFO_NOUSE25	BFLAG(25)
#define UFO_NOUSE26	BFLAG(26) 
#define UFO_NOUSE27	BFLAG(27)
#define UFO_NOUSE28	BFLAG(28)
#define UFO_NOUSE29	BFLAG(29)
#define UFO_SUPERCLOAK	BFLAG(30)	/* 1: �i�J���� */
#define UFO_ACL		BFLAG(31)	/* 1: �ϥ� ACL */

/* �s���U�b���Bguest ���w�] ufo */

#define UFO_DEFAULT_NEW		(UFO_MOVIE | UFO_MOTD | UFO_NOCOLOR |  UFO_BMWDISPLAY | UFO_SHOWSIGN)
#define UFO_DEFAULT_GUEST	(UFO_MOVIE | UFO_MOTD | UFO_NOCOLOR | UFO_QUIET | UFO_NOALOHA | UFO_NWLOG | UFO_NTLOG)


/* ----------------------------------------------------- */
/* Status : flags in UTMP.status			 */
/* ----------------------------------------------------- */


#define STATUS_BIFF	BFLAG(0)	/* ���s�H�� */
#define STATUS_REJECT	BFLAG(1)	/* true if reject any body */
#define STATUS_BIRTHDAY	BFLAG(2)	/* ���ѥͤ� */
#define STATUS_COINLOCK	BFLAG(3)	/* ������w */
#define STATUS_DATALOCK	BFLAG(4)	/* �����w */
#define STATUS_MQUOTA	BFLAG(5)	/* �H�c�����L�����H�� */
#define STATUS_MAILOVER	BFLAG(6)	/* �H�c�L�h�H�� */
#define STATUS_MGEMOVER	BFLAG(7)	/* �ӤH��ذϹL�h */
#define STATUS_EDITHELP	BFLAG(8)	/* �b edit �ɶi�J help */
#define STATUS_PALDIRTY BFLAG(9)	/* ���H�b�L���B�ͦW��s�W�β����F�� */


#define	HAS_STATUS(x)	(cutmp->status&(x))


/* ----------------------------------------------------- */
/* �U�زߺD������N�q					 */
/* ----------------------------------------------------- */


/* itoc.000320: �W��حn��� NUMUFOS_* �j�p, �]�O�ѤF�� STR_UFO */

#define NUMUFOS		32
#define NUMUFOS_GUEST	5	/* guest �i�H�Ϋe 5 �� (0~4) */
#define NUMUFOS_USER	24	/* �@��ϥΪ� �i�H�Ϋe 24 �� (0~23) */

#define STR_UFO		"-dpsnemPBQFANbwtMS-Jfi-rC-----Sa"	/* itoc: �s�W�ߺD���ɭԧO�ѤF��o�̰� */
                      /* 01234567890123456789012345678901 */

#ifdef _ADMIN_C_

char *ufo_tbl[NUMUFOS] =
{
  "�O�d",				/* UFO_NOUSE */
  "�ʺA�ݪO���    (�}��/����)",	/* UFO_MOVIE */
  "�ݪO�C�����    (�峹/�s��)",	/* UFO_BRDPOST */
  "�ݪO�C��Ƨ�    (�r��/����)",	/* UFO_BRDNAME */	/* itoc.010413: �ݪO�̷Ӧr��/�����Ƨ� */
  "�C�����C��    (�h��/���)",	/* UFO_NOCOLOR */
  "�峹�s��Ҧ�    (²��/����)",	/* UFO_VEDIT */
  "�i�X���x�e��    (²��/����)",	/* UFO_MOTD */
  "�����I�s����    (�n��/�Ҧ�)",	/* UFO_PAGER */

#ifdef HAVE_NOBROAD
  "�s���ѽu�}��    (�ڦ�/����)",	/* UFO_RCVER */
#else
  "�O�d",
#endif

  "�����@�U����    (�w�R/����)",	/* UFO_QUITE */
  "�W��ƦC���    (�n��/����)",	/* UFO_PAL */

#ifdef HAVE_ALOHA
  "�����W���q��    (�q��/����)",	/* UFO_ALOHA */
#else
  "�O�d",
#endif

#ifdef HAVE_NOALOHA
  "�W���o�e�q��    (���e/�q��)",	/* UFO_NOALOHA */
#else
  "�O�d",
#endif

#ifdef BMW_DISPLAY
  "���y�^�U����    (����/�W��)",	/* UFO_BMWDISPLAY */
#else
  "�O�d",
#endif

  "�x�s���y����    (�R��/�O�d)",	/* UFO_NWLOG */
  "�O�d��Ѭ���    (�R��/�O�d)",	/* UFO_NTLOG */
  "�ڦ��a�H�ӫH    (�ڦ�/����)",	/* UFO_MAILBAD */
  "ñ�W�ɨϥ�      (�}��/����)",	/* UFO_SHOWSIGN */
  "�O�d",				/* UFO_NOUSE18 */

#ifdef AUTO_JUMPBRD
  "���h��Ū�ݪO    (���h/����)",	/* UFO_JUMPBRD */
#else
  "�O�d",
#endif

  "���äW���ӷ�    (����/���)",	/* UFO_HIDFROM */
  "�ڦ����~�H��    (�ڦ�/����)",	/* UFO_NOINCOME */
  "�O�d",                       	/* UFO_NEWARTICLE */
  "�峹�ܰʴ���    (����/�}��)",	/* UFO_READMD */
  "�����N          (����/�{��)",	/* UFO_CLOAK */

  "�O�d",				/* UFO_NOUSE25 */
  "�O�d",				/* UFO_NOUSE26 */
  "�O�d",                               /* UFO_NOUSE27 */
  "�O�d",                               /* UFO_NOUSE28 */  
  "�O�d",                               /* UFO_NOUSE29 */
 
#ifdef HAVE_SUPERCLOAK
  "�W�������N      (����/�{��)",	/* UFO_SUPERCLOAK */
#else
  "�O�d",
#endif
 
  "�����W���ӷ�    (����/���N)"         /* UFO_ACL */
  
};
#endif

#endif				/* _UFO_H_ */
