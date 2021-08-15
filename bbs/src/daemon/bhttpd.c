/*-------------------------------------------------------*/
/* bhttpd.c        ( NTHU CS MapleBBS Ver 3.10 )         */
/*-------------------------------------------------------*/
/* target : BBS's HTTP daemon                            */
/* create : 05/07/11                                     */
/* update : 06/19/12                                     */
/* author : guessi.bbs@bbs.ndhu.edu.tw                   */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/* author : yiting.bbs@bbscs.tku.edu.tw                  */
/*-------------------------------------------------------*/

#define _MODES_C_

#include "bbs.h"
#include <sys/wait.h>
#include <netinet/tcp.h>
#include <sys/resource.h>

#define SERVER_USAGE
#define LOG_VERBOSE                        /* 詳細紀錄 */
#define BHTTP_PIDFILE    "run/bhttp.pid"
#define BHTTP_LOGFILE    "run/bhttp.log"
#define BHTTP_PERIOD     (60 * 5)          /* 每 5 分鐘 check 一次 */
#define BHTTP_TIMEOUT    (60 * 3)          /* 超過 3 分鐘的連線就視為錯誤 */
#define BHTTP_FRESH      86400             /* 每 1 天整理一次 log 檔 */
#define TCP_BACKLOG      3
#define TCP_RCVSIZ       2048
#define MIN_DATA_SIZE    (TCP_RCVSIZ + 3)
#define MAX_DATA_SIZE    262143            /* POST 的大小限制(byte) */
#define HTML_TALL        20

/* Thor.000425: POSIX 用 O_NONBLOCK */
#ifndef O_NONBLOCK
#  define M_NONBLOCK     FNDELAY
#else
#  define M_NONBLOCK     O_NONBLOCK
#endif

#ifdef HAVE_OPENGRAPH
#  define OG_TITLE       "東方小城 BBS 網頁版"
#  define OG_URL         "http://" MYHOSTNAME 
#  define OG_IMAGE       OG_URL "/link?thumbnail.png"
#  define OG_DESCRIPTION "滑鼠輕易瀏覽分享公開資訊"
#  define OG_TYPE        "school"
#endif

#ifdef HAVE_GOOGLE_ANALYTICS
#  define GA_UID         "UA-30484107-1"
#endif

/* ----------------------------------------------------- */
/* HTTP commands                                         */
/* ----------------------------------------------------- */

typedef struct
{
  int (*func) ();
  char *cmd;
  int len;            /* strlen(Command.cmd) */
}      Command;


/* ----------------------------------------------------- */
/* client connection structure                           */
/* ----------------------------------------------------- */

typedef struct Agent
{
  struct Agent *anext;
  int sock;

  unsigned int ip_addr;

  time_t tbegin;        /* 連線開始時間 */
  time_t uptime;        /* 上次下指令的時間 */

  char url[48];         /* 欲瀏覽的網頁 */
  char *urlp;

  char modified[30];

  usint userlevel;

  /* 所能看到的看板列表或使用者名單 */

#if MAXBOARD > MAXACTIVE
  void *myitem[MAXBOARD];
#else
  void *myitem[MAXACTIVE];
#endif
  int total_item;

  /* input 用 */

  char *data;
  int size;            /* 目前 data 所 malloc 的空間大小 */
  int used;

  /* output 用 */

  FILE *fpw;
}     Agent;


/* ----------------------------------------------------- */
/* http state code                                       */
/* ----------------------------------------------------- */

enum
{
  HS_END,

  HS_ERROR,                /* 語法錯誤 */
  HS_ERR_MORE,             /* 文章讀取錯誤 */
  HS_ERR_BOARD,            /* 看板讀取錯誤 */
  HS_ERR_MAIL,             /* 信件讀取錯誤 */
  HS_ERR_CLASS,            /* 分類讀取錯誤 */
  HS_ERR_PERM,             /* 權限不足 */

  HS_OK,

  HS_REDIRECT,             /* 重新導向 */
  HS_NOTMOIDIFY,           /* 檔案沒有變更 */
  HS_BADREQUEST,           /* 錯誤的要求 */
  HS_FORBIDDEN,            /* 未授權的頁面 */
  HS_NOTFOUND,             /* 找不到檔案 */

  LAST_HS
};


static char *http_msg[LAST_HS] =
{
  NULL,

  "語法錯誤",
  "操作錯誤：您所選取的文章不存在或已刪除",
  "操作錯誤：無此看板或您的權限不足",
  "操作錯誤：無此信件或您尚未登入",
  "操作錯誤：您所選取的分類不存在或已刪除",
  "操作錯誤：您尚未登入或權限不足，無法進行這項操作",

  "200 OK",

  "302 Found",
  "304 Not Modified",
  "400 Bad Request",
  "403 Forbidden",
  "404 Not Found",
};


#define HS_REFRESH    0x0100    /* 自動跳頁(預設是3秒) */


/* ----------------------------------------------------- */
/* AM : Agent Mode                                       */
/* ----------------------------------------------------- */

#define AM_GET        0x010
#define AM_POST       0x020


/* ----------------------------------------------------- */
/* operation log and debug information                   */
/* ----------------------------------------------------- */
/* @START | ... | time                                   */
/* ----------------------------------------------------- */

static FILE *flog;

extern int errno;
extern char *crypt();

static void
log_fresh()
{
  int count;
  char fsrc[64], fdst[64];
  char *fpath = BHTTP_LOGFILE;

  if (flog)
    fclose(flog);

  count = 9;
  do
  {
    sprintf(fdst, "%s.%d", fpath, count);
    sprintf(fsrc, "%s.%d", fpath, --count);
    rename(fsrc, fdst);
  } while (count);

  rename(fpath, fsrc);
  flog = fopen(fpath, "a");
}


static void
logit(key, msg)
  char *key;
  char *msg;
{
  time_t now;
  struct tm *p;

  time(&now);
  p = localtime(&now);
  /* Thor.990329: y2k */
  fprintf(flog,
          "%s\t%s\t%02d/%02d/%02d %02d:%02d:%02d\n",
          key, msg, p->tm_year % 100, p->tm_mon + 1, p->tm_mday,
          p->tm_hour, p->tm_min, p->tm_sec);
}


static void
log_open()
{
  FILE *fp;

  umask(077);

  if ((fp = fopen(BHTTP_PIDFILE, "w")))
  {
    fprintf(fp, "%d\n", getpid());
    fclose(fp);
  }

  flog = fopen(BHTTP_LOGFILE, "a");
  logit("START", "MTA daemon");
}


/* ----------------------------------------------------- */
/* target : ANSI text to HTML tag                        */
/* author : yiting.bbs@bbs.cs.tku.edu.tw                 */
/* ----------------------------------------------------- */

#define    ANSI_TAG       27
#define    is_ansi(ch)    ((ch >= '0' && ch <= '9') || ch == ';' || ch == '[')

#define    HAVE_HYPERLINK         /* 處理超連結 */
#define    HAVE_SAKURA            /* 櫻花日文自動轉Unicode */

static int old_color, now_color;
static char ansi_buf[1024];       /* ANSILINELEN * 4 */


#ifdef HAVE_HYPERLINK

static uschar *linkEnd = NULL;

static void
txt2hyperlink(fpw, src)
  FILE *fpw;
  uschar *src;
{
  int ch;
  char link_buf[128];
  uschar *tmp = link_buf;
  *tmp = '\0';

  linkEnd = src;

  while ((ch = *linkEnd))
  {
    if (ch < '#' || ch > '~' || ch == '<' || ch == '>' || ch == '@' || ch == '\\')
      break;
    linkEnd++;
    *tmp++ = ch;
  }
  *tmp = '\0';

  /* image link restriction: link must end with ".ext" */
  if ((strlen(link_buf) - (strrchr(link_buf, '.') - link_buf) == 4) &&
      (strstr(link_buf, ".jpg") || strstr(link_buf, ".png") || strstr(link_buf, ".gif")))
  {
      fprintf(fpw, "<a target=\"_blank\" href=\"%s\">", link_buf);
      fprintf(fpw, "<img src=\"%s\" alt=\"%s\"><br />", link_buf, link_buf);
  }
  else
    if (!strncmp(link_buf, "http://www.youtube.com/watch?", 29))
  {
    char video_id[16], *p, *q;

    p = q = NULL;

    if (p = strstr(link_buf, "v="))
    {
      if (q = strchr(p, '&'))
        strncpy(video_id, p + 2, q - (p + 2));
      else
        strcpy(video_id, p + 2);
    }

    if (strlen(video_id) >= 10)
    {
      fprintf(fpw, "<div class=\"youtube-container\">"
                   "  <iframe class=\"youtube-player\" type=\"text/html\""
                   " width=\"640\" height=\"385\" frameborder=\"0\""
                   " src=\"http://www.youtube.com/embed/%s\"></iframe>"
                   "</div>", video_id);
      fprintf(fpw, "影音連結: <a target=\"_blank\" href=\"%s\">", link_buf);
    }
  }
  else
  {
    fprintf(fpw, "<a target=\"_blank\" href=\"%s\">", link_buf);
  }
}
#endif


#ifdef HAVE_SAKURA
static int
sakura2unicode(code)
  int code;
{
  if (code > 0xC6DD && code < 0xC7F3)
  {
    if (code > 0xC7A0)
      code -= 38665;
    else if (code > 0xC700)
      code -= 38631;
    else if (code > 0xC6E6)
      code -= 38566;
    else if (code == 0xC6E3)
      return 0x30FC;
    else
      code -= 38619;
    if (code > 0x3093)
      code += 13;
    return code;
  }
  return 0;
}
#endif


static int
ansi_remove(psrc)
  uschar **psrc;
{
  uschar *src = *psrc;
  int ch = *src;

  while (is_ansi(ch))
    ch = *(++src);

  if (ch && ch != '\n')
    ch = *(++src);

  *psrc = src;
  return ch;
}


