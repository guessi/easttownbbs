/*-------------------------------------------------------*/
/* new_credit.c     ( NTHU CS MapleBBS Ver 3.10 )        */
/*-------------------------------------------------------*/
/* target : 新記帳手札                                   */
/* author : kyo.bbs@cszone.org                           */
/* create : 05/01/03                                     */
/* update :   /  /                                       */
/*-------------------------------------------------------*/


#include "bbs.h"

#define BCOLOR "\033[37;44m"

#ifdef HAVE_CREDIT

extern XZ xz[];
extern char xo_pool[];
extern int TagNum;
extern TagItem TagList[];

static char CreditAnchor[64], CreditSailor[TTLEN + 1];

static int credit_anchor(XO *xo);
void XoCredit(char *folder, char *title);
static void credit_do_delete();

typedef struct
{
  int year;                     /* 年 */
  char month;                   /* 月 */
  char day;                     /* 日 */

  char flag;                    /* 支出/收入 */
  int money;                    /* 金額 */
  char useway;                  /* 類別(食衣住行育樂) */
  char desc[112];               /* 說明 */
}      CREDIT_OLD;

static time_t
make_time(year, month, day)
  int year, month, day;
{
  struct tm ptime;

  ptime.tm_sec = 0;
  ptime.tm_min = 0;
  ptime.tm_hour = 0;
  ptime.tm_mday = day;
  ptime.tm_mon = month - 1;
  ptime.tm_year = year - 1900;
  ptime.tm_isdst = 0;
#ifndef CYGWIN
  ptime.tm_zone = "GMT";
  ptime.tm_gmtoff = 0;
#endif

  return mktime(&ptime);
}

static char datemsg[40];

char *
BAtime(clock)
  time_t *clock;
{
  struct tm *t = localtime(clock);
  sprintf(datemsg, "%04d/%02d/%02d %02d:%02d:%02d",
  t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
  t->tm_hour, t->tm_min, t->tm_sec);
  return (datemsg);
}

void
credit_fpath(fpath, userid, fname)
  char *fpath;
  char *userid;
  char *fname;
{
  char buf[IDLEN + 1];

  str_lower(buf, userid);

  if (fname)
    sprintf(fpath, "usr/%c/%s/CREDIT/%s", buf[0], buf, fname);
  else
    sprintf(fpath, "usr/%c/%s/CREDIT", buf[0], buf);
}

void
credit_stampfpath(fpath, stamp)
  char *fpath;
  time_t stamp;
{
  char xname[32];

  xname[0] = 'F';
  archiv32(stamp, xname + 1);
  credit_fpath(fpath, cuser.userid, xname);
}

