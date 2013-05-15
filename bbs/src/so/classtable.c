/*-------------------------------------------------------*/
/* classtable.c	( YZU WindTopBBS Ver 3.02 )		 */
/*-------------------------------------------------------*/
/* target : �\�Ҫ�					 */
/* create :   /  /                                       */
/* update : 02/07/12                                     */
/* author :						 */
/* modify : itoc.bbs@bbs.ee.nctu.edu.tw			 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_CLASSTABLE

/* ----------------------------------------------------- */
/* classtable.c ���B�Ϊ���Ƶ��c			 */
/* ----------------------------------------------------- */


#define MAX_WEEKDAY	6	/* �@�P���� 6 �� */
#define	MAX_DAYCLASS	14	/* �@�Ѧ� 16 �` */


typedef struct
{
  char name[9];		/* �ҦW */
  char teacher[9];	/* �Юv */
  char class[5];	/* �Ы� */
  char objid[7];	/* �Ҹ� */
}   CLASS;


typedef struct
{
  CLASS table[MAX_WEEKDAY][MAX_DAYCLASS];	/* �@�P�� MAX_WEEKDAY * MAX_DAYCLASS ��� */
}   CLASS_TABLE;


typedef struct
{
  char c_class[5];	/* �ĴX�` */
  char c_start[6];	/* �W�Үɶ� */
  char c_break[6];	/* �U�Үɶ� */
}  CLOCK;


static CLOCK class_time[MAX_DAYCLASS] = 	/* �Ұ�ɶ� */
{
  {" �@ ", "06:10", "07:00"}, 
  {" �G ", "07:10", "08:00"}, 
  {" �T ", "08:10", "09:00"}, 
  {" �| ", "09:10", "10:00"}, 
  {" �� ", "10:10", "11:00"}, 
  {" �� ", "11:10", "12:00"}, 
  {" �C ", "12:10", "13:00"}, 
  {" �K ", "13:10", "14:00"}, 
  {" �E ", "14:10", "15:00"}, 
  {" �Q ", "15:10", "16:00"}, 
  {"�Q�@", "16:10", "17:00"}, 
  {"�Q�G", "17:10", "18:00"}, 
  {"�Q�T", "18:10", "19:00"}, 
  {"�Q�|", "19:10", "20:00"}
};


/* ----------------------------------------------------- */
/* CLASS �B�z���					 */
/* ----------------------------------------------------- */


static void
class_show(x, y, class)
  int x, y;
  CLASS *class;
{
  move(x, y);
  prints("�ҦW�G%s", class->name);
  move(x + 1, y);
  prints("�Юv�G%s", class->teacher);
  move(x + 2, y);
  prints("�ЫǡG%s", class->class);
  move(x + 3, y);
  prints("�Ҹ��G%s", class->objid);
}


static void
class_edit(class)
  CLASS *class;
{
  int echo;

  echo = *(class->name) ? GCARRY : DOECHO;
  vget(4, 0, "�ҦW�G", class->name, sizeof(class->name), echo);
  vget(5, 0, "�Юv�G", class->teacher, sizeof(class->teacher), echo);
  vget(6, 0, "�ЫǡG", class->class, sizeof(class->class), echo);
  vget(7, 0, "�Ҹ��G", class->objid, sizeof(class->objid), echo);
}


static int			/* 1:���T 0:���~ */
class_number(day, class)	/* �Ǧ^�P���X�ĴX�` */
  int *day;
  int *class;
{
  char ans[5];

  move(2, 0);
  outs("503 ��ܬP�����ĤT�`");
  *day = vget(3, 0, "�W�Үɶ��G", ans, 4, DOECHO) - '1';	/* 503 ��ܬP�����ĤT�` */
  *class = atoi(ans + 1) - 1;
  if (*day > MAX_WEEKDAY - 1 || *day < 0 || *class > MAX_DAYCLASS - 1 || *class < 0)
    return 0;

  return 1;
}


/* ----------------------------------------------------- */
/* CLASS_TABLE �B�z���					 */
/* ----------------------------------------------------- */


