#include "bbs.h"

#define UPDATE_USER_COUNT  /* 確認沒問題再改成 #define */

static BCACHE *bshm;
static UCACHE *ushm;

static void
init_bshm()
{
  bshm = shm_new(BRDSHM_KEY, sizeof(BCACHE));

  if (bshm->uptime <= 0)
    exit(0);
}

static void
init_ushm()
{
  ushm = shm_new(UTMPSHM_KEY, sizeof(UCACHE));
}

int
main()
{
  int bno, count;
  char *bname;
  BRD *head, *tail;
  UTMP *uentp, *uceil, *up;

  chdir(BBSHOME);

  init_bshm();
  init_ushm();

  bno = -1;
  head = bshm->bcache;
  tail = head + bshm->number;

  uentp = ushm->uslot;
  uceil = (void *) uentp + ushm->offset;

  /* guessi.120125 循序讀取各個看板 並計算其看板人氣值 */
  do
  {
    bname = head->brdname;

    if (!(*bname))
      continue;

    count = 0;
    up = uentp;

    /* guessi.120125 找出狀態為線上 且目前動態為閱讀看板的使用者 */
    do
    {
      if (up->userno && (up->mode == M_READA) && !strcmp(up->reading, bname))
        count++;
    } while (++up <= uceil);

    /* guessi.120125 更新看板人氣值 */
    bshm->mantime[++bno] = count;

  } while (++head < tail);

#ifdef UPDATE_USER_COUNT
  /* guessi.120125 更新站上使用者總和 */
  count = 0;
  up = uentp;

  do
  {
    if (up->userno)
      count++;
  } while (++up <= uceil);

  ushm->count = count;
#endif

  return 0;
}