int
credit_edit(credit, echo)
  CREDIT *credit;
  int echo;
{
  char buf_command[3] = {0}, buf[80], month, day;
  char *menu[] = {buf_command,
      "1  支出",
      "2  收入",
      "Q  取消", NULL};
  char *menu2[] = {buf_command,
      "1  其他",
      "2  [食]",
      "3  [衣]",
      "4  [住]",
      "5  [行]",
      "6  [育]",
      "7  [樂]",
      "8  自訂", NULL};
  int i, year, mmday[12] = {31, 28, 31, 30, 31, 30,
                            31, 31, 30, 31, 30, 31}; /* 每個月的天數 */
  time_t now;
  struct tm *ptime;

  i = 3;
  move(i, 0);
  clrtobot();

  if (echo == GCARRY)
  {
    if (credit->flag == CREDIT_OUT)
      strcpy(buf_command, "1Q");
    else
      strcpy(buf_command, "2Q");
    ptime = localtime(&credit->chrono);
  }
  else
  {
    memset(credit, 0, sizeof(CREDIT));
    credit->flag = CREDIT_OUT;
    strcpy(buf_command, "1Q");
    now = time(NULL);
    ptime = localtime(&now);
  }

  credit->flag = pans(3, 20, "選項", menu);

  if (credit->flag == 'q')
    return 0;

  if (credit->flag == '1')
    credit->flag = CREDIT_OUT;
  else
    credit->flag = CREDIT_IN;

  move(++i, 0);
  prints("收支：%s%s\033[m",
    (credit->flag == CREDIT_OUT) ? C_CREDIT_OUT : C_CREDIT_IN,
    menu[credit->flag] + 3);

  sprintf(buf, "%04d", ptime->tm_year + 1900);
  do
  {
    if (vget(++i, 0, "日期(年)：", buf, 5, GCARRY))
      year = atoi(buf);
    else
      return 0;
  } while (year < 1);

  sprintf(buf, "%02d", ptime->tm_mon + 1);
  do
  {
    if (vget(++i, 0, "日期(月)：", buf, 3, GCARRY))
      month = atoi(buf);
    else
      return 0;
  } while (month < 1 || month > 12);

  /* 平閏年的判別 */
  if ((year % 400 == 0) ||
     (year % 100 != 0 && year % 4 == 0))
  mmday[1] = 29;

  sprintf(buf, "%02d", ptime->tm_mday);
  do
  {
    if (vget(++i, 0, "日期(日)：", buf, 3, GCARRY))
      day = atoi(buf);
    else
      return 0;
  } while (day < 1 || day > mmday[month - 1]);

  credit->chrono = make_time(year, month, day);

  if (echo == GCARRY)
    sprintf(buf, "%d", credit->money);
  do
  {
    if (vget(++i, 0,
      (credit->flag == CREDIT_OUT) ? "支出金額(元)：" : "收入金額(元)：",
      buf, 9, echo))
    {
      credit->money = atoi(buf);
    }
    else
      return 0;
  } while (credit->money < 1);

  if (credit->flag == CREDIT_OUT)        /* 支出才有記錄用途 */
  {
    if (echo == GCARRY)
      sprintf(buf_command, "%c%c",
        (credit->useway + '1'), (credit->useway + '1'));
    else
      strcpy(buf_command, "11");

    if ((credit->useway = (pans(3, 20, "用途", menu2) - '1')) == 7)
    {
      if (!(vget(++i, 0, "用途：", credit->desc_way, BCLEN, echo)))
        return 0;
    }
    else
    {
      move(++i, 0);
      prints("用途：" C_CREDIT_OTHER "%s\033[m",
        menu2[credit->useway + 1] + 3);
    }
  }

  vget(++i, 0, "說明：", credit->desc, TTLEN, echo);

  return 1;
}

static int
credit_stamp(credit)
  CREDIT *credit;
{
  char fpath[64];
  int fd;

  credit_stampfpath(fpath, credit->stamp);
  if ((fd = open(fpath, O_WRONLY | O_CREAT | O_EXCL, 0600)) >= 0)
    close(fd);

  return fd;
}

static int
credit_add(xo, ctype)
  XO *xo;
  int ctype;
{
  CREDIT credit;
  int ans;
  char *menu[] = {"AQ",
      "A  記帳資料",
      "F  資料夾",
      "G  海錨功\能",
      "Q  離開", NULL};

  char *menu2[] = {"AQ",
      "A  加到最後",
      "I  插入目前位置",
      "N  下一個",
      "Q  離開", NULL};

  if ((ans = ctype) == 0)
    ans = pans(3, 20, "選項", menu);

  switch (ans)
  {
    case 'q':
      return XO_NONE;
    case 'a':
    case 'f':

      if (ans == 'f')
      {
        if (!(vget(b_lines, 0, "標題：", credit.desc, TTLEN, DOECHO)))
          return XO_INIT;

        time(&credit.stamp);
        credit.type = CREDIT_FOLDER;
        if (credit_stamp(&credit) < 0)
          return XO_INIT;
      }
      else
      {
        if (!(credit_edit(&credit, DOECHO)))
          return XO_INIT;

        time(&credit.stamp);
        credit.type = CREDIT_DATA;
      }

      break;
    case 'g':
      credit_anchor(xo);
      return XO_NONE;
  }

  ans = pans(3, 20, "存放位置", menu2);

  switch (ans)
  {
  case 'q':
    break;

  case 'i':
  case 'n':

    rec_ins(xo->dir, &credit, sizeof(CREDIT), xo->pos + (ans == 'n'), 1);
    break;

  case 'a':
  default:

    rec_add(xo->dir, &credit, sizeof(CREDIT));
    break;
  }

  return XO_INIT;
}

