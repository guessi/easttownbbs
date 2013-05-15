/*-------------------------------------------------------*/
/* util/fixrform.c        ( NTHU CS MapleBBS Ver 3.00 )  */
/*-------------------------------------------------------*/
/* target : 重整註冊單                                   */
/* create : 05/01/10                                     */
/* update :   /  /                                       */
/* author : chwaian.bbs@cpu.tfcis.org                    */
/*-------------------------------------------------------*/


#include "bbs.h"


static int
find_rform(rform2, fp)
  RFORM *rform2;
  FILE *fp;
{
  RFORM rform;
  int rc;

  rc = 0;

  fseek(fp, sizeof(RFORM), 0);  /* reset fread */

  while (fread(&rform, sizeof(RFORM), 1, fp) == 1)
  {
    if (rform2->userno == rform.userno)
    {
      rform2->rtime = rform.rtime;

      strcpy(rform2->userid, rform.userid);
      strcpy(rform2->agent, rform.agent);
//      strcpy(rform2->realname, rform.realname);
      strcpy(rform2->year, rform.year);
      strcpy(rform2->month, rform.month);
      strcpy(rform2->day, rform.day);
      strcpy(rform2->career, rform.career);
      strcpy(rform2->address, rform.address);
      strcpy(rform2->phone, rform.phone);
      strcpy(rform2->reply, rform.reply);
      rc = 1;
    }
  }

  return rc;
}


/*-------------------------------------------------------*/
/* 主程式                                                */
/*-------------------------------------------------------*/


int
main()
{
  int numuser;          /* 總共有幾個註冊帳號 */
  int point;
  RFORM rform2;
  FILE *fp;

  chdir(BBSHOME);

  unlink("run/rform.log.bak");
  numuser = rec_num(FN_SCHEMA, sizeof(SCHEMA));

  if (!(fp = fopen(FN_RUN_RFORM_LOG, "r")))
    return 0;

  for (point = 1; point < numuser; point++)
  {
    rform2.userno = point;
    if (find_rform(&rform2, fp))
    rec_add("run/rform.log.bak", &rform2, sizeof(RFORM));
  }

  fclose(fp);

  /* f_mv("run/rform.log.bak","run/rform.log"); */
  /* test過，覺得都沒問題的話，才用這行 */

  return 0;
}