static int
ansi_color(psrc)
  uschar **psrc;
{
  uschar *src, *ptr;
  int ch, value;
  int color = old_color;
  uschar *cptr = (uschar *) & color;

  src = ptr = (*psrc) + 1;

  ch = *src;
  while (ch)
  {
    if (ch == ';' || ch == 'm')
    {
      *src = '\0';
      value = atoi(ptr);
      ptr = src + 1;
      if (value == 0)
        color = 0x00003740;
      else if (value >= 30 && value <= 37)
        cptr[1] = value + 18;
      else if (value >= 40 && value <= 47)
        cptr[0] = value + 24;
      else if (value == 1)
        cptr[2] = 1;

      if (ch == 'm')
      {
        now_color = color;

        ch = *(++src);
        break;
      }
    }
    else if (ch < '0' || ch > '9')
    {
      ch = *(++src);
      break;
    }
    ch = *(++src);
  }

  *psrc = src;
  return ch;
}

static void
ansi_classname(fpw, color)
  FILE *fpw;
  int color;
{
  char tmp[6];
  sprintf(tmp, "%05X", color);
  fprintf(fpw, "f%.3s b%2s", tmp, tmp + 3);
}

static int is_newline = 1;

static void
ansi_tag(fpw)
  FILE *fpw;
{
  /* 顏色不同才需要印出 */
  if (old_color != now_color)
  {
    if (is_newline == 0)
      fputs("</span>", fpw);
    fputs("<span class=\"", fpw);
    ansi_classname(fpw, now_color);
    fputs("\">", fpw);

    old_color = now_color;
    is_newline = 0;
  }
}

static void
handle_line(fpw, src)
  FILE *fpw;
  uschar *src;
{
  int ch1, ch2;
  int has_ansi = 0;

#ifdef HAVE_SAKURA
  int scode;
#endif

  fputs("<span class=\"line\">", fpw);

  ch2 = *src;
  while (ch2)
  {
    ch1 = ch2;
    ch2 = *(++src);

    if (ch1 & 0x80)
    {
      while (ch2 == ANSI_TAG)
      {
        if (*(++src) == '[')    /* 顏色 */
        {
          ch2 = ansi_color(&src);
          has_ansi = 1;
        }
        else            /* 其他直接刪除 */
          ch2 = ansi_remove(&src);
      }
      if (ch2)
      {
        if (ch2 < ' ')        /* 怕出現\n */
          fputc(ch2, fpw);
#ifdef HAVE_SAKURA
        else if ((scode = sakura2unicode((ch1 << 8) | ch2)))
          fprintf(fpw, "&#%d;", scode);
#endif
        else
        {
          fputc(ch1, fpw);
          fputc(ch2, fpw);
        }
        ch2 = *(++src);
      }
      if (has_ansi)
      {
        has_ansi = 0;
        if (ch2 != ANSI_TAG)
        {
          ansi_tag(fpw);
        }
      }
      continue;
    }
    else if (ch1 == ANSI_TAG)
    {
      do
      {
        if (ch2 == '[')             /* 顏色 */
          ch2 = ansi_color(&src);
        else if (ch2 == '*')        /* 控制碼 */
          fputc('*', fpw);
        else                        /* 其他直接刪除 */
          ch2 = ansi_remove(&src);
      } while (ch2 == ANSI_TAG && (ch2 = *(++src)));

      ansi_tag(fpw);

      continue;
    }
    /* 剩下的字元做html轉換 */
    if (ch1 == '<')
    {
      fputs("&lt;", fpw);
    }
    else if (ch1 == '>')
    {
      fputs("&gt;", fpw);
    }
    else if (ch1 == ' ')
    {
      fputs("&nbsp;", fpw);
    }
    else if (ch1 == '\n')
    {
      if (!is_newline)
        fputs("</span>", fpw);

      fputs("</span>\n", fpw);

      is_newline = 1;
    }
    else if (ch1 == '&')
    {
      fputc(ch1, fpw);
      if (ch2 == '#')        /* Unicode字元不轉換 */
      {
        fputc(ch2, fpw);
        ch2 = *(++src);
      }
      else if (ch2 >= 'A' && ch2 <= 'z')
      {
        fputs("amp;", fpw);
        fputc(ch2, fpw);
        ch2 = *(++src);
      }
    }
#ifdef HAVE_HYPERLINK
    else if (linkEnd)        /* 處理超連結 */
    {
      fputc(ch1, fpw);
      if (linkEnd <= src)
      {
        fputs("</a>", fpw);
        linkEnd = NULL;
      }
    }
#endif
    else
    {
#ifdef HAVE_HYPERLINK
      /* 其他的自己加吧 :) */
      if (!str_ncmp(src - 1, "http://", 7))
        txt2hyperlink(fpw, src - 1);
      else if (!str_ncmp(src - 1, "https://", 8))
        txt2hyperlink(fpw, src - 1);
      else if (!str_ncmp(src - 1, "telnet://", 9))
        txt2hyperlink(fpw, src - 1);
#endif
      fputc(ch1, fpw);
    }
  }
}


static char *
str_html(src, len)
  uschar *src;
  int len;
{
  int in_chi, ch;
  uschar *dst = ansi_buf, *end = src + len;

  ch = *src;
  while (ch && src < end)
  {
    if (ch & 0x80)
    {
      in_chi = *(++src);
      while (in_chi == ANSI_TAG)
      {
        src++;
        in_chi = ansi_remove(&src);
      }

      if (in_chi)
      {
        if (in_chi < ' ')    /* 可能只有半個字，前半部就不要了 */
          *dst++ = in_chi;
#ifdef HAVE_SAKURA
        else if (len = sakura2unicode((ch << 8) + in_chi))
        {
          sprintf(dst, "&#%d;", len);    /* 12291~12540 */
          dst += 8;
        }
#endif
        else
        {
          *dst++ = ch;
          *dst++ = in_chi;
        }
      }
      else
        break;
    }
    else if (ch == ANSI_TAG)
    {
      src++;
      ch = ansi_remove(&src);
      continue;
    }
    else if (ch == '<')
    {
      strcpy(dst, "&lt;");
      dst += 4;
    }
    else if (ch == '>')
    {
      strcpy(dst, "&gt;");
      dst += 4;
    }
    else if (ch == '\n')
    {
      /* skip */
    }
    else if (ch == '&')
    {
      ch = *(++src);
      if (ch == '#')
      {
        if ((uschar *) strchr(src + 1, ';') >= end)    /* 可能會不是或長度沒超過 */
          break;
        *dst++ = '&';
        *dst++ = '#';
      }
      else
      {
        strcpy(dst, "&amp;");
        dst += 5;
        continue;
      }
    }
    else
      *dst++ = ch;
    ch = *(++src);
  }

  *dst = '\0';
  return ansi_buf;
}

static int
is_quote(fpw, src)
  FILE *fpw;
  uschar *src;
{
  int ch1 = src[0];
  int ch2 = src[1];

  if (!strncmp(src, "※", 2))
    now_color = 0x00013640;
  else if ((ch2 == ' ') && ((ch1 == QUOTE_CHAR1) || (ch1 == QUOTE_CHAR2)))
    now_color = ((ch2 == QUOTE_CHAR1) || (ch2 == QUOTE_CHAR2)) ? 0x00003540 : 0x00003640;
  else
    return 0;

  fputs("<span class=\"", fpw);
  ansi_classname(fpw, now_color);
  fputs("\">", fpw);
  fputs(str_html(src, ANSILINELEN), fpw);
  fputs("</span>\n", fpw);

  /* restore */
  now_color = 0x00003740;

  return 1;
}

static void
txt2htm(fpw, fp)
  FILE *fpw;
  FILE *fp;
{
  int i;
  static const char header1[LINE_HEADER][LEN_AUTHOR1] = {"作者", "標題", "時間"};
  static const char header2[LINE_HEADER][LEN_AUTHOR2] = {"發信人", "標  題", "發信站"};
  char *headvalue, *pbrd, *board;
  char buf[ANSILINELEN];

  fputs("<div class=\"article\">\n<br />\n", fpw);
  fputs("  <div class=\"article_header\">\n", fpw);

  /* 處理檔頭 */
  for (i = 0; i < LINE_HEADER; i++)
  {
    if (!fgets(buf, ANSILINELEN, fp))    /* 雖然連檔頭都還沒印完，但是檔案已經結束，直接離開 */
    {
      fputs("  </div>\n", fpw);/* end of #article_header (if empty) */
      return;
    }

    if (memcmp(buf, header1[i], LEN_AUTHOR1 - 1) &&
        memcmp(buf, header2[i], LEN_AUTHOR2 - 1))    /* 不是檔頭 */
      break;

    /* 作者/看板 檔頭有二欄，特別處理 */
    if (i == 0 && ((pbrd = strstr(buf, "看板:")) ||
                   (pbrd = strstr(buf, "站內:"))))
    {
      if ((board = strchr(pbrd, '\n')))
        *board = '\0';
      board = pbrd + 6;
      pbrd[-1] = '\0';
      pbrd[4] = '\0';
    }

    if (!(headvalue = strchr(buf, ':')))
      break;

    fprintf(fpw, "    <div>\n"
                 "      <span class=\"h_ltopl\">%s</span>\n", header1[i]);

    str_html(headvalue + 2, TTLEN);

    if (i == 0 && pbrd)
    {
      fprintf(fpw,
              "      <span class=\"h_mid\">%s</span>\n"
              "      <span class=\"h_rtopl\">%s</span>\n"
              "      <span class=\"h_rtopr\">%s</span>\n", ansi_buf, pbrd, board);
    }
    else
    {
      fprintf(fpw, "      <span class=\"h_long\">%s</span>\n", ansi_buf);
    }
    fputs("    </div>\n", fpw);
  }

  fputs("  </div>\n" /* end of #article_header */
        "  <br />\n"
        "  <div id=\"ansi_text\">\n"
        "    <pre>", fpw);

  /* restore */
  old_color = now_color = 0x00003740;

  if (i >= LINE_HEADER)        /* 最後一行是檔頭 */
    fgets(buf, ANSILINELEN, fp);

  /* 處理內文 */
  do
  {
    if (!is_quote(fpw, buf))
      handle_line(fpw, buf);
  } while (fgets(buf, ANSILINELEN, fp));

  fputs("</pre>\n"
        "  </div>\n" /* end of #ansi_text */
        "</div>\n", fpw); /* end of #aritcle */
}


