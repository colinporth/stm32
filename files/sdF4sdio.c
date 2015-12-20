// sdF4sdio.c
// CLK   = PC12
// CMD   = PD02
// D0:3  = PC08:11
//{{{  includes
#include <stdio.h>

#include "sd.h"

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_sdio.h"
//}}}
//{{{  defines
#define SDIO_SLOW_CLK_DIV               118  // 0x76 = 120-2 = (48Mhz/400Khz)-2
#define SDIO_FAST_CLK_DIV               0    //    0 = (48Mhz/24Mhz)-2, 1=16Mhz, 2=12Mhz, 3=9Hz, 4=8Mhz

//{{{  CMD defines
#define CMD_GO_IDLE_STATE                       ((uint8_t)0)
#define CMD_SEND_OP_COND                        ((uint8_t)1)
#define CMD_ALL_SEND_CID                        ((uint8_t)2)
#define CMD_SET_REL_ADDR                        ((uint8_t)3) /*!< SDIO_SEND_REL_ADDR for SD Card */
#define CMD_SET_DSR                             ((uint8_t)4)
#define CMD_SDIO_SEN_OP_COND                    ((uint8_t)5)
#define CMD_HS_SWITCH                           ((uint8_t)6)
#define CMD_SEL_DESEL_CARD                      ((uint8_t)7)
#define SEND_IF_COND                            ((uint8_t)8)
#define CMD_SEND_CSD                            ((uint8_t)9)
#define CMD_SEND_CID                            ((uint8_t)10)

#define CMD_STOP_TRANSMISSION                   ((uint8_t)12)
#define CMD_SEND_STATUS                         ((uint8_t)13)
#define CMD_HS_BUSTEST_READ                     ((uint8_t)14)
#define CMD_GO_INACTIVE_STATE                   ((uint8_t)15)
#define CMD_SET_BLOCKLEN                        ((uint8_t)16)
#define CMD_READ_SINGLE_BLOCK                   ((uint8_t)17)
#define CMD_READ_MULT_BLOCK                     ((uint8_t)18)
#define CMD_HS_BUSTEST_WRITE                    ((uint8_t)19)

#define CMD_SET_BLOCK_COUNT                     ((uint8_t)23) /*!< SD Card doesn't support it */
#define CMD_WRITE_SINGLE_BLOCK                  ((uint8_t)24)
#define CMD_WRITE_MULT_BLOCK                    ((uint8_t)25)
#define CMD_PROG_CSD                            ((uint8_t)27)
#define CMD_SET_WRITE_PROT                      ((uint8_t)28)
#define CMD_CLR_WRITE_PROT                      ((uint8_t)29)

#define CMD_SEND_WRITE_PROT                     ((uint8_t)30)
#define CMD_SD_ERASE_GRP_START                  ((uint8_t)32) /*first write block to be erased. (For SD card only) */
#define CMD_SD_ERASE_GRP_END                    ((uint8_t)33) /*last write block  to be erased. (For SD card only) */
#define CMD_ERASE                               ((uint8_t)38)

#define CMD_LOCK_UNLOCK                         ((uint8_t)42)
#define CMD_APP_CMD                             ((uint8_t)55)
#define CMD_GEN_CMD                             ((uint8_t)56)
#define CMD_NO_CMD                              ((uint8_t)64)

#define CMD_APP_SD_SET_BUSWIDTH                 ((uint8_t)6)  /*!< For SD Card only */
#define CMD_SD_APP_STAUS                        ((uint8_t)13) /*!< For SD Card only */
#define CMD_SD_APP_SEND_NUM_WRITE_BLOCKS        ((uint8_t)22) /*!< For SD Card only */
#define CMD_SD_APP_OP_COND                      ((uint8_t)41) /*!< For SD Card only */
#define CMD_SD_APP_SET_CLR_CARD_DETECT          ((uint8_t)42) /*!< For SD Card only */
#define CMD_SD_APP_SEND_SCR                     ((uint8_t)51) /*!< For SD Card only */

#define CMD_SD_APP_GET_MKB                      ((uint8_t)43) /*!< For SD Card only */
#define CMD_SD_APP_GET_MID                      ((uint8_t)44) /*!< For SD Card only */
#define CMD_SD_APP_SET_CER_RN1                  ((uint8_t)45) /*!< For SD Card only */
#define CMD_SD_APP_GET_CER_RN2                  ((uint8_t)46) /*!< For SD Card only */
#define CMD_SD_APP_SET_CER_RES2                 ((uint8_t)47) /*!< For SD Card only */
#define CMD_SD_APP_GET_CER_RES1                 ((uint8_t)48) /*!< For SD Card only */
#define CMD_SD_APP_SECURE_READ_MULTIPLE_BLOCK   ((uint8_t)18) /*!< For SD Card only */
#define CMD_SD_APP_SECURE_WRITE_MULTIPLE_BLOCK  ((uint8_t)25) /*!< For SD Card only */
#define CMD_SD_APP_SECURE_ERASE                 ((uint8_t)38) /*!< For SD Card only */
#define CMD_SD_APP_CHANGE_SECURE_AREA           ((uint8_t)49) /*!< For SD Card only */
#define CMD_SD_APP_SECURE_WRITE_MKB             ((uint8_t)48) /*!< For SD Card only */
//}}}
//{{{  SDTransferState
typedef enum {
  SD_TRANSFER_OK    = 0,
  SD_TRANSFER_BUSY  = 1,
  SD_TRANSFER_ERROR = 2
} SDTransferState;
//}}}
//{{{  SDCardState
typedef enum {
  SD_CARD_READY                  = ((uint32_t)0x00000001),
  SD_CARD_IDENTIFICATION         = ((uint32_t)0x00000002),
  SD_CARD_STANDBY                = ((uint32_t)0x00000003),
  SD_CARD_TRANSFER               = ((uint32_t)0x00000004),
  SD_CARD_SENDING                = ((uint32_t)0x00000005),
  SD_CARD_RECEIVING              = ((uint32_t)0x00000006),
  SD_CARD_PROGRAMMING            = ((uint32_t)0x00000007),
  SD_CARD_DISCONNECTED           = ((uint32_t)0x00000008),
  SD_CARD_ERROR                  = ((uint32_t)0x000000FF)
}SDCardState;
//}}}

//{{{  dma
#define SD_SDIO_DMA                     DMA2
#define SD_SDIO_DMA_CLK                 RCC_AHB1Periph_DMA2
#define SD_SDIO_DMA_STREAM              DMA2_Stream3
#define SD_SDIO_DMA_CHANNEL             DMA_Channel_4

#define SD_SDIO_DMA_FLAG_FEIF           DMA_FLAG_FEIF3
#define SD_SDIO_DMA_FLAG_DMEIF          DMA_FLAG_DMEIF3
#define SD_SDIO_DMA_FLAG_TEIF           DMA_FLAG_TEIF3
#define SD_SDIO_DMA_FLAG_HTIF           DMA_FLAG_HTIF3
#define SD_SDIO_DMA_FLAG_TCIF           DMA_FLAG_TCIF3

#define SD_SDIO_DMA_IRQn                DMA2_Stream3_IRQn
#define SD_SDIO_DMA_IRQHANDLER          DMA2_Stream3_IRQHandler

#define SDIO_FIFO_ADDRESS               ((uint32_t)0x40012C80)
//}}}
//{{{  ocr
#define SD_OCR_ADDR_OUT_OF_RANGE        ((uint32_t)0x80000000)
#define SD_OCR_ADDR_MISALIGNED          ((uint32_t)0x40000000)
#define SD_OCR_BLOCK_LEN_ERR            ((uint32_t)0x20000000)
#define SD_OCR_ERASE_SEQ_ERR            ((uint32_t)0x10000000)
#define SD_OCR_BAD_ERASE_PARAM          ((uint32_t)0x08000000)
#define SD_OCR_WRITE_PROT_VIOLATION     ((uint32_t)0x04000000)
#define SD_OCR_LOCK_UNLOCK_FAILED       ((uint32_t)0x01000000)
#define SD_OCR_COM_CRC_FAILED           ((uint32_t)0x00800000)
#define SD_OCR_ILLEGAL_CMD              ((uint32_t)0x00400000)
#define SD_OCR_CARD_ECC_FAILED          ((uint32_t)0x00200000)
#define SD_OCR_CC_ERROR                 ((uint32_t)0x00100000)
#define SD_OCR_GENERAL_UNKNOWN_ERROR    ((uint32_t)0x00080000)
#define SD_OCR_STREAM_READ_UNDERRUN     ((uint32_t)0x00040000)
#define SD_OCR_STREAM_WRITE_OVERRUN     ((uint32_t)0x00020000)
#define SD_OCR_CID_CSD_OVERWRIETE       ((uint32_t)0x00010000)
#define SD_OCR_WP_ERASE_SKIP            ((uint32_t)0x00008000)
#define SD_OCR_CARD_ECC_DISABLED        ((uint32_t)0x00004000)
#define SD_OCR_ERASE_RESET              ((uint32_t)0x00002000)
#define SD_OCR_AKE_SEQ_ERROR            ((uint32_t)0x00000008)
#define SD_OCR_ERRORBITS                ((uint32_t)0xFDFFE008)
//}}}
//{{{  r6
#define SD_R6_GENERAL_UNKNOWN_ERROR     ((uint32_t)0x00002000)
#define SD_R6_ILLEGAL_CMD               ((uint32_t)0x00004000)
#define SD_R6_COM_CRC_FAILED            ((uint32_t)0x00008000)
//}}}

