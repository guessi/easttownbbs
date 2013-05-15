/*-------------------------------------------------------*/
/* util/showACCT.c      ( NTHU CS MapleBBS Ver 3.10 )    */
/*-------------------------------------------------------*/
/* target : ��� ACCT ���                               */
/* create : 01/07/16                                     */
/* update :                                              */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"

static char *
_bitmsg(str, level)
  char *str;
  int level;
{
  int cc;
  int num;
  static char msg[33];

  num = 0;
  while (cc = *str)
  {
    msg[num] = (level & 1) ? cc : '-';
    level >>= 1;
    str++;
    num++;
  }
  msg[num] = 0;

  return msg;
}


static inline void
showACCT(acct)
  ACCT *acct;
{
  char msg1[40], msg2[40], msg3[40], msg4[40], msg5[40], msg6[40];

  strcpy(msg1, _bitmsg(STR_PERM, acct->userlevel));
  strcpy(msg2, _bitmsg(STR_UFO, acct->ufo));
  strcpy(msg3, Btime(&(acct->firstlogin)));
  strcpy(msg4, Btime(&(acct->lastlogin)));
  strcpy(msg5, Btime(&(acct->tcheck)));
  strcpy(msg6, Btime(&(acct->tvalid)));

  printf("--\n"
    "�s��: %-15d [ID]: %-15s �m�W: %-15s �ʺ�: %-15s \n" 
    "�v��: %-37s �]�w: %-37s \n" 
    "ñ�W: %-37d �ʧO: %-15d \n" 
    "�ȹ�: %-15d ����: %-15d �ͤ�: %02d/%02d/%02d \n" 
    "�W��: %-15d �峹: %-15d �o�H: %-15d \n" 
    "����: %-37s �W��: %-30s \n" 
    "�ˬd: %-37s �q�L: %-30s \n" 
    "�n�J: %-30s \n" 
    "����: %d��%02d��\n"
    "�H�c: %-60s \n", 
    acct->userno, acct->userid, acct->realname, acct->username, 
    msg1, msg2,
    acct->signature, acct->sex,
    acct->money, acct->gold, acct->year, acct->month, acct->day, 
    acct->numlogins, acct->numposts, acct->numemails, 
    msg3, msg4, 
    msg5, msg6, 
    acct->lasthost, 
    (acct->staytime / 60) / 60, (acct-> staytime / 60) % 60,
    acct->email);
}


int
main(argc, argv)
  int argc;
  char **argv;
{
  int i, mode;

  if (( strcmp("-r", argv[1]) && argc < 2) ||
      (!strcmp("-r", argv[1]) && argc < 3))
  {
    printf("Usage: %s UserID [UserID] ...\n", argv[0]);
    printf("       %s -r UserID [UserID] ...\n", argv[0]);
    return -1;
  }

  if (strcmp("-r", argv[1]))
  {
    i = 1;
    mode = 0;
  }
  else
  {
    i = 2;
    mode = 1;
  }

  chdir(BBSHOME);

  for ( ; i < argc; i++)
  {
    ACCT acct;
    char fpath[64];

    if (!mode)
      usr_fpath(fpath, argv[i], FN_ACCT);
    else
      sprintf(fpath, "usr/@/%s/%s", argv[i], FN_ACCT);

    if (rec_get(fpath, &acct, sizeof(ACCT), 0) < 0)
    {
      printf("%s: read error (maybe no such id?)\n", argv[i]);
      continue;
    }

    showACCT(&acct);
  }
  printf("--\n");

  return 0;
}
