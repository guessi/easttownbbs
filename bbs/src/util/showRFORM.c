/*-------------------------------------------------------*/
/* util/showRFORM.c     ( NTHU CS MapleBBS Ver 3.10 )    */
/*-------------------------------------------------------*/
/* target : show register form                           */
/* create : 01/10/05                                     */
/* update :   /  /                                       */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/
/* syntax : showRFORM [target_user]                      */
/*-------------------------------------------------------*/


#include "bbs.h"

static void
showRFORM(r)
  RFORM *r;
{
  printf("用戶編號：%d\n", r->userno);
  printf("填表時間：%s\n", Btime(&r->rtime));
  printf("申請代號：%s\n", r->userid);
  printf("服務單位：%s\n", r->career);
  printf("目前住址：%s\n", r->address);
  printf("連絡電話：%s\n", r->phone);
  printf("審核站長：%s\n", r->agent);
  printf("退件理由：%s\n", r->reply);
  printf(MSG_SEPERATOR "\n");
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int show_allrform;
  RFORM rform;
  FILE *fp;

  if (argc < 2)
    show_allrform = 1;
  else
    show_allrform = 0;

  chdir(BBSHOME);
 
  if (!(fp = fopen(FN_RUN_RFORM_LOG, "r")))
    return -1;

  while (fread(&rform, sizeof(RFORM), 1, fp) == 1)
  {
    if (show_allrform || !str_cmp(rform.userid, argv[1]))
      showRFORM(&rform);
  }

  fclose(fp);

  return 0;
}