#define SDIO_STATIC_FLAGS               ((uint32_t)0x000005FF)

#define SD_VOLTAGE_WINDOW_SD            ((uint32_t)0x80100000)
#define SD_HIGH_CAPACITY                ((uint32_t)0x40000000)
#define SD_STD_CAPACITY                 ((uint32_t)0x00000000)
#define SD_CHECK_PATTERN                ((uint32_t)0x000001AA)

#define SD_MAX_VOLT_TRIAL               ((uint32_t)0x0000FFFF)
#define SD_CARD_LOCKED                  ((uint32_t)0x02000000)

#define SD_DATATIMEOUT                  ((uint32_t)0xFFFFFFFF)
#define SD_0TO7BITS                     ((uint32_t)0x000000FF)
#define SD_8TO15BITS                    ((uint32_t)0x0000FF00)
#define SD_16TO23BITS                   ((uint32_t)0x00FF0000)
#define SD_24TO31BITS                   ((uint32_t)0xFF000000)
//}}}

uint32_t CardType = STD_CAPACITY_SD_CARD_V1_1;
SD_CardInfo CardInfo;
SD_CardStatus CardStatus;
uint8_t SDstatus[64];
uint32_t Scr[2] = {0, 0};

__IO SD_Error TransferDone = 0xFF;

//{{{
static SD_Error cmdError() {

  while (SDIO_GetFlagStatus(SDIO_FLAG_CMDSENT) == RESET) {}

  SDIO_ClearFlag (SDIO_STATIC_FLAGS);

  return SD_OK;
  }
//}}}
//{{{
static SD_Error cmdResp1Error (uint8_t cmd) {

  SD_Error error = SD_OK;

  uint32_t status = SDIO->STA;
  while (!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT)))
    status = SDIO->STA;

  if (status & SDIO_FLAG_CTIMEOUT) {
    SDIO_ClearFlag (SDIO_FLAG_CTIMEOUT);
    return SD_CMD_RSP_TIMEOUT;
    }

  if (status & SDIO_FLAG_CCRCFAIL) {
    SDIO_ClearFlag (SDIO_FLAG_CCRCFAIL);
    return SD_CMD_CRC_FAIL;
    }

  // Check response received is of desired command
  if (SDIO_GetCommandResponse() != cmd)
    return SD_ILLEGAL_CMD;

  // Clear all the static flags
  SDIO_ClearFlag (SDIO_STATIC_FLAGS);

  // We have received response, retrieve it for analysis
  uint32_t response_r1 = SDIO_GetResponse (SDIO_RESP1);
  if ((response_r1 & SD_OCR_ERRORBITS) == 0)
    return error;
  if (response_r1 & SD_OCR_ADDR_OUT_OF_RANGE)
    return (SD_ADDR_OUT_OF_RANGE);
  if (response_r1 & SD_OCR_ADDR_MISALIGNED)
    return (SD_ADDR_MISALIGNED);
  if (response_r1 & SD_OCR_BLOCK_LEN_ERR)
    return (SD_BLOCK_LEN_ERR);
  if (response_r1 & SD_OCR_ERASE_SEQ_ERR)
    return (SD_ERASE_SEQ_ERR);
  if (response_r1 & SD_OCR_BAD_ERASE_PARAM)
    return (SD_BAD_ERASE_PARAM);
  if (response_r1 & SD_OCR_WRITE_PROT_VIOLATION)
    return (SD_WRITE_PROT_VIOLATION);
  if (response_r1 & SD_OCR_LOCK_UNLOCK_FAILED)
    return (SD_LOCK_UNLOCK_FAILED);
  if (response_r1 & SD_OCR_COM_CRC_FAILED)
    return (SD_COM_CRC_FAILED);
  if (response_r1 & SD_OCR_ILLEGAL_CMD)
    return (SD_ILLEGAL_CMD);
  if (response_r1 & SD_OCR_CARD_ECC_FAILED)
    return (SD_CARD_ECC_FAILED);
  if (response_r1 & SD_OCR_CC_ERROR)
    return (SD_CC_ERROR);
  if (response_r1 & SD_OCR_GENERAL_UNKNOWN_ERROR)
    return (SD_GENERAL_UNKNOWN_ERROR);
  if (response_r1 & SD_OCR_STREAM_READ_UNDERRUN)
    return (SD_STREAM_READ_UNDERRUN);
  if (response_r1 & SD_OCR_STREAM_WRITE_OVERRUN)
    return (SD_STREAM_WRITE_OVERRUN);
  if (response_r1 & SD_OCR_CID_CSD_OVERWRIETE)
    return (SD_CID_CSD_OVERWRITE);
  if (response_r1 & SD_OCR_WP_ERASE_SKIP)
    return (SD_WP_ERASE_SKIP);
  if (response_r1 & SD_OCR_CARD_ECC_DISABLED)
    return (SD_CARD_ECC_DISABLED);
  if (response_r1 & SD_OCR_ERASE_RESET)
    return (SD_ERASE_RESET);
  if (response_r1 & SD_OCR_AKE_SEQ_ERROR)
    return (SD_AKE_SEQ_ERROR);

  return error;
  }
//}}}
//{{{
static SD_Error cmdResp2Error() {

  uint32_t status = SDIO->STA;
  while (!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CTIMEOUT | SDIO_FLAG_CMDREND)))
    status = SDIO->STA;

  if (status & SDIO_FLAG_CTIMEOUT) {
    SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
    return SD_CMD_RSP_TIMEOUT;
    }

  if (status & SDIO_FLAG_CCRCFAIL) {
    SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
    return SD_CMD_CRC_FAIL;
    }

  SDIO_ClearFlag (SDIO_STATIC_FLAGS);

  return SD_OK;
  }
//}}}
//{{{
static SD_Error cmdResp3Error() {

  uint32_t status = SDIO->STA;
  while (!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT)))
    status = SDIO->STA;

  if (status & SDIO_FLAG_CTIMEOUT) {
    SDIO_ClearFlag (SDIO_FLAG_CTIMEOUT);
    return SD_CMD_RSP_TIMEOUT;
    }

  SDIO_ClearFlag (SDIO_STATIC_FLAGS);

  return SD_OK;
  }
//}}}
//{{{
static SD_Error cmdResp6Error (uint8_t cmd, uint16_t* prca) {

  uint32_t status = SDIO->STA;
  while (!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CTIMEOUT | SDIO_FLAG_CMDREND)))
    status = SDIO->STA;

  if (status & SDIO_FLAG_CTIMEOUT) {
    SDIO_ClearFlag (SDIO_FLAG_CTIMEOUT);
    return SD_CMD_RSP_TIMEOUT;
    }

  if (status & SDIO_FLAG_CCRCFAIL) {
    SDIO_ClearFlag (SDIO_FLAG_CCRCFAIL);
    return SDIO_FLAG_CCRCFAIL;
    }

  // Check response received is of desired command
  if (SDIO_GetCommandResponse() != cmd)
    return SD_ILLEGAL_CMD;

  SDIO_ClearFlag (SDIO_STATIC_FLAGS);

  uint32_t response_r1 = SDIO_GetResponse(SDIO_RESP1);

  if (response_r1 & SD_R6_GENERAL_UNKNOWN_ERROR)
    return SD_GENERAL_UNKNOWN_ERROR;
  if (response_r1 & SD_R6_ILLEGAL_CMD)
    return SD_ILLEGAL_CMD;
  if (response_r1 & SD_R6_COM_CRC_FAILED)
    return SD_COM_CRC_FAILED;

  *prca = (uint16_t)(response_r1 >> 16);
  return SD_OK;
  }
