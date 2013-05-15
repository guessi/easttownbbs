/*-------------------------------------------------------*/
/* util/sob.h 	                                         */
/*-------------------------------------------------------*/
/* target : SOB 至 Maple 3.02 轉換			 */
/* create : 02/10/26					 */
/* author : ernie@micro8.ee.nthu.edu.tw                  */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#if 0

  1. 設定 OLD_BBSHOME、FN_PASSWD、FN_BOARD
  2. 修改所有的 old struct

  3. 必須在 brd 轉完才可以轉換 gem
  4. 必須在 usr 及 brd 都轉完才可以轉換 pal
  5. 建議轉換順序為 usr -> brd -> gem ->pal

#endif


#include "bbs.h"


#define OLD_BBSHOME     "/home/oldbbs"		/* SOB */
#define FN_PASSWD	"/home/oldbbs/.PASSWDS"	/* SOB */
#define FN_BOARD        "/home/oldbbs/.BOARDS"	/* SOB */


#undef	HAVE_PERSONAL_GEM			/* SOB 是沒有個人精華區的 */


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
  usint exmailbox;                /* 信箱封數     4 bytes */
  usint exmailboxk;               /* 信箱K數      4 bytes */
  usint toquery;                  /* 好奇度       4 bytes */
  usint bequery;                  /* 人氣度       4 bytes */
  usint userlevel;                      //  4 bytes     total 76
  usint sendmsg;                  /* 發訊息次數   4 bytes */
  usint receivemsg;               /* 收訊息次數   4 bytes */
  ushort numlogins;                     //  2 bytes     total 78
  ushort numposts;                      //  2 bytes     total 80
  time_t award;                   /* 獎懲判斷     4 bytes */
  time_t firstlogin;                    //  4 bytes     total 84
  time_t lastlogin;                     //  4 bytes     total 88
  char toqid[13];          /* 前次查誰    13 bytes */
  char beqid[13];          /* 前次被誰查  13 bytes */
  unsigned long int totaltime;    /* 上線總時數   8 bytes */
  char lasthost[16];                    // 16 bytes     total 104
  char email[50];                       // 50 bytes     total 162
  char address[50];                     // 50 bytes     total 212
  char justify[39];             // 39 bytes     total 251
  uschar month;                         //  1 bytes     total 252
  uschar day;                           //  1 bytes     total 253
  uschar year;                          //  1 bytes     total 254
  uschar sex;                           //  1 bytes     total 255
  uschar pager;                   /* 心情顏色     1 bytes */
  time_t registertime;          /* 4 bytes 認證時間 */
  // int scoretimes;          /* 評分次數     4 bytes */
  char pad[185];                  /* 空著填滿至512用      */

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
