#include "diskio.h"
#include "ff.h"

#include "sd.h"
#include <stdio.h>

//{{{
unsigned long get_fattime (void) {
  return 0;
};
//}}}

//{{{
DSTATUS disk_initialize (BYTE pdrv) {

  //DSTATUS stat;
  //printf ("disk_initialize\n");
  return 0;
}
//}}}
//{{{
DSTATUS disk_status (BYTE pdrv) {

  //DSTATUS stat;
  //printf ("disk_status\n");
  return 0;
}
//}}}
//{{{
DRESULT disk_read (BYTE pdrv, BYTE* buff, DWORD sector, BYTE count) {

  //printf ("diskRead %d %d\n", (int)sector, (int)count);
  return (SD_readBlocks (buff, sector, 512, count) == SD_OK) ? RES_OK : RES_ERROR;
}
//}}}
//{{{
DRESULT disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, BYTE count) {

  return (SD_writeBlocks (buff, sector, 512, count) == SD_OK) ? RES_OK : RES_ERROR;
  }
//}}}

//{{{
DRESULT disk_ioctl(BYTE drv, BYTE ctrl, void *buff)
{
  switch (ctrl) {
  case CTRL_SYNC:
    return RES_OK;
  case GET_SECTOR_SIZE:
    *(WORD*)buff = 512;
    return RES_OK;
  case GET_SECTOR_COUNT:
    *(DWORD*)buff = 512000000/512;
    return RES_OK;
  case GET_BLOCK_SIZE:
    *(DWORD*)buff = 1;
    return RES_OK;
  }
  return RES_PARERR;
}
//}}}
