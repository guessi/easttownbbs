/*-------------------------------------------------------*/
/* theme.h	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : custom theme				 */
/* create : 02/08/17				 	 */
/* update :   /  /  				 	 */
/*-------------------------------------------------------*/


#ifndef	_THEME_H_
#define	_THEME_H_


/* ----------------------------------------------------- */
/* 基本顏色定義，以利介面修改				 */
/* ----------------------------------------------------- */

#define COLOR1		"\033[37;44m"	/* footer/feeter 的前段顏色 */
#define COLOR2		"\033[34;47m"	/* footer/feeter 的後段顏色 */
#define COLOR3		"\033[34;47m"	/* neck 的顏色 */
#define COLOR4		"\033[44m"	/* 光棒 的顏色 */
#define COLOR5		"\033[34;47m"	/* more 檔頭的標題顏色 */
#define COLOR6		"\033[37;44m"	/* more 檔頭的內容顏色 */
#define COLOR7		"\033[36m"	/* 作者在線上的顏色 */
#define COLOR8		"\033[1;37m"	/* both good / online */

/* ----------------------------------------------------- */
/* 使用者名單顏色					 */
/* ----------------------------------------------------- */

#define COLOR_NORMAL	""		/* 一般使用者 */
#define COLOR_MYBAD	"\033[1;31m"	/* 壞人 */
#define COLOR_MYGOOD	"\033[1;32m"	/* 我的好友 */
#define COLOR_OGOOD	"\033[1;33m"	/* 與我為友 */
#define COLOR_CLOAK	"\033[1;35m"	/* 隱形 */	/* itoc.註解: 沒用到，要的人請自行加入 ulist_body() */
#define COLOR_SELF	"\033[1;36m"	/* 自己 */
#define COLOR_BOTHGOOD	"\033[1;37m"	/* 互設好友 */
#define COLOR_BRDMATE	"\033[36m"	/* 板伴 */

/* ----------------------------------------------------- */
/* 調用pmore.c:PMORE_STYLE_ANSI                          */
/* ----------------------------------------------------- */

#define ESC_NUM (0x1b)
#define ESC_STR "\x1b"
#define ESC_CHR '\x1b'
	
#define ANSI_RESET      ESC_STR "[m"
#define ANSI_COLOR(x)   ESC_STR "[" #x "m"
#define ANSI_CLRTOEND   ESC_STR "[K"
#define ANSI_MOVETO(y,x) ESC_STR "[" #y ";" #x "H"

#define ANSI_IN_ESCAPE(x) (((x) >= '0' && (x) <= '9') || \
	        (x) == ';' || (x) == ',' || (x) == '[')

/* ----------------------------------------------------- */
/* grayout 參數設定                                      */
/* ----------------------------------------------------- */

#define GRAYOUT_DARK    0
#define GRAYOUT_BOLD    1
#define GRAYOUT_NORMAL  2


/* ----------------------------------------------------- */
/* 選單位置						 */
/* ----------------------------------------------------- */

/* itoc.註解: 注意 MENU_XPOS 要 >= MENU_XNOTE + MOVIE_LINES */

#define MENU_XNOTE	1		/* 動態看板由 (1, 0) 開始 */
#define MOVIE_LINES	11		/* 動畫最多有 11 列 */

#define MENU_XPOS	12		/* 選單開始的 (x, y) 座標 */
#define MENU_YPOS	((d_cols >> 1) + 23)


/* ----------------------------------------------------- */
/* 訊息字串：*_neck() 時的 necker 都抓出來定義在這	 */
/* ----------------------------------------------------- */

/* necker 的行數都是二行，從 (1, 0) 到 (2, 80) */

/* 所有的 XZ_* 都有 necker，只是有些在 *_neck()，有些藏在 *_head() */

/* ulist_neck() 及 xpost_head() 的第一行比較特別，不在此定義 */

#define NECKER_CLASS	"[←]主選單 [→]閱\讀 [↑↓]選擇 [c]篇數 [y]載入 [/?]搜尋 [s]看板 [h]說明\n" \
			COLOR3 "  %s   看  板       類別 轉 中   文   敘   述%*s               人 板     主%*s    \033[m"

#define NECKER_ULIST	"\n" \
			COLOR3 "  編號  代號         暱稱%*s               %-*s            動態       心情   閒置\033[m"

#define NECKER_PAL	"[←]離開 [a]新增 [c]修改 [d]刪除 [m]寄信 [w]水球 [s]整理 [→]查詢 [h]說明\n" \
			COLOR3 "  編號    代 號         友       誼%*s                                           \033[m"