//}}}
//{{{
static SD_Error cmdResp7Error() {

  uint32_t countDown = 10000;
  uint32_t status = SDIO->STA;
  while (!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT)) && (countDown > 0)) {
    countDown--;
    status = SDIO->STA;
    }

  if ((countDown == 0) || (status & SDIO_FLAG_CTIMEOUT)) {
    // Card is not V2.0 or card does not support the set voltage range
    SDIO_ClearFlag (SDIO_FLAG_CTIMEOUT);
    return SD_CMD_RSP_TIMEOUT;
    }

  // Card is SD V2.0 compliant
  SDIO_ClearFlag (SDIO_STATIC_FLAGS);

  return SD_OK;
  }
//}}}

//{{{
static SD_Error getStatus (uint32_t* status) {

  // send CMD_SEND_STATUS
  SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) CardInfo.RCA << 16;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_SEND_STATUS;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  if (cmdResp1Error (CMD_SEND_STATUS) != SD_OK)
    return SD_TRANSFER_ERROR;

  *status = SDIO_GetResponse (SDIO_RESP1);

  if (SDIO_GetResponse (SDIO_RESP1) & SD_CARD_LOCKED)
    return SD_LOCK_UNLOCK_FAILED;

  return SD_OK;
  }
//}}}
//{{{
static SD_Error getSCR() {

  // send CMD_SET_BLOCKLEN set Block Size To 8 Bytes
  SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)8;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_SET_BLOCKLEN;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  SD_Error error = cmdResp1Error (CMD_SET_BLOCKLEN);
  if (error != SD_OK)
    return error;

  // Send CMD55 APP_CMD with argument as card's RCA
  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)CardInfo.RCA << 16;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_APP_CMD;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  error = cmdResp1Error (CMD_APP_CMD);
  if (error != SD_OK)
    return error;

  SDIO_DataInitTypeDef SDIO_DataInitStructure;
  SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
  SDIO_DataInitStructure.SDIO_DataLength = 8;
  SDIO_DataInitStructure.SDIO_DataBlockSize = SDIO_DataBlockSize_8b;
  SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
  SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
  SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
  SDIO_DataConfig (&SDIO_DataInitStructure);

  // Send ACMD51 SD_APP_SEND_SCR with argument as 0
  SDIO_CmdInitStructure.SDIO_Argument = 0x0;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_SD_APP_SEND_SCR;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  error = cmdResp1Error (CMD_SD_APP_SEND_SCR);
  if (error != SD_OK)
    return error;

  uint32_t index = 0;
  uint32_t tempscr[2] = {0, 0};
  while (!(SDIO->STA & (SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL |
                        SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DBCKEND | SDIO_FLAG_STBITERR)))
    if (SDIO_GetFlagStatus (SDIO_FLAG_RXDAVL) != RESET) {
      *(tempscr + index) = SDIO_ReadData();
      index++;
      }

  if (SDIO_GetFlagStatus (SDIO_FLAG_DTIMEOUT) != RESET) {
    SDIO_ClearFlag (SDIO_FLAG_DTIMEOUT);
    return SD_DATA_TIMEOUT;
    }
  if (SDIO_GetFlagStatus (SDIO_FLAG_DCRCFAIL) != RESET) {
    SDIO_ClearFlag (SDIO_FLAG_DCRCFAIL);
    return SD_DATA_CRC_FAIL;
    }
  if (SDIO_GetFlagStatus (SDIO_FLAG_RXOVERR) != RESET) {
    SDIO_ClearFlag (SDIO_FLAG_RXOVERR);
    return SD_RX_OVERRUN;
    }
  if (SDIO_GetFlagStatus (SDIO_FLAG_STBITERR) != RESET) {
    SDIO_ClearFlag (SDIO_FLAG_STBITERR);
    return SD_START_BIT_ERR;
    }

  // Clear all the static flags
  SDIO_ClearFlag (SDIO_STATIC_FLAGS);

  Scr[1] = ((tempscr[0] & SD_0TO7BITS) << 24) |
           ((tempscr[0] & SD_8TO15BITS) << 8) |
           ((tempscr[0] & SD_16TO23BITS) >> 8) |
           ((tempscr[0] & SD_24TO31BITS) >> 24);
  Scr[0] = ((tempscr[1] & SD_0TO7BITS) << 24) |
           ((tempscr[1] & SD_8TO15BITS) << 8) |
           ((tempscr[1] & SD_16TO23BITS) >> 8) |
           ((tempscr[1] & SD_24TO31BITS) >> 24);

  return SD_OK;
  }
//}}}
//{{{
static SDTransferState getTransferState() {

  uint32_t state;
  if (getStatus (&state) != SD_OK)
    return SD_TRANSFER_ERROR;
  else if ((SDCardState)((state >> 9) & 0x0F) == SD_CARD_TRANSFER)
    return SD_TRANSFER_OK;
  else
    return SD_TRANSFER_BUSY;
  }
//}}}

#ifdef unused
//{{{
static SD_Error getSDStatus() {

  if (SDIO_GetResponse (SDIO_RESP1) & SD_CARD_LOCKED)
    return SD_LOCK_UNLOCK_FAILED;

  // Set block size for card if it is not equal to current block size for card
  SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
  SDIO_CmdInitStructure.SDIO_Argument = 64;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_SET_BLOCKLEN;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);
  SD_Error error = cmdResp1Error(CMD_SET_BLOCKLEN);
  if (error != SD_OK)
    return error;

  // send CMD55 CMD_APP_CMD
  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) CardInfo.RCA << 16;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_APP_CMD;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_SendCommand (&SDIO_CmdInitStructure);
  error = cmdResp1Error (CMD_APP_CMD);
  if (error != SD_OK)
    return error;

  SDIO_DataInitTypeDef SDIO_DataInitStructure;
  SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
  SDIO_DataInitStructure.SDIO_DataLength = 64;
  SDIO_DataInitStructure.SDIO_DataBlockSize = SDIO_DataBlockSize_64b;
  SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
  SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
  SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
  SDIO_DataConfig (&SDIO_DataInitStructure);

  // Send ACMD13 CMD_SD_APP_STAUS with RCA argument
  SDIO_CmdInitStructure.SDIO_Argument = 0;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_SD_APP_STAUS;
  SDIO_SendCommand (&SDIO_CmdInitStructure);
  error = cmdResp1Error (CMD_SD_APP_STAUS);
  if (error != SD_OK)
    return error;

  uint8_t* SDstatus = SDstatus;
  while (!(SDIO->STA & (SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL |
                        SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DBCKEND | SDIO_FLAG_STBITERR)))
    if (SDIO_GetFlagStatus (SDIO_FLAG_RXFIFOHF) != RESET)
      for (int count = 0; count < 8; count++)
        *SDstatus++ = SDIO_ReadData();

  if (SDIO_GetFlagStatus (SDIO_FLAG_DTIMEOUT) != RESET) {
    SDIO_ClearFlag (SDIO_FLAG_DTIMEOUT);
    return SD_DATA_TIMEOUT;
    }
  if (SDIO_GetFlagStatus (SDIO_FLAG_DCRCFAIL) != RESET) {
    SDIO_ClearFlag (SDIO_FLAG_DCRCFAIL);
    return SD_DATA_CRC_FAIL;
    }
  if (SDIO_GetFlagStatus (SDIO_FLAG_RXOVERR) != RESET) {
    SDIO_ClearFlag (SDIO_FLAG_RXOVERR);
    return SD_RX_OVERRUN;
    }
  if (SDIO_GetFlagStatus (SDIO_FLAG_STBITERR) != RESET) {
    SDIO_ClearFlag (SDIO_FLAG_STBITERR);
    return SD_START_BIT_ERR;
    }

  while (SDIO_GetFlagStatus (SDIO_FLAG_RXDAVL) != RESET)
    *SDstatus++ = SDIO_ReadData();

  SDIO_ClearFlag (SDIO_STATIC_FLAGS);

  // SDstatus decode
  uint8_t tmp = (uint8_t)((SDstatus[0] & 0xC0) >> 6);
  CardStatus.DAT_BUS_WIDTH = tmp;

  tmp = (uint8_t)((SDstatus[0] & 0x20) >> 5);
  CardStatus.SECURED_MODE = tmp;

  tmp = (uint8_t)((SDstatus[2] & 0xFF));
  CardStatus.SD_CARD_TYPE = tmp << 8;

  tmp = (uint8_t)((SDstatus[3] & 0xFF));
  CardStatus.SD_CARD_TYPE |= tmp;

  tmp = (uint8_t)(SDstatus[4] & 0xFF);
  CardStatus.SIZE_OF_PROTECTED_AREA = tmp << 24;

  tmp = (uint8_t)(SDstatus[5] & 0xFF);
  CardStatus.SIZE_OF_PROTECTED_AREA |= tmp << 16;

  tmp = (uint8_t)(SDstatus[6] & 0xFF);
  CardStatus.SIZE_OF_PROTECTED_AREA |= tmp << 8;

  tmp = (uint8_t)(SDstatus[7] & 0xFF);
  CardStatus.SIZE_OF_PROTECTED_AREA |= tmp;

  tmp = (uint8_t)((SDstatus[8] & 0xFF));
  CardStatus.SPEED_CLASS = tmp;

  tmp = (uint8_t)((SDstatus[9] & 0xFF));
  CardStatus.PERFORMANCE_MOVE = tmp;

  tmp = (uint8_t)((SDstatus[10] & 0xF0) >> 4);
  CardStatus.AU_SIZE = tmp;

  tmp = (uint8_t)(SDstatus[11] & 0xFF);
  CardStatus.ERASE_SIZE = tmp << 8;

  tmp = (uint8_t)(SDstatus[12] & 0xFF);
  CardStatus.ERASE_SIZE |= tmp;

  tmp = (uint8_t)((SDstatus[13] & 0xFC) >> 2);
  CardStatus.ERASE_TIMEOUT = tmp;

  tmp = (uint8_t)((SDstatus[13] & 0x3));
  CardStatus.ERASE_OFFSET = tmp;

  return error;
  }