static int
credit_add_data(xo)     /* itoc.010419: 快速鍵 */
  XO *xo;
{
  return credit_add(xo, 'a');
}


static int
credit_add_folder(xo)       /* itoc.010419: 快速鍵 */
  XO *xo;
{
  return credit_add(xo, 'f');
}

static int
credit_change(xo)
  XO *xo;
{
  CREDIT *credit, old_credit;
  int pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  credit = (CREDIT *) xo_pool + cur;

  if (credit->type & CREDIT_DATA)
  {
    old_credit = *credit;

    if (credit_edit(&old_credit, GCARRY))
    {
      if (memcmp(credit, &old_credit, sizeof(CREDIT)))
      {
        if (vans("確定要修改嗎？[N] ") == 'y')
        {
          rec_put(xo->dir, &old_credit, sizeof(CREDIT), pos, NULL);
          return XO_LOAD;
        }
      }
    }
  }

  return XO_BODY;
}

static int
credit_title(xo)
  XO *xo;
{
  CREDIT *credit, old_credit;
  int pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  credit = (CREDIT *) xo_pool + cur;

  old_credit = *credit;
  if (old_credit.type & CREDIT_FOLDER)
  {
    if (!(vget(b_lines, 0, "標題：", old_credit.desc, TTLEN, GCARRY)))
      return XO_FOOT;
  }
  else if (old_credit.type & CREDIT_DATA)
    vget(b_lines, 0, "說明：", old_credit.desc, TTLEN, GCARRY);

  if (memcmp(credit, &old_credit, sizeof(CREDIT)))
    rec_put(xo->dir, &old_credit, sizeof(CREDIT), pos, NULL);

  return XO_LOAD;
}

