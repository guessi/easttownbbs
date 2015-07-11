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
/* ���C��w�q�A�H�Q�����ק�				 */
/* ----------------------------------------------------- */

#define COLOR1		"\033[37;44m"	/* footer/feeter ���e�q�C�� */
#define COLOR2		"\033[34;47m"	/* footer/feeter ����q�C�� */
#define COLOR3		"\033[34;47m"	/* neck ���C�� */
#define COLOR4		"\033[44m"	/* ���� ���C�� */
#define COLOR5		"\033[34;47m"	/* more ���Y�����D�C�� */
#define COLOR6		"\033[37;44m"	/* more ���Y�����e�C�� */
#define COLOR7		"\033[36m"	/* �@�̦b�u�W���C�� */
#define COLOR8		"\033[1;37m"	/* both good / online */

/* ----------------------------------------------------- */
/* �ϥΪ̦W���C��					 */
/* ----------------------------------------------------- */

#define COLOR_NORMAL	""		/* �@��ϥΪ� */
#define COLOR_MYBAD	"\033[1;31m"	/* �a�H */
#define COLOR_MYGOOD	"\033[1;32m"	/* �ڪ��n�� */
#define COLOR_OGOOD	"\033[1;33m"	/* �P�ڬ��� */
#define COLOR_CLOAK	"\033[1;35m"	/* ���� */	/* itoc.����: �S�Ψ�A�n���H�Цۦ�[�J ulist_body() */
#define COLOR_SELF	"\033[1;36m"	/* �ۤv */
#define COLOR_BOTHGOOD	"\033[1;37m"	/* ���]�n�� */
#define COLOR_BRDMATE	"\033[36m"	/* �O�� */

/* ----------------------------------------------------- */
/* �ե�pmore.c:PMORE_STYLE_ANSI                          */
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
/* grayout �ѼƳ]�w                                      */
/* ----------------------------------------------------- */

#define GRAYOUT_DARK    0
#define GRAYOUT_BOLD    1
#define GRAYOUT_NORMAL  2


/* ----------------------------------------------------- */
/* ����m						 */
/* ----------------------------------------------------- */

/* itoc.����: �`�N MENU_XPOS �n >= MENU_XNOTE + MOVIE_LINES */

#define MENU_XNOTE	1		/* �ʺA�ݪO�� (1, 0) �}�l */
#define MOVIE_LINES	11		/* �ʵe�̦h�� 11 �C */

#define MENU_XPOS	12		/* ���}�l�� (x, y) �y�� */
#define MENU_YPOS	((d_cols >> 1) + 23)


/* ----------------------------------------------------- */
/* �T���r��G*_neck() �ɪ� necker ����X�өw�q�b�o	 */
/* ----------------------------------------------------- */

/* necker ����Ƴ��O�G��A�q (1, 0) �� (2, 80) */

/* �Ҧ��� XZ_* ���� necker�A�u�O���Ǧb *_neck()�A�����æb *_head() */

/* ulist_neck() �� xpost_head() ���Ĥ@�����S�O�A���b���w�q */

#define NECKER_CLASS	"[��]�D��� [��]�\\Ū [����]��� [c]�g�� [y]���J [/?]�j�M [s]�ݪO [h]����\n" \
			COLOR3 "  %s   ��  �O       ���O �� ��   ��   ��   �z%*s               �H �O     �D%*s    \033[m"

#define NECKER_ULIST	"\n" \
			COLOR3 "  �s��  �N��         �ʺ�%*s               %-*s            �ʺA       �߱�   ���m\033[m"

#define NECKER_PAL	"[��]���} [a]�s�W [c]�ק� [d]�R�� [m]�H�H [w]���y [s]��z [��]�d�� [h]����\n" \
			COLOR3 "  �s��    �N ��         ��       ��%*s                                           \033[m"

#define NECKER_ALOHA	"[��]���} [a]�s�W [d]�R�� [D]�Ϭq�R�� [m]�H�H [w]���y [s]���� [f]�ޤJ [h]����\n" \
			COLOR3 "  �s��   �W �� �q �� �W ��%*s                                                    \033[m"

#define NECKER_VOTE	"[��]���} [R]���G [^P]�|�� [E]�ק� [V]�w�� [^Q]��� [o]�W�� [h]����\n" \
			COLOR3 "  �s��      �}����   �D��H       ��  ��  �v  ��%*s                              \033[m"