//}}}
//{{{
static void reportSDStatus() {

  printf ("CardStatus\n");
  printf ("DAT_BUS_WIDTH:%x\n",CardStatus.DAT_BUS_WIDTH);
  printf ("SECURED_MODE:%x\n",CardStatus.SECURED_MODE);
  printf ("SD_CARD_TYPE:%x\n",CardStatus.SD_CARD_TYPE);
  printf ("SIZE_OF_PROTECTED_AREA:%x\n",CardStatus.SIZE_OF_PROTECTED_AREA);
  printf ("SPEED_CLASS:%x\n",CardStatus.SPEED_CLASS);
  printf ("PERFORMANCE_MOVE:%x\n",CardStatus.PERFORMANCE_MOVE);
  printf ("AU_SIZE:%x\n",CardStatus.AU_SIZE);
  printf ("ERASE_SIZE:%x\n",CardStatus.ERASE_SIZE);
  printf ("ERASE_TIMEOUT:%x\n",CardStatus.ERASE_TIMEOUT);
  printf ("ERASE_OFFSET:%x\n",CardStatus.ERASE_OFFSET);
  }
//}}}
//{{{
static void reportCID() {

  printf ("CID\n");
  printf ("Man id:%x\n", CardInfo.SD_cid.ManufacturerID);
  printf ("OEM id:%x\n", CardInfo.SD_cid.OEM_AppliID);
  printf ("prod1 :%x\n", CardInfo.SD_cid.ProdName1);
  printf ("prod2 :%x\n", CardInfo.SD_cid.ProdName2);
  printf ("prodrv:%x\n", CardInfo.SD_cid.ProdRev);
  printf ("prodsn:%x\n", CardInfo.SD_cid.ProdSN);
  printf ("date  :%x\n", CardInfo.SD_cid.ManufactDate);
  }
//}}}
//{{{
static void reportCSD() {

  printf ("CSD\n");
  printf ("Struct :%d\n", CardInfo.SD_csd.CSDStruct);
  printf (":%d\n", CardInfo.SD_csd.SysSpecVersion);
  printf ("TAAC   :%d\n", CardInfo.SD_csd.TAAC);
  printf ("NSAC   :%d\n", CardInfo.SD_csd.NSAC);
  printf ("Max fre:%d\n", CardInfo.SD_csd.MaxBusClkFrec);
  printf ("CardComdClasses:%d\n", CardInfo.SD_csd.CardComdClasses);
  printf ("rdblkle:%d\n", CardInfo.SD_csd.RdBlockLen);
  printf ("PartBlockRead:%d\n", CardInfo.SD_csd.PartBlockRead);

  printf ("WrBlockMisalign:%d\n", CardInfo.SD_csd.WrBlockMisalign);
  printf ("RdBlockMisalign:%d\n", CardInfo.SD_csd.RdBlockMisalign);
  printf ("DSRImpl:%d\n", CardInfo.SD_csd.DSRImpl);

  if ((CardType == STD_CAPACITY_SD_CARD_V1_1) ||
      (CardType == STD_CAPACITY_SD_CARD_V2_0)) {
     printf ("MaxRdCurrentVDDMin:%d\n", CardInfo.SD_csd.MaxRdCurrentVDDMin);
     printf ("MaxRdCurrentVDDMax:%d\n", CardInfo.SD_csd.MaxRdCurrentVDDMax);
     printf ("MaxWrCurrentVDDMin:%d\n", CardInfo.SD_csd.MaxWrCurrentVDDMin);
     printf ("MaxWrCurrentVDDMax:%d\n", CardInfo.SD_csd.MaxWrCurrentVDDMax);
     printf ("DeviceSizeMul:%d\n", CardInfo.SD_csd.DeviceSizeMul);
     }

  printf ("DeviceSize:%d\n", CardInfo.SD_csd.DeviceSize);
  printf ("CardCapacity:%d\n", CardInfo.CardCapacity);
  printf ("CardBlockSize:%d\n", CardInfo.CardBlockSize);

  printf ("EraseGrSize:%d\n", CardInfo.SD_csd.EraseGrSize);
  printf ("EraseGrMul:%d\n", CardInfo.SD_csd.EraseGrMul);
  printf ("WrProtectGrSize:%d\n", CardInfo.SD_csd.WrProtectGrSize);
  printf ("WrProtectGrEnable:%d\n", CardInfo.SD_csd.WrProtectGrEnable);
  printf ("ManDeflECC:%d\n", CardInfo.SD_csd.ManDeflECC);
  printf ("WrSpeedFact:%d\n", CardInfo.SD_csd.WrSpeedFact);
  printf ("MaxWrBlockLen:%d\n", CardInfo.SD_csd.MaxWrBlockLen);
  printf ("WriteBlockPaPartial:%d\n", CardInfo.SD_csd.WriteBlockPaPartial);
  printf ("Reserved3:%d\n", CardInfo.SD_csd.Reserved3);
  printf ("ContentProtectAppli:%d\n", CardInfo.SD_csd.ContentProtectAppli);
  printf ("FileFormatGrouop:%d\n", CardInfo.SD_csd.FileFormatGrouop);
  printf ("CopyFlag:%d\n", CardInfo.SD_csd.CopyFlag);
  printf ("PermWrProtect:%d\n", CardInfo.SD_csd.PermWrProtect);
  printf ("TempWrProtect:%d\n", CardInfo.SD_csd.TempWrProtect);
  printf ("FileFormat:%d\n", CardInfo.SD_csd.FileFormat);
  }
//}}}
//{{{
static SD_Error powerOff() {

  SDIO_SetPowerState (SDIO_PowerState_OFF);
  return SD_OK;
  }
