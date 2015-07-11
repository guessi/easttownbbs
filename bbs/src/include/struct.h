/*-------------------------------------------------------*/
/* struct.h	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : all definitions about data structure	 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


#ifndef _STRUCT_H_
#define _STRUCT_H_


/* screen control */

#define STRLEN		80	/* Length of most string data */
#define ANSILINELEN	500	/* Maximum Screen width in chars�A����W�L 1023 */

#define SCR_WIDTH	79	/* edit/talk/camera screen width */

#define TAB_STOP	4	/* �� TAB �����X��ť� (�n�O 2 ������) */
#define TAB_WIDTH	(TAB_STOP - 1)

#define T_LINES		50	/* maximum total lines */
#define T_COLS		120	/* maximum total columns�A�n�� ANSILINELEN �p */

/* board structure */

#define BNLEN		12	/* Length of board name */
#define BCLEN		4	/* Length of board class */
#define BTLEN		43	/* Length of board title */
#define BMLEN		36	/* Length of board managers */

/* file structure */

#define TTLEN		72	/* Length of title */
#define FNLEN		28	/* Length of filename  */

/* user structure */
/* DES �s�X�k�A���� 8 �Ӧr�����ת��r��A���ͪ���Ȭ� 13 �줸�� */

#define IDLEN		12	/* Length of user id */
#define PASSLEN		13	/* Length of encrypted passwd field */
#define PSWDLEN		8	/* Length of passwd (���[�K������) */
#define RNLEN		19	/* Length of real name */
#define UNLEN		23	/* Length of user name */

#define FLLEN		4	/* Length of user feeling */

typedef char const *STRING;
typedef unsigned char uschar;	/* length = 1 */
typedef unsigned int usint;	/* length = 4 */
typedef struct UTMP UTMP;


/* ----------------------------------------------------- */
/* �ϥΪ̱b�� .ACCT struct : 512 bytes			 */
/* ----------------------------------------------------- */


typedef struct
{
  int userno;			/* unique positive code */

  char userid[IDLEN + 1];	/* ID */
  char passwd[PASSLEN + 1];	/* �K�X */
  char realname[RNLEN + 1];	/* �u��m�W */
  char username[UNLEN + 1];	/* �ʺ� */
  char feeling[FLLEN + 1];	/* �߱� */

  usint userlevel;		/* �v�� */
  usint ufo;			/* habit */
  uschar signature;		/* �w�]ñ�W�� */

  char year;			/* �ͤ�(����~) */
  char month;			/* �ͤ�(��) */
  char day;			/* �ͤ�(��) */
  char sex;			/* �ʧO 0:���� �_��:�k�� ����:�k�� */
  int money;			/* �ȹ� */
  int gold;			/* ���� */

  int numlogins;		/* �W������ */
  int numposts;			/* �o���� */
  int numemails;		/* �H�o Inetrnet E-mail ���� */

  time_t firstlogin;		/* �Ĥ@���W���ɶ� */
  time_t lastlogin;		/* �W�@���W���ɶ� */
  time_t tcheck;		/* �W�� check �H�c/�B�ͦW�檺�ɶ� */
  time_t tvalid;		/* �Y���v�A���v�������ɶ��F
                                   �Y�����v�B�q�L�{�ҡA�q�L�{�Ҫ��ɶ��F
                                   �Y�����v�B���q�L�{�ҡA�{�Ҩ窺 time-seed */
  time_t staytime;


  char lasthost[30];		/* �W���n�J�ӷ� */
  char email[60];		/* �ثe�n�O���q�l�H�c */
}      ACCT;


typedef struct			/* 16 bytes */
{
  time_t uptime;
  char userid[IDLEN];
}      SCHEMA;


#ifdef HAVE_REGISTER_FORM

/* itoc.041025: RFROM �����X�{�M ACCT �@�˦��ȥi�ण�P�����
   RFORM �M ACCT �ߤG�ۦP�����O userno�Buserid */