#define NECKER_BMW	"[��]���} [d]�R�� [D]�Ϭq�R�� [C]�M�� [m]�H�H [M]�x�s [w]���y [s]��s [h]����\n" \
			COLOR3 "  �s�� �ɶ�  ���� �N  ��        ��       �e%*s                                   \033[m"

#define NECKER_MF	"[��]���} [��]�i�J [^P/a]�s�W [S]�Ƨ� [B]�ƥ� [C]�ƻs [^V]�K�W [m]���� [h]����\n" \
			COLOR3 "  %s   ��  �O       ���O �� ��   ��   ��   �z%*s               �H �O    �D%*s     \033[m"

#define NECKER_COSIGN	"[��]���} [��]�\\Ū [^P]�ӽ� [d]�R�� [o]�}�O [h]���� [n]�����аO\n" \
			COLOR3 "  �s��   �� ��  �|��H       ��  �O  ��  �D%*s                                   \033[m"

#define NECKER_SONG	"[��]���} [��]�s�� [o]�I�q��ݪO [m]�I�q��H�c [Enter]�s�� [h]����\n" \
			COLOR3 "  �s��     �D              �D%*s                            [�s      ��] [��  ��]\033[m"

#define NECKER_NEWS	"[��]���} [��]�\\Ū [h]����\n" \
			COLOR3 "  �s��     �� �� �@  ��       �s  �D  ��  �D%*s                                  \033[m"

#define NECKER_XPOST	"\n" \
			COLOR3 "  �s��     �� �� �@  ��       ��  ��  ��  �D%*s                                  \033[m"

#define NECKER_MBOX	"[��]���} [��,r]Ū�H [d]�R�� [R,y](�s��)�^�H [x]��� [F]��H (%3d �g / %3d KB)\n" \
			COLOR3 "  �s��   �� �� �@  ��       �H  ��  ��  �D%*s                                    \033[m"

#define NECKER_POST	"[��]���} [��]�\\Ū [^P]�o�� [t]�аO�峹 [d]�R�� [V]�벼 [TAB]��ذ� [h]����\n" \
			COLOR3 "  �s��     �� �� �@  ��       ��  ��  ��  �D%*s                       �H��:%-5d \033[m"

#define NECKER_GEM	"[��]���} [��]�s�� [B]�Ҧ� [C]�Ȧs [F]��H [^S]�Ƨ� [h]����  %s\n" \
			COLOR3 "  �s��     �D              �D%*s                            [�s      ��] [��  ��]\033[m"

#define NECKER_TOEAT    "[��]���} [��]�s�� [a, ^P]�s�W [d]�R�� [E]�ק� [T]���D [S]�]�w [h]����\n" \
                        COLOR3 "  �s��   �� �� �̫�s���    �D              �D%*s                               \033[m"

/* �H�U�o�ǫh�O�@���� XZ_* ���c�� necker */

#define NECKER_VOTEALL	"[��/��]�W�U [PgUp/PgDn]�W�U�� [Home/End]���� [��]�벼 [��][q]���}\n" \
			COLOR3 "  �s��   ��  �O       ���O��H ��   ��   ��   �z%*s                 �O    �D%*s     \033[m"

#ifdef HAVE_CREDIT
#define C_CREDIT_IN "\033[1;31m"
#define C_CREDIT_OUT    "\033[1;32m"
#define C_CREDIT_N      "\033[1;37m"
#define C_CREDIT_OTHER  "\033[1;33m"
#define C_CREDIT_TITLE  "\033[1;35m"
#endif

#define NECKER_CREDIT	 "[��]���} [T]�ק��ƻ��� [t]���� [d/D/^D]�R�� [m]���� [^G]����\\�� [h]����\n" \
			COLOR3 "  �s��  ��      ��  ���� ��    �B  �γ~  ��              ��%*s                    \033[m"
			
#define NECKER_HELP	"[��]���} [��]�\\Ū [^P]�s�W [d]�R�� [T]���D [E]�s�� [m]����\n" \
			COLOR3 "  �s��    �� ��         ��       �D%*s                                           \033[m"

#define NECKER_INNBBS	"[��]���} [^P]�s�W [d]�R�� [E]�s�� [/]�j�M [Enter]�Բ�\n" \
			COLOR3 "  �s��              �]         �w%*s                                              \033[m"

/* ----------------------------------------------------- */
/* �T���r��Gmore() �ɪ� footer ����X�өw�q�b�o	 */
/* ----------------------------------------------------- */

/* itoc.010914.����: ��@�g�A�ҥH�s FOOTER�A���O 78 char */