//}}}
//{{{
static SD_Error enableHighSpeed() {

  TransferDone = SD_OK;

  SDIO->DCTRL = 0x0;

  // Get SCR Register
  SD_Error error = getSCR();
  if (error != SD_OK)
    return (error);

  // if HS supported by the card
  uint32_t SD_SPEC = (Scr[1] & 0x01000000) || (Scr[1] & 0x02000000);
  #ifdef DEBUG
  printf ("enableHighSpeed supported scr1:%x ok:%d\n", Scr[1], SD_SPEC);
  #endif

  if (SD_SPEC != 0) {
    // send CMD_SET_BLOCKLEN set Block Size for Card
    SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
    SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)64;
    SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_SET_BLOCKLEN;
    SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
    SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand (&SDIO_CmdInitStructure);
    error = cmdResp1Error (CMD_SET_BLOCKLEN);
    if (error != SD_OK) {
      #ifdef DEBUG
      printf ("enableHighSpeed blocklen error\n");
      #endif
      return error;
      }

    SDIO_DataInitTypeDef SDIO_DataInitStructure;
    SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
    SDIO_DataInitStructure.SDIO_DataLength = 64;
    SDIO_DataInitStructure.SDIO_DataBlockSize = SDIO_DataBlockSize_64b ;
    SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
    SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
    SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
    SDIO_DataConfig (&SDIO_DataInitStructure);

    // Send CMD6 CMD_HS_SWITCH
    SDIO_CmdInitStructure.SDIO_Argument = 0x80FFFF01;
    SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_HS_SWITCH;
    SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
    SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
    SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
    SDIO_SendCommand (&SDIO_CmdInitStructure);
    error = cmdResp1Error (CMD_HS_SWITCH);
    if (error != SD_OK) {
      #ifdef DEBUG
      printf ("enableHighSpeed CMD_HS_SWITCH error\n");
      #endif
      return error;
      }

    uint32_t count = 0;
    uint8_t hs[64] = {0};
    uint32_t *tempbuff = (uint32_t*)hs;
    while (!(SDIO->STA & (SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL |
                          SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DBCKEND | SDIO_FLAG_STBITERR)))
      if (SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET) {
        for (count = 0; count < 8; count++)
          *(tempbuff + count) = SDIO_ReadData();
        tempbuff += 8;
        }

    if (SDIO_GetFlagStatus (SDIO_FLAG_DTIMEOUT) != RESET) {
      SDIO_ClearFlag (SDIO_FLAG_DTIMEOUT);
      error = SD_DATA_TIMEOUT;
      return error;
      }
    if (SDIO_GetFlagStatus (SDIO_FLAG_DCRCFAIL) != RESET) {
      SDIO_ClearFlag (SDIO_FLAG_DCRCFAIL);
      error = SD_DATA_CRC_FAIL;
      return error;
      }
    if (SDIO_GetFlagStatus (SDIO_FLAG_RXOVERR) != RESET) {
      SDIO_ClearFlag (SDIO_FLAG_RXOVERR);
      error = SD_RX_OVERRUN;
      return error;
      }
    if (SDIO_GetFlagStatus (SDIO_FLAG_STBITERR) != RESET) {
      SDIO_ClearFlag (SDIO_FLAG_STBITERR);
      error = SD_START_BIT_ERR;
      return error;
      }

    count = 100000;
    while ((SDIO_GetFlagStatus (SDIO_FLAG_RXDAVL) != RESET) && (count > 0)) {
      *tempbuff = SDIO_ReadData();
      tempbuff++;
      count--;
      }

    #ifdef DEBUG
    printf ("enableHighSpeed count %d hs[13]:%x\n", count, hs[13]);
    #endif

    // Clear all the static flags */
    SDIO_ClearFlag (SDIO_STATIC_FLAGS);

    // Test if the switch mode HS is ok
    if ((hs[13] & 0x2) != 0x2)
      return SD_UNSUPPORTED_FEATURE;

    #ifdef DEBUG
    printf ("enableHighSpeed supports HS\n");
    #endif
    }

  return SD_OK;
  }
//}}}
//{{{
SD_Error stopTransfer() {

  SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
  SDIO_CmdInitStructure.SDIO_Argument = 0x0;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_STOP_TRANSMISSION;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  return cmdResp1Error (CMD_STOP_TRANSMISSION);
  }
//}}}
#endif

//{{{
static SD_Error enableWideBus() {

  if ((CardType != STD_CAPACITY_SD_CARD_V1_1) &&
      (CardType != STD_CAPACITY_SD_CARD_V2_0) &&
      (CardType != HIGH_CAPACITY_SD_CARD))
    return SD_REQUEST_NOT_APPLICABLE;

  if (SDIO_GetResponse (SDIO_RESP1) & SD_CARD_LOCKED)
    return SD_LOCK_UNLOCK_FAILED;

  SD_Error error = getSCR();
  if (error != SD_OK)
    return error;

  // Send CMD55 with RCA argument
  SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)CardInfo.RCA << 16;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_APP_CMD;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  error = cmdResp1Error (CMD_APP_CMD);
  if (error != SD_OK)
    return error;

  // Send CMD6 with argument 2 for wide bus mode
  SDIO_CmdInitStructure.SDIO_Argument = 0x2;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_APP_SD_SET_BUSWIDTH;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  error = cmdResp1Error (CMD_APP_SD_SET_BUSWIDTH);
  if (error != SD_OK)
    return error;

  // Configure the SDIO peripheral
  SDIO_InitTypeDef SDIO_InitStructure;
  SDIO_InitStructure.SDIO_ClockDiv = SDIO_FAST_CLK_DIV;
  SDIO_InitStructure.SDIO_ClockEdge = SDIO_ClockEdge_Rising;
  SDIO_InitStructure.SDIO_ClockBypass = SDIO_ClockBypass_Disable;
  SDIO_InitStructure.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;
  SDIO_InitStructure.SDIO_BusWide = SDIO_BusWide_4b;
  SDIO_InitStructure.SDIO_HardwareFlowControl = SDIO_HardwareFlowControl_Disable;
  SDIO_Init (&SDIO_InitStructure);

  return SD_OK;
  }
//}}}

//{{{
void SDIO_IRQHandler() {

  if (SDIO_GetITStatus (SDIO_IT_DATAEND) != RESET) {
    SDIO_ClearITPendingBit (SDIO_IT_DATAEND);
    TransferDone = SD_OK;
    }

  else if (SDIO_GetITStatus (SDIO_IT_DCRCFAIL) != RESET) {
    SDIO_ClearITPendingBit (SDIO_IT_DCRCFAIL);
    TransferDone = SD_DATA_CRC_FAIL;
    }

  else if (SDIO_GetITStatus (SDIO_IT_DTIMEOUT) != RESET) {
    SDIO_ClearITPendingBit (SDIO_IT_DTIMEOUT);
    TransferDone = SD_DATA_TIMEOUT;
    }

  else if (SDIO_GetITStatus (SDIO_IT_RXOVERR) != RESET) {
    SDIO_ClearITPendingBit (SDIO_IT_RXOVERR);
    TransferDone = SD_RX_OVERRUN;
    }

  else if (SDIO_GetITStatus (SDIO_IT_TXUNDERR) != RESET) {
    SDIO_ClearITPendingBit (SDIO_IT_TXUNDERR);
    TransferDone = SD_TX_UNDERRUN;
    }

  else if (SDIO_GetITStatus (SDIO_IT_STBITERR) != RESET) {
    SDIO_ClearITPendingBit (SDIO_IT_STBITERR);
    TransferDone = SD_START_BIT_ERR;
    }

  SDIO_ITConfig (SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_DATAEND |
                 SDIO_IT_TXFIFOHE | SDIO_IT_RXFIFOHF | SDIO_IT_TXUNDERR |
                 SDIO_IT_RXOVERR | SDIO_IT_STBITERR,
                 DISABLE);
  }
//}}}

//{{{
static SD_Error powerOn() {

  SD_Error error = SD_OK;
  uint32_t SDType = SD_STD_CAPACITY;

  SDIO_SetPowerState (SDIO_PowerState_ON);
  SDIO_ClockCmd (ENABLE);

  // send CMD0 GO_IDLE_STATE no CMD response required
  SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
  SDIO_CmdInitStructure.SDIO_Argument = 0x0;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_GO_IDLE_STATE;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_No;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  error = cmdError();
  if (error != SD_OK)
    return error;

  // send CMD8 SEND_IF_COND - verify SD card interface operating condition
  //   [31:12]: Reserved (shall be set to '0')
  //   [11:8]: Supply Voltage (VHS) 0x1 (Range: 2.7-3.6 V)
  //   [7:0]: Check Pattern (recommended 0xAA) */
  SDIO_CmdInitStructure.SDIO_Argument = SD_CHECK_PATTERN;
  SDIO_CmdInitStructure.SDIO_CmdIndex = SEND_IF_COND;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  error = cmdResp7Error();
  if (error == SD_OK) {
    CardType = STD_CAPACITY_SD_CARD_V2_0;
    SDType = SD_HIGH_CAPACITY;
    }
  else {
    // send CMD55 CMD_APP_CMD
    SDIO_CmdInitStructure.SDIO_Argument = 0x00;
    SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_APP_CMD;
    SDIO_SendCommand (&SDIO_CmdInitStructure);
    error = cmdResp1Error (CMD_APP_CMD);
    }

  // send CMD55 CMD_APP_CMD
  SDIO_CmdInitStructure.SDIO_Argument = 0x00;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_APP_CMD;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  error = cmdResp1Error (CMD_APP_CMD);
  if (error == SD_OK) {
    // SD card 2.0, voltage range mismatch or SD card 1.x
    uint32_t response = 0;
    uint32_t count = 0;
    bool validVoltage = false;
    while ((!validVoltage) && (count < SD_MAX_VOLT_TRIAL)) {
      // send CMD55 APP_CMD with RCA as 0
      SDIO_CmdInitStructure.SDIO_Argument = 0x00;
      SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_APP_CMD;
      SDIO_SendCommand (&SDIO_CmdInitStructure);

      error = cmdResp1Error (CMD_APP_CMD);
      if (error != SD_OK)
        return error;

      // send CMD41 APP_OP_COND arg 0x80100000 = SD_VOLTAGE_WINDOW_SD | SDType
      SDIO_CmdInitStructure.SDIO_Argument = SD_VOLTAGE_WINDOW_SD | SDType;
      SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_SD_APP_OP_COND;
      SDIO_SendCommand (&SDIO_CmdInitStructure);

      error = cmdResp3Error();
      if (error != SD_OK)
        return error;

      response = SDIO_GetResponse (SDIO_RESP1);
      validVoltage = (response >> 31) == 1;

      count++;
      }

    if (count >= SD_MAX_VOLT_TRIAL)
      return SD_INVALID_VOLTRANGE;

    if (response &= SD_HIGH_CAPACITY)
      CardType = HIGH_CAPACITY_SD_CARD;
    }

  #ifdef DEBUG
  if (CardType == STD_CAPACITY_SD_CARD_V1_1)
    printf ("STD_CAPACITY_SD_CARD_V1_1\n");
  else if (CardType == STD_CAPACITY_SD_CARD_V2_0)
    printf ("STD_CAPACITY_SD_CARD_V2_0\n");
  else if (CardType == HIGH_CAPACITY_SD_CARD)
    printf ("HIGH_CAPACITY_SD_CARD\n");
  #endif

  return SD_OK;
  }