static int
credit_sum(xo)
  XO *xo;
{
  CREDIT *credit;
  struct stat st;
  int fd, ch, i;
  int qyear, qafter, stop_sum;
  time_t qtime_src, qtime_desc;
  struct tm *ptime;
  char *menu[] = {"QQ",
      "1  全部統計",
      "2  依年份統計",
      "3  依年月統計",
      "4  依年月日期間隔統計",
      "Q  取消", NULL};
  char qmonth, qday, buf[100];
  usint way[8], moneyin, moneyout;
  int mmday[12] = {31, 28, 31, 30, 31, 30,
                   31, 31, 30, 31, 30, 31}; /* 每個月的天數 */

  ch = pans(3, 20, "統計方式", menu);

  switch (ch)
  {
    case 'q':
      return XO_NONE;
    case '2':
    case '3':
    case '4':

      i = 3;
      move(i++, 0);
      clrtobot();

      move(i++, 0);
      prints("統計方式：" C_CREDIT_TITLE "%s\033[m", menu[ch - '0'] + 3);

      credit = (CREDIT *) xo_pool + xo->pos - xo->top;

      if (credit->type & CREDIT_FOLDER)
        ptime = localtime(&credit->stamp);
      else
        ptime = localtime(&credit->chrono);

      sprintf(buf, "%04d", ptime->tm_year + 1900);
      do
      {
        if (vget(++i, 0, "[篩選條件] 年份：", buf, 5, GCARRY))
          qyear = atoi(buf);
        else
          return XO_FOOT;
      } while (qyear < 1);

      if (ch == '2')
        break;

      sprintf(buf, "%02d", ptime->tm_mon + 1);
      do
      {
        if (vget(++i, 0, "[篩選條件] 月份：", buf, 3, GCARRY))
          qmonth = atoi(buf);
        else
          return XO_FOOT;
      } while (qmonth < 1 || qmonth > 12);

      if (ch == '3')
        break;

      /* 平閏年的判別 */
      if (((qyear + 1911) % 400 == 0) ||
        ((qyear + 1911) % 100 != 0 && (qyear + 1911) % 4 == 0))
      mmday[1] = 29;

      sprintf(buf, "%02d", ptime->tm_mday);
      do
      {
        if (vget(++i, 0, "[篩選條件] 日期：", buf, 3, GCARRY))
          qday = atoi(buf);
        else
          return XO_FOOT;
      } while (qday < 1 || qday > mmday[qmonth - 1]);

      qtime_src = make_time(qyear, qmonth, qday);

      qafter = 7;
      sprintf(buf, "%d", qafter);
      do
      {
        if (vget(++i, 0, "[篩選條件] 日期間隔天數：", buf, 3, GCARRY))
          qafter = atoi(buf);
        else
          qafter = 0;
      } while (qafter < 0 || qday > 100);

      qtime_desc = qtime_src + qafter * 86400;
  }

  if ((fd = open(xo->dir, O_RDONLY)) >= 0 && !fstat(fd, &st) && st.st_size > 0)
  {
    memset(way, 0, sizeof(way));
    moneyin = 0;
    moneyout = 0;

    mgets(-1);
    while (credit = mread(fd, sizeof(CREDIT)))
    {
      stop_sum = 0;

      if (credit->type & CREDIT_FOLDER)
        continue;

      ptime = localtime(&credit->chrono);

      switch (ch)
      {
        case '3':
          stop_sum = ((stop_sum == 0) &&
                     ((ptime->tm_mon + 1) == qmonth)) ? 0 : 1;
        case '2':
          stop_sum = ((stop_sum == 0) &&
                     ((ptime->tm_year + 1900) == qyear)) ? 0 : 1;
          break;
        case '4':
          stop_sum = ((stop_sum == 0) && ((credit->chrono >= qtime_src) &&
                     (credit->chrono <= qtime_desc))) ? 0 : 1;
          break;
      }

      if (stop_sum == 0)
      {
        if (credit->flag == CREDIT_OUT)
          way[credit->useway] += credit->money;
        else
          moneyin += credit->money;
      }
    }
  }
  close(fd);

  for (i = 0; i <= 7; i++)
    moneyout += way[i];

  i = 3;

  move(i, 0);
  clrtobot();
  move(++i, 0);

  switch (ch)
  {
    case '1':
      strcpy(buf, C_CREDIT_TITLE "全部統計結果");
      break;
    case '2':
      sprintf(buf, C_CREDIT_N " %d " C_CREDIT_TITLE "年統計結果", qyear);
      break;
    case '3':
      sprintf(buf, C_CREDIT_N " %d " C_CREDIT_TITLE "年"
                   C_CREDIT_N " %d " C_CREDIT_TITLE "月份統計結果",
        qyear, qmonth);
      break;
    case '4':
      {
        struct tm *ptime;

        ptime = localtime(&qtime_src);
        prints(C_CREDIT_N " %d " C_CREDIT_TITLE "年"
               C_CREDIT_N " %d " C_CREDIT_TITLE "月"
               C_CREDIT_N " %d " C_CREDIT_TITLE "日～",
	       ptime->tm_year + 1900, ptime->tm_mon + 1, ptime->tm_mday);

        ptime = localtime(&qtime_desc);
	
        sprintf(buf, C_CREDIT_N " %d " C_CREDIT_TITLE "年"
                     C_CREDIT_N " %d " C_CREDIT_TITLE "月"
                     C_CREDIT_N " %d " C_CREDIT_TITLE "日期間隔統計結果",
		     ptime->tm_year + 1900, ptime->tm_mon + 1, ptime->tm_mday);

        break;
      }
  }
  
  prints("%s\033[m\n" BCOLOR "%s\033[m\n", buf, msg_seperator);
  prints(C_CREDIT_IN "總收入  %12u\033[m 元\n", moneyin);
  prints(C_CREDIT_OUT "總支出  %12u\033[m 元\n", moneyout);

  if (moneyin > moneyout)
    prints(BCOLOR "%s\033[m\n" C_CREDIT_IN "  結餘  %12u\033[m 元\n\n", msg_seperator, moneyin - moneyout);
  else if (moneyin < moneyout)
    prints(BCOLOR "%s\033[m\n" C_CREDIT_OUT "  透支  %12u\033[m 元\n\n", msg_seperator, moneyout - moneyin);
  else
    prints(BCOLOR "%s\033[m\n" C_CREDIT_N "  打平  %12u\033[m 元\n\n", msg_seperator, 0);

  prints(C_CREDIT_TITLE "%s\033[m\n" BCOLOR "%s\033[m\n", "支出類別明細", msg_seperator);
  prints(C_CREDIT_OTHER " [食]   " C_CREDIT_OUT "%12u\033[m 元"
     C_CREDIT_OTHER "     [衣]   " C_CREDIT_OUT "%12u\033[m 元\n",
     way[CREDIT_EAT], way[CREDIT_WEAR]);
  prints(C_CREDIT_OTHER " [住]   " C_CREDIT_OUT "%12u\033[m 元"
     C_CREDIT_OTHER "     [行]   " C_CREDIT_OUT "%12u\033[m 元\n",
     way[CREDIT_LIVE], way[CREDIT_MOVE]);
  prints(C_CREDIT_OTHER " [育]   " C_CREDIT_OUT "%12u\033[m 元"
     C_CREDIT_OTHER "     [樂]   " C_CREDIT_OUT "%12u\033[m 元\n",
     way[CREDIT_EDU], way[CREDIT_PLAY]);
  prints(C_CREDIT_OTHER " 其他   " C_CREDIT_OUT "%12u\033[m 元"
     C_CREDIT_OTHER "     自訂   " C_CREDIT_OUT "%12u\033[m 元\n",
     way[CREDIT_OTHER], way[CREDIT_CUSTOM]);
  prints(BCOLOR "%s\033[m\n", msg_seperator);

  vmsg(NULL);

  return XO_BODY;
}

