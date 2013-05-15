/*-------------------------------------------------------*/
/* target : �n�Y���� (Where To Eat)                      */
/* create : 10.05.21                                     */
/* update : 11.08.21                                     */
/* author : guessi.bbs@bbs.ndhu.edu.tw                   */
/*-------------------------------------------------------*/
/* run/wtoeat/_/.DIR - control header                    */
/* run/wtoeat/_/@/@_ - description file                  */
/* run/wtoeat/_/@/G_ - log file                          */
/*-------------------------------------------------------*/

#include "bbs.h"

extern int TagNum;
extern XZ xz[];
extern char xo_pool[];

static int wte_add();
static int wte_body();
static int wte_head();

static int
cmpbtime(wte)
  WTE *wte;
{
  return wte->btime == currchrono;
}


static char
wte_attr(wte)
  WTE *wte;
{
  if (wte->mode & WTE_MARKED)
    return 'm';

  return ' ';
}


static int
wte_stamp(folder, wte, fpath)
  char *folder;
  WTE *wte;
  char *fpath;
{
  char *fname;
  char *family = NULL;
  int rc;
  int token;

  fname = fpath;
  while (rc = *folder++)
  {
    *fname++ = rc;
    if (rc == '/')
      family = fname;
  }

  fname = family;
  *family++ = '@';

  token = time(0);

  archiv32(token, family);

  rc = open(fpath, O_WRONLY | O_CREAT | O_EXCL, 0600);
  wte->btime = token;
  str_stamp(wte->date, &wte->btime);
  strcpy(wte->xname, fname);

  return rc;
}


static void
wte_fpath(fpath, folder, wte)
  char *fpath;
  char *folder;
  WTE *wte;
{
  char *str;
  int cc;

  while (cc = *folder++)
  {
    *fpath++ = cc;
    if (cc == '/')
      str = fpath;
  }
  strcpy(str, wte->xname);
}


static int
wte_init(xo)
  XO *xo;
{
  xo_load(xo, sizeof(WTE));
  return wte_head(xo);
}

static int
wte_load(xo)
  XO *xo;
{
  xo_load(xo, sizeof(WTE));
  return wte_body(xo);
}

int
tag_char(chrono)
  int chrono;
{
  return TagNum && !Tagger(chrono, 0, TAG_NIN) ? '*' : ' ';
}

static void
wte_item(num, wte)
  int num;
  WTE *wte;
{
  prints("%6d %s%c\033[m%c%-5s %-13s [\033[1;37m%s\033[m] %s\n",
         num, (wte->mode & WTE_MARKED) ? "\033[1;36m" : "", wte_attr(wte),
         tag_char(wte->btime), wte->date + 3, wte->owner, wte->class, wte->title);
}

static int
wte_body(xo)
  XO *xo;
{
  WTE *wte;
  int num, max, tail;

  max = xo->max;

  if (max <= 0)
  {
    if (vans("�n�s�W������T��(Y/N)�H[N] ") == 'y')
      return wte_add(xo);
    return XO_QUIT;
  }

  wte = (WTE *) xo_pool;
  num = xo->top;
  tail = num + XO_TALL;

  if (max > tail)
    max = tail;

  move(3, 0);
  do
  {
    wte_item(++num, wte++);
  } while (num < max);
  clrtobot();

  return XO_FOOT;
}


static int
wte_head(xo)
  XO *xo;
{
  vs_head("�����Ѧa", str_site);
  prints(NECKER_TOEAT, d_cols, "");
  return wte_body(xo);
}

static int
wte_tag(xo)
  XO *xo;
{
  WTE *wte;
  int tag, pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  wte = (WTE *) xo_pool + cur;

  if (tag = Tagger(wte->btime, pos, TAG_TOGGLE))
  {
    move(3 + cur, 0);
    wte_item(pos + 1, wte);
  }

  /* return XO_NONE; */
  return xo->pos + 1 + XO_MOVE; /* lkchu.981201: ���ܤU�@�� */
}

