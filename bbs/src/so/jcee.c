/*-------------------------------------------------------*/
/* jcee.c       ( NTU FPG BBS Ver 2.00 )                 */
/*-------------------------------------------------------*/
/* target : 聯考榜單查詢                                 */
/* create : 98/08/08                                     */
/* update : 01/08/21                                     */
/* author : 吳紹衍                                       */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#if 0

 資料來源
 http://www.csie.nctu.edu.tw/service/jcee/歷年

#endif


#include "bbs.h"


#define END_YEAR   94      /* 最後一年的榜單是民國 94 年 */


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
    outs("\033[1;33m本頁查詢已滿，換頁查詢請按任意鍵，結束查詢請按 Q\033[m");

    if (vkey() == 'q' || vkey() == 'Q')
      return 0;

    i = 0;
    vs_bar("聯考榜單查詢");
    move(2, 0);
    outs(COLOR2 "   准考證    姓  名    錄 取 大 學             "
      "錄 取 科 系                   \033[m");
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
  prints(COLOR1 " 榜單查詢 " COLOR2 " 目前共查詢到 %4d 筆資料。"
    "本資料僅供參考，錄取結果以聯招會公布為準 \033[m", count);

  return i + 1;
}


static void
ueequery_foot(count)
  int count;
{
  if (count == 0)
  {
    move(4, 0);
    outs("沒有查詢到任何資料");
  }
  move(b_lines, 0);
  clrtoeol();
  prints(COLOR1 " 榜單查詢 " COLOR2 " 目前共查詢到 %4d 筆資料。"
    "查詢結束，按任意鍵離開                   \033[m", count);
  vkey();
}


static int
x_ueequery()
{
  char buf[256], query[20], now[80], *ptr;
  int i, count, year, examnum;
  FILE *fp;

  vs_bar("聯考榜單查詢");

  outs("\n注意事項\n"
    "1. 完整准考證號碼及部份姓名兩項至少需填寫一項才能夠查詢\n"
    "2. 若姓名只有兩字，請於兩字中間加全形空白\n"
    "3. 83-86 年需要六位准考證號碼，87-90 年需要七位准考證號碼\n"
    "4. 91年起需要八位准考證號碼\n");

  sprintf(buf, "請輸入聯考年度(83 ~ %d)？[%d] ", END_YEAR, END_YEAR);

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

  if (vget(11, 0, "請輸入完整准考證號碼：", query, examnum + 1, DOECHO))
  {
    if (strlen(query) != examnum)
    {
      vmsg("不正確的完整准考證號碼");
      return 0;
    }
  }
  else if (!vget(13, 0, "請輸入部份姓名：", query, 10, DOECHO))
  {
    vmsg("請於准考證號碼於姓名二欄中擇一填寫");
    return 0;
  }

  sprintf(buf, "etc/jcee/%d.txt", year);

  if (fp = fopen(buf, "r"))
  {
    int j;
    char *pquery;

    count = 0;  /* 算全部有幾個 */
    i = 0;      /* 算本頁的第幾個 */

    vs_bar("聯考榜單查詢");
    prints("查詢《%s》中....\n", query);
    outs(COLOR2 "   准考證    姓  名    錄 取 大 學             "
      "錄 取 科 系                   \033[m");

    while (fgets(buf, 256, fp))
    {
      if (buf[0] == ' ')        /* 這樣就是科系的開頭 */
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
          j = 2;               /* 表示未找到 */

        if (j == (pquery - ptr) + examnum + 1 || j % 2)/* 處理中文字的問題 */
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

  vs_bar("聯考榜單查詢");

  outs("\n注意事項\n"
    "1. 大學及科系兩項至少需填寫一項才能夠查詢\n"
    "2. 輸入部分字串即可\n");

  sprintf(buf, "請輸入聯考年度(83 ~ %d)？[%d] ", END_YEAR, END_YEAR);

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

  vget(11, 0, "請輸入查詢大學：", query, 20, DOECHO);
  vget(13, 0, "請輸入查詢科系：", query2, 20, DOECHO);

  if (!*query && !*query2)
    return 0;

  sprintf(buf, "etc/jcee/%d.txt", year);

  if (fp = fopen(buf, "r"))
  {
    count = 0;  /* 算全部有幾個 */
    i = 0;      /* 算本頁的第幾個 */

    vs_bar("聯考榜單查詢");
    prints("查詢《%s%s》中....\n", query, query2);
    outs(COLOR2 "   准考證    姓  名    錄 取 大 學             "
      "錄 取 科 系                   \033[m");

    while (fgets(buf, 256, fp))
    {
      if (buf[0] == ' ')         /* 這樣就是科系的開頭 */
      {
        ptr = strtok(buf + 5, " ");
        strcpy(now, ptr);

        if (!*query || strstr(now, query))      /* 找到學校 */
          school = 1;
        else
          school = 0;

        ptr = strtok(NULL, " ");
        if (!*query2 || strstr(ptr, query2))    /* 找到科系 */
          dept = 1;
        else
          dept = 0;

        sprintf(now, "%-24s%-30s", now, ptr);
        continue;
      }

      ptr = strtok(buf, "    ");

      do
      {
        if (school && dept && strlen(ptr) >= 6)/* 處理中文字的問題 */
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

  ch = vans("聯考榜單查詢，請選擇 1)金榜查詢 2)科系錄取名單 [Q]離開 ");
  if (ch == '1')
    x_ueequery();
  else if (ch == '2')
    x_ueequery2();
  else
    return XEASY;

  return 0;
}