/* ----------------------------------------------------- */
/* HTML output basic function                            */
/* ----------------------------------------------------- */

static char *
Gtime(time)
  time_t *time;
{
  static char datemsg[32];
  strftime(datemsg, sizeof(datemsg), "%a, %d %b %Y %T GMT", gmtime(time));
  return datemsg;
}


static FILE *
out_http(ap, code, type)
  Agent *ap;
  int code;
  char *type;
{
  time_t now, exp;
  FILE *fpw;
  int state;

  fpw = ap->fpw;
  state = code & ~HS_REFRESH;

  /* HTTP 1.0 檔頭 */
  time(&now);

  exp = now + 86400 * 30;

  fprintf(fpw,
          "HTTP/1.0 %s\r\n"
          "Date: %s\r\n"
          "Expires: %s\r\n"
          "Server: MapleBBS 3.10\r\n"
          "Connection: close\r\n", http_msg[state], Gtime(&now), Gtime(&exp));

  if (state == HS_NOTMOIDIFY)
  {
    fputs("\r\n", fpw);
  }
  else if (state == HS_REDIRECT) /* Location之後不需要內容 */
  {
#if BHTTP_PORT == 80
    fprintf(fpw, "Location: http://" MYHOSTNAME "/\r\n\r\n");
#else
    fprintf(fpw, "Location: http://" MYHOSTNAME ":%d/\r\n\r\n", BHTTP_PORT);
#endif
  }
  else
  {
    if (code & HS_REFRESH)
    {
      if (!type)
        type = "/";
      fprintf(fpw, "Refresh: 3; url=%s\r\n", type);
    }
    if ((code & HS_REFRESH) || !type)
    {
      fputs("Pragma: no-cache\r\n"    /* 網頁一律不讓proxy做cache */
            "Content-Type: text/html; charset=" MYCHARSET "\r\n", fpw);
    }
    else
      fprintf(fpw, "Content-Type: %s\r\n", type);
  }

  return fpw;
}


static void
out_error(ap, code)        /* code不可以是HS_OK */
  Agent *ap;
  int code;
{
  char *msg;

  if (code < HS_OK)
  {
    fprintf(ap->fpw, "<br />%s<br /><br />\n", http_msg[code]);
    return;
  }

  out_http(ap, code, NULL);
  switch (code)
  {
  case HS_BADREQUEST:
    msg = "Your browser sent a request that this server could not understand.";
    break;
  case HS_FORBIDDEN:
    msg = "You don't have permission to access the URL on this server.";
    break;
  case HS_NOTFOUND:
    msg = "The requested URL was not found on this server.";
    break;
  default:            /* HS_REDIRECT, HS_NOTMOIDIFY */
    return;
  }

  fprintf(ap->fpw,
          "\r\n<!DOCTYPE html>\n"
          "<html>\n"
          "<head>\n"
          "  <title>%s</title>\n"
          "</head>\n"
          "<body>\n"
          "  <h1>%s</h1>\n"
          "  %s<br />\n"
          "  <hr />\n"
          "  <address>MapleBBS Server at " MYHOSTNAME "</address>\n"
          "</body>\n"
          "</html>\n", http_msg[code], http_msg[code] + 4, msg);
}

static void
out_title(fpw, header, url, title, parameter)
  FILE *fpw;
  char *header;
  char *url;
  char *title;
  char *parameter;
{
  char og_title[256];

  sprintf(og_title, "%s%s%s%s%s", title, strlen(title) > 0 ? " - " : "",
                                  header, strlen(header) > 0 ? " - " : "", OG_TITLE);

  fprintf(fpw,
    "\r\n"
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "  <title>%s</title>\n"
    "  <meta name=\"robots\" content=\"noindex,nofollow\" />\n"
    "  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=" MYCHARSET "\" />\n", og_title);

#ifdef HAVE_OPENGRAPH
  fprintf(fpw, "  <meta property=\"og:title\" content=\"%s\" />\n"
               "  <meta property=\"og:description\" content=\"" OG_DESCRIPTION "\" />\n"
               "  <meta property=\"og:image\" content=\"" OG_IMAGE "\" />\n"
               "  <meta property=\"og:url\" content=\"" OG_URL "/%s%s%s\" />\n"
               "  <meta property=\"og:type\" content=\"" OG_TYPE "\" />\n", og_title, url, strlen(parameter) > 0 ? "&amp;" : "", parameter);
#endif

    fputs("  <link rel=\"stylesheet\" href=\"link?style.min.css\" type=\"text/css\">\n"
          "  <link rel=\"stylesheet\" href=\"link?color.min.css\" type=\"text/css\">\n"
          "  <link rel=\"icon\" href=\"link?favicon.ico\">\n"
          "</head>\n"
          "<body>\n"
          "  <div id=\"top\"></div>\n"
          "  <div id=\"container\">\n"
          "    <div id=\"content\">\n", fpw);
}

static FILE *
out_head(ap, header)
  Agent *ap;
  char *header;
{
  FILE *fpw = out_http(ap, HS_OK, NULL);
  out_title(fpw, header, ap->url, "", "");
  return fpw;
}

static FILE *
out_head_article(ap, header, title, parameter)
  Agent *ap;
  char *header;
  char *title;
  char *parameter;
{
  FILE *fpw = out_http(ap, HS_OK, NULL);
  out_title(fpw, header, ap->url, title, parameter);
  return fpw;
}

static void
out_article(fpw, fpath)
  FILE *fpw;
  char *fpath;
{
  FILE *fp;

  if ((fp = fopen(fpath, "r")))
  {
    txt2htm(fpw, fp);
    fclose(fp);
  }
}


static void
out_extra(fpw)
  FILE *fpw;
{
  fputs("  <input type=\"hidden\" id=\"curPos\" value=\"1\"/>\n", fpw);
  fputs("  <div id=\"home\">\n"
        "    <a href=\"/home\" title=\"(快速鍵 m)\">[ 回到首頁 ]</a>\n"
        "  </div>\n", fpw); /* end of #footer */

  fputs("  <div id=\"share\">\n"
        "    <span>[ 分享 ]</span>\n"
        "    <div class=\"hidden\">\n"
        "      <div class=\"plurk-share\"></div>\n"
        "      <div class=\"g-plusone\"></div>\n"
        "      <div class=\"fb-like\" data-href=\"" OG_URL "\" data-layout=\"button_count\" data-send=\"false\" data-width=\"80\"></div>\n"
        "    </div>\n"
        "  </div>\n"
        "  <div id=\"message\"><a href=\"#top\">回到頁首</a></div>\n"
        "  <script type=\"text/javascript\" src=\"https://ajax.googleapis.com/ajax/libs/jquery/1.7.1/jquery.min.js\"></script>\n"
        "  <script type=\"text/javascript\" src=\"https://apis.google.com/js/plusone.js\"></script>\n"
#ifdef HAVE_GOOGLE_ANALYTICS
        "  <script type=\"text/javascript\">"
        "  var _gaq = _gaq || [];"
        "  _gaq.push([\'_setAccount\', \'" GA_UID "\']);"
        "  _gaq.push([\'_trackPageview\']);"
        "  (function() {"
        "  var ga = document.createElement(\'script\');"
        "  ga.type = \'text/javascript\';ga.async = true;"
        "  ga.src = (\'https:\' == document.location.protocol ?"
        "   \'https://ssl\' : \'http://www\') + \'.google-analytics.com/ga.js\';"
        "  var s = document.getElementsByTagName(\'script\')[0];"
        "  s.parentNode.insertBefore(ga, s);"
        "  })();"
        "  </script>\n"
#endif
        "  <script type=\"text/javascript\" src=\"link?script.min.js\"></script>\n", fpw);
}


static void
out_tail(fpw)
  FILE *fpw;
{
  fputs("    </div>\n" /* end of #content */
        "  </div>\n", fpw); /* end of #container */

  out_extra(fpw);

  fputs("</body>\n"
        "</html>\n", fpw);
}


/* ----------------------------------------------------- */
/* 解碼分析參數                                          */
/* ----------------------------------------------------- */

#define hex2int(x)    ((x >= 'A') ? (x - 'A' + 10) : (x - '0'))

static int                /* 1:成功 */
arg_analyze(argc, mark, str, arg1, arg2, arg3, arg4)
  int argc;               /* 有幾個參數 */
  int mark;               /* !=0: str 要是 mark 開頭的字串 */
  char *str;              /* 引數 */
  char **arg1;            /* 參數一 */
  char **arg2;            /* 參數二 */
  char **arg3;            /* 參數三 */
  char **arg4;            /* 參數四 */
{
  int i, ch;
  char *dst;

  if ((mark && *str++ != mark) || !(ch = *str))
  {
    *arg1 = NULL;
    return 0;
  }

  *arg1 = dst = str;
  i = 2;

  while (ch)
  {
    if (ch == '&' || ch == '\r')
    {
      if (i > argc)
        break;

      *dst++ = '\0';
      if (i == 2)
        *arg2 = dst;
      else if (i == 3)
        *arg3 = dst;
      else /* if (i == 4) */
        *arg4 = dst;
      i++;
    }
    else if (ch == '+')
    {
      *dst++ = ' ';
    }
    else if (ch == '%')
    {
      ch = *(++str);
      if (isxdigit(ch) && isxdigit(str[1]))
      {
        ch = (hex2int(ch) << 4) + hex2int(str[1]);
        str++;
        if (ch != '\r')        /* '\r' 就不要了 */
          *dst++ = ch;
      }
      else
      {
        *dst++ = '%';
        continue;
      }
    }
    else
      *dst++ = ch;

    ch = *(++str);
  }
  *dst = '\0';

  return i > argc;
}