typedef struct	/* ���U��� (Register From) 256 bytes */
{
  int userno;
  time_t rtime;
  char userid[IDLEN + 1];
  char agent[IDLEN + 1];
  char year[3];
  char month[3];
  char day[3];
  char nouse[11];
  char career[50];
  char address[60];
  char phone[20];
  char reply[72];
}      RFORM;

#endif


#include "hdr.h"


/* ----------------------------------------------------- */
/* control of board vote : 256 bytes			 */
/* ----------------------------------------------------- */


/* itoc.041101.����: VCH �M HDR �� xname ��m�B���׭n�ǰt */

typedef struct VoteControlHeader
{
  time_t chrono;		/* �벼�}��ɶ� */	/* Thor: �� key �ӥB match HDR chrono */
  time_t bstamp;		/* �ݪO���ѥN�X */	/* Thor: �� key */
  time_t vclose;		/* �벼�����ɶ� */

  char xname[32];		/* �D�ɦW */		/* Thor: match HDR �� xname */
  char date[9];			/* �}�l��� */		/* Thor: match HDR �� date */
  char cdate[9];		/* ������� */		/* Thor: �u����ܡA������� */  
  char owner[IDLEN + 1];	/* �|��H */
  char title[TTLEN + 1];	/* �벼�D�D */

  char vgamble;			/* �O�_����L        '$':��L  ' ':�@��벼 */
  char vsort;			/* �}�����G�O�_�Ƨ�  's':�Ƨ�  ' ':���Ƨ� */
  char vpercent;		/* �O�_��ܦʤ����  '%':�ʤ�  ' ':�@�� */
  char vprivate;		/* �O�_���p�H�벼    ')':�p�H  ' ':���} */

  int maxblt;			/* �C�H�i��X�� */
  int price;			/* �C�i�䲼����� */

  char nouse[96];
}      VCH;


typedef char vitem_t[32];	/* �벼�ﶵ */


typedef struct
{
  char userid[IDLEN + 1];
  char numvotes;		/* ��X�i */
  usint choice;
}      VLOG;


/* filepath : brd/<board>/.VCH, brd/<board>/@/... */


/* ----------------------------------------------------- */
/* Mail-Queue struct : 256 bytes			 */
/* ----------------------------------------------------- */


typedef struct
{
  time_t mailtime;		/* �H�H�ɶ� */
  char method;
  char sender[IDLEN + 1];
  char username[UNLEN + 1];
  char subject[TTLEN + 1];
  char rcpt[60];
  char filepath[77];
  char *niamod;			/* reverse domain */
}      MailQueue;


#define MQ_JUSTIFY	0x01	/* �����{�ҫH�� */
#define MQ_ATTACH	0x02	/* �� attachment ���H�� */


/* ----------------------------------------------------- */
/* PAL : friend struct : 64 bytes			 */
/* ----------------------------------------------------- */


typedef struct
{
  char userid[IDLEN + 1];
  char ftype;
  char ship[46];
  int userno;
}      PAL;

#define	PAL_BAD		0x02	/* �n�� vs �a�H */


/* ----------------------------------------------------- */
/* structure for call-in message : 100 bytes		 */
/* ----------------------------------------------------- */


typedef struct
{
  time_t btime;
  UTMP *caller;			/* who call-in me ? */
  int sender;			/* calling userno */
  int recver;			/* called userno */
  char userid[IDLEN + 1 + 2];	/* itoc.010529: �O�d 2 byte ���s���Ÿ� > */
  char msg[69];			/* ���y */
}      BMW;			/* bbs message write */


#ifdef LOGIN_NOTIFY

/* ----------------------------------------------------- */
/* BENZ : �t�Ψ�M�W�� : 20 bytes			 */
/* ----------------------------------------------------- */


typedef struct
{
  int userno;
  char userid[IDLEN + 1];
}       BENZ;

#endif	/* LOGIN_NOTIFY */


#ifdef HAVE_ALOHA

/* ----------------------------------------------------- */
/* ALOHA : �W���q���W�� : 20 bytes			 */
/* ----------------------------------------------------- */


