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


#define UFO_NOUSE00	BFLAG(0)	/* 沒用到 */
#define UFO_MOVIE	BFLAG(1)	/* 動態看板顯示 */
#define UFO_BRDPOST	BFLAG(2)	/* 1: 看板列表顯示篇數  0: 看板列表顯示號碼 itoc.010912: 恆為新文章模式 */
#define UFO_BRDNAME	BFLAG(3)	/* itoc.010413: 看板列表依 1:brdname 0:class+title 排序 */
#define UFO_NOCOLOR	BFLAG(4)	/* 列表日期不顯示顏色 */
#define UFO_VEDIT	BFLAG(5)	/* 簡化編輯器 */
#define UFO_MOTD	BFLAG(6)	/* 簡化進/離站畫面 */

#define UFO_PAGER	BFLAG(7)	/* 關閉呼叫器 */
#define UFO_RCVER	BFLAG(8)	/* itoc.010716: 拒收廣播 */
#define UFO_QUIET	BFLAG(9)	/* 結廬在人境，而無車馬喧 */
#define UFO_PAL		BFLAG(10)	/* 使用者名單只顯示好友 */
#define UFO_ALOHA	BFLAG(11)	/* 接受上站通知 */
#define UFO_NOALOHA	BFLAG(12)	/* itoc.010716: 上站不通知/協尋 */

#define UFO_BMWDISPLAY	BFLAG(13)	/* itoc.010315: 水球回顧介面 */
#define UFO_NWLOG       BFLAG(14)	/* lkchu.990510: 不存對話紀錄 */
#define UFO_NTLOG       BFLAG(15)	/* lkchu.990510: 不存聊天紀錄 */

#define UFO_MAILBAD	BFLAG(16)	/* 拒收壞人來信 */
#define UFO_SHOWSIGN	BFLAG(17)	/* itoc.000319: 存檔前顯示簽名檔 */

#define UFO_NOUSE18	BFLAG(18)
#define UFO_JUMPBRD	BFLAG(19)	/* itoc.020122: 自動跳去下一個未讀看板 */
#define UFO_HIDFROM	BFLAG(20)	/* guessi.060327 隱藏來源 */
#define UFO_NOINCOME	BFLAG(21)	/* 拒收外部信件 */

#define UFO_NOUSE22	BFLAG(22)
#define UFO_READMD	BFLAG(23)	/* guessi.060825 文章未讀判斷 */
/***/
#define UFO_CLOAK	BFLAG(24)	/* 1: 進入隱形 */
#define UFO_NOUSE25	BFLAG(25)
#define UFO_NOUSE26	BFLAG(26) 
#define UFO_NOUSE27	BFLAG(27)
#define UFO_NOUSE28	BFLAG(28)
#define UFO_NOUSE29	BFLAG(29)
#define UFO_SUPERCLOAK	BFLAG(30)	/* 1: 進入隱身 */
#define UFO_ACL		BFLAG(31)	/* 1: 使用 ACL */

/* 新註冊帳號、guest 的預設 ufo */

#define UFO_DEFAULT_NEW		(UFO_MOVIE | UFO_MOTD | UFO_NOCOLOR |  UFO_BMWDISPLAY | UFO_SHOWSIGN)
#define UFO_DEFAULT_GUEST	(UFO_MOVIE | UFO_MOTD | UFO_NOCOLOR | UFO_QUIET | UFO_NOALOHA | UFO_NWLOG | UFO_NTLOG)


/* ----------------------------------------------------- */
/* Status : flags in UTMP.status			 */
/* ----------------------------------------------------- */


#define STATUS_BIFF	BFLAG(0)	/* 有新信件 */
#define STATUS_REJECT	BFLAG(1)	/* true if reject any body */
#define STATUS_BIRTHDAY	BFLAG(2)	/* 今天生日 */
#define STATUS_COINLOCK	BFLAG(3)	/* 錢幣鎖定 */
#define STATUS_DATALOCK	BFLAG(4)	/* 資料鎖定 */
#define STATUS_MQUOTA	BFLAG(5)	/* 信箱中有過期之信件 */
#define STATUS_MAILOVER	BFLAG(6)	/* 信箱過多信件 */
#define STATUS_MGEMOVER	BFLAG(7)	/* 個人精華區過多 */
#define STATUS_EDITHELP	BFLAG(8)	/* 在 edit 時進入 help */
#define STATUS_PALDIRTY BFLAG(9)	/* 有人在他的朋友名單新增或移除了我 */