/* ----------------------------------------------------- */
/* UTMP shm 部分須與 cache.c 相容                        */
/* ----------------------------------------------------- */

static UCACHE *ushm;

static void
init_ushm()
{
  ushm = shm_new(UTMPSHM_KEY, sizeof(UCACHE));
}

/* ----------------------------------------------------- */
/* board：shm 部份須與 cache.c 相容                      */
/* ----------------------------------------------------- */

static BCACHE *bshm;

static void
init_bshm()
{
  /* itoc.030727: 在開啟 bbsd 之前，應該就要執行過 account，
     所以 bshm 應該已設定好 */

  if (bshm)
    return;

  bshm = shm_new(BRDSHM_KEY, sizeof(BCACHE));

  if (bshm->uptime <= 0)    /* bshm 未設定完成 */
    exit(0);
}


static int
Ben_Perm(brd, ulevel)    /* 參考 board.c:Ben_Perm() */
  BRD *brd;
  usint ulevel;
{
  usint readlevel, postlevel, bits;
  char *bname;

  if (!brd)
    return 0;

  bname = brd->brdname;

  if (!(*bname))
    return 0;

  readlevel = brd->readlevel;

  if (!readlevel || (readlevel & ulevel))
  {
    bits = BRD_L_BIT | BRD_R_BIT;

    postlevel = brd->postlevel;
    if (!postlevel || (postlevel & ulevel))
      bits |= BRD_W_BIT;
  }
  else
    bits = 0;

  return bits;
}


static BRD *
brd_get(bname)
  char *bname;
{
  BRD *bhdr, *tail;

  bhdr = bshm->bcache;
  tail = bhdr + bshm->number;
  do
  {
    if (!strcmp(bname, bhdr->brdname))
      return bhdr;
  } while (++bhdr < tail);
  return NULL;
}


static BRD *
allow_brdname(ap, brdname)
  Agent *ap;
  char *brdname;
{
  BRD *bhdr;

  if ((bhdr = brd_get(brdname)))
  {
    /* 若 readlevel == 0，表示 guest 可讀，無需 acct_fetch() */
    if (!(bhdr->readlevel))
      return bhdr;

    ap->userlevel = 0;

    if (Ben_Perm(bhdr, ap->userlevel) & BRD_R_BIT)
      return bhdr;
  }
  return NULL;
}


/* ----------------------------------------------------- */
/* movie：shm 部份須與 cache.c 相容                      */
/* ----------------------------------------------------- */

static FCACHE *fshm;

static void
init_fshm()
{
  fshm = shm_new(FILMSHM_KEY, sizeof(FCACHE));
}

/* ----------------------------------------------------- */
/* command dispatch (GET)                                */
/* ----------------------------------------------------- */

  /* --------------------------------------------------- */
  /* 通用清單                                            */
  /* --------------------------------------------------- */

static void
list_neck(fpw, start, total, title)
  FILE *fpw;
  int start, total;
  char *title;
{
  fputs("      <div id=\"neck\">\n"
        "        <span class=\"prev cmdPgUp\">", fpw);

  if (start != 1)
    fprintf(fpw, "<a href=\"?%d\">往上(PgUp)</a>", (start > HTML_TALL) ? (start - HTML_TALL) : 1);

  fputs("</span>\n"
        "        <span class=\"next cmdPgDn\">", fpw);

  start += HTML_TALL;

  if (start <= total)
    fprintf(fpw, "<a href=\"?%d\">往下(PgDn)</a>", start);

  fputs("</span>\n"
        "        <span class=\"title\">", fpw);
  fprintf(fpw, title, total);
  fputs("</span>\n"
        "        <span class=\"top cmdHome\"><a href=\"?1\">(Home)</a></span>\n"
        "        <span class=\"end cmdEnd\"><a href=\"?0\">(End)</a></span>\n"
        "      </div>\n", fpw); /* end of #neck */
}

static void
cmdlist_list(ap, title, list_tie, list_item)
  Agent *ap;
  char *title;
  void (*list_tie) (FILE *);
  void (*list_item) (FILE *, void *, int);
{
  int i, pos, start, end, total;
  char *number;
  FILE *fpw;

  fpw = ap->fpw;
  pos = (!arg_analyze(1, '?', ap->urlp, &number, NULL, NULL, NULL)) ? 1 : atoi(number);
  total = ap->total_item;
  start = end = 0;

  if (total < HTML_TALL)
  {
    start = 1;
    end = total;
  }
  else if (pos == 0 || (pos + HTML_TALL) > total)
  {
    start = total - HTML_TALL + 1;
    end = total;
  }
  else  if (pos < 0 || pos > total)
  {
    start = 1;
    end = HTML_TALL;
  }
  else
  {
    start = pos;
    end = pos + HTML_TALL - 1;
  }

  list_neck(fpw, start, total, title);
  list_tie(fpw);

  /* list start */
  fputs("      <div id=\"list\">\n", fpw);

  for (i = start - 1; i < end; i++)
    list_item(fpw, ap->myitem[i], i + 1);

  fputs("      </div>\n" /* end of #list */
        "      <span class=\"cmdLeft\"><a href=\"/\"></a></span>\n", fpw);
}


  /* --------------------------------------------------- */
  /* 看板列表                                            */
  /* --------------------------------------------------- */

static int
brdtitle_cmp(a, b)
  BRD **a, **b;
{
  /* itoc.010413: 分類/板名交叉比對 */
  int k = strcmp((*a)->brdname, (*b)->brdname);
  return k ? k : str_cmp((*a)->class, (*b)->class);
}

static int
brdmantime_cmp(a, b)
  BRD **a, **b;
{
  return (bshm->mantime[(*b) - bshm->bcache] - bshm->mantime[(*a) - bshm->bcache]);
}


static void
init_mybrd(ap, mode)
  Agent *ap;
  int mode; /* 0: normal 1: hotboard */
{
  int num;
  usint ulevel;
  BRD *bhdr, *tail;

  ulevel = ap->userlevel = 0;
  bhdr = bshm->bcache;
  tail = bhdr + bshm->number;
  num = 0;

  do
  {
    if (Ben_Perm(bhdr, ulevel) & BRD_R_BIT)
    {
      if (!mode)
      {
        ap->myitem[num] = bhdr;
        num++;
      }
      else
      {
        if (bshm->mantime[bhdr - bshm->bcache] >= 5)
        {
          ap->myitem[num] = bhdr;
          num++;
        }
      }
    }
  } while (++bhdr < tail);

  if (num > 1)
  {
    qsort(ap->myitem, num, sizeof(BRD *), mode ? brdmantime_cmp : brdtitle_cmp);
  }
  ap->total_item = num;
}


static void
boardlist_tie(fpw)
  FILE *fpw;
{
  fputs("      <div id=\"tie\">"
        "        <span class=\"b_number\">編號</span>\n"
        "        <span class=\"b_name\">看板</span>\n"
        "        <span class=\"b_class\">類別</span>\n"
        "        <span class=\"b_token\">轉</span>\n"
        "        <span class=\"b_desc\">中文敘述</span>\n"
        "        <span class=\"b_hotcount\">人氣</span>\n"
        "      </div><!-- end of #tie -->\n", fpw);
}


static void
boardlist_item(fpw, bhdr, n)
  FILE *fpw;
  BRD *bhdr;
  int n;
{
  int count = bshm->mantime[bhdr - bshm->bcache];

  char cnum[3];

  if (count > 99)
    sprintf(cnum, "爆");
  else if (count > 0)
    sprintf(cnum, "%d", count);
  else
    sprintf(cnum, ""); 

  fprintf(fpw,
          "        <div class=\"listitem\">\n"
          "          <a href=\"/brd?%s\" class=\"cmdRight\">\n"
          "            <span class=\"b_number cmdIndex\">%d</span>\n"
          "            <span class=\"b_name\">%s</span>\n"
          "            <span class=\"b_class\">%s</span>\n"
          "            <span class=\"b_token\">%s</span>\n"
          "            <span class=\"b_desc cmdRight\">%.36s</span>\n"
          "            <span class=\"b_hotcount\">%s</span>\n"
          "          </a>\n"
          "        </div>\n",
          bhdr->brdname, n, bhdr->brdname, bhdr->class,
          (bhdr->battr & BRD_NOTRAN) ? ICON_NOTRAN_BRD : ICON_TRAN_BRD,
          str_html(bhdr->title, 33), cnum);
}

static int
cmd_boardlist(ap)
  Agent *ap;
{
  init_mybrd(ap, 0);
  out_head(ap, "看板列表");
  cmdlist_list(ap, "目前站上有 %d 公開看板", boardlist_tie, boardlist_item);
  return HS_END;
}

static int
cmd_hotboardlist(ap)
  Agent *ap;
{
  init_mybrd(ap, 1);
  out_head(ap, "熱門看板");
  cmdlist_list(ap, "目前站上有 %d 熱門看板", boardlist_tie, boardlist_item);
  return HS_END;
}


  /* --------------------------------------------------- */
  /* 分類看板列表                                        */
  /* --------------------------------------------------- */