typedef struct
{
  char userid[IDLEN + 1];
  int userno;
}      ALOHA;


/* ----------------------------------------------------- */
/* FRIENZ : �W���q���W�� : 100 bytes			 */
/* ----------------------------------------------------- */


/* itoc.041011.����: �ڥ��S���n�γo��j */

typedef struct
{
  int nouse1;
  int nouse2;
  int nouse3;
  int userno;
  char userid[IDLEN + 1];
  char nouse4[71];
}      FRIENZ;

#endif	/* HAVE_ALOHA */


/* ----------------------------------------------------- */
/* PAYCHECK : �䲼 : 32 bytes				 */
/* ----------------------------------------------------- */


typedef struct
{
  time_t tissue;		/* �o�䲼�ɶ� */
  int money;
  int gold;
  char reason[20];		/* "[�ʧ@] brdname/userid"�A���] BNLEN�BIDLEN ���W�L 12 */
}      PAYCHECK;


/* ----------------------------------------------------- */
/* Structure used in UTMP file : 128 bytes		 */
/* ----------------------------------------------------- */


struct UTMP
{
  pid_t pid;			/* process ID */
  int userno;			/* user number */

  int mode;			/* bbsmode */
  usint userlevel;		/* the save as ACCT.userlevel */
  usint ufo;			/* the same as ACCT.ufo */
  usint status;			/* status */

  time_t idle_time;		/* active time for last event */
  u_long in_addr;		/* Internet address */
  int sockport;			/* socket port for talk */
  UTMP *talker;			/* who talk-to me ? */

  BMW *mslot[BMW_PER_USER];

  char userid[IDLEN + 1];	/* user's ID */
  char mateid[IDLEN + 1];	/* partner's ID */
  char username[UNLEN + 1];	/* user's nickname */
  char feeling[FLLEN + 1];
  char from[34];		/* remote host */
#ifdef HAVE_BRDMATE
  char reading[BNLEN + 1];	/* reading board */
#endif

  int pal_max;			/* ���X�ӪB�� */
  int pal_spool[PAL_MAX];	/* �Ҧ��B�ͪ� userno */

#ifdef BMW_COUNT
  int bmw_count;		/* �O�����F�X�Ӥ��y */
#endif
  char nouse[20];
};


/* ----------------------------------------------------- */
/* BOARDS struct : 128 bytes				 */
/* ----------------------------------------------------- */


typedef struct BoardHeader
{
  char brdname[BNLEN + 1];	/* board name */
  char class[BCLEN + 1];
  char title[BTLEN + 1];
  char BM[BMLEN + 1];		/* BMs' uid, token '/' */

  char bvote;			/* 0:�L�벼 -1:����L(�i�঳�벼) 1:���벼 */

  time_t bstamp;		/* �إ߬ݪO���ɶ�, unique */
  usint readlevel;		/* �\Ū�峹���v�� */
  usint postlevel;		/* �o��峹���v�� */
  usint battr;			/* �ݪO�ݩ� */
  time_t btime;			/* -1:bpost/blast �ݭn��s */
  int bpost;			/* �@���X�g post */
  time_t blast;			/* �̫�@�g post ���ɶ� */
}           BRD;


typedef struct
{
  int pal_max;			/* ���X�ӪO�� */
  int pal_spool[PAL_MAX];	/* �Ҧ��O�ͪ� userno */
}	BPAL;


#ifdef HAVE_CREDIT

typedef struct
{
  time_t stamp;         /* ��ƫإߤ���ɶ� */
  int type;             /* ������O */
  time_t chrono;        /* �ϥΪ̦ۭq����Ƥ���ɶ� */
  char flag;            /* ��X/���J */
  int money;            /* ���B */
  char useway;          /* ����(������|��) */
  char desc[75];        /* ���� */
  char desc_way[5];     /* �����ۭq */
}      CREDIT;

