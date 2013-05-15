/*-------------------------------------------------------*/
/* util/showMailsize.c  ( NTHU CS MapleBBS Ver 3.00 )    */
/*-------------------------------------------------------*/
/* target : 觀察全站信箱使用狀況                         */
/* create : 03/09/13                                     */
/* update :   /  /                                       */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw (util/topusr.c) */
/* modify : qazq.bbs@bbs.cs.nchu.edu.tw                  */
/*-------------------------------------------------------*/

#include "bbs.h"

#define OUTFILE_MAILSIZE    BBSHOME"/gem/@/@-mailsize"

typedef struct
{
  char userid[IDLEN + 1];
  int mbox_size;
  int mbox_count;
} DATA;

static int
sort_compare(p1, p2)
  const void *p1;
  const void *p2;
{
  int k;
  DATA *a1 = (DATA *) p1;
  DATA *a2 = (DATA *) p2;
  k = a2->mbox_size - a1->mbox_size;
  return k ? k : str_cmp(a1->userid, a2->userid);
}

static int
mail_size(uid)
 char *uid;
{
  int fd, fsize,total;
  struct stat st;
  HDR *head, *tail;
  char *base, *folder, fpath[80], mbox_dir[64];
  
  sprintf(mbox_dir, BBSHOME "/usr/%c/%s/" FN_DIR, *uid, uid);

  if ((fd = open(folder = mbox_dir, O_RDWR)) < 0)
    return 0;

  fsize = 0;
  total = 0;

  if (!fstat(fd, &st) &&
      (fsize = st.st_size) >= sizeof(HDR) &&
      (base = (char *) malloc(fsize)))
  {
    f_exlock(fd);

    if ((fsize = read(fd, base, fsize)) >= sizeof(HDR))
    {
      head = (HDR *) base;
      tail = (HDR *) (base + fsize);

      do
      {
        if (head->xid > 0)
          total += head->xid;
        else
        {
          hdr_fpath(fpath, folder, head);
          stat(fpath, &st);
          total += st.st_size;
          head->xid = st.st_size;
        }
      } while (++head < tail);
    }

    lseek(fd, (off_t) 0, SEEK_SET);
    write(fd, base, fsize);

    ftruncate(fd, fsize);
    f_unlock(fd);

    free(base);
  }
  close(fd);

  return (total / 1024);
}

void write_data(fp, data, max)
  FILE *fp;
  DATA *data;
  int max;
{
  int i, end = (max < 100) ? max : 100;
  for (i = 0; i < end; i++)
  {
    if (data[i].mbox_size > 1024)
      fprintf(fp, "%-13s  %10d MB (%d)\n", data[i].userid, data[i].mbox_size/1024, data[i].mbox_count);
    else
      fprintf(fp, "%-13s  %10d KB (%d)\n", data[i].userid, data[i].mbox_size, data[i].mbox_count);
  }
  fprintf(fp, "\n");
}


int
main()
{
  FILE *fp;
  DIR *dirp;
  int i = 0, fd, max;
  struct dirent *de;
  char c, *fname, buf[64], fpath[64];
  DATA *data;

  chdir(BBSHOME);
  max = rec_num(FN_SCHEMA, sizeof(SCHEMA));

  if (!(fp = fopen(OUTFILE_MAILSIZE, "w")))
    return -1;

  data = (DATA *) malloc(sizeof(DATA) * max);
  memset(data, 0, sizeof(DATA) * max);

  for (c = 'a'; c <= 'z'; c++)
  {
    /* check the existence ofthe parent directory */
    sprintf(buf, BBSHOME "/usr/%c", c);
    chdir(buf);

    if (!(dirp = opendir(".")))
      continue;

    while (de = readdir(dirp))
    {
      chdir(buf);
      fname = de->d_name;
      if (*fname <= ' ' || *fname == '.')
        continue;

      /* if .ACCT exist, then user account must exist */
      sprintf(fpath, "%s/.ACCT", fname);
      if ((fd = open(fpath, O_RDONLY)) < 0)
        continue;
      close(fd);

      sprintf(fpath, "%s/" FN_DIR, fname);
      if ((fd = open(fname, O_RDONLY)) < 0)
        continue;

      strcpy(data[i].userid, fname);
      data[i].mbox_count = rec_num(fpath, sizeof(HDR));
      data[i].mbox_size = mail_size(fname);

      i++;
    }
    closedir(dirp);
  }

  qsort(data, i, sizeof(DATA), sort_compare);
  write_data(fp, data, i);
  free(data);
  fclose(fp);

  return 0;
}