static int
credit_query(xo)
  XO *xo;
{
  CREDIT *credit;
  int pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  credit = (CREDIT *) xo_pool + cur;

  if (credit->type & CREDIT_FOLDER)
  {
    char fpath[64];
    char title[TTLEN + 1];

    str_ncpy(title, credit->desc, sizeof(title));
    credit_stampfpath(fpath, credit->stamp);
    XoCredit(fpath, title);
    return XO_INIT;
  }
  else if (credit->type & CREDIT_DATA)
  {
    char *menu[] = {"QQ",
        "A  新增記帳資料",
        "F  新增資料夾",
        "T  修改記帳資料說明",
        "E  修改記帳資料",
        "S  記帳資料統計",
        "Q  取消", NULL};

    switch (pans(3, 20, "功\能選單", menu))
    {
      case 'a':
        return credit_add_data(xo);
      case 'f':
        return credit_add_folder(xo);
      case 't':
        return credit_title(xo);
      case 'e':
        return credit_change(xo);
      case 's':
        return credit_sum(xo);
      default:
        return XO_NONE;
    }
  }
}

static void
delcredit(xo, credit)
  XO *xo;
  CREDIT *credit;
{
  if (credit->type & CREDIT_FOLDER)
  {
    char fpath[64];

    credit_stampfpath(fpath, credit->stamp);
    if (str_ncmp(CreditAnchor, fpath, sizeof(CreditAnchor)) == 0)
      CreditAnchor[0] = '\0';
    credit_do_delete(xo, fpath);
  }
}

static void
credit_do_delete(xo, folder)
  XO *xo;
  char *folder;
{
  CREDIT credit;
  FILE *fp;

  if (!(fp = fopen(folder, "r")))
    return;

  while (fread(&credit, sizeof(CREDIT), 1, fp) == 1)
    delcredit(xo, &credit);

  fclose(fp);
  unlink(folder);
}