static int
cmd_class(ap)
  Agent *ap;
{
  int fd, i;
  usint ulevel;
  char folder[64], *xname;
  BRD *brd;
  HDR hdr;
  FILE *fpw = out_head(ap, "分類看板");

  if (!arg_analyze(1, '?', ap->urlp, &xname, NULL, NULL, NULL))
    xname = CLASS_INIFILE;

  if (strlen(xname) > 0 && strlen(xname) > 12)
    return HS_ERROR;

  sprintf(folder, "gem/@/@%s", xname);
  if ((fd = open(folder, O_RDONLY)) < 0)
    return HS_ERR_CLASS;

  ulevel = ap->userlevel = 0;

  /* class_neck */
  fputs("      <div id=\"neck\">\n"
        "        <span class=\"upper cmdLeft\"><a href=\"/class\">回最上層(←)</a></span>\n"
        "      </div>\n", fpw); /* end of #neck */

  boardlist_tie(fpw);

  fputs("      <div id=\"list\">\n", fpw);

  i = 1;

  while (read(fd, &hdr, sizeof(HDR)) == sizeof(HDR))
  {
    if (hdr.xmode & GEM_BOARD)    /* 看板 */
    {
      if ((brd = brd_get(hdr.xname)) &&
           str_cmp(hdr.xname, "newboard") &&
           Ben_Perm(brd, ulevel) & BRD_R_BIT)
      {
        boardlist_item(fpw, brd, i);
      }
      else
        continue;
    }
    else if ((hdr.xmode & GEM_FOLDER) && *hdr.xname == '@') /* 分類 */
    {
      /* 轉信看板不顯示 */
      if (!str_cmp(hdr.xname + 1, "TRANSBRD"))
        continue;

               fputs("        <div class=\"listitem\">\n", fpw);

      if (str_cmp(hdr.xname + 1, "HOTBOARD"))
        fprintf(fpw, "          <a href=\"/class?%s\" class=\"cmdRight\">\n", hdr.xname + 1);
      else
        fprintf(fpw, "          <a href=\"/hotboard\" class=\"cmdRight\">\n");

      fprintf(fpw,   "            <span class=\"b_number cmdIndex\">%d</span>\n"
                     "            <span class=\"b_name\">%s/</span>\n"
                     "            <span class=\"b_class\">分類</span>\n"
                     "            <span class=\"b_token\">□</span>\n"
                     "            <span class=\"b_desc\">%s</span>\n"
                     "            <span class=\"b_bmlist\"></span>\n"
                     "          </a>\n"
                     "        </div>\n", i, hdr.xname + 1, str_html(hdr.title + 21, 52));
    }
    else
    {
      continue;
    }

    i++;
  }

  fputs("      </div>\n", fpw); /* end of #list */

  close(fd);

  return HS_END;
}


  /* --------------------------------------------------- */
  /* 文章列表                                            */
  /* --------------------------------------------------- */

static void
postlist_item(fpw, hdr, start, brdname)
  FILE *fpw;
  HDR hdr;
  int start;
  char *brdname;
{
  char owner[80], *tmp;

  strcpy(owner, hdr.owner);

  if ((tmp = strchr(owner, '.')))    /* 站外作者 */
    *(tmp + 1) = '\0';

  if ((tmp = strchr(owner, '@')))    /* 站外作者 */
  {
    *tmp = '.';
    *(tmp + 1)= '\0';
  }

  fputs("        <div class=\"listitem\">\n", fpw);


  if (brdname)
    fprintf(fpw, "          <a href=\"/bmore?%s&%d\" class=\"cmdRight\">\n", brdname, start);
  else
    fprintf(fpw, "          <a href=\"/mmore?%d&\" class=\"cmdRight\">\n", start);

  if (hdr.xmode & POST_BOTTOM)
    fprintf(fpw, "            <span class=\"p_number cmdIndex\"><span class=\"f132\">重要</span></span>\n");
  else
    fprintf(fpw, "            <span class=\"p_number cmdIndex\">%d</span>\n", start);

  fprintf(fpw, "            <span class=\"p_mark\">%s</span>\n",
               hdr.xmode & POST_MARKED ? "<span class=\"f136\">m</span>" :
               hdr.xmode & POST_DELETE ? "<span class=\"f132\">D</span>" :
               hdr.xmode & POST_DIGEST ? "<span class=\"f136\">#</span>" :
               hdr.xmode & POST_DONE ? "s" :
               hdr.xmode & POST_RESTRICT ? "L" : "");

#ifdef HAVE_SCORE
  if (hdr.xmode & POST_SCORE && !(hdr.xmode & POST_BOTTOM))
  {
    if (hdr.score >= 99)
      fprintf(fpw, "            <span class=\"p_score f133\">爆</span>\n");
    else if (hdr.score <= -99)
      fprintf(fpw, "            <span class=\"p_score f037\">爛</span>\n");
    else
      fprintf(fpw, "            <span class=\"p_score %s\">%d</span>\n",
                    hdr.score >= 0 ? "f131" : "f132", abs(hdr.score));
  }
  else
    fputs("            <span class=\"p_score\">&nbsp;</span>\n", fpw);
#else
  fputs("            <span class=\"p_score\">&nbsp;</span>\n", fpw);
#endif

  fprintf(fpw, "            <span class=\"p_date\">%s</span>\n"
               "            <span class=\"p_author\">%s</span>\n", hdr.date + 3, owner);

  fprintf(fpw, "            <span class=\"p_title cmdRight\">%s",
               (strlen(hdr.title) > 4 && !strncmp(hdr.title, "Re: ", 4)) ? "Re " : "□ ");


#ifdef HAVE_REFUSEMARK
  if (hdr.xmode & POST_RESTRICT)
    fputs("<文章保密></span>\n", fpw);
  else
#endif
    fprintf(fpw, "%.60s</span>\n", (strlen(hdr.title) > 4 && !strncmp(hdr.title, "Re: ", 4)) ?
                                    str_html(hdr.title, 64) + 4 : str_html(hdr.title, 60));
  fputs("          </a>\n"
        "        </div>\n", fpw);
}

static void
postlist_list(fpw, folder, brdname, start, end)
  FILE *fpw;
  char *folder, *brdname;
  int start;
  int end;
{
  int fd;
  HDR hdr;

  fputs("      <div id=\"list\">\n", fpw);

  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    lseek(fd, (off_t) (sizeof(HDR) * (start - 1)), SEEK_SET);

    while ((start < (end + 1)) && (read(fd, &hdr, sizeof(HDR)) == sizeof(HDR)))
    {
      postlist_item(fpw, hdr, start, brdname);
      start++;
    }
    close(fd);
  }
  fputs("      </div>\n", fpw); /* end of #list */
}


static void
postlist_neck(fpw, start, total, brdname)
  FILE *fpw;
  int start;
  int total;
  char *brdname;
{
  fputs("      <div id=\"neck\">\n", fpw);

  fputs("        <span class=\"prev cmdPgUp\">", fpw);
  if (start > 1)
    fprintf(fpw, "<a href=\"?%s&amp;%d\">上(PgUp)</a>", brdname, (start > HTML_TALL) ? (start - HTML_TALL) : 1);
  fputs("        </span>\n", fpw);

  start += HTML_TALL;

  fputs("        <span class=\"next cmdPgDn\">", fpw);
  if (start <= total)
    fprintf(fpw, "<a href=\"?%s&amp;%d\">下(PgDn)</a>", brdname, start);
  fputs("        </span>\n", fpw);

  fprintf(fpw, "        <span class=\"rest cmdLeft\"><a href=\"/brdlist\">看板列表(←)</a></span>\n", brdname, brdname, brdname, brdname);

  fputs("      </div>\n", fpw); /* end of #neck */
}

static void
postlist_tie(fpw)
  FILE *fpw;
{
  fputs("      <div id=\"tie\">\n"
        "        <span class=\"p_number\">編號</span>\n"
        "        <span class=\"p_mark\">&nbsp</span>\n"
        "        <span class=\"p_score\">評</span>\n"
        "        <span class=\"p_date\">日期</span>\n"
        "        <span class=\"p_author\">作者</span>\n"
        "        <span class=\"p_title\">標題</span>\n"
        "      </div>\n", fpw); /* end of #tie */
}

static int
cmd_postlist(ap)
  Agent *ap;
{
  int pos, start, end, total;
  char folder[64], *brdname, *number;
  FILE *fpw = out_head(ap, "文章列表");

  if (!arg_analyze(2, '?', ap->urlp, &brdname, &number, NULL, NULL))
  {
    if (brdname)
      number = "0";
    else
      return HS_ERROR;
  }

  if (!allow_brdname(ap, brdname))
    return HS_ERR_BOARD;

  brd_fpath(folder, brdname, FN_DIR);

  pos = atoi(number);
  total = rec_num(folder, sizeof(HDR));

  start = end = 0;

  if (total < HTML_TALL)
  {
    start = 1;
    end = total;
  }
  else if (pos == 0 || (pos + HTML_TALL) > total)
  {
    start = total - HTML_TALL + 1;
    end = total;
  }
  else  if (pos < 0 || pos > total)
  {
    start = 1;
    end = HTML_TALL;
  }
  else
  {
    start = pos;
    end = pos + HTML_TALL - 1;
  }

  postlist_neck(fpw, start, total, brdname);
  postlist_tie(fpw);
  postlist_list(fpw, folder, brdname, start, end);

  return HS_END;
}


  /* --------------------------------------------------- */
  /* 閱讀看板文章                                        */
  /* --------------------------------------------------- */

static void
more_neck(fpw, pos, total, brdname, xname)
  FILE *fpw;
  int pos, total;
  char *brdname, *xname;
{

  fputs("    <div id=\"neck\">\n"
        "      <span class=\"prev\">", fpw);

  if (pos > 1)
  {
    if (brdname)
      fprintf(fpw, "<a href=\"bmore?%s&amp;", brdname);
    if (xname)
      fprintf(fpw, "<a href=\"?%s&amp;", xname);
    fprintf(fpw, "%d\">上一篇</a>", pos - 1);
  }
  fputs("</span>\n", fpw);

  fputs("      <span class=\"next\">", fpw);
  if (pos < total)
  {
    if (brdname)
      fprintf(fpw, "<a href=\"/bmore?%s&amp;", brdname);
    if (xname)
      fprintf(fpw, "<a href=\"?%s&amp;", xname);
    fprintf(fpw, "%d\">下一篇</a>", pos + 1);
  }
  fputs("</span>\n", fpw);

  fputs("      <span class=\"title\">", fpw);
  if (brdname)
  {
    fprintf(fpw, "<a href=\"/bmost?%s&amp;%d\" target=\"_blank\">同標題文章</a></span>\n"
                 "      <span class=\"list cmdLeft\"><a href=\"/brd?%s\">文章列表(←)</a>",
                  brdname, pos, brdname, brdname);
  }
  fputs("      </span>\n"
        "    </div>\n", fpw); /* end of #neck */
}

