/*-------------------------------------------------------*/
/* util/sob.h 	                                         */
/*-------------------------------------------------------*/
/* target : SOB �� Maple 3.02 �ഫ			 */
/* create : 02/10/26					 */
/* author : ernie@micro8.ee.nthu.edu.tw                  */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#if 0

  1. �]�w OLD_BBSHOME�BFN_PASSWD�BFN_BOARD
  2. �ק�Ҧ��� old struct

  3. �����b brd �৹�~�i�H�ഫ gem
  4. �����b usr �� brd ���৹�~�i�H�ഫ pal
  5. ��ĳ�ഫ���Ǭ� usr -> brd -> gem ->pal

#endif


#include "bbs.h"


#define OLD_BBSHOME     "/home/oldbbs"		/* SOB */
#define FN_PASSWD	"/home/oldbbs/.PASSWDS"	/* SOB */
#define FN_BOARD        "/home/oldbbs/.BOARDS"	/* SOB */


#undef	HAVE_PERSONAL_GEM			/* SOB �O�S���ӤH��ذϪ� */


/* ----------------------------------------------------- */
/* old .PASSWDS struct : 256 bytes                       */
/* ----------------------------------------------------- */

struct userec
{
  char userid[13];               // 13 bytes
  char passwd[14];                 // 14 bytes
  usint uflag;                          //  1 bytes
  char realname[20];                    // 20 bytes
  char username[24];                    // 24 bytes
  usint state;                          //  4 bytes
  usint habit;                          // 4 bytes
  usint exmailbox;                /* �H�c�ʼ�     4 bytes */
  usint exmailboxk;               /* �H�cK��      4 bytes */
  usint toquery;                  /* �n�_��       4 bytes */
  usint bequery;                  /* �H���       4 bytes */
  usint userlevel;                      //  4 bytes     total 76
  usint sendmsg;                  /* �o�T������   4 bytes */
  usint receivemsg;               /* ���T������   4 bytes */
  ushort numlogins;                     //  2 bytes     total 78
  ushort numposts;                      //  2 bytes     total 80
  time_t award;                   /* ���g�P�_     4 bytes */
  time_t firstlogin;                    //  4 bytes     total 84
  time_t lastlogin;                     //  4 bytes     total 88
  char toqid[13];          /* �e���d��    13 bytes */
  char beqid[13];          /* �e���Q�֬d  13 bytes */
  unsigned long int totaltime;    /* �W�u�`�ɼ�   8 bytes */
  char lasthost[16];                    // 16 bytes     total 104
  char email[50];                       // 50 bytes     total 162
  char address[50];                     // 50 bytes     total 212
  char justify[39];             // 39 bytes     total 251
  uschar month;                         //  1 bytes     total 252
  uschar day;                           //  1 bytes     total 253
  uschar year;                          //  1 bytes     total 254
  uschar sex;                           //  1 bytes     total 255
  uschar pager;                   /* �߱��C��     1 bytes */
  time_t registertime;          /* 4 bytes �{�Үɶ� */
  // int scoretimes;          /* ��������     4 bytes */
  char pad[185];                  /* �ŵ۶񺡦�512��      */

};
typedef struct userec userec;


/* ----------------------------------------------------- */
/* old DIR of board struct : 128 bytes                   */
/* ----------------------------------------------------- */

struct fileheader
{
  char filename[33];		/* M.9876543210.A */
  char savemode;		/* file save mode */
  char owner[14];		/* uid[.] */
  char date[6];			/* [02/02] or space(5) */
  char title[73];
  uschar filemode;		/* must be last field @ boards.c */
};
typedef struct fileheader fileheader;


/* ----------------------------------------------------- */
/* old BOARDS struct : 128 bytes                         */
/* ----------------------------------------------------- */

struct boardheader
{
  char brdname[13];		/* bid */
  char title[49];
  char BM[39];			/* BMs' uid, token '/' */
  char pad[11];
  time_t bupdate;		/* note update time */
  char pad2[3];
  uschar bvote;			/* Vote flags */
  time_t vtime;			/* Vote close time */
  usint level;
};
typedef struct boardheader boardheader;