static int
credit_delete(xo)
  XO *xo;
{
  if (vans(msg_del_ny) == 'y')
  {
    CREDIT *credit;

    credit = (CREDIT *) xo_pool + (xo->pos - xo->top);
    delcredit(xo, credit);

    if (!rec_del(xo->dir, sizeof(CREDIT), xo->pos, NULL))
      return XO_LOAD;
  }
  return XO_FOOT;
}

static int
credit_rangedel(xo)
  XO *xo;
{
  return xo_rangedel(xo, sizeof(CREDIT), NULL, delcredit);
}


static int
vfycredit(credit, pos)
  CREDIT *credit;
  int pos;
{
  return Tagger(credit->stamp, pos, TAG_NIN);
}


static int
credit_prune(xo)
  XO *xo;
{
  return xo_prune(xo, sizeof(CREDIT), vfycredit, delcredit);
}

static void
credit_item(num, credit)
  int num;
  CREDIT *credit;
{
  char *credit_way[] =
      {"其他", "[食]", "[衣]", "[住]", "[行]", "[育]", "[樂]", NULL};

  if (credit->type & CREDIT_FOLDER)
    prints("%6d %c" "%-*.*s\033[m",
      num, tag_char(credit->stamp), d_cols + 68, d_cols + 68,
      credit->desc);
  else if (credit->type & CREDIT_DATA)
  {
    credit_way[7] = credit->desc_way;
    prints("%6d %c%-10.10s  %s %8d  " C_CREDIT_OTHER "%4s %-*.*s\033[m",
      num, tag_char(credit->stamp),
      BAtime(&credit->chrono),
      (credit->flag == CREDIT_OUT) ? C_CREDIT_OUT "支出" : C_CREDIT_IN "收入",
      credit->money,
      (credit->flag == CREDIT_OUT) ? credit_way[credit->useway] : "",
      d_cols + 38, d_cols + 38,
      credit->desc);
  }
}

static int
credit_body(xo)
  XO *xo;
{
  CREDIT *credit;
  int max, num, tail;

  move(3, 0);
  clrtobot();
  max = xo->max;
  if (max <= 0)
  {
    if ((max = credit_add(xo, 0)) != XO_NONE)
      return max;

    return XO_QUIT;
  }

  credit = (CREDIT *) xo_pool;
  num = xo->top;
  tail = num + XO_TALL;
  if (max > tail)
    max = tail;

  do
  {
    move(3 + num - xo->top, 0);
    credit_item(++num, credit++);
  } while (num < max);

  return XO_FOOT;   /* itoc.010403: 把 b_lines 填上 feeter */
}


static int
credit_head(xo)
  XO *xo;
{
  vs_head("記帳手札", xo->xyz);
  prints(NECKER_CREDIT, d_cols, "");

  return credit_body(xo);
}


static int
credit_load(xo)
  XO *xo;
{
  xo_load(xo, sizeof(CREDIT));
  return credit_body(xo);
}


static int
credit_init(xo)
  XO *xo;
{
  xo_load(xo, sizeof(CREDIT));
  return credit_head(xo);
}

static int
credit_tag(xo)
  XO *xo;
{
  CREDIT *credit;
  int tag, pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  credit = (CREDIT *) xo_pool + cur;

  if (tag = Tagger(credit->stamp, pos, TAG_TOGGLE))
  {
    move(3 + cur, 0);
    credit_item(++pos, credit);
  }

  return xo->pos + 1 + XO_MOVE; /* lkchu.981201: 跳至下一項 */
}

static int
credit_cmp(a, b)
  CREDIT *a, *b;
{
  int iSeq, jSeq;
  int i, j;

  iSeq = (a->type & CREDIT_FOLDER) ? 1 : 0;
  jSeq = (b->type & CREDIT_FOLDER) ? 1 : 0;

  i = j = 0;

  if (!iSeq)
    i = a->chrono;

  if (!jSeq)
    j = b->chrono;

  if (!iSeq && !jSeq)
    return (i == j) ? str_cmp(a->desc, b->desc) : (i - j);
  else if (!jSeq)
    return -1;
  else if (!iSeq)
    return 1;
  else
    return str_cmp(a->desc, b->desc);
}