static int
more_item(fpw, folder, pos, brdname)
  FILE *fpw;
  char *folder;
  int pos;
  char *brdname;
{
  int fd, total;
  HDR hdr;
  char fpath[64];

  total = rec_num(folder, sizeof(HDR));
  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    int find = 0;

    lseek(fd, (off_t) (sizeof(HDR) * (pos - 1)), SEEK_SET);
    find = read(fd, &hdr, sizeof(HDR)) == sizeof(HDR);
    close(fd);

    if (find)
    {
      more_neck(fpw, pos, total, brdname, NULL);

#ifdef HAVE_REFUSEMARK
      if (!(hdr.xmode & POST_RESTRICT))
      {
#endif

        hdr_fpath(fpath, folder, &hdr);
        out_article(fpw, fpath);

#ifdef HAVE_PERMANENT_LINK
        fprintf(fpw, "    <div id=\"plink\">\n"
                     "      <span>永久連結: <input type=\"text\" value=\"" OG_URL "/plink?%s&%s\" /></span>\n"
                     "    </div>\n", brdname, hdr.xname, brdname, hdr.xname);
#endif

#ifdef HAVE_REFUSEMARK
      }
      else
      {
        fputs("<br />這是加密的文章，您無法閱\讀<br /><br />\n", fpw);
      }
#endif

      return HS_END;
    }
  }

  return HS_ERR_MORE;
}

static int
more_item2(fpw, folder, fname, brdname)
  FILE *fpw;
  char *folder;
  char *fname;
  char *brdname;
{
  HDR hdr;
  int fd, total;
  char fpath[64];

  total = rec_num(folder, sizeof(HDR));
  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    int find = 0, pos = 0;
    lseek(fd, (off_t) (sizeof(HDR) * pos), SEEK_SET);
    while (pos < total && read(fd, &hdr, sizeof(HDR)))
    {
      if (!strcmp(hdr.xname, fname))
      {
        find = 1;
        break;
      }
      pos++;
    }

    if (find)
    {
      more_neck(fpw, pos + 1, total, brdname, NULL);

#ifdef HAVE_REFUSEMARK
      if (!(hdr.xmode & POST_RESTRICT))
      {
#endif
        hdr_fpath(fpath, folder, &hdr);
        out_article(fpw, fpath);
#ifdef HAVE_PERMANENT_LINK
        fprintf(fpw, "    <div id=\"plink\">\n"
                     "      <span>永久連結: <input type=\"text\" value=\"" OG_URL "/plink?%s&%s\" /></span>\n"
                     "    </div>\n", brdname, fname, brdname, fname);
#endif

#ifdef HAVE_REFUSEMARK
      }
      else
      {
        fputs("<br />這是加密的文章，您無法閱\讀<br /><br />\n", fpw);
      }
#endif

      return HS_END;
    }
  }

  return HS_ERR_MORE;
}


static void
get_article_title(title, folder, pos, brdname)
  char *title;
  char *folder;
  int pos;
  char *brdname;
{
  HDR hdr;
  int fd, total, find;

  total = rec_num(folder, sizeof(HDR));
  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    find = 0;
      
    lseek(fd, (off_t) (sizeof(HDR) * (pos - 1)), SEEK_SET);
    find = read(fd, &hdr, sizeof(HDR)) == sizeof(HDR);
    close(fd);

    if (find)
    {
#ifdef HAVE_REFUSEMARK
      if (!(hdr.xmode & POST_RESTRICT))
      {
#endif
        if (strlen(hdr.title) > 0)
          strcpy(title, hdr.title);
#ifdef HAVE_REFUSEMARK
      }
      else
        strcpy(title, "加密文章");
#endif
    }
  }
}


static void
get_article_title2(title, folder, fname, brdname)
  char *title;
  char *folder;
  char *fname;
  char *brdname;
{
  HDR hdr;
  int fd, total, find, pos;

  total = rec_num(folder, sizeof(HDR));
  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    find = pos = 0;
    lseek(fd, (off_t) (sizeof(HDR) * pos), SEEK_SET);
    while (pos < total && read(fd, &hdr, sizeof(HDR)))
    {
      if (!strcmp(hdr.xname, fname))
      {
        find = 1;
        break;
      }
      pos++;
    }

    if (find)
    {
#ifdef HAVE_REFUSEMARK
      if (!(hdr.xmode & POST_RESTRICT))
      {
#endif
        if (strlen(hdr.title) > 0)
          strcpy(title, hdr.title);
#ifdef HAVE_REFUSEMARK
      }
      else
        strcpy(title, "加密文章");
#endif
    }    
  }
}


static int
cmd_brdmore(ap, mode)
  Agent *ap;
  int mode;
{
  FILE *fpw;
  int pos, code = HS_END;
  char folder[64], bname[32], *brdname, *idx;
  char *title = (char *) malloc(sizeof(char) * 128);

  /* idx: number(0), fname(1) */
  if (!arg_analyze(2, '?', ap->urlp, &brdname, &idx, NULL, NULL))
    code = HS_ERROR;

  if (!allow_brdname(ap, brdname))
    code = HS_ERR_BOARD;

  if (code > HS_END && code < HS_OK)
  {
    fpw = out_head(ap, "閱\讀文章");
    return code;
  }

  brd_fpath(folder, brdname, FN_DIR);

  if (!mode)
  {
    pos = (atoi(idx) <= 0) ? 1 : atoi(idx);
    get_article_title(title, folder, pos, brdname);
  }
  else
  {
    get_article_title2(title, folder, idx, brdname);
  }

  snprintf(bname, strlen(brdname) + 6, "看板 %s", brdname);
  fpw = out_head_article(ap, bname, title, idx);
  free(title);
 
  return (mode == 0) ? more_item(fpw, folder, pos, brdname) 
                     : more_item2(fpw, folder, idx, brdname);
}

static int
cmd_article(ap)
  Agent *ap;
{
  return cmd_brdmore(ap, 0);
}

static int
cmd_article2(ap)
  Agent *ap;
{
  return cmd_brdmore(ap, 1);
}

  /* --------------------------------------------------- */
  /* 閱讀看板同標題文章                                  */
  /* --------------------------------------------------- */

static void
do_brdmost(fpw, folder, brdname, title)
  FILE *fpw;
  char *folder;
  char *brdname;
  char *title;
{
  int fd;
  char fpath[64];
  FILE *fp;
  HDR hdr;

  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    while (read(fd, &hdr, sizeof(HDR)) == sizeof(HDR))
    {
#ifdef HAVE_REFUSEMARK
      if (hdr.xmode & POST_RESTRICT)
       continue;
#endif

      if (!strcmp(str_ttl(hdr.title), title))
      {
        hdr_fpath(fpath, folder, &hdr);
        if (fp = fopen(fpath, "r"))
        {
          txt2htm(fpw, fp);
          fclose(fp);
        }
      }
    }
    close(fd);
  }
}


static void
brdmost_neck(fpw)
  FILE *fpw;
{
  fputs("      <div id=\"neck\">\n"
        "        <span class=\"m_title\">同標題閱\讀</span>\n"
        "      </div>\n", fpw); /* end of #neck */
}


static int
cmd_brdmost(ap)
  Agent *ap;
{
  int fd, pos;
  char folder[64], *brdname, *number;
  HDR hdr;
  FILE *fpw = out_head(ap, "閱\讀看板同標題文章");

  if (!arg_analyze(2, '?', ap->urlp, &brdname, &number, NULL, NULL))
    return HS_ERROR;

  if (!allow_brdname(ap, brdname))
    return HS_ERR_BOARD;

  brd_fpath(folder, brdname, FN_DIR);

  if ((pos = atoi(number)) <= 0)
    pos = 1;

  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    int find;

    lseek(fd, (off_t) (sizeof(HDR) * (pos - 1)), SEEK_SET);
    find = read(fd, &hdr, sizeof(HDR)) == sizeof(HDR);
    close(fd);

    if (find)
    {
      brdmost_neck(fpw);
      do_brdmost(fpw, folder, brdname, str_ttl(hdr.title));
      return HS_END;
    }
  }
  return HS_ERR_MORE;
}

  /* --------------------------------------------------- */
  /* 顯示圖片                                            */
  /* --------------------------------------------------- */

static int
valid_path(str)
  char *str;
{
  int ch;

  if (!(*str))
    return 0;

  while (ch = *str++)
  {
    if (!is_alnum(ch) && ch != '.' && ch != '-' && ch != '_' && ch != '+')
      return 0;
  }
  return 1;
}


static int
cmd_media(ap)
  Agent *ap;
{
  FILE *fpw;
  struct stat st;
  char *fname, *ptr, fpath[64];

  if (!arg_analyze(1, '?', ap->urlp, &fname, NULL, NULL, NULL))
    return HS_NOTFOUND;

  if (!valid_path(fname) || !(ptr = strrchr(fname, '.')))
    return HS_NOTFOUND;

  /* supported file formats, sort alphabetically */
       if (!str_cmp(ptr, ".css"))  ptr = "text/css";
  else if (!str_cmp(ptr, ".gif"))  ptr = "image/gif";
  else if (!str_cmp(ptr, ".htm"))  ptr = "text/html";
  else if (!str_cmp(ptr, ".html")) ptr = "text/html";
  else if (!str_cmp(ptr, ".ico"))  ptr = "image/x-icon";
  else if (!str_cmp(ptr, ".jpg"))  ptr = "image/jpeg";
  else if (!str_cmp(ptr, ".js"))   ptr = "application/x-javascript";
  else if (!str_cmp(ptr, ".png"))  ptr = "image/png";
  else                             return HS_NOTFOUND;

  /* locate the file */
  sprintf(fpath, "run/html/%.40s", fname);

  if (stat(fpath, &st))
    return HS_NOTFOUND;

  /* if not modified */
  if (ap->modified[0] && !strcmp(Gtime(&st.st_mtime), ap->modified))
    return HS_NOTMOIDIFY;

  fpw = out_http(ap, HS_OK, ptr);
  fprintf(fpw, "Content-Length: %d\r\n", st.st_size);
  fprintf(fpw, "Last-Modified: %s\r\n\r\n", Gtime(&st.st_mtime));
  f_suck(fpw, fpath);

  return HS_OK;
}


  /* --------------------------------------------------- */
  /* Robot Exclusion                                     */
  /* --------------------------------------------------- */