/* itoc.010821: �`�N \\ �O \�A�̫�O�|�F�@�Ӫť��� :p */

#define FOOTER_POST	\
COLOR1 " �峹��Ū " COLOR2 " (ry)�^�� (=\\[]<>-+;'`)�D�D (|?QA)�j�M���D�@�� (kj)�W�U�g (C)�Ȧs   "

#define FOOTER_MAILER	\
COLOR1 " �������� " COLOR2 " (ry)�^�H/�s�� (F)��H (d)�R�� (m)�аO (C)�Ȧs (����)���� (��)���}   "

#define FOOTER_GEM	\
COLOR1 " ��ؿ�Ū " COLOR2 " (=\\[]<>-+;'`)�D�D (|?QA)�j�M���D�@�� (kj)�W�U�g (������)�W�U���}   "

#ifdef HAVE_GAME
#define FOOTER_TALK	\
COLOR1 " ��ͼҦ� " COLOR2 " (^O)�﫳�Ҧ� (^C,^D)������� (^T)�����I�s�� (^Z)�ֱ��C�� (^G)�͹�  "
#else
#define FOOTER_TALK	\
COLOR1 " ��ͼҦ� " COLOR2 " (^C,^D)������� (^T)�����I�s�� (^Z)�ֱ��C�� (^G)�͹� (^Y)�M��      "
#endif

#define FOOTER_COSIGN	\
COLOR1 " �s�p���� " COLOR2 " (ry)�[�J�s�p (kj)�W�U�g (������)�W�U���} (h)����                   " 

#define FOOTER_TOEAT	\
COLOR1 " �����Ѧa " COLOR2 "(kj)�W�U�g (������)�W�U���} (q)�H������ (h)����                     " 

#define FOOTER_MORE	\
COLOR1 " �s�� P.%d (%ld%%) " COLOR2 " (h)���� [PgUp][PgDn][0][$]���� (/n)�j�M (C)�Ȧs (��q)����  "

#define FOOTER_MORE_HAS_USERINFO	\
COLOR1 " �s�� P.%d (%ld%%) " ANSI_COLOR(1;37;41) " ���`�N�G������T�t������X�A���e�i��]�\\Ū�̦Ӳ�  " COLOR2 " (��q)���} "

#define FOOTER_VEDIT	\
COLOR1 " %s " COLOR2 " (^Z)���� (^W)�Ÿ� (^L)��ø (^X)�ɮ׳B�z ��%s�x%s��%5d:%3d    \033[m"


/* ----------------------------------------------------- */
/* �T���r��Gxo_foot() �ɪ� feeter ����X�өw�q�b�o      */
/* ----------------------------------------------------- */


/* itoc.010914.����: �C��h�g�A�ҥH�s FEETER�A���O 78 char */

#define FEETER_CLASS	\
COLOR1 " �ݪO��� " COLOR2 " (c)�s�峹 (vV)�аO�wŪ��Ū (y)�����C�X (z)��q (A)����j�M (S)�Ƨ� "

#define FEETER_ULIST	\
COLOR1 " ���ͦC�� " COLOR2 " (f)�n�� (t)��� (b)�H�c (ad)��� (m)�H�H (w)���y (s)��s (TAB)���� "

#define FEETER_PAL	\
COLOR1 " �I�B�ަ� " COLOR2 " (a)�s�W (d)�R�� (c)�ͽ� (m)�H�H (f)�ޤJ�n�� (r^Q)�d�� (s)��s      "

#define FEETER_ALOHA	\
COLOR1 " �W���q�� " COLOR2 " (a)�s�W (d)�R�� (D)�Ϭq�R�� (f)�ޤJ�n�� (r^Q)�d�� (s)��s          "

#define FEETER_VOTE	\
COLOR1 " �ݪO�벼 " COLOR2 " (��/r/v)�벼 (R)���G (^P)�s�W�벼 (E)�ק� (V)�w�� (b)�}�� (o)�W��  "

#define FEETER_BMW	\
COLOR1 " ���y�^�U " COLOR2 " (d)�R�� (D)�Ϭq (m)�H�H (w)���y (^R)�^�T (u)���O�x�s (s)��s       "

#define FEETER_MF	\
COLOR1 " �̷R�ݪO " COLOR2 " (^P)�s�W (Cg)�ƻs (p^V)�K�W (d)�R�� (c)�s�峹 (vV)�аO�wŪ/��Ū    "