static int
wte_memo_edit(xo)
  XO *xo;
{
  int mode;
  char fpath[64];

  if (!cuser.userlevel)
    return XO_NONE;

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  mode = vans("�i�O�e�� (D)�R�� (E)�ק� (Q)�����H[E] ");

  if (mode != 'q')
  {
    sprintf(fpath, "run/wtoeat/%s", fn_note);

    if (mode == 'd')
      unlink(fpath);

    if (vedit(fpath, 0))  /* Thor.981020: �`�N�Qtalk�����D */
      vmsg(msg_cancel);
  }
  return XO_HEAD;
}

static int
wte_memo(xo)
  XO *xo;
{
  char fpath[64];

  sprintf(fpath, "run/wtoeat/%s", fn_note);

  if (more(fpath, NULL) < 0)
  {
    vmsg("�ثe�S���i�O�e����");
    return XO_FOOT;
  }

  return XO_HEAD;
}


static int
wte_mark(xo)
  XO *xo;
{
  WTE *wte;

  if (!cuser.userlevel)
    return XO_NONE;

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  wte = (WTE *) xo_pool + (xo->pos - xo->top);

  wte->mode ^= WTE_MARKED;
  currchrono = wte->btime;
  rec_put(xo->dir, wte, sizeof(WTE), xo->pos, cmpbtime);

  move(xo->pos - xo->top + 3, 0);
  wte_item(xo->pos + 1, wte);

  return XO_NONE;
}

#define NUM_PREFIX 7

static int
wte_add(xo)
  XO *xo;
{
  WTE wte;
  FILE *fp;
  int fd, ans;
  char *dir, fpath[64], path[64];
  char *class, *title, *address, *phone, *during, *description;
  char *prefix[NUM_PREFIX] = {"�Ӿ�", "����", "����", "���I", "�I��", "�d�]", "��L"};

  if (!cuser.userlevel)
    return XO_NONE;

  /* �S���o���v�����ϥΪ̱N�L�k�ϥΥ��\�� */
  if (!HAS_PERM(PERM_POST))
    return XO_FOOT;

  memset(&wte, 0, sizeof(WTE));

  class = wte.class;
  title = wte.title;
  address = wte.address;
  phone = wte.phone;
  during = wte.during;
  description = wte.description;

  clear();
  film_out(FILM_POST, 0);

  move(b_lines - 5, 0);
  prints("������T\n");
  outs("���O�G");
  for (ans = 0; ans < NUM_PREFIX; ans++)
     prints("%d.[%s] ", ans + 1, prefix[ans]);
  ans = vget(b_lines - 3, 0, "�п�ܬ��������]�� Enter ���L�^�G", fpath, 3, DOECHO) - '1';

  if (ans < 0 || ans >= NUM_PREFIX)   /* ��J�Ʀr�ﶵ */
    ans = 0;

  strcpy(class, prefix[ans]);


  if (!vget(b_lines - 2, 0, "�Ӯa�W�١G", title, sizeof(wte.title), DOECHO) &&
      !vget(b_lines - 1, 0, "�Ӯa�a�}�G", address, sizeof(wte.address), DOECHO) &&
      !vget(b_lines, 0, "�p���q�ܡG", phone, sizeof(wte.phone), DOECHO) &&
      !vget(b_lines, 0, "��~�ɶ��G", during, sizeof(wte.during), DOECHO) &&
      !vget(b_lines, 0, "�Ӯa²�z�G", description, sizeof(wte.description), DOECHO))
  {
    vmsg(msg_cancel);
    return wte_head(xo);
  }

  sprintf(path, "tmp/%s.wte", cuser.userid);  /* �s�p��]���Ȧs�ɮ� */
  if (fd = vedit(path, 0))
  {
    unlink(path);
    vmsg(msg_cancel);
    return wte_head(xo);
  }

  dir = xo->dir;
  if ((fd = wte_stamp(dir, &wte, fpath)) < 0)
    return wte_head(xo);
  close(fd);

  strcpy(wte.owner, cuser.userid);

  fp = fopen(fpath, "a");
  /* ���Y��r �榡�T�w */
  fprintf(fp, "�@��: %s (%s) ����: ��������\n", cuser.userid, cuser.username);
  fprintf(fp, "���D: %s\n", wte.title);
  fprintf(fp, "�ɶ�: %s\n\n", Now());
  /* ���e��r �榡�ۭq */
  fprintf(fp, "���������G%s\n", wte.class);
  fprintf(fp, "�Ӯa�W�١G%s\n", wte.title);
  fprintf(fp, "�a    �}�G%s\n", wte.address);
  fprintf(fp, "�p���q�ܡG%s\n", wte.phone);
  fprintf(fp, "��~�ɶ��G%s\n", wte.during);
  fprintf(fp, "�Ӯa²�z�G%s\n", wte.description);
  fprintf(fp, "�i�K����G%s\n", wte.date);
  fprintf(fp, "--\n");
  fprintf(fp, "�ԲӴy�z�G\n");
  f_suck(fp, path);
  unlink(path);
  fclose(fp);

  rec_add(dir, &wte, sizeof(WTE));

  move(0, 0);
  clrtobot();
  outs("���Q�K�X�峹�A���§A�����ɡI");
  vmsg(NULL);

  /* �s�W��A����Ӷ���(�̫�@��) */
  xo->pos = xo->max;

  return XO_INIT;
}