static int
cmd_robots(ap)
  Agent *ap;
{
  FILE *fpw = out_http(ap, HS_OK, NULL);

  time_t now;
  time(&now);

  fprintf(fpw, "Content-Length: 41\r\n"
               "Last-Modified: %s\r\n\r\n"
               "User-agent: *\r\n"
               "Disallow: /bmore\r\n"
               "Allow: /\r\n", Gtime(&now));
      
  return HS_OK;        
}


static int
cmd_frontpage(ap)
  Agent *ap;
{
  FILE *fpw = out_head(ap, "");
  fputs("<a href=\"/home\" id=\"front\"><img src=\"/link?frontpage.png\" title=\"(enter)\" /></a>\n", fpw);
  return HS_END;
}


  /* --------------------------------------------------- */
  /* 首頁                                                */
  /* --------------------------------------------------- */

static int
cmd_mainpage(ap)
  Agent *ap;
{
  FILE *fpw = out_head(ap, "");

  fputs("      <div id=\"main_menu\">\n"
        "        <span class=\"m_hotboard\"><a href=\"/hotboard\" title=\"(快速鍵 h)\">熱門看板</a></span>\n"
        "        <span class=\"m_class\"><a href=\"/class\" title=\"(快速鍵 c)\">看板分類</a></span>\n"
        "        <span class=\"m_brdlist\"><a href=\"/brdlist\" title=\"(快速鍵 b)\">看板列表</a></span>\n"
        "      </div>\n", fpw);

  return HS_END;
}


  /* --------------------------------------------------- */
  /* 指令集                                              */
  /* --------------------------------------------------- */

static Command cmd_table_get[] =
{
  /* accepted commands, sort alphabetically */
  cmd_article,       "bmore",     5,
  cmd_brdmost,       "bmost",     5,
  cmd_boardlist,     "brdlist",   7,
  cmd_postlist,      "brd",       3,
  cmd_class,         "class",     5,
  cmd_frontpage,     "\0",        1,
  cmd_hotboardlist,  "hotboard",  8,
  cmd_media,         "link",      4,
  cmd_article2,      "plink",     5,
  cmd_robots,        "robots.txt",9,
  cmd_mainpage,      "home",      4,
  NULL,              NULL,        0
};


/* ----------------------------------------------------- */
/* close a connection & release its resource             */
/* ----------------------------------------------------- */

static void
agent_fire(ap)
  Agent *ap;
{
  int csock;

  csock = ap->sock;
  if (csock > 0)
  {
    fcntl(csock, F_SETFL, M_NONBLOCK);
    shutdown(csock, 2);

    /* fclose(ap->fpw); */
    close(csock);
  }

  if (ap->data)
    free(ap->data);
}


/* ----------------------------------------------------- */
/* receive request from client                           */
/* ----------------------------------------------------- */

static int        /* >=0:mode -1:結束 */
do_cmd(ap, str, end, mode)
  Agent *ap;
  uschar *str, *end;        /* command line 的開頭和結尾 */
  int mode;
{
  int code;
  char *ptr;

  if (!(mode & (AM_GET | AM_POST)))
  {
    if (!str_ncmp(str, "GET ", 4))        /* str 格式為 GET /index.htm HTTP/1.0 */
    {
      mode ^= AM_GET;
      str += 4;

      if (*str != '/')
      {
        out_error(ap, HS_BADREQUEST);
        return -1;
      }

      if (ptr = strchr(str, ' '))
      {
        *ptr = '\0';
        str_ncpy(ap->url, str + 1, sizeof(ap->url));
      }
      else
      {
        *ap->url = '\0';
      }

      logit("ACCEPT", ap->url);
    }
  }
  else
  {
    if (*str)        /* 不是空行：檔頭 */
    {
      /* 分析 If-Modified-Since */
      /* str 格式為 If-Modified-Since: Sat, 29 Oct 1994 19:43:31 GMT */
      if ((mode & AM_GET) && !str_ncmp(str, "If-Modified-Since: ", 19))
        str_ncpy(ap->modified, str + 19, sizeof(ap->modified));
    }
    else        /* 空行 */
    {
      Command *cmd;
      char *url;

      if (mode & AM_GET)
      {
        cmd = cmd_table_get;
        url = ap->url;
      }

      for (; ptr = cmd->cmd; cmd++)
      {
        if (!str_ncmp(url, ptr, cmd->len))
          break;
      }

      /* waynesan.081018: 如果在 command_table 裡面找不到，那麼送 404 Not Found */
      if (!ptr)
      {
        out_error(ap, HS_NOTFOUND);
        return -1;
      }

      ap->urlp = url + cmd->len;

      code = (*cmd->func) (ap);
      if (code != HS_OK)
      {
        if (code != HS_END)
          out_error(ap, code);
        if (code < HS_OK)
          out_tail(ap->fpw);
      }
      return -1;
    }
  }

  return mode;
}


static int
agent_recv(ap)
  Agent *ap;
{
  int cc, mode, size, used;
  uschar *data, *head;

  used = ap->used;
  data = ap->data;

  if (used > 0)
  {
    /* check the available space */

    size = ap->size;
    cc = size - used;

    if (cc < TCP_RCVSIZ + 3)
    {
      if (size < MAX_DATA_SIZE)
      {
        size += TCP_RCVSIZ + (size >> 2);

        if (data = (uschar *) realloc(data, size))
        {
          ap->data = data;
          ap->size = size;
        }
        else
        {
#ifdef LOG_VERBOSE
          fprintf(flog, "ERROR\trealloc: %d\n", size);
#endif
          return 0;
        }
      }
      else
      {
#ifdef LOG_VERBOSE
        fprintf(flog, "WARN\tdata too long\n");
#endif
        return 0;
      }
    }
  }

  head = data + used;
  cc = recv(ap->sock, head, TCP_RCVSIZ, 0);

  if (cc <= 0)
  {
    cc = errno;
    if (cc != EWOULDBLOCK)
    {
#ifdef LOG_VERBOSE
      fprintf(flog, "RECV\t%s\n", strerror(cc));
#endif
      return 0;
    }

    /* would block, so leave it to do later */

    return -1;
  }

  head[cc] = '\0';
  ap->used = (used += cc);

  /* itoc.050807: recv() 一次還讀不完的，一定是 cmd_dopost 或 cmd_domail，這二者的結束都有 &end= */
  if (used >= TCP_RCVSIZ)
  {
    /* 多 -2 是因為有些瀏覽器會自動補上 \r\n */
    if (!strstr(head + cc - strlen("&end=") - 2, "&end="))    /* 還沒讀完，繼續讀 */
      return 1;
  }

  mode = 0;
  head = data;

  while (cc = *head)
  {
    if (cc == '\n')
    {
      data++;
    }
    else if (cc == '\r')
    {
      *head = '\0';

      if ((mode = do_cmd(ap, data, head, mode)) < 0)
      {
        fflush(ap->fpw);    /* do_cmd() 回傳 -1 表示結束，就 fflush 所有結果 */
        return 0;
      }

      data = head + 1;
    }
    head++;
  }

  return 0;
}


/* ----------------------------------------------------- */
/* accept a new connection                               */
/* ----------------------------------------------------- */

static int
agent_accept(ipaddr)
  unsigned int *ipaddr;
{
  int csock;
  int value;
  struct sockaddr_in csin;

  for (;;)
  {
    value = sizeof(csin);
    csock = accept(0, (struct sockaddr *) & csin, &value);
    /* if (csock > 0) */
    if (csock >= 0)        /* Thor.000126: more proper */
      break;

    csock = errno;
    if (csock != EINTR)
    {
#ifdef LOG_VERBOSE
      fprintf(flog, "ACCEPT\t%s\n", strerror(csock));
#endif
      return -1;
    }

    while (waitpid(-1, NULL, WNOHANG | WUNTRACED) > 0);
  }

  value = 1;
  /* Thor.000511: 註解: don't delay send to coalesce(聯合) packets */
  setsockopt(csock, IPPROTO_TCP, TCP_NODELAY, (char *)&value, sizeof(value));

  *ipaddr = csin.sin_addr.s_addr;

  return csock;
}


/* ----------------------------------------------------- */
/* signal routines                                       */
/* ----------------------------------------------------- */