#define CREDIT_FOLDER   0x01    /* ��Ƨ� */
#define CREDIT_DATA     0x02    /* �@���� */
#define CREDIT_OUT      0x1     /* ��X */
#define CREDIT_IN       0x2     /* ���J */
#define CREDIT_OTHER    0       /* ��L */
#define CREDIT_EAT      1       /* �� */
#define CREDIT_WEAR     2       /* �� */
#define CREDIT_LIVE     3       /* �� */
#define CREDIT_MOVE     4       /* �� */
#define CREDIT_EDU      5       /* �| */
#define CREDIT_PLAY     6       /* �� */
#define CREDIT_CUSTOM   7       /* �ۭq */

#endif


/* ----------------------------------------------------- */
/* Class image						 */
/* ----------------------------------------------------- */


#define CLASS_INIFILE		"Class"

/* itoc.010413: �� class.img �����G�� */
#define CLASS_IMGFILE_NAME	"run/classname.img"
#define CLASS_IMGFILE_TITLE	"run/classtitle.img"


#define CH_MAX		100	/* �̤j�����ƥ� */
#define	CH_END		-1
#define	CH_TTLEN	64


typedef	struct
{
  int count;
  char title[CH_TTLEN];
  short chno[0];
}	ClassHeader;


#ifdef MY_FAVORITE

/* ----------------------------------------------------- */
/* favor.c ���B�Ϊ���Ƶ��c                              */
/* ----------------------------------------------------- */


typedef struct MF
{
  time_t chrono;		/* �إ߮ɶ� */
  int mftype;			/* type */
  char xname[BNLEN + 1];	/* �O�W���ɦW */
  char class[BCLEN + 1];	/* ���� */
  char title[BTLEN + 1];	/* �D�D */
}	MF;

#define	MF_MARK		0x01	/* �Q mark �_�Ӫ� */
#define	MF_BOARD    	0x02	/* �ݪO���| */
#define	MF_FOLDER   	0x04	/* ���v */
#define	MF_GEM      	0x08	/* ��ذϱ��| */
#define MF_LINE		0x10	/* ���j�u */
#define MF_CLASS	0x20	/* �����s�� */

#endif  /* MY_FAVORITE */



/* ----------------------------------------------------- */
/* wtoeat.c                                              */
/* ----------------------------------------------------- */

typedef struct WhereToEatHeader
{
  char class[BCLEN + 1];
  char title[BTLEN + 1];
  char owner[IDLEN + 1];
  char xname[32];
  char date[9];
  char address[64];
  char phone[16];
  char during[32];
  char description[64];
  time_t btime;
  usint mode;
} WTE;

#define WTE_MARKED    0x00001
#define WTE_NOUSE1    0x00002
#define WTE_NOUSE2    0x00004
#define WTE_NOUSE3    0x00008


#ifdef HAVE_COSIGN

/* ----------------------------------------------------- */
/* newbrd.c ���B�Ϊ���Ƶ��c                             */
/* ----------------------------------------------------- */


typedef struct NewBoardHeader
{
  char brdname[BNLEN + 1];
  char class[BCLEN + 1];
  char title[BTLEN + 1];
  time_t btime;
  time_t etime;
  char xname[32];
  char owner[IDLEN + 1];
  char date[9];
  usint mode;
  int total;
}	NBRD;


#define NBRD_FINISH	0x00001	/* �w���� */
#define NBRD_END	0x00002	/* �s�p���� */
#define NBRD_START	0x00004	/* �s�p�}�l */
#define NBRD_ANONYMOUS	0x00100	/* �ΦW */
#define NBRD_NEWBOARD	0x10000	/* �s�O�s�p */
#define NBRD_OTHER	0x20000	/* ��L�s�p */
#define NBRD_DELBOARD	0x40000 /* �o���ݪO */

#endif	/* HAVE_COSIGN */


#ifdef LOG_SONG_USIES

/* ----------------------------------------------------- */
/* SONG log ���B�Ϊ���Ƶ��c                             */
/* ----------------------------------------------------- */

