/*-------------------------------------------------------*/
/* jcee.c       ( NTU FPG BBS Ver 2.00 )                 */
/*-------------------------------------------------------*/
/* target : �p�Һ]��d��                                 */
/* create : 98/08/08                                     */
/* update : 01/08/21                                     */
/* author : �d�Эl                                       */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#if 0

 ��ƨӷ�
 http://www.csie.nctu.edu.tw/service/jcee/���~

#endif


#include "bbs.h"


#define END_YEAR   94      /* �̫�@�~���]��O���� 94 �~ */


static int
ueequery_item(count, i, examnum, ptr, now)
  int count, i, examnum;
  char *now, *ptr;
{
  char buf[32];

  if (count > XO_TALL && (count % XO_TALL) == 1)
  {
    move(1, 0);
    clrtoeol();
    outs("\033[1;33m�����d�ߤw���A�����d�߽Ы����N��A�����d�߽Ы� Q\033[m");

    if (vkey() == 'q' || vkey() == 'Q')
      return 0;

    i = 0;
    vs_bar("�p�Һ]��d��");
    move(2, 0);
    outs(COLOR2 "   �����    �m  �W    �� �� �j ��             "
      "�� �� �� �t                   \033[m");
  }

  move(3 + i, 0);

  sprintf(buf, " %%-%d.%ds%%%ds   %%-8s  %%s", examnum, examnum, 9 - examnum);
  prints(buf, ptr, "", ptr + examnum, now);

#if 0
  if (examnum == 6)
    prints(" %-6.6s   %-8s  %s", ptr, ptr + examnum, now);
  else if (examnum == 7)
    prints(" %-7.7s  %-8s  %s", ptr, ptr + examnum, now);
  else if (examnum == 8)
    prints(" %-8.8s %-8s  %s", ptr, ptr + examnum, now);
#endif

  move(b_lines, 0);
  clrtoeol();
  prints(COLOR1 " �]��d�� " COLOR2 " �ثe�@�d�ߨ� %4d ����ơC"
    "����ƶȨѰѦҡA�������G�H�p�۷|�������� \033[m", count);

  return i + 1;
}


static void
ueequery_foot(count)
  int count;
{
  if (count == 0)
  {
    move(4, 0);
    outs("�S���d�ߨ������");
  }
  move(b_lines, 0);
  clrtoeol();
  prints(COLOR1 " �]��d�� " COLOR2 " �ثe�@�d�ߨ� %4d ����ơC"
    "�d�ߵ����A�����N�����}                   \033[m", count);
  vkey();
}


static int
x_ueequery()
{
  char buf[256], query[20], now[80], *ptr;
  int i, count, year, examnum;
  FILE *fp;

  vs_bar("�p�Һ]��d��");

  outs("\n�`�N�ƶ�\n"
    "1. �������Ҹ��X�γ����m�W�ⶵ�ܤֻݶ�g�@���~����d��\n"
    "2. �Y�m�W�u����r�A�Щ��r�����[���Ϊť�\n"
    "3. 83-86 �~�ݭn�������Ҹ��X�A87-90 �~�ݭn�C�����Ҹ��X\n"
    "4. 91�~�_�ݭn�K�����Ҹ��X\n");

  sprintf(buf, "�п�J�p�Ҧ~��(83 ~ %d)�H[%d] ", END_YEAR, END_YEAR);

  if (!vget(9, 0, buf, query, 3, DOECHO))
  {
    year = END_YEAR;
  }
  else
  {
    year = atoi(query);
    if (year < 83 || year > END_YEAR)
      return 0;
  }

  if (year <= 86)
    examnum = 6;
  else if (year <= 90)
    examnum = 7;
  else
    examnum = 8;

  if (vget(11, 0, "�п�J�������Ҹ��X�G", query, examnum + 1, DOECHO))
  {
    if (strlen(query) != examnum)
    {
      vmsg("�����T���������Ҹ��X");
      return 0;
    }
  }
  else if (!vget(13, 0, "�п�J�����m�W�G", query, 10, DOECHO))
  {
    vmsg("�Щ����Ҹ��X��m�W�G�椤�ܤ@��g");
    return 0;
  }

  sprintf(buf, "etc/jcee/%d.txt", year);

  if (fp = fopen(buf, "r"))
  {
    int j;
    char *pquery;

    count = 0;  /* ��������X�� */
    i = 0;      /* �⥻�����ĴX�� */

    vs_bar("�p�Һ]��d��");
    prints("�d�ߡm%s�n��....\n", query);
    outs(COLOR2 "   �����    �m  �W    �� �� �j ��             "
      "�� �� �� �t                   \033[m");

    while (fgets(buf, 256, fp))
    {
      if (buf[0] == ' ')        /* �o�˴N�O��t���}�Y */
      {
        ptr = strtok(buf + 5, " ");
        strcpy(now, ptr);
        ptr = strtok(NULL, " ");
        sprintf(now, "%-24s%-30s", now, ptr);
        continue;
      }

      ptr = strtok(buf, "    ");

      do
      {
        if (pquery = strstr(ptr, query))
          j = (pquery - ptr) + examnum + 1;
        else
          j = 2;               /* ��ܥ���� */

        if (j == (pquery - ptr) + examnum + 1 || j % 2)/* �B�z����r�����D */
        {
          count++;
          if (!(i = ueequery_item(count, i, examnum, ptr, now)))
          {
            fclose(fp);
            ueequery_foot(count);
            return 0;
          }
        }
      } while (ptr = strtok(NULL, "    "));
    }
    fclose(fp);
  }

  ueequery_foot(count);
  return 0;
}