#ifdef  SERVER_USAGE
static void
servo_usage()
{
  struct rusage ru;

  if (getrusage(RUSAGE_SELF, &ru))
    return;

  fprintf(flog,
    "\n[Server Usage]\n\n"
    " user time: %.6f\n"
    " system time: %.6f\n"
    " maximum resident set size: %lu P\n"
    " integral resident set size: %lu\n"
    " page faults not requiring physical I/O: %ld\n"
    " page faults requiring physical I/O: %ld\n"
    " swaps: %ld\n"
    " block input operations: %ld\n"
    " block output operations: %ld\n"
    " messages sent: %ld\n"
    " messages received: %ld\n"
    " signals received: %ld\n"
    " voluntary context switches: %ld\n"
    " involuntary context switches: %ld\n",

    (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec / 1000000.0,
    (double)ru.ru_stime.tv_sec + (double)ru.ru_stime.tv_usec / 1000000.0,
    ru.ru_maxrss,
    ru.ru_idrss,
    ru.ru_minflt,
    ru.ru_majflt,
    ru.ru_nswap,
    ru.ru_inblock,
    ru.ru_oublock,
    ru.ru_msgsnd,
    ru.ru_msgrcv,
    ru.ru_nsignals,
    ru.ru_nvcsw,
    ru.ru_nivcsw);

  fflush(flog);
}
#endif


static void
sig_term(sig)
  int sig;
{
  char buf[80];

  sprintf(buf, "sig: %d, errno: %d => %s", sig, errno, strerror(errno));
  logit("EXIT", buf);
  fclose(flog);
  exit(0);
}


static void
reaper()
{
  while (waitpid(-1, NULL, WNOHANG | WUNTRACED) > 0);
}


static void
servo_signal()
{
  struct sigaction act;

  /* sigblock(sigmask(SIGPIPE)); *//* Thor.981206: 統一 POSIX 標準用法 */

  /* act.sa_mask = 0; *//* Thor.981105: 標準用法 */
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  act.sa_handler = sig_term;
  sigaction(SIGTERM, &act, NULL);    /* forced termination */
  sigaction(SIGSEGV, &act, NULL);    /* if rlimit violate */
  sigaction(SIGBUS, &act, NULL);

#if 1    /* Thor.990203: 抓 signal */
  sigaction(SIGURG, &act, NULL);
  sigaction(SIGXCPU, &act, NULL);
  sigaction(SIGXFSZ, &act, NULL);

#ifdef SOLARIS
  sigaction(SIGLOST, &act, NULL);
  sigaction(SIGPOLL, &act, NULL);
  sigaction(SIGPWR, &act, NULL);
#endif

#ifdef LINUX
  sigaction(SIGSYS, &act, NULL);
  /* sigaction(SIGEMT, &act, NULL); */
  /* itoc.010317: 我的 linux 沒有這個說 :p */
#endif

  sigaction(SIGFPE, &act, NULL);
  sigaction(SIGWINCH, &act, NULL);
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGQUIT, &act, NULL);
  sigaction(SIGILL, &act, NULL);
  sigaction(SIGTRAP, &act, NULL);
  sigaction(SIGABRT, &act, NULL);
  sigaction(SIGTSTP, &act, NULL);
  sigaction(SIGTTIN, &act, NULL);
  sigaction(SIGTTOU, &act, NULL);
  sigaction(SIGVTALRM, &act, NULL);
#endif

  sigaction(SIGHUP, &act, NULL);

  act.sa_handler = reaper;
  sigaction(SIGCHLD, &act, NULL);

#ifdef  SERVER_USAGE
  act.sa_handler = servo_usage;
  sigaction(SIGPROF, &act, NULL);
#endif

  /* Thor.981206: lkchu patch: 統一 POSIX 標準用法 */
  /* 在此借用 sigset_t act.sa_mask */
  sigaddset(&act.sa_mask, SIGPIPE);
  sigprocmask(SIG_BLOCK, &act.sa_mask, NULL);
}


/* ----------------------------------------------------- */
/* server core routines                                  */
/* ----------------------------------------------------- */

static void
servo_daemon(inetd)
  int inetd;
{
  int fd, value;
  char buf[80];
  struct linger ld;
  struct sockaddr_in sin;

#ifdef HAVE_RLIMIT
  struct rlimit limit;
#endif

  /* More idiot speed-hacking --- the first time conversion makes the C     *
   * library open the files containing the locale definition and time zone. *
   * If this hasn't happened in the parent process, it happens in the       *
   * children, once per connection --- and it does add up.                  */

  time((time_t *) & value);
  gmtime((time_t *) & value);
  strftime(buf, 80, "%d/%b/%Y:%H:%M:%S", localtime((time_t *) & value));

#ifdef HAVE_RLIMIT
  /* --------------------------------------------------- */
  /* adjust the resource limit                           */
  /* --------------------------------------------------- */

  getrlimit(RLIMIT_NOFILE, &limit);
  limit.rlim_cur = limit.rlim_max;
  setrlimit(RLIMIT_NOFILE, &limit);

  limit.rlim_cur = limit.rlim_max = 16 * 1024 * 1024;
  setrlimit(RLIMIT_FSIZE, &limit);

  limit.rlim_cur = limit.rlim_max = 16 * 1024 * 1024;
  setrlimit(RLIMIT_DATA, &limit);

#ifdef SOLARIS
#define RLIMIT_RSS RLIMIT_AS    /* Thor.981206: port for solaris 2.6 */
#endif

  setrlimit(RLIMIT_RSS, &limit);

  limit.rlim_cur = limit.rlim_max = 0;
  setrlimit(RLIMIT_CORE, &limit);
#endif

  /* --------------------------------------------------- */
  /* detach daemon process                               */
  /* --------------------------------------------------- */

  close(1);
  close(2);

  if (inetd)
    return;

  close(0);

  if (fork())
    exit(0);

  setsid();

  if (fork())
    exit(0);

  /* --------------------------------------------------- */
  /* setup socket                                        */
  /* --------------------------------------------------- */

  fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  value = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&value, sizeof(value));

  ld.l_onoff = ld.l_linger = 0;
  setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *)&ld, sizeof(ld));

  sin.sin_family = AF_INET;
  sin.sin_port = htons(BHTTP_PORT);
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  memset((char *)&sin.sin_zero, 0, sizeof(sin.sin_zero));

  if (bind(fd, (struct sockaddr *) & sin, sizeof(sin)) || listen(fd, TCP_BACKLOG))
    exit(1);
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int n, sock, cc;
  time_t uptime, tcheck, tfresh;
  Agent **FBI, *Scully, *Mulder, *agent;
  fd_set rset;
  struct timeval tv;

  cc = 0;

  while ((n = getopt(argc, argv, "i")) != -1)
  {
    switch (n)
    {
    case 'i':
      cc = 1;
      break;

    default:
      fprintf(stderr, "Usage: %s [options]\n"
    "\t-i  start from inetd with wait option\n",
    argv[0]);
      exit(0);
    }
  }

  servo_daemon(cc);

  setgid(BBSGID);
  setuid(BBSUID);
  chdir(BBSHOME);

  init_bshm();
  init_ushm();
  init_fshm();

  servo_signal();

  log_open();
  dns_init();

  uptime = time(0);
  tcheck = uptime + BHTTP_PERIOD;
  tfresh = uptime + BHTTP_FRESH;

  Scully = Mulder = NULL;

  for (;;)
  {
    /* maintain : resource and garbage collection */

    uptime = time(0);
    if (tcheck < uptime)
    {
      /* ----------------------------------------------- */
      /* 將過久沒有動作的 agent 踢除                     */
      /* ----------------------------------------------- */

      tcheck = uptime - BHTTP_TIMEOUT;

      for (FBI = &Scully; agent = *FBI;)
      {
        if (agent->uptime < tcheck)
        {
          agent_fire(agent);

          *FBI = agent->anext;

          agent->anext = Mulder;
          Mulder = agent;
        }
        else
        {
          FBI = &(agent->anext);
        }
      }

      /* ----------------------------------------------- */
      /* maintain server log                             */
      /* ----------------------------------------------- */

      if (tfresh < uptime)
      {
        tfresh = uptime + BHTTP_FRESH;
#ifdef SERVER_USAGE
        servo_usage();
#endif
        log_fresh();
      }
      else
      {
        fflush(flog);
      }

      tcheck = uptime + BHTTP_PERIOD;
    }

    /* ------------------------------------------------- */
    /* Set up the fdsets                                 */
    /* ------------------------------------------------- */

    FD_ZERO(&rset);
    FD_SET(0, &rset);

    n = 0;
    for (agent = Scully; agent; agent = agent->anext)
    {
      sock = agent->sock;

      if (n < sock)
        n = sock;

      FD_SET(sock, &rset);
    }

    /* in order to maintain history, timeout every BHTTP_PERIOD seconds in case no connections */
    tv.tv_sec = BHTTP_PERIOD;
    tv.tv_usec = 0;
    if (select(n + 1, &rset, NULL, NULL, &tv) <= 0)
      continue;

    /* ------------------------------------------------- */
    /* serve active agents                               */
    /* ------------------------------------------------- */

    uptime = time(0);

    for (FBI = &Scully; agent = *FBI;)
    {
      sock = agent->sock;

      if (FD_ISSET(sock, &rset))
        cc = agent_recv(agent);
      else
        cc = -1;

      if (cc == 0)
      {
        agent_fire(agent);

        *FBI = agent->anext;

        agent->anext = Mulder;
        Mulder = agent;

        continue;
      }

      if (cc > 0)        /* 還有資料要 recv */
        agent->uptime = uptime;

      FBI = &(agent->anext);
    }

    /* ------------------------------------------------- */
    /* serve new connection                              */
    /* ------------------------------------------------- */

    /* Thor.000209: 考慮移前此部分, 免得卡在 accept() */
    if (FD_ISSET(0, &rset))
    {
      unsigned int ip_addr;

      sock = agent_accept(&ip_addr);
      if (sock > 0)
      {
        Agent *anext;

        if (agent = Mulder)
        {
          anext = agent->anext;
        }
        else
        {
          if (!(agent = (Agent *) malloc(sizeof(Agent))))
          {
            fcntl(sock, F_SETFL, M_NONBLOCK);
            shutdown(sock, 2);
            close(sock);

#ifdef LOG_VERBOSE
            fprintf(flog, "ERROR\tNot enough space in main()\n");
#endif
            continue;
          }
          anext = NULL;
        }

        /* variable initialization */

        memset(agent, 0, sizeof(Agent));

        agent->sock = sock;
        agent->tbegin = agent->uptime = uptime;

        agent->ip_addr = ip_addr;

        if (!(agent->data = (char *) malloc(MIN_DATA_SIZE)))
        {
          agent_fire(agent);
#ifdef LOG_VERBOSE
          fprintf(flog, "ERROR\tNot enough space in agent->data\n");
#endif
          continue;
        }
        agent->size = MIN_DATA_SIZE;
        agent->used = 0;

        agent->fpw = fdopen(sock, "w");

        Mulder = anext;
        *FBI = agent;
      }
    }

    /* ------------------------------------------------- */
    /* tail of main loop                                 */
    /* ------------------------------------------------- */
  }

  logit("EXIT", "shutdown");
  fclose(flog);

  exit(0);
}