//}}}
//{{{
static SD_Error initializeCards() {

  SD_Error error = SD_OK;
  uint8_t tmp = 0;

  CardInfo.CardType = (uint8_t)CardType;

  // send CMD2 CMD_ALL_SEND_CID
  SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
  SDIO_CmdInitStructure.SDIO_Argument = 0x0;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_ALL_SEND_CID;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Long;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  error = cmdResp2Error();
  if (SD_OK != error)
    return error;

  uint32_t CID_Tab[4];
  CID_Tab[0] = SDIO_GetResponse (SDIO_RESP1);
  CID_Tab[1] = SDIO_GetResponse (SDIO_RESP2);
  CID_Tab[2] = SDIO_GetResponse (SDIO_RESP3);
  CID_Tab[3] = SDIO_GetResponse (SDIO_RESP4);
  //{{{  CID decode
  tmp = (uint8_t)((CID_Tab[0] & 0xFF000000) >> 24);
  CardInfo.SD_cid.ManufacturerID = tmp;

  tmp = (uint8_t)((CID_Tab[0] & 0x00FF0000) >> 16);
  CardInfo.SD_cid.OEM_AppliID = tmp << 8;

  tmp = (uint8_t)((CID_Tab[0] & 0x000000FF00) >> 8);
  tmp = (uint8_t)(CID_Tab[0] & 0x000000FF);
  CardInfo.SD_cid.ProdName1 = tmp << 24;

  tmp = (uint8_t)((CID_Tab[1] & 0xFF000000) >> 24);
  CardInfo.SD_cid.ProdName1 |= tmp << 16;

  tmp = (uint8_t)((CID_Tab[1] & 0x00FF0000) >> 16);
  CardInfo.SD_cid.ProdName1 |= tmp << 8;

  tmp = (uint8_t)((CID_Tab[1] & 0x0000FF00) >> 8);
  CardInfo.SD_cid.ProdName1 |= tmp;

  tmp = (uint8_t)(CID_Tab[1] & 0x000000FF);
  CardInfo.SD_cid.ProdName2 = tmp;

  tmp = (uint8_t)((CID_Tab[2] & 0xFF000000) >> 24);
  CardInfo.SD_cid.ProdRev = tmp;

  tmp = (uint8_t)((CID_Tab[2] & 0x00FF0000) >> 16);
  CardInfo.SD_cid.ProdSN = tmp << 24;

  tmp = (uint8_t)((CID_Tab[2] & 0x0000FF00) >> 8);
  CardInfo.SD_cid.ProdSN |= tmp << 16;

  tmp = (uint8_t)(CID_Tab[2] & 0x000000FF);
  CardInfo.SD_cid.ProdSN |= tmp << 8;

  tmp = (uint8_t)((CID_Tab[3] & 0xFF000000) >> 24);
  CardInfo.SD_cid.ProdSN |= tmp;

  tmp = (uint8_t)((CID_Tab[3] & 0x00FF0000) >> 16);
  CardInfo.SD_cid.Reserved1 |= (tmp & 0xF0) >> 4;
  CardInfo.SD_cid.ManufactDate = (tmp & 0x0F) << 8;

  tmp = (uint8_t)((CID_Tab[3] & 0x0000FF00) >> 8);
  CardInfo.SD_cid.ManufactDate |= tmp;

  tmp = (uint8_t)(CID_Tab[3] & 0x000000FF);
  CardInfo.SD_cid.CID_CRC = (tmp & 0xFE) >> 1;
  CardInfo.SD_cid.Reserved2 = 1;
  //}}}

  // send CMD3 CMD_SET_REL_ADDR with argument 0 - SD Card publishes its RCA
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_SET_REL_ADDR;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  error = cmdResp6Error (CMD_SET_REL_ADDR, &CardInfo.RCA);
  if (SD_OK != error)
    return error;

  // Send CMD9 CMD_SEND_CSD with argument as card's RCA
  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)(CardInfo.RCA << 16);
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_SEND_CSD;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Long;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  error = cmdResp2Error();
  if (SD_OK != error)
    return error;

  uint32_t CSD_Tab[4];
  CSD_Tab[0] = SDIO_GetResponse (SDIO_RESP1);
  CSD_Tab[1] = SDIO_GetResponse (SDIO_RESP2);
  CSD_Tab[2] = SDIO_GetResponse (SDIO_RESP3);
  CSD_Tab[3] = SDIO_GetResponse (SDIO_RESP4);
  //{{{  CSD decode
  tmp = (uint8_t)((CSD_Tab[0] & 0xFF000000) >> 24);
  CardInfo.SD_csd.CSDStruct = (tmp & 0xC0) >> 6;
  CardInfo.SD_csd.SysSpecVersion = (tmp & 0x3C) >> 2;
  CardInfo.SD_csd.Reserved1 = tmp & 0x03;

  tmp = (uint8_t)((CSD_Tab[0] & 0x00FF0000) >> 16);
  CardInfo.SD_csd.TAAC = tmp;

  tmp = (uint8_t)((CSD_Tab[0] & 0x0000FF00) >> 8);
  CardInfo.SD_csd.NSAC = tmp;

  tmp = (uint8_t)(CSD_Tab[0] & 0x000000FF);
  CardInfo.SD_csd.MaxBusClkFrec = tmp;

  tmp = (uint8_t)((CSD_Tab[1] & 0xFF000000) >> 24);
  CardInfo.SD_csd.CardComdClasses = tmp << 4;

  tmp = (uint8_t)((CSD_Tab[1] & 0x00FF0000) >> 16);
  CardInfo.SD_csd.CardComdClasses |= (tmp & 0xF0) >> 4;
  CardInfo.SD_csd.RdBlockLen = tmp & 0x0F;

  tmp = (uint8_t)((CSD_Tab[1] & 0x0000FF00) >> 8);
  CardInfo.SD_csd.PartBlockRead = (tmp & 0x80) >> 7;
  CardInfo.SD_csd.WrBlockMisalign = (tmp & 0x40) >> 6;
  CardInfo.SD_csd.RdBlockMisalign = (tmp & 0x20) >> 5;
  CardInfo.SD_csd.DSRImpl = (tmp & 0x10) >> 4;
  CardInfo.SD_csd.Reserved2 = 0; /*!< Reserved */

  if ((CardType == STD_CAPACITY_SD_CARD_V1_1) ||
      (CardType == STD_CAPACITY_SD_CARD_V2_0)) {
     //{{{  std capacity size
     CardInfo.SD_csd.DeviceSize = (tmp & 0x03) << 10;

     tmp = (uint8_t)(CSD_Tab[1] & 0x000000FF);
     CardInfo.SD_csd.DeviceSize |= (tmp) << 2;

     tmp = (uint8_t)((CSD_Tab[2] & 0xFF000000) >> 24);
     CardInfo.SD_csd.DeviceSize |= (tmp & 0xC0) >> 6;
     CardInfo.SD_csd.MaxRdCurrentVDDMin = (tmp & 0x38) >> 3;
     CardInfo.SD_csd.MaxRdCurrentVDDMax = (tmp & 0x07);

     tmp = (uint8_t)((CSD_Tab[2] & 0x00FF0000) >> 16);
     CardInfo.SD_csd.MaxWrCurrentVDDMin = (tmp & 0xE0) >> 5;
     CardInfo.SD_csd.MaxWrCurrentVDDMax = (tmp & 0x1C) >> 2;
     CardInfo.SD_csd.DeviceSizeMul = (tmp & 0x03) << 1;

     tmp = (uint8_t)((CSD_Tab[2] & 0x0000FF00) >> 8);
     CardInfo.SD_csd.DeviceSizeMul |= (tmp & 0x80) >> 7;
     CardInfo.CardCapacity = (CardInfo.SD_csd.DeviceSize + 1) ;
     CardInfo.CardCapacity *= (1 << (CardInfo.SD_csd.DeviceSizeMul + 2));
     CardInfo.CardBlockSize = 1 << (CardInfo.SD_csd.RdBlockLen);
     CardInfo.CardCapacity *= CardInfo.CardBlockSize;
     }
     //}}}
  else if (CardType == HIGH_CAPACITY_SD_CARD) {
    //{{{  high capacity size
    tmp = (uint8_t)(CSD_Tab[1] & 0x000000FF);
    CardInfo.SD_csd.DeviceSize = (tmp & 0x3F) << 16;

    tmp = (uint8_t)((CSD_Tab[2] & 0xFF000000) >> 24);
    CardInfo.SD_csd.DeviceSize |= (tmp << 8);

    tmp = (uint8_t)((CSD_Tab[2] & 0x00FF0000) >> 16);
    CardInfo.SD_csd.DeviceSize |= (tmp);

    tmp = (uint8_t)((CSD_Tab[2] & 0x0000FF00) >> 8);

    CardInfo.CardCapacity = (uint64_t)(CardInfo.SD_csd.DeviceSize + 1) * (uint64_t)(512 * 1024);
    CardInfo.CardBlockSize = 512;
    }
    //}}}

  CardInfo.SD_csd.EraseGrSize = (tmp & 0x40) >> 6;
  CardInfo.SD_csd.EraseGrMul = (tmp & 0x3F) << 1;

  tmp = (uint8_t)(CSD_Tab[2] & 0x000000FF);
  CardInfo.SD_csd.EraseGrMul |= (tmp & 0x80) >> 7;
  CardInfo.SD_csd.WrProtectGrSize = (tmp & 0x7F);

  tmp = (uint8_t)((CSD_Tab[3] & 0xFF000000) >> 24);
  CardInfo.SD_csd.WrProtectGrEnable = (tmp & 0x80) >> 7;
  CardInfo.SD_csd.ManDeflECC = (tmp & 0x60) >> 5;
  CardInfo.SD_csd.WrSpeedFact = (tmp & 0x1C) >> 2;
  CardInfo.SD_csd.MaxWrBlockLen = (tmp & 0x03) << 2;

  tmp = (uint8_t)((CSD_Tab[3] & 0x00FF0000) >> 16);
  CardInfo.SD_csd.MaxWrBlockLen |= (tmp & 0xC0) >> 6;
  CardInfo.SD_csd.WriteBlockPaPartial = (tmp & 0x20) >> 5;
  CardInfo.SD_csd.Reserved3 = 0;
  CardInfo.SD_csd.ContentProtectAppli = (tmp & 0x01);

  tmp = (uint8_t)((CSD_Tab[3] & 0x0000FF00) >> 8);
  CardInfo.SD_csd.FileFormatGrouop = (tmp & 0x80) >> 7;
  CardInfo.SD_csd.CopyFlag = (tmp & 0x40) >> 6;
  CardInfo.SD_csd.PermWrProtect = (tmp & 0x20) >> 5;
  CardInfo.SD_csd.TempWrProtect = (tmp & 0x10) >> 4;
  CardInfo.SD_csd.FileFormat = (tmp & 0x0C) >> 2;
  CardInfo.SD_csd.ECC = (tmp & 0x03);

  tmp = (uint8_t)(CSD_Tab[3] & 0x000000FF);
  CardInfo.SD_csd.CSD_CRC = (tmp & 0xFE) >> 1;
  CardInfo.SD_csd.Reserved4 = 1;
  //}}}

  return SD_OK;
  }