typedef struct SONGDATA
{
  time_t chrono;
  int count;		/* �Q�I���� */
  char title[80];
}	SONGDATA;

#endif	/* LOG_SONG_USIES */


/* ----------------------------------------------------- */
/* cache.c ���B�Ϊ���Ƶ��c				 */
/* ----------------------------------------------------- */


typedef struct
{
  int shot[MOVIE_MAX];	/* Thor.980805: �X�z�d�� 0..MOVIE_MAX - 1 */
  char film[MOVIE_SIZE];
  char today[16];
} FCACHE;


#define	FILM_SIZ	4000	/* max size for each film */


#define FILM_OPENING0	0	/* �}�Y�e��(��) */
#define FILM_OPENING1	1	/* �}�Y�e��(��) */
#define FILM_OPENING2	2	/* �}�Y�e��(��) */
#define FILM_BELOGIN	3	/* �_�l�e��(�u��) */
#define FILM_GOODBYE	4	/* �A���e�� */
#define FILM_NOTIFY	5	/* �|���q�L�{�ҳq�� */
#define FILM_MQUOTA	6	/* �H��W�L�O�s�����q�� */
#define FILM_MAILOVER	7	/* �H��ʼƹL�h�q�� */
#define FILM_MGEMOVER	8	/* �ӤH��ذϹL�h�q�� */
#define FILM_BIRTHDAY	9	/* �ͤ��Ѫ��W���e�� */
#define FILM_APPLY	10	/* ���U���ܵe�� */
#define FILM_JUSTIFY	11	/* �����{�Ҫ���k */
#define FILM_REREG	12	/* ���s�{�һ��� */
#define FILM_EMAIL	13	/* �l��H�c�{�һ��� */
#define FILM_NEWUSER	14	/* �s��W������ */
#define FILM_TRYOUT	15	/* �K�X���~ */
#define FILM_POST	16	/* �峹�o����� */
#define FILM_MOVIE	17	/* �ʺA�ݪO FILM_MOVIE �n��b�̫᭱ */


typedef struct
{
  UTMP uslot[MAXACTIVE];	/* UTMP slots */
  usint count;			/* number of active session */
  usint offset;			/* offset for last active UTMP */

  double sysload[3];
  int avgload;

  BMW *mbase;			/* sequential pointer for BMW */
  BMW mpool[BMW_MAX];
} UCACHE;


typedef struct
{
  BRD bcache[MAXBOARD];
  BPAL pcache[MAXBOARD];
  int mantime[MAXBOARD];	/* �U�O�ثe�����h�֤H�b�\Ū */
  int number;			/* �����ݪO���ƥ� */
  int numberOld;		/* ��}���ɬݪO���ƥ� */
  int min_chn;			/* �O���`�@���X�Ӥ��� */
  time_t uptime;
} BCACHE;


/* ----------------------------------------------------- */
/* visio.c ���B�Ϊ���Ƶ��c				 */
/* ----------------------------------------------------- */


/* Screen Line buffer modes */


#define SL_MODIFIED	(1)	/* if line has been modifed, screen output */
#define SL_STANDOUT	(2)	/* if this line contains standout code */
#define SL_ANSICODE	(4)	/* if this line contains ANSI code */


typedef struct screenline
{
  int oldlen;			/* previous line length */
  int len;			/* current length of line */
  int width;			/* padding length of ANSI codes */
  int smod;			/* start of modified data */
  int emod;			/* end of modified data */
  int sso;			/* start of standout data */
  int eso;			/* end of standout data */
  uschar mode;			/* status of line, as far as update */	/* �ѩ� SL_* �� mode ���W�L�K�ӡA�G�� uschar �Y�i */
  uschar data[ANSILINELEN];
}          screenline;


typedef struct LinkList
{
  struct LinkList *next;
  char data[0];
}        LinkList;


/* ----------------------------------------------------- */
/* xover.c ���B�Ϊ���Ƶ��c				 */
/* ----------------------------------------------------- */