#define NECKER_ALOHA	"[←]離開 [a]新增 [d]刪除 [D]區段刪除 [m]寄信 [w]水球 [s]重整 [f]引入 [h]說明\n" \
			COLOR3 "  編號   上 站 通 知 名 單%*s                                                    \033[m"

#define NECKER_VOTE	"[←]離開 [R]結果 [^P]舉行 [E]修改 [V]預覽 [^Q]改期 [o]名單 [h]說明\n" \
			COLOR3 "  編號      開票日   主辦人       投  票  宗  旨%*s                              \033[m"

#define NECKER_BMW	"[←]離開 [d]刪除 [D]區段刪除 [C]清空 [m]寄信 [M]儲存 [w]水球 [s]更新 [h]說明\n" \
			COLOR3 "  編號 時間  類型 代  號        內       容%*s                                   \033[m"

#define NECKER_MF	"[←]離開 [→]進入 [^P/a]新增 [S]排序 [B]備份 [C]複製 [^V]貼上 [m]移動 [h]說明\n" \
			COLOR3 "  %s   看  板       類別 轉 中   文   敘   述%*s               人 板    主%*s     \033[m"

#define NECKER_COSIGN	"[←]離開 [→]閱\讀 [^P]申請 [d]刪除 [o]開板 [h]說明 [n]消除標記\n" \
			COLOR3 "  編號   日 期  舉辦人       看  板  標  題%*s                                   \033[m"

#define NECKER_SONG	"[←]離開 [→]瀏覽 [o]點歌到看板 [m]點歌到信箱 [Enter]瀏覽 [h]說明\n" \
			COLOR3 "  編號     主              題%*s                            [編      選] [日  期]\033[m"

#define NECKER_NEWS	"[←]離開 [→]閱\讀 [h]說明\n" \
			COLOR3 "  編號     日 期 作  者       新  聞  標  題%*s                                  \033[m"

#define NECKER_XPOST	"\n" \
			COLOR3 "  編號     日 期 作  者       文  章  標  題%*s                                  \033[m"

#define NECKER_MBOX	"[←]離開 [→,r]讀信 [d]刪除 [R,y](群組)回信 [x]轉錄 [F]轉寄 (%3d 篇 / %3d KB)\n" \
			COLOR3 "  編號   日 期 作  者       信  件  標  題%*s                                    \033[m"

#define NECKER_POST	"[←]離開 [→]閱\讀 [^P]發表 [t]標記文章 [d]刪除 [V]投票 [TAB]精華區 [h]說明\n" \
			COLOR3 "  編號     日 期 作  者       文  章  標  題%*s                       人氣:%-5d \033[m"

#define NECKER_GEM	"[←]離開 [→]瀏覽 [B]模式 [C]暫存 [F]轉寄 [^S]排序 [h]說明  %s\n" \
			COLOR3 "  編號     主              題%*s                            [編      選] [日  期]\033[m"

#define NECKER_TOEAT    "[←]離開 [→]瀏覽 [a, ^P]新增 [d]刪除 [E]修改 [T]標題 [S]設定 [h]說明\n" \
                        COLOR3 "  編號   日 期 最後編輯者    主              題%*s                               \033[m"

/* 以下這些則是一些類 XZ_* 結構的 necker */

#define NECKER_VOTEALL	"[↑/↓]上下 [PgUp/PgDn]上下頁 [Home/End]首尾 [→]投票 [←][q]離開\n" \
			COLOR3 "  編號   看  板       類別轉信 中   文   敘   述%*s                 板    主%*s     \033[m"

#ifdef HAVE_CREDIT
#define C_CREDIT_IN "\033[1;31m"
#define C_CREDIT_OUT    "\033[1;32m"
#define C_CREDIT_N      "\033[1;37m"
#define C_CREDIT_OTHER  "\033[1;33m"
#define C_CREDIT_TITLE  "\033[1;35m"
#endif

#define NECKER_CREDIT	 "[←]離開 [T]修改資料說明 [t]標籤 [d/D/^D]刪除 [m]移動 [^G]海錨功\能 [h]說明\n" \
			COLOR3 "  編號  日      期  收支 金    額  用途  說              明%*s                    \033[m"
			
#define NECKER_HELP	"[←]離開 [→]閱\讀 [^P]新增 [d]刪除 [T]標題 [E]編輯 [m]移動\n" \
			COLOR3 "  編號    檔 案         標       題%*s                                           \033[m"