//}}}
//{{{
static SD_Error selectDeselect (uint32_t addr) {

  // send CMD_SEL_DESEL_CARD
  SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
  SDIO_CmdInitStructure.SDIO_Argument =  addr;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_SEL_DESEL_CARD;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  return cmdResp1Error (CMD_SEL_DESEL_CARD);
  }
//}}}

//{{{
SD_Error SD_readBlocks (uint8_t* buf, uint32_t sector, uint32_t blockSize, uint32_t blocks) {

  SD_Error error = SD_OK;

  if (((uint32_t)buf) & 3)
    return SD_ERROR;

  TransferDone = 0xFF;

  SDIO->DCTRL = 0x0;

  if (CardType == STD_CAPACITY_SD_CARD_V1_1)
    sector *= blockSize;
  else
    blockSize = 512;

  // Send  CMD_SET_BLOCKLEN
  SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)blockSize;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_SET_BLOCKLEN;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  error = cmdResp1Error (CMD_SET_BLOCKLEN);
  if (SD_OK != error) {
    #ifdef DEBUG
    printf ("***SD_readBlocks s:%d:%d:%d CMD_SET_BLOCKLEN\n", (int)sector, (int)blocks, (int)blockSize);
    #endif
    return error;
    }

  SDIO_DMACmd (ENABLE);

  DMA_ClearFlag (SD_SDIO_DMA_STREAM,
                 SD_SDIO_DMA_FLAG_FEIF | SD_SDIO_DMA_FLAG_DMEIF |
                 SD_SDIO_DMA_FLAG_TEIF | SD_SDIO_DMA_FLAG_HTIF | SD_SDIO_DMA_FLAG_TCIF);

  // DMA2 Stream disable
  DMA_Cmd (SD_SDIO_DMA_STREAM, DISABLE);
  DMA_DeInit (SD_SDIO_DMA_STREAM);

  DMA_InitTypeDef SDDMA_InitStructure;
  SDDMA_InitStructure.DMA_Channel = SD_SDIO_DMA_CHANNEL;
  SDDMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)SDIO_FIFO_ADDRESS;
  SDDMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)buf;
  SDDMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  SDDMA_InitStructure.DMA_BufferSize = 0;
  SDDMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  SDDMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  SDDMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  SDDMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
  SDDMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  SDDMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  SDDMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
  SDDMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  SDDMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_INC4;
  SDDMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_INC4;
  DMA_Init (SD_SDIO_DMA_STREAM, &SDDMA_InitStructure);

  DMA_ITConfig (SD_SDIO_DMA_STREAM, DMA_IT_TC, ENABLE);
  DMA_FlowControllerConfig (SD_SDIO_DMA_STREAM, DMA_FlowCtrl_Peripheral);

  // DMA2 Stream enable
  DMA_Cmd (SD_SDIO_DMA_STREAM, ENABLE);

  SDIO_ITConfig (SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT |
                 SDIO_IT_DATAEND | SDIO_IT_RXOVERR | SDIO_IT_STBITERR, ENABLE);

  SDIO_DataInitTypeDef SDIO_DataInitStructure;
  SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
  SDIO_DataInitStructure.SDIO_DataLength = blocks * blockSize;
  SDIO_DataInitStructure.SDIO_DataBlockSize = (uint32_t)9 << 4;
  SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
  SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
  SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
  SDIO_DataConfig (&SDIO_DataInitStructure);

  // Send CMD17 CMD_READ_SINGLE_BLOCK with argument data address
  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)sector;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_READ_SINGLE_BLOCK;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  error = cmdResp1Error (CMD_READ_SINGLE_BLOCK);
  if (error != SD_OK) {
    #ifdef DEBUG
    printf ("***SD_readBlocks s:%d:%d:%d CMD_READ_SINGLE_BLOCK\n", (int)sector, (int)blocks, (int)blockSize);
    #endif
    return error;
    }

  uint32_t countDown = 100000;

  // wait for interrupt
  while ((TransferDone == 0xFF) && countDown)
    countDown--;
  if (countDown == 0) {
    #ifdef DEBUG
    printf ("***SD_readBlocks s:%d:%d:%d tx timeout\n", (int)sector, (int)blocks, (int)blockSize);
    #endif
    }

  // wait for RXACT
  while ((SDIO->STA & SDIO_FLAG_RXACT) && countDown)
    countDown--;
  if (countDown == 0) {
    #ifdef DEBUG
    printf ("***SD_readBlocks %d:%d:%d rxact timeout\n", (int)sector, (int)blocks, (int)blockSize);
    #endif
    }

  SDIO_ClearFlag (SDIO_STATIC_FLAGS);

  if (TransferDone != SD_OK) {
    #ifdef DEBUG
    printf ("***SD_readBlocks %d:%d:%d TransferDone= %d\n", (int)sector, (int)blocks, (int)blockSize, TransferDone);
    #endif
    return TransferDone;
    }

  // wait for status not busy
  while (getTransferState() == SD_TRANSFER_BUSY)
    countDown--;
  if (countDown == 0) {
    #ifdef DEBUG
    printf ("***SD_readBlocks %d:%d:%d SD_TRANSFER_BUSY timeout\n", (int)sector, (int)blocks, (int)blockSize);
    #endif
    }

  if (getTransferState() == SD_TRANSFER_ERROR) {
    #ifdef DEBUG
    printf ("***SD_readBlocks %d:%d:%d SD_TRANSFER_ERROR\n", (int)sector, (int)blocks, (int)blockSize);
    #endif
    return SD_ERROR;
    }

  return SD_OK;
  }