static int
credit_sort(xo)
  XO *xo;
{
  if (vans("執行資料排序嗎？[N] ") == 'y')
  {
    rec_sync(xo->dir, sizeof(CREDIT), credit_cmp, NULL);
    return credit_load(xo);
  }
  else
    return XO_NONE;
}


static int
credit_move(xo)
  XO *xo;
{
  CREDIT *credit;
  int pos, cur, i;
  char buf[40], ans[5];

  pos = xo->pos;
  cur = pos - xo->top;
  credit = (CREDIT *) xo_pool + cur;

  sprintf(buf, "請輸入第 %d 選項的新位置：", pos + 1);
  if (vget(b_lines, 0, buf, ans, 5, DOECHO))
  {
    i = atoi(ans) - 1;
    if (i < 0)
      i = 0;
    else if (i >= xo->max)
      i = xo->max - 1;

    if (i != pos)
    {
      if (!rec_del(xo->dir, sizeof(CREDIT), pos, NULL))
      {
        rec_ins(xo->dir, credit, sizeof(CREDIT), i, 1);
        xo->pos = i;
        return credit_load(xo);
      }
    }
  }
  return XO_FOOT;
}

static int
Ask_creditTag(msg)
  char *msg;
/* ----------------------------------------------------- */
/* return value :                                        */
/* -1   : 取消                                           */
/* 0    : single article                                 */
/* o.w. : whole tag list                                 */
/* ----------------------------------------------------- */
{
  int num;
  char buf_command[3] = {0};
  char *menu[] = {buf_command,
      "A  此項記帳資料",
      "T  已標籤記帳資料",
      "Q  離開", NULL};


  if (num = TagNum)
    strcpy(buf_command, "TQ");
  else
    strcpy(buf_command, "AQ");

  if (num)      /* itoc.020130: 有 TagNum 才問 */
  {
    switch (pans(3, 20, msg, menu))
    {
    case 'q':
      return -1;

    case 'a':
      return 0;
    }
  }
  return num;
}


int
credit_gather(xo)
  XO *xo;
{
  CREDIT *credit, xcredit;
  int tag, locus;
  char *dir, *folder;
  time_t now;

  folder = CreditAnchor;

  if (!*folder)
  {
    zmsg("請先定錨以後再直接收錄至定錨區");
    return XO_NONE;
  }

  if (str_ncmp(folder, xo->dir, sizeof(CreditAnchor)) == 0)
  {
    zmsg("無法收錄記帳資料！\n目前資料夾為記帳手札定錨區【Ψ】。");
    return XO_NONE;
  }

  tag = Ask_creditTag("收錄至記帳手札定錨區【Ψ】");

  if (tag < 0)
    return XO_NONE;

  dir = xo->dir;
  credit = tag ? &xcredit : (CREDIT *) xo_pool + xo->pos - xo->top;

  locus = 0;
  now = time(0);

  do
  {
    if (tag)
      EnumTag(credit, dir, locus, sizeof(CREDIT));

    if (credit->type & CREDIT_DATA)
    {
      credit->stamp = now + locus;
      rec_add(folder, credit, sizeof(CREDIT));
    }
  } while (++locus < tag);

  zmsg("記帳資料收錄完成，但是資料夾不會被收錄。");

  return XO_NONE;
}