#define FEETER_COSIGN	\
COLOR1 " �s�p�p�� " COLOR2 " (ry)Ū���s�p/�^���s�p (^P)�s�W (d)�R�� (o)�}�O (c)�פ�s�p (S)�]�w "

#define FEETER_SONG	\
COLOR1 " �I�q�t�� " COLOR2 " (r)Ū�� (o)�I�q��ݪO (m)�I�q��H�c (E)�s���ɮ� (T)�s����D        "

#define FEETER_NEWS	\
COLOR1 " �s�D�I�� " COLOR2 " (��/��)�W�U (PgUp/PgDn)�W�U�� (Home/End)���� (��r)��� (��)(q)���} "

#define FEETER_XPOST	\
COLOR1 " ��C�j�M " COLOR2 " (y)�^�� (x)��� (m)�аO (d)�R�� (^P)�o�� (^Q)�d�ߧ@�� (t)����      "

#define FEETER_MBOX	\
COLOR1 " �H�H�۱� " COLOR2 " (y)�^�H (F/x)��H/��F/��� (d)�R�� (D)�Ϭq�R�� (m)�аO (E)�s��    "

#define FEETER_POST	\
COLOR1 " �峹�C�� " COLOR2 " (ry)�^�� (S)�j�M (U)��K (I)�ݪO�ݩ� (x)��� (V)�벼 (u)�s�D�Ҧ�   "

#define FEETER_GEM	\
COLOR1 " �ݪO��� " COLOR2 " (^P/a/f)�s�W/�峹/�ؿ� (E)�s�� (T)���D (m)���� (c)�ƻs (p^V)�K�W   "

#define FEETER_VOTEALL	\
COLOR1 " �벼���� " COLOR2 " (��/��)�W�U (PgUp/PgDn)�W�U�� (Home/End)���� (��)�벼 (��)(q)���}  "

#define FEETER_HELP	\
COLOR1 " ������� " COLOR2 " (��/��)�W�U (PgUp/PgDn)�W�U�� (Home/End)���� (��r)�s�� (��)(q)���} "

#define FEETER_INNBBS	\
COLOR1 " ��H�]�w " COLOR2 " (��/��)�W�U (PgUp/PgDn)�W�U�� (Home/End)���� (��)(q)���}           "

#define FEETER_CREDIT   \
COLOR1 " �O�b�⥾ " COLOR2 " (a/f)�s�W���/��Ƨ� (E)�ק� (s)�έp (S)�Ƨ� (g)������� (��)���}  "


#define FEETER_TOEAT    \
COLOR1 " �����Ѧa " COLOR2 " (b)�i�O (B)�i�O�ק� (^S)����Ƨ� (^Q)�d�ߧ@�� (q)�H������ (��)���} "


/* ----------------------------------------------------- */
/* ���x�ӷ�ñ�W						 */
/* ----------------------------------------------------- */

/* itoc: ��ĳ banner ���n�W�L�T��A�L������ñ�i��|�y���Y�ǨϥΪ̪��ϷP */

#define EDIT_BANNER	"--\n\033[m�� Origin: �F��p�� \033[1;35m(bbs.ndhu.edu.tw)\033[m �� From: \033[33m%s\033[m\n"

#define MODIFY_BANNER	"\033[1;30m�� %s:�峹���e�ץ�%-*s\033[0;30m%-15.15s\033[1;30m %02d/%02d %02d:%02d\033[m\n"


/* ----------------------------------------------------- */
/* ��L�T���r��						 */
/* ----------------------------------------------------- */

#define VMSG_NULL	"                                                           \033[1;33m [ �Ы����N���~�� ] \033[m"

#define ICON_UNREAD_BRD		"\033[1;32m�E\033[37m"/* ��Ū�ݪO */
#define ICON_READ_BRD		"�C"			/* �wŪ�ݪO */

#define ICON_GAMBLED_BRD	"\033[1;31m��\033[m"	/* �|���L�����ݪO */
#define ICON_VOTED_BRD		"\033[1;33m��\033[m"	/* �|��벼�����ݪO */
#define ICON_NOTRAN_BRD		"��"			/* ����H�O */
#define ICON_TRAN_BRD		"��"			/* ��H�O */

#define TOKEN_ZAP_BRD		'-'			/* zap �O */
#define TOKEN_FRIEND_BRD	'P'			/* �n�ͪO */
#define TOKEN_SECRET_BRD	')'			/* ���K�O */

#endif				/* _THEME_H_ */