typedef struct OverView
{
  int pos;			/* current position */
  int top;			/* top */
  int max;			/* max */
  int key;			/* key */
  char *xyz;			/* staff */
  struct OverView *nxt;		/* next */
  char dir[0];			/* data path */
}        XO;


typedef struct
{
  int key;
  int (*func) ();
}      KeyFunc;


typedef struct
{
  XO *xo;
  KeyFunc *cb;
  int mode;
  char *feeter;
} XZ;


typedef struct
{
  time_t chrono;
  int recno;
}      TagItem;


/* ----------------------------------------------------- */
/* poststat.c ���B�Ϊ���Ƶ��c				 */
/* ----------------------------------------------------- */


typedef struct
{
  char author[IDLEN + 1];
  char board[BNLEN + 1];
  char title[66];
  time_t date;		/* last post's date */
  int number;		/* post number */
} POSTLOG;


#ifdef MODE_STAT

/* ----------------------------------------------------- */
/* modestat.c ���B�Ϊ���Ƶ��c				 */
/* ----------------------------------------------------- */

typedef struct
{
  time_t logtime;
  time_t used_time[M_MAX + 1];	/* itoc.010901: depend on mode.h */
} UMODELOG;


typedef struct
{
  time_t logtime;
  time_t used_time[M_MAX + 1];	/* itoc.010901: depend on mode.h */
  int count[30];
  int usercount;
} MODELOG;

#endif	/* MODE_STAT */


/* ----------------------------------------------------- */
/* innbbsd ���B�Ϊ���Ƶ��c				 */
/* ----------------------------------------------------- */


typedef struct
{
  time_t chrono;	/* >=0:stamp -1:cancel */
  char board[BNLEN + 1];

  /* �H�U��쪺�j�p�P HDR �ۦP */
  char xname[32];
  char owner[80];
  char nick[49];
  char title[TTLEN + 1];
} bntp_t;


#define INN_USEIHAVE	0x0001
#define INN_USEPOST	0x0002
#define INN_FEEDED	0x0010

typedef struct
{
  char name[13];	/* �ӯ��� short-name */
  char host[83];	/* �ӯ��� host */
  int port;		/* �ӯ��� port */
  usint xmode;		/* �ӯ��� xmode */
  char blank[20];	/* �O�d */
  int feedfd;		/* bbslink.c �ϥ� */
} nodelist_t;


#define INN_NOINCOME	0x0001
#define INN_ERROR	0x0004

typedef struct
{
  char path[13];	/* �Ӹs�թҹ��઺���x */
  char newsgroup[83];	/* �Ӹs�ժ��W�� */
  char board[BNLEN + 1];/* �Ӹs�թҹ������ݪO */
  char charset[11];	/* �Ӹs�ժ��r�� */
  usint xmode;		/* �Ӹs�ժ� xmode */
  int high;		/* �ثe���Ӹs�ժ����@�g */
} newsfeeds_t;


typedef struct
{
  char issuer[128];	/* NCM message ���o�H�H */
  char type[64];	/* NCM message �������W�� */
  int perm;		/* ���\�� NCM message �R�H (1:�} 0:��) */
  char blank[60];	/* �O�d */
} ncmperm_t;


#define INN_SPAMADDR	0x0001
#define INN_SPAMNICK	0x0002
#define INN_SPAMSUBJECT	0x0010
#define INN_SPAMPATH	0x0020
#define INN_SPAMMSGID	0x0040
#define INN_SPAMBODY	0x0100
#define INN_SPAMSITE	0x0200
#define INN_SPAMPOSTHOST 0x0400

typedef struct
{
  char detail[80];	/* �ӳW�h�� ���e */
  usint xmode;		/* �ӳW�h�� xmode */
  char board[BNLEN + 1];/* �ӳW�h�A�Ϊ��ݪO */
  char path[13];	/* �ӳW�h�A�Ϊ����x */
  char blank[18];	/* �O�d */
} spamrule_t;

#endif				/* _STRUCT_H_ */