static int
credit_anchor(xo)
  XO *xo;
{
  char *folder;
  char *menu[] = {"QQ",
      "A  定錨",
      "D  拔錨",
      "J  就位",
      "Q  取消", NULL};


  folder = CreditAnchor;

  switch (pans(3, 20, (*folder) ? "記帳手札【Ψ】" : "記帳手札", menu))
  {
  case 'a':
    strcpy(folder, xo->dir);
    str_ncpy(CreditSailor, xo->xyz, sizeof(CreditSailor));
    zmsg("記帳手札已定錨【Ψ】");
    break;
  case 'd':
    *folder = '\0';
    zmsg("記帳手札已拔錨");
    break;
  case 'j':
    if (!*folder)                     /* 沒有定錨就進入資源回收筒 */
    {
      zmsg("記帳手札尚未定錨！");
      return XO_NONE;
    }

    if (str_ncmp(folder, xo->dir, sizeof(CreditAnchor)) == 0)
      return XO_NONE;

    XoCredit(folder, CreditSailor);
    return XO_INIT;
    break;
  }

  /* return XO_NONE; */
  return XO_NONE;   /* itoc.010726: 把 b_lines 填上 feeter */
}

static int
credit_help(xo)
  XO *xo;
{
  xo_help("credit");
  return XO_HEAD;
}


static KeyFunc credit_cb[] =
{
  XO_INIT, credit_init,
  XO_LOAD, credit_load,
  XO_HEAD, credit_head,
  XO_BODY, credit_body,

  'r', credit_query,
  'a', credit_add_data,
  'f', credit_add_folder,
  Ctrl('D'), credit_prune,
  Ctrl('G'), credit_anchor,
  'E', credit_change,
  'T', credit_title,
  's', credit_sum,
  'd', credit_delete,
  'D', credit_rangedel,
  't', credit_tag,
  'g', credit_gather,
  'm', credit_move,
  'S', credit_sort,
  'h', credit_help
};

int
f_exists(fpath)
  char *fpath;
{
  int fd, fsize, ret;
  struct stat st;

  ret = 0;
  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    if (fstat(fd, &st) || (fsize = st.st_size) > 0)
      ret = 1;

    close(fd);
  }

  return ret;
}

static int
check_old_credit(void)
{
  int i;
  char fpath[64], f_desc[64];
  char buf[80];
  CREDIT_OLD credit_old;
  CREDIT credit;

  usr_fpath(fpath, cuser.userid, FN_CREDIT);
  if (!(f_exists(fpath)))
    return 1;

  if (vans("使用前必須先轉換舊版記帳手札資料，轉檔嗎？[N] ") == 'y')
  {
    time_t now;

    credit_fpath(f_desc, cuser.userid, FN_CREDIT);
    i = 0;

    now = time(0);
    while (!(rec_get(fpath, &credit_old, sizeof(CREDIT_OLD), i)))
    {
        memset(&credit, 0, sizeof(CREDIT));
        credit.stamp = now + i;
        credit.type = CREDIT_DATA;
        credit.chrono = make_time(credit_old.year,
                                 credit_old.month,
                                 credit_old.day);
        credit.flag = credit_old.flag;
        credit.money = credit_old.money;
        credit.useway = credit_old.useway;
        str_ncpy(credit.desc, credit_old.desc, sizeof(credit.desc));
        rec_add(f_desc, &credit, sizeof(CREDIT));
        i++;
    }

    sprintf(buf, "rm %s", fpath);
    system(buf);
    return 1;
  }
  return 0;
}

void
XoCredit(folder, title)
  char *folder;
  char *title;
{
  XO *xo, *last;

  last = xz[XZ_CREDIT - XO_ZONE].xo;    /* record */

  xz[XZ_CREDIT - XO_ZONE].xo = xo = xo_new(folder);
  xz[XZ_CREDIT - XO_ZONE].cb = credit_cb;
  xo->xyz = title;

  xover(XZ_CREDIT);
  free(xo);

  xz[XZ_CREDIT - XO_ZONE].xo = last;    /* restore */
}

int
credit_main()
{
  char fpath[64];

  credit_fpath(fpath, cuser.userid, NULL);
  if (mkdir(fpath, 0700) != 0 && errno != EEXIST)
  {
    vmsg("建立記帳手札目錄時發生錯誤，請向站長提出");
    return XEASY;
  }

  if (check_old_credit())
  {
    credit_fpath(fpath, cuser.userid, FN_CREDIT);
    XoCredit(fpath, "主頁");
  }

  return 0;
}

#endif              /* HAVE_CREDIT */