static int
wte_browse(xo)
  XO *xo;
{
  int key;
  WTE *wte;
  char fpath[80];

  /* itoc.010304: ���F���\Ū��@�b�]�i�H�[�J�s�p�A�Ҽ{ more �Ǧ^�� */
  for (;;)
  {
    wte = (WTE *) xo_pool + (xo->pos - xo->top);
    wte_fpath(fpath, xo->dir, wte);

    if ((key = more(fpath, FOOTER_TOEAT)) < 0)
      break;

    if (!key)
      key = vkey();

    switch (key)
    {
      case KEY_UP:
      case KEY_PGUP:
      case '[':
      case 'j':
        key = xo->pos - 1;

        if (key < 0)
        break;

        xo->pos = key;

        if (key <= xo->top)
        {
          xo->top = (key / XO_TALL) * XO_TALL;
          wte_load(xo);
        }
        continue;

      case KEY_DOWN:
      case KEY_PGDN:
      case ']':
      case 'k':
      case ' ':
        key = xo->pos + 1;

        if (key >= xo->max)
          break;

        xo->pos = key;

        if (key >= xo->top + XO_TALL)
        {
          xo->top = (key / XO_TALL) * XO_TALL;
          wte_load(xo);
        }
        continue;

      case 'h':
        xo_help("wtoeat");
        break;
    }
    break;
  }

  return wte_head(xo);
}