#define NECKER_INNBBS	"[←]離開 [^P]新增 [d]刪除 [E]編輯 [/]搜尋 [Enter]詳細\n" \
			COLOR3 "  編號              設         定%*s                                              \033[m"

/* ----------------------------------------------------- */
/* 訊息字串：more() 時的 footer 都抓出來定義在這	 */
/* ----------------------------------------------------- */

/* itoc.010914.註解: 單一篇，所以叫 FOOTER，都是 78 char */

/* itoc.010821: 注意 \\ 是 \，最後別漏了一個空白鍵 :p */

#define FOOTER_POST	\
COLOR1 " 文章選讀 " COLOR2 " (ry)回應 (=\\[]<>-+;'`)主題 (|?QA)搜尋標題作者 (kj)上下篇 (C)暫存   "

#define FOOTER_MAILER	\
COLOR1 " 魚雁往返 " COLOR2 " (ry)回信/群組 (F)轉寄 (d)刪除 (m)標記 (C)暫存 (↑↓)移動 (←)離開   "

#define FOOTER_GEM	\
COLOR1 " 精華選讀 " COLOR2 " (=\\[]<>-+;'`)主題 (|?QA)搜尋標題作者 (kj)上下篇 (↑↓←)上下離開   "

#ifdef HAVE_GAME
#define FOOTER_TALK	\
COLOR1 " 交談模式 " COLOR2 " (^O)對奕模式 (^C,^D)結束交談 (^T)切換呼叫器 (^Z)快捷列表 (^G)嗶嗶  "
#else
#define FOOTER_TALK	\
COLOR1 " 交談模式 " COLOR2 " (^C,^D)結束交談 (^T)切換呼叫器 (^Z)快捷列表 (^G)嗶嗶 (^Y)清除      "
#endif

#define FOOTER_COSIGN	\
COLOR1 " 連署機制 " COLOR2 " (ry)加入連署 (kj)上下篇 (↑↓←)上下離開 (h)說明                   " 

#define FOOTER_TOEAT	\
COLOR1 " 美食天地 " COLOR2 "(kj)上下篇 (↑↓←)上下離開 (q)隨機推薦 (h)說明                     " 

#define FOOTER_MORE	\
COLOR1 " 瀏覽 P.%d (%ld%%) " COLOR2 " (h)說明 [PgUp][PgDn][0][$]移動 (/n)搜尋 (C)暫存 (←q)結束  "

#define FOOTER_MORE_HAS_USERINFO	\
COLOR1 " 瀏覽 P.%d (%ld%%) " ANSI_COLOR(1;37;41) " ※注意：本頁資訊含有控制碼，內容可能因閱\讀者而異  " COLOR2 " (←q)離開 "

#define FOOTER_VEDIT	\
COLOR1 " %s " COLOR2 " (^Z)說明 (^W)符號 (^L)重繪 (^X)檔案處理 ║%s│%s║%5d:%3d    \033[m"


/* ----------------------------------------------------- */
/* 訊息字串：xo_foot() 時的 feeter 都抓出來定義在這      */
/* ----------------------------------------------------- */


/* itoc.010914.註解: 列表多篇，所以叫 FEETER，都是 78 char */

#define FEETER_CLASS	\
COLOR1 " 看板選擇 " COLOR2 " (c)新文章 (vV)標記已讀未讀 (y)全部列出 (z)選訂 (A)全域搜尋 (S)排序 "

#define FEETER_ULIST	\
COLOR1 " 網友列表 " COLOR2 " (f)好友 (t)聊天 (b)信箱 (ad)交友 (m)寄信 (w)水球 (s)更新 (TAB)切換 "

#define FEETER_PAL	\
COLOR1 " 呼朋引伴 " COLOR2 " (a)新增 (d)刪除 (c)友誼 (m)寄信 (f)引入好友 (r^Q)查詢 (s)更新      "

#define FEETER_ALOHA	\
COLOR1 " 上站通知 " COLOR2 " (a)新增 (d)刪除 (D)區段刪除 (f)引入好友 (r^Q)查詢 (s)更新          "

#define FEETER_VOTE	\
COLOR1 " 看板投票 " COLOR2 " (→/r/v)投票 (R)結果 (^P)新增投票 (E)修改 (V)預覽 (b)開票 (o)名單  "

#define FEETER_BMW	\
COLOR1 " 水球回顧 " COLOR2 " (d)刪除 (D)區段 (m)寄信 (w)水球 (^R)回訊 (u)分別儲存 (s)更新       "