static void
table_file(fpath, table)	/* �� table �g�J FN_CLASSTBL_LOG */
  char *fpath;
  CLASS_TABLE *table;
{
  int i, j;
  FILE *fp;

  fp = fopen(fpath, "w");

  fprintf(fp, "           �P���@    �P���G    �P���T    �P���|    �P����    �P����\n");
  for (i = 0; i < MAX_DAYCLASS; i++)
  {
    fprintf(fp, "��%s�`  ", class_time[i].c_class);
    for (j = 0; j < MAX_WEEKDAY; j++)
      fprintf(fp, "%-8.8s  ", table->table[j][i].name);

    fprintf(fp, "\n  %s   ", class_time[i].c_start);
    for (j = 0; j < MAX_WEEKDAY; j++)
      fprintf(fp, "%-8.8s  ", table->table[j][i].teacher);

    fprintf(fp, "\n   ��     ");
    for (j = 0; j < MAX_WEEKDAY; j++)
      fprintf(fp, "%-8.8s  ", table->table[j][i].class);

    fprintf(fp, "\n  %s   ", class_time[i].c_break);
    for (j = 0; j < MAX_WEEKDAY; j++)
      fprintf(fp, "%-8.8s  ", table->table[j][i].objid);

    fprintf(fp, "\n\n");
  }
  fclose(fp);
}


static void
table_show(table)
  CLASS_TABLE *table;
{
  char fpath[64];

  usr_fpath(fpath, cuser.userid, FN_CLASSTBL_LOG);
  table_file(fpath, table);
  more(fpath, NULL);
}


static void
table_mail(table)
  CLASS_TABLE *table;
{
  char fpath[64];

  usr_fpath(fpath, cuser.userid, FN_CLASSTBL_LOG);
  table_file(fpath, table);
  mail_self(fpath, cuser.userid, "�ӤH�Ҫ�", MAIL_READ);
}


static void
table_edit(table)
  CLASS_TABLE *table;
{
  int i, j;

  vs_bar("�s��ӤH�Ҫ�");

  if (class_number(&i, &j))
  {
    class_edit(&(table->table[i][j]));
    class_show(10, 0, &(table->table[i][j]));
  }
}


static void
table_del(table)
  CLASS_TABLE *table;
{
  int i, j;

  vs_bar("�R���ӤH�Ҫ�");

  if (!class_number(&i, &j))
    return;

  class_show(10, 0, &(table->table[i][j]));

  if (vans(msg_sure_ny) == 'y')
    memset(&(table->table[i][j]), 0, sizeof(CLASS));
}


static void
table_copy(table)
  CLASS_TABLE *table;
{
  int i, j, x, y;

  vs_bar("�ӤH�Ҫ�");

  move(9, 0);
  outs("�ӷ��G");
  if (!class_number(&i, &j))
    return;

  class_show(10, 0, &(table->table[i][j]));

  move(9, 39);
  outs("�ت��G");
  if (!class_number(&x, &y))
    return;

  class_show(10, 39, &(table->table[x][y]));

  if (vans(msg_sure_ny) == 'y')
    memcpy(&(table->table[x][y]), &(table->table[i][j]), sizeof(CLASS));
}


int
main_classtable()
{
  char fpath[64];
  CLASS_TABLE newtable, oldtable, *ptr;

  usr_fpath(fpath, cuser.userid, FN_CLASSTBL);
  ptr = &newtable;

  if (rec_get(fpath, ptr, sizeof(CLASS_TABLE), 0))
    memset(ptr, 0, sizeof(CLASS_TABLE));
  memcpy(&oldtable, ptr, sizeof(CLASS_TABLE));

  for (;;)
  {
    switch (vans("�Ҫ�t�� (E/C/D)�s��/�ƻs/�R�� P)�L�X K)���� S)�s�� M)�H�c Q)���} [Q] "))
    {
    case 'e':
      table_edit(ptr);
      break;
    case 'd':
      table_del(ptr);
      break;
    case 'c':
      table_copy(ptr);
      break;

    case 'p':
      table_show(ptr);
      break;
    case 'm':
      table_mail(ptr);
      break;

    case 's':
      rec_put(fpath, ptr, sizeof(CLASS_TABLE), 0, NULL);
      memcpy(&oldtable, ptr, sizeof(CLASS_TABLE));
      vmsg("�x�s����");
      break;
    case 'k':
      if (vans(msg_sure_ny) == 'y')
      {
	unlink(fpath);
	memset(ptr, 0, sizeof(CLASS_TABLE));
	memset(&oldtable, 0, sizeof(CLASS_TABLE));
      }
      break;

    default:
      goto end_loop;
    }
  }

end_loop:

  /* �ˬd�s�¬O�_�@�ˡA�Y���@�˭n�ݬO�_�s�� */
  if (memcmp(&oldtable, ptr, sizeof(CLASS_TABLE)))
  {
    if (vans("�O�_�x�s(Y/N)�H[Y] ") != 'n')
      rec_put(fpath, ptr, sizeof(CLASS_TABLE), 0, NULL);
  }

  return 0;  
}
#endif	/* HAVE_CLASSTABLE */