static int
x_ueequery2()
{
  char buf[256], query[30], query2[30], now[80], *ptr;
  int i, count, year, examnum;
  int school, dept;
  FILE *fp;

  vs_bar("�p�Һ]��d��");

  outs("\n�`�N�ƶ�\n"
    "1. �j�Ǥά�t�ⶵ�ܤֻݶ�g�@���~����d��\n"
    "2. ��J�����r��Y�i\n");

  sprintf(buf, "�п�J�p�Ҧ~��(83 ~ %d)�H[%d] ", END_YEAR, END_YEAR);

  if (!vget(9, 0, buf, query, 3, DOECHO))
  {
    year = END_YEAR;
  }
  else
  {
    year = atoi(query);
    if (year < 83 || year > END_YEAR)
      return 0;
  }

  if (year <= 86)
    examnum = 6;
  else if (year <= 90)
    examnum = 7;
  else
    examnum = 8;

  vget(11, 0, "�п�J�d�ߤj�ǡG", query, 20, DOECHO);
  vget(13, 0, "�п�J�d�߬�t�G", query2, 20, DOECHO);

  if (!*query && !*query2)
    return 0;

  sprintf(buf, "etc/jcee/%d.txt", year);

  if (fp = fopen(buf, "r"))
  {
    count = 0;  /* ��������X�� */
    i = 0;      /* �⥻�����ĴX�� */

    vs_bar("�p�Һ]��d��");
    prints("�d�ߡm%s%s�n��....\n", query, query2);
    outs(COLOR2 "   �����    �m  �W    �� �� �j ��             "
      "�� �� �� �t                   \033[m");

    while (fgets(buf, 256, fp))
    {
      if (buf[0] == ' ')         /* �o�˴N�O��t���}�Y */
      {
        ptr = strtok(buf + 5, " ");
        strcpy(now, ptr);

        if (!*query || strstr(now, query))      /* ���Ǯ� */
          school = 1;
        else
          school = 0;

        ptr = strtok(NULL, " ");
        if (!*query2 || strstr(ptr, query2))    /* ����t */
          dept = 1;
        else
          dept = 0;

        sprintf(now, "%-24s%-30s", now, ptr);
        continue;
      }

      ptr = strtok(buf, "    ");

      do
      {
        if (school && dept && strlen(ptr) >= 6)/* �B�z����r�����D */
        {
          count++;
          if (!(i = ueequery_item(count, i, examnum, ptr, now)))
          {
            fclose(fp);
            ueequery_foot(count);
            return 0;
          }
        }
      } while (ptr = strtok(NULL, "    "));
    }

    fclose(fp);
  }

  ueequery_foot(count);
  return 0;
}


int
main_jcee()
{
  int ch;

  ch = vans("�p�Һ]��d�ߡA�п�� 1)���]�d�� 2)��t�����W�� [Q]���} ");
  if (ch == '1')
    x_ueequery();
  else if (ch == '2')
    x_ueequery2();
  else
    return XEASY;

  return 0;
}