static int
wte_delete(xo)
  XO *xo;
{
  WTE *wte;
  char *fname, fpath[80];
  char *list = "@G";    /* itoc.����: �M newbrd file */

  wte = (WTE *) xo_pool + (xo->pos - xo->top);

  if (!cuser.userlevel)
    return XO_NONE;

  if (wte->mode & WTE_MARKED)
    return XO_NONE;

  if (strcmp(cuser.userid, wte->owner) && !HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  if (vans(msg_del_ny) != 'y')
    return XO_FOOT;

  wte_fpath(fpath, xo->dir, wte);
  fname = strrchr(fpath, '@');
  while (*fname = *list++)
  {
    unlink(fpath);  /* Thor: �T�w�W�r�N�� */
  }

  currchrono = wte->btime;
  rec_del(xo->dir, sizeof(WTE), xo->pos, cmpbtime);
  return wte_init(xo);
}



void
update_content(xo, wte)  /* itoc.010709: �ק�峹���D���K�ק鷺�媺���D */
  XO *xo;
  WTE *wte;
{
  FILE *fpr, *fpw;
  char srcfile[64], tmpfile[64], buf[ANSILINELEN];

  wte_fpath(srcfile, xo->dir, wte);
  strcpy(tmpfile, "tmp/");
  strcat(tmpfile, wte->xname);

  f_cp(srcfile, tmpfile, O_TRUNC);

  if (!(fpr = fopen(tmpfile, "r")))
    return;

  if (!(fpw = fopen(srcfile, "w")))
  {
    fclose(fpr);
    return;
  }

  while(fgets(buf, sizeof(buf), fpr)) /* �[�J��L */
  {
    if (!str_ncmp(buf, "���D:", 5))
      sprintf(buf, "���D: [%s] %s\n", wte->class, wte->title);

    if (!str_ncmp(buf, "��������", 8))
      sprintf(buf, "���������G%s\n", wte->class);

    if (!str_ncmp(buf, "�Ӯa�W��", 8))
      sprintf(buf, "�Ӯa�W�١G%s\n", wte->title);

    if (!str_ncmp(buf, "�a    �}", 8))
      sprintf(buf, "�a    �}�G%s\n", wte->address);

    if (!str_ncmp(buf, "�p���q��", 8))
      sprintf(buf, "�p���q�ܡG%s\n", wte->phone);

    if (!str_ncmp(buf, "��~�ɶ�", 8))
      sprintf(buf, "��~�ɶ��G%s\n", wte->during);

    if (!str_ncmp(buf, "�Ӯa²�z", 8))
      sprintf(buf, "�Ӯa²�z�G%s\n", wte->description);

    fputs(buf, fpw);
  }
  fclose(fpr);
  fclose(fpw);
  f_rm(tmpfile);
}


static int
wte_edit(xo)
  XO *xo;
{
  WTE *wte, xwte;
  char ans[6];

  if (!cuser.userlevel)
    return XO_NONE;

  vs_bar("�Բӳ]�w");

  wte = (WTE *) xo_pool + (xo->pos - xo->top);
  memcpy(&xwte, wte, sizeof(WTE));

  prints("\n");
  prints("���������G%s\n", xwte.class);
  prints("�Ӯa�W�١G%s\n", xwte.title);
  prints("�a    �}�G%s\n", xwte.address);
  prints("�p���q�ܡG%s\n", xwte.phone);
  prints("��~�ɶ��G%s\n", xwte.during);
  prints("�Ӯa²�z�G%s\n", xwte.description);

  if (vget(9, 0, "�O�_�n�ק�o����� (E)�]�w (Q)�����H[Q] ", ans, 3, LCECHO) == 'e')
  {
    /* �̫�ק諸�ϥΪ��ܧ󬰾֦��� */
    strcpy(xwte.owner, cuser.userid);
    vget(11, 0, "���������G", xwte.class, sizeof(xwte.class), GCARRY);
    vget(12, 0, "�Ӯa�W�١G", xwte.title, sizeof(xwte.title), GCARRY);
    vget(13, 0, "�a    �}�G", xwte.address, sizeof(xwte.address), GCARRY);
    vget(14, 0, "�p���q�ܡG", xwte.phone, sizeof(xwte.phone), GCARRY);
    vget(15, 0, "��~�ɶ��G", xwte.during, sizeof(xwte.during), GCARRY);
    vget(16, 0, "�Ӯa²�z�G", xwte.description, sizeof(xwte.description), GCARRY);

    move(b_lines - 2, 0);
    prints("�Y���Ӯa�ԲӤ��СA�Ы�[\033[1;35m�j�gS\033[m]�]�w\n");

    if (memcmp(&xwte, wte, sizeof(WTE)) &&
        vans("�нT�{�O�_�ק�H (Y/N)[N] ") == 'y')
    {
      memcpy(wte, &xwte, sizeof(WTE));
      currchrono = wte->btime;
      rec_put(xo->dir, wte, sizeof(WTE), xo->pos, cmpbtime);
      update_content(xo, wte);
    }
  }

  return wte_init(xo);
}

static int
wte_setup(xo)
  XO *xo;
{
  char fpath[64];
  WTE *wte;

  wte = (WTE *) xo_pool + (xo->pos - xo->top);

  if (!cuser.userlevel)
    return XO_NONE;

  wte_fpath(fpath, xo->dir, wte);
  vedit(fpath, 0);

  strcpy(wte->owner, cuser.userid);
  currchrono = wte->btime;
  rec_put(xo->dir, wte, sizeof(WTE), xo->pos, cmpbtime);

  return wte_head(xo);
}

void
update_title(xo, wte)  /* itoc.010709: �ק�峹���D���K�ק鷺�媺���D */
  XO *xo;
  WTE *wte;
{
  FILE *fpr, *fpw;
  char srcfile[64], tmpfile[64], buf[ANSILINELEN];

  wte_fpath(srcfile, xo->dir, wte);
  strcpy(tmpfile, "tmp/");
  strcat(tmpfile, wte->xname);

  f_cp(srcfile, tmpfile, O_TRUNC);

  if (!(fpr = fopen(tmpfile, "r")))
    return;

  if (!(fpw = fopen(srcfile, "w")))
  {
    fclose(fpr);
    return;
  }

  fgets(buf, sizeof(buf), fpr);   /* �[�J�@�� */
  fputs(buf, fpw);

  fgets(buf, sizeof(buf), fpr);   /* �[�J���D */
  if (!str_ncmp(buf, "��", 2))    /* �p�G�� header �~�� */
  {
    sprintf(buf, "���D: [%s] %s\n", wte->class, wte->title);
  }
  fputs(buf, fpw);

  while(fgets(buf, sizeof(buf), fpr)) /* �[�J��L */
    fputs(buf, fpw);

  fclose(fpr);
  fclose(fpw);
  f_rm(tmpfile);
}

static int
wte_title(xo)
  XO *xo;
{
  int ans;
  WTE *wte, xwte;
  wte = (WTE *) xo_pool + (xo->pos - xo->top);
  char *prefix[NUM_PREFIX] = {"����", "�Ӿ�", "����", "����", "���I", "�d�]", "��L"};

  if (!cuser.userlevel)
    return XO_NONE;

  memcpy(&xwte, wte, sizeof(WTE));

  move(b_lines - 2, 0);
  clrtobot();
  prints("\n");
  outs("���O�G");
  for (ans = 0; ans < NUM_PREFIX; ans++)
    prints("%d.[%s] ", ans + 1, prefix[ans]);

  ans = vans("�п�ܷj�M�ؼФ����]�� Enter ���L�^�G ") - '1';

  if (ans >= 0 && ans < NUM_PREFIX)   /* ��J�Ʀr�ﶵ */
    strcpy(xwte.class, prefix[ans]);
  else
    strcpy(xwte.class, prefix[0]);

  move(b_lines - 2, 0);
  clrtobot();

  if (!vget(b_lines, 0, "�Ӯa�W�١G", xwte.title, sizeof(xwte.title), GCARRY))
    return XO_FOOT;

  if (memcmp(wte, &xwte, sizeof(WTE)) &&
      vans("�нT�{�O�_�ק� (Y/N)[N] ") == 'y')
  {
    memcpy(wte, &xwte, sizeof(WTE));
    currchrono = wte->btime;
    rec_put(xo->dir, wte, sizeof(WTE), xo->pos, cmpbtime);
    update_title(xo, wte);
  }

  return wte_head(xo);
}

int
wte_recommend(xo)
  XO *xo;
{
  WTE *wte, *wtmp;
  int *list, i, ans, fsize, count;
  char type[BCLEN + 1], *fimage, *prefix[NUM_PREFIX] = {"����", "�Ӿ�", "����", "����", "���I", "�d�]", "��L"};

  /* �Y�b�Ҧ����ؤ����S�����w�����O �N����F */
  fimage = f_map(xo->dir, &fsize);

  if (fimage == (char *) -1)
  {
    vmsg("�ثe�L�k�}�ү�����");
    return XO_FOOT;
  }

  count = 0;

  do {
    count = 0;

    move(b_lines - 2, 0);
    clrtobot();
    prints("\n");
    outs("���O�G");
    for (ans = 0; ans < NUM_PREFIX; ans++)
      prints("%d.[%s] ", ans + 1, prefix[ans]);

    ans = vans("�п�ܷj�M�ؼФ����]�� Enter ���L�^�G ") - '1';

    srand((unsigned) time(NULL));
    /* �Y�S�����w���� �H���D�@�ӦL�X */
    if (ans > 0 && ans < NUM_PREFIX)
      strcpy(type, prefix[ans]);
    else
      strcpy(type, prefix[rand() % NUM_PREFIX]);

    /* �Ȧs�����j�M���G (�ܦh�� xo->max ��) */
    list = (int *) malloc(sizeof(int) * xo->max);

    for (i = 0; i < xo->max; i++)
    {
      wtmp = (WTE *) fimage + i;
      if (!strcmp(wtmp->class, type))
        list[count++] = i;
    }

    if (count <= 0)
      vmsg("�������G���L���ءA�Э��s��ܤ���");

  } while (count <= 0);

  do {
    /* �w��list��������m �H���D�X�@�� */
    srand((unsigned) time(NULL));
    xo->pos = list[rand() % count];
    xo_load(xo, sizeof(WTE));
    wte = (WTE *) xo_pool + (xo->pos - xo->top);

    /* ����N���G�C�L�X */
    move(b_lines - 8, 0);
    clrtobot();

    prints("\n�z�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�{\n" \
           "�x���������G%s%*s�x\n" \
           "�x�Ӯa�W�١G%s%*s�x\n" \
           "�x�a    �}�G%s%*s�x\n" \
           "�x�Ӯa²�z�G%s%*s�x\n" \
           "�|�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�}\n", \
           wte->class,       64 - strlen(wte->class),       "", \
           wte->title,       64 - strlen(wte->title),       "", \
           wte->address,     64 - strlen(wte->address),     "", \
           wte->description, 64 - strlen(wte->description), "");
  } while (vans("���п�ܰʧ@�H C)�~�� Q)�h�X [C] ") != 'q');

  free(list);

  return XO_HEAD;
}


int
wte_write(xo)      /* itoc.010328: ��u�W�@�̤��y */
  XO *xo;
{
  if (HAS_PERM(PERM_PAGE))
  {
    WTE *wte;
    UTMP *up;

    wte = (WTE *) xo_pool + (xo->pos - xo->top);

    /* ����i�K���@�w�O�����ϥΪ� */
    if (up = utmp_get(0, wte->owner))
      do_write(up);
  }
  return XO_NONE;
}

static int
wte_cmp(a, b)
  WTE *a;
  WTE *b;
{
  return str_cmp(a->class, b->class);
}

static int
wte_sort(xo)
  XO *xo;
{
  if (vans("���O�_����ƧǡH Y)�T�{ N)���� [N] ") != 'y')
    return XO_FOOT;

  rec_sync(xo->dir, sizeof(WTE), wte_cmp, NULL);

  return wte_load(xo);
}

static int
wte_uquery(xo)
  XO *xo;
{
  WTE *wte;

  wte = (WTE *) xo_pool + (xo->pos - xo->top);

  move(1, 0);
  clrtobot();
  my_query(wte->owner);
  return wte_head(xo);
}


static int
wte_help(xo)
  XO *xo;
{
  xo_help("wtoeat");
  return wte_head(xo);
}


static KeyFunc wte_cb[] =
{
  XO_INIT, wte_init,
  XO_LOAD, wte_load,
  XO_HEAD, wte_head,
  XO_BODY, wte_body,

  'r', wte_browse,
  'm', wte_mark,
  'd', wte_delete,
  'S', wte_setup,
  'E', wte_edit,
  'T', wte_title,
  'w', wte_write,
  't', wte_tag,
  'q', wte_recommend,
  'b', wte_memo,
  'B', wte_memo_edit,
  'a', wte_add,

  Ctrl('S'), wte_sort,
  Ctrl('P'), wte_add,
  Ctrl('Q'), wte_uquery,

  'h', wte_help
};


int
eat_where()
{
  XO *xo;
  char fpath[64];

  sprintf(fpath, "run/eatwhere/%s", fn_dir);
  xz[XZ_TOEAT - XO_ZONE].xo = xo = xo_new(fpath);
  xz[XZ_TOEAT - XO_ZONE].cb = wte_cb;
  xo->key = XZ_TOEAT;

  sprintf(fpath, "run/eatwhere/%s", fn_note);
  more(fpath, NULL);

  xover(XZ_TOEAT);
  free(xo);

  return 0;
}