//}}}
//{{{
SD_Error SD_writeBlocks (uint8_t* buf, uint32_t sector, uint32_t blockSize, uint32_t blocks) {

  SD_Error error = SD_OK;
  TransferDone = 0xFF;

  SDIO->DCTRL = 0x0;

  if (CardType == HIGH_CAPACITY_SD_CARD)
    blockSize = 512;
  else // Convert to Bytes for NON SDHC
    sector *= blockSize;

  // Set Block Size for Card
  SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)blockSize;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_SET_BLOCKLEN;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  error = cmdResp1Error (CMD_SET_BLOCKLEN);
  if (SD_OK != error)
    return error;

  // To improve performance
  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) (CardInfo.RCA << 16);
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_APP_CMD;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  error = cmdResp1Error (CMD_APP_CMD);
  if (error != SD_OK)
    return error;

  // To improve performance
  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)blocks;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_SET_BLOCK_COUNT;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand (&SDIO_CmdInitStructure);

  error = cmdResp1Error (CMD_SET_BLOCK_COUNT);
  if (error != SD_OK)
    return error;

  // Send CMD25 WRITE_MULT_BLOCK with argument data address
  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)sector;
  SDIO_CmdInitStructure.SDIO_CmdIndex = CMD_WRITE_MULT_BLOCK;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  error = cmdResp1Error (CMD_WRITE_MULT_BLOCK);
  if (SD_OK != error)
    return error;

  SDIO_DataInitTypeDef SDIO_DataInitStructure;
  SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
  SDIO_DataInitStructure.SDIO_DataLength = blocks * blockSize;
  SDIO_DataInitStructure.SDIO_DataBlockSize = (uint32_t) 9 << 4;
  SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToCard;
  SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
  SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
  SDIO_DataConfig (&SDIO_DataInitStructure);

  SDIO_ITConfig (SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT | SDIO_IT_DATAEND | SDIO_IT_RXOVERR | SDIO_IT_STBITERR, ENABLE);
  SDIO_DMACmd (ENABLE);

  DMA_ClearFlag (SD_SDIO_DMA_STREAM,
                 SD_SDIO_DMA_FLAG_FEIF | SD_SDIO_DMA_FLAG_DMEIF |
                 SD_SDIO_DMA_FLAG_TEIF | SD_SDIO_DMA_FLAG_HTIF | SD_SDIO_DMA_FLAG_TCIF);

  // DMA2 Stream disable
  DMA_Cmd (SD_SDIO_DMA_STREAM, DISABLE);
  DMA_DeInit (SD_SDIO_DMA_STREAM);

  DMA_InitTypeDef SDDMA_InitStructure;
  SDDMA_InitStructure.DMA_Channel = SD_SDIO_DMA_CHANNEL;
  SDDMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)SDIO_FIFO_ADDRESS;
  SDDMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)buf;
  SDDMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  SDDMA_InitStructure.DMA_BufferSize = 0;
  SDDMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  SDDMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  SDDMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  SDDMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
  SDDMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  SDDMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  SDDMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
  SDDMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  SDDMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_INC4;
  SDDMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_INC4;
  DMA_Init (SD_SDIO_DMA_STREAM, &SDDMA_InitStructure);

  DMA_ITConfig (SD_SDIO_DMA_STREAM, DMA_IT_TC, ENABLE);
  DMA_FlowControllerConfig (SD_SDIO_DMA_STREAM, DMA_FlowCtrl_Peripheral);

  // DMA2 Stream enable
  DMA_Cmd (SD_SDIO_DMA_STREAM, ENABLE);

  uint32_t countDown = 100000;
  while ((TransferDone == 0xFF) && (countDown > 0)) {
    countDown--;
    }

  while(((SDIO->STA & SDIO_FLAG_TXACT)) && (countDown > 0)) {
    countDown--;
    }

  if ((countDown == 0) && (error == SD_OK))
    error = SD_DATA_TIMEOUT;

  // Clear all the static flags
  SDIO_ClearFlag (SDIO_STATIC_FLAGS);

  if (TransferDone != SD_OK)
    return TransferDone;
  else
    return error;
  }
//}}}

//{{{
SD_Error SD_init (bool wideBus) {

  //{{{  config NVIC Preemption Priority Bits
  NVIC_PriorityGroupConfig (NVIC_PriorityGroup_1);

  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init (&NVIC_InitStructure);
  //}}}
  //{{{  init GPIO and clocks
  // GPIOC, GPIOD Periph clock enable
  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE);

  // - set as GPIO hi to stabilise init
  GPIO_InitTypeDef  GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init (GPIOC, &GPIO_InitStructure);
  GPIOC->BSRRL = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11; // set
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_Init (GPIOD, &GPIO_InitStructure);
  GPIOD->BSRRL = GPIO_Pin_2; // set
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init (GPIOC, &GPIO_InitStructure);
  GPIOD->BSRRL = GPIO_Pin_12; // set

  // set as SDIO AF
  GPIO_PinAFConfig (GPIOC, GPIO_PinSource8, GPIO_AF_SDIO);
  GPIO_PinAFConfig (GPIOC, GPIO_PinSource9, GPIO_AF_SDIO);
  GPIO_PinAFConfig (GPIOC, GPIO_PinSource10, GPIO_AF_SDIO);
  GPIO_PinAFConfig (GPIOC, GPIO_PinSource11, GPIO_AF_SDIO);
  GPIO_PinAFConfig (GPIOC, GPIO_PinSource12, GPIO_AF_SDIO);
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource2, GPIO_AF_SDIO);

  // set GPIO SDIO AF
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_Init (GPIOC, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_Init (GPIOD, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init (GPIOC, &GPIO_InitStructure);

  // SD_SPI_DETECT_PIN pin: SD Card detect pin
  //GPIO_InitStructure.GPIO_Pin = SD_DETECT_PIN;
  //GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  //GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  //GPIO_Init (SD_DETECT_GPIO_PORT, &GPIO_InitStructure);

  // Enable the SDIO APB2 Clock
  RCC_APB2PeriphClockCmd (RCC_APB2Periph_SDIO, ENABLE);

  // Enable the DMA2 Clock
  RCC_AHB1PeriphClockCmd (SD_SDIO_DMA_CLK, ENABLE);
  //}}}

  SDIO_DeInit();

  // config SDIO slowClock, 1b wide bus
  SDIO_InitTypeDef SDIO_InitStructure;
  SDIO_InitStructure.SDIO_ClockDiv = SDIO_SLOW_CLK_DIV;
  SDIO_InitStructure.SDIO_ClockEdge = SDIO_ClockEdge_Rising;
  SDIO_InitStructure.SDIO_ClockBypass = SDIO_ClockBypass_Disable;
  SDIO_InitStructure.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;
  SDIO_InitStructure.SDIO_BusWide = SDIO_BusWide_1b;
  SDIO_InitStructure.SDIO_HardwareFlowControl = SDIO_HardwareFlowControl_Disable;
  SDIO_Init (&SDIO_InitStructure);

  SD_Error error = powerOn();
  if (error != SD_OK)
    return error;

  error = initializeCards();
  if (error != SD_OK)
    return error;

  // config SDIO fastClock
  SDIO_InitStructure.SDIO_ClockDiv = SDIO_FAST_CLK_DIV;
  SDIO_Init (&SDIO_InitStructure);

  error = selectDeselect ((uint32_t)(CardInfo.RCA << 16));
  if (error != SD_OK)
    return error;

  if (wideBus)
    error = enableWideBus();

  return error;
  }
//}}}