#define FEETER_MF	\
COLOR1 " 最愛看板 " COLOR2 " (^P)新增 (Cg)複製 (p^V)貼上 (d)刪除 (c)新文章 (vV)標記已讀/未讀    "

#define FEETER_COSIGN	\
COLOR1 " 連署小站 " COLOR2 " (ry)讀取連署/回應連署 (^P)新增 (d)刪除 (o)開板 (c)終止連署 (S)設定 "

#define FEETER_SONG	\
COLOR1 " 點歌系統 " COLOR2 " (r)讀取 (o)點歌到看板 (m)點歌到信箱 (E)編輯檔案 (T)編輯標題        "

#define FEETER_NEWS	\
COLOR1 " 新聞點選 " COLOR2 " (↑/↓)上下 (PgUp/PgDn)上下頁 (Home/End)首尾 (→r)選取 (←)(q)離開 "

#define FEETER_XPOST	\
COLOR1 " 串列搜尋 " COLOR2 " (y)回應 (x)轉錄 (m)標記 (d)刪除 (^P)發表 (^Q)查詢作者 (t)標籤      "

#define FEETER_MBOX	\
COLOR1 " 信信相惜 " COLOR2 " (y)回信 (F/x)轉寄/轉達/轉錄 (d)刪除 (D)區段刪除 (m)標記 (E)編輯    "

#define FEETER_POST	\
COLOR1 " 文章列表 " COLOR2 " (ry)回文 (S)搜尋 (U)文摘 (I)看板屬性 (x)轉錄 (V)投票 (u)新聞模式   "

#define FEETER_GEM	\
COLOR1 " 看板精華 " COLOR2 " (^P/a/f)新增/文章/目錄 (E)編輯 (T)標題 (m)移動 (c)複製 (p^V)貼上   "

#define FEETER_VOTEALL	\
COLOR1 " 投票中心 " COLOR2 " (↑/↓)上下 (PgUp/PgDn)上下頁 (Home/End)首尾 (→)投票 (←)(q)離開  "

#define FEETER_HELP	\
COLOR1 " 說明文件 " COLOR2 " (↑/↓)上下 (PgUp/PgDn)上下頁 (Home/End)首尾 (→r)瀏覽 (←)(q)離開 "

#define FEETER_INNBBS	\
COLOR1 " 轉信設定 " COLOR2 " (↑/↓)上下 (PgUp/PgDn)上下頁 (Home/End)首尾 (←)(q)離開           "

#define FEETER_CREDIT   \
COLOR1 " 記帳手札 " COLOR2 " (a/f)新增資料/資料夾 (E)修改 (s)統計 (S)排序 (g)收錄資料 (←)離開  "


#define FEETER_TOEAT    \
COLOR1 " 美食天地 " COLOR2 " (b)進板 (B)進板修改 (^S)重整排序 (^Q)查詢作者 (q)隨機推薦 (←)離開 "


/* ----------------------------------------------------- */
/* 站台來源簽名						 */
/* ----------------------------------------------------- */

/* itoc: 建議 banner 不要超過三行，過長的站簽可能會造成某些使用者的反感 */

#define EDIT_BANNER	"--\n\033[m※ Origin: 東方小城 \033[1;35m(bbs.ndhu.edu.tw)\033[m ◆ From: \033[33m%s\033[m\n"

#define MODIFY_BANNER	"\033[1;30m修 %s:文章內容修正%-*s\033[0;30m%-15.15s\033[1;30m %02d/%02d %02d:%02d\033[m\n"


/* ----------------------------------------------------- */
/* 其他訊息字串						 */
/* ----------------------------------------------------- */

#define VMSG_NULL	"                                                           \033[1;33m [ 請按任意鍵繼續 ] \033[m"

#define ICON_UNREAD_BRD		"\033[1;32m‧\033[37m"/* 未讀看板 */
#define ICON_READ_BRD		"。"			/* 已讀看板 */

#define ICON_GAMBLED_BRD	"\033[1;31m賭\033[m"	/* 舉行賭盤中的看板 */
#define ICON_VOTED_BRD		"\033[1;33m投\033[m"	/* 舉行投票中的看板 */
#define ICON_NOTRAN_BRD		"○"			/* 不轉信板 */
#define ICON_TRAN_BRD		"◎"			/* 轉信板 */

#define TOKEN_ZAP_BRD		'-'			/* zap 板 */
#define TOKEN_FRIEND_BRD	'P'			/* 好友板 */
#define TOKEN_SECRET_BRD	')'			/* 秘密板 */

#endif				/* _THEME_H_ */