#define	HAS_STATUS(x)	(cutmp->status&(x))


/* ----------------------------------------------------- */
/* 各種習慣的中文意義					 */
/* ----------------------------------------------------- */


/* itoc.000320: 增減項目要更改 NUMUFOS_* 大小, 也別忘了改 STR_UFO */

#define NUMUFOS		32
#define NUMUFOS_GUEST	5	/* guest 可以用前 5 個 (0~4) */
#define NUMUFOS_USER	24	/* 一般使用者 可以用前 24 個 (0~23) */

#define STR_UFO		"-dpsnemPBQFANbwtMS-Jfi-rC-----Sa"	/* itoc: 新增習慣的時候別忘了改這裡啊 */
                      /* 01234567890123456789012345678901 */

#ifdef _ADMIN_C_

char *ufo_tbl[NUMUFOS] =
{
  "保留",				/* UFO_NOUSE */
  "動態看板顯示    (開啟/關閉)",	/* UFO_MOVIE */
  "看板列表顯示    (文章/編號)",	/* UFO_BRDPOST */
  "看板列表排序    (字母/分類)",	/* UFO_BRDNAME */	/* itoc.010413: 看板依照字母/分類排序 */
  "列表日期顏色    (去色/顯示)",	/* UFO_NOCOLOR */
  "文章編輯模式    (簡化/完整)",	/* UFO_VEDIT */
  "進出站台畫面    (簡化/完整)",	/* UFO_MOTD */
  "切換呼叫扣機    (好友/所有)",	/* UFO_PAGER */

#ifdef HAVE_NOBROAD
  "廣播天線開關    (拒收/接收)",	/* UFO_RCVER */
#else
  "保留",
#endif

  "遠離世俗塵囂    (安靜/接收)",	/* UFO_QUITE */
  "名單排列顯示    (好友/全部)",	/* UFO_PAL */

#ifdef HAVE_ALOHA
  "接受上站通知    (通知/取消)",	/* UFO_ALOHA */
#else
  "保留",
#endif

#ifdef HAVE_NOALOHA
  "上站發送通知    (不送/通知)",	/* UFO_NOALOHA */
#else
  "保留",
#endif

#ifdef BMW_DISPLAY
  "水球回顧介面    (完整/上次)",	/* UFO_BMWDISPLAY */
#else
  "保留",
#endif

  "儲存水球紀錄    (刪除/保留)",	/* UFO_NWLOG */
  "保留聊天紀錄    (刪除/保留)",	/* UFO_NTLOG */
  "拒收壞人來信    (拒收/接收)",	/* UFO_MAILBAD */
  "簽名檔使用      (開啟/關閉)",	/* UFO_SHOWSIGN */
  "保留",				/* UFO_NOUSE18 */

#ifdef AUTO_JUMPBRD
  "跳去未讀看板    (跳去/不跳)",	/* UFO_JUMPBRD */
#else
  "保留",
#endif

  "隱藏上站來源    (隱藏/顯示)",	/* UFO_HIDFROM */
  "拒收站外信件    (拒收/收取)",	/* UFO_NOINCOME */
  "保留",                       	/* UFO_NEWARTICLE */
  "文章變動提示    (關閉/開啟)",	/* UFO_READMD */
  "隱身術          (隱身/現身)",	/* UFO_CLOAK */

  "保留",				/* UFO_NOUSE25 */
  "保留",				/* UFO_NOUSE26 */
  "保留",                               /* UFO_NOUSE27 */
  "保留",                               /* UFO_NOUSE28 */  
  "保留",                               /* UFO_NOUSE29 */
 
#ifdef HAVE_SUPERCLOAK
  "超級隱身術      (紫隱/現身)",	/* UFO_SUPERCLOAK */
#else
  "保留",
#endif
 
  "站長上站來源    (限制/任意)"         /* UFO_ACL */
  
};
#endif

#endif				/* _UFO_H_ */
