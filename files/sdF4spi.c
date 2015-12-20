// sdF4spi.c - sd spi4
// PE02 = SCK
// PE03 = CS
// PE05 = MISO
// PE06 = MOSI
//{{{  includes
#include "sd.h"

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_spi.h"
//}}}
//{{{  spi2 pin defines
// PD12 = GPIO CS  normally high
// PB13 = SPI2 SCK
// PB14 = SPI2 MISO
// PB15 = SPI2 MOSI

//#define GPIO_CS  GPIOD
//#define CS_PIN   GPIO_Pin_12
//#define RCC_AHB1Periph_GPIO  RCC_AHB1Periph_GPIOD

//#define SPI          SPI2
//#define GPIO_SPI     GPIOB
//#define GPIO_AF_SPI  GPIO_AF_SPI2
//#define RCC_APB1Periph_SPI   RCC_APB1Periph_SPI2
//#define RCC_AHB1Periph_SPI_GPIO  RCC_AHB1Periph_GPIOB

//#define SPI_SCK_PIN     GPIO_Pin_13
//#define SPI_SCK_SOURCE  GPIO_PinSource13
//#define SPI_MISO_PIN    GPIO_Pin_14
//#define SPI_MISO_SOURCE GPIO_PinSource14
//#define SPI_MOSI_PIN    GPIO_Pin_15
//#define SPI_MOSI_SOURCE GPIO_PinSource15
//}}}
//{{{  spi4 pin defines
// PE02 = SCK
// PE03 = CS
// PE05 = MISO
// PE06 = MOSI

#define GPIO_CS                  GPIOE
#define CS_PIN                   GPIO_Pin_3
#define RCC_AHB1Periph_GPIO      RCC_AHB1Periph_GPIOE

#define SPI                      SPI4
#define GPIO_SPI                 GPIOE
#define GPIO_AF_SPI              GPIO_AF_SPI4
#define RCC_APB2Periph_SPI       RCC_APB2Periph_SPI4
#define RCC_AHB1Periph_SPI_GPIO  RCC_AHB1Periph_GPIOE

#define SPI_SCK_PIN              GPIO_Pin_2
#define SPI_SCK_SOURCE           GPIO_PinSource2
#define SPI_MISO_PIN             GPIO_Pin_5
#define SPI_MISO_SOURCE          GPIO_PinSource5
#define SPI_MOSI_PIN             GPIO_Pin_6
#define SPI_MOSI_SOURCE          GPIO_PinSource6
//}}}

//{{{  SD spi command, data, response defines
#define CMD_GO_IDLE_STATE        0  // CMD0  = 0x40
#define CMD_SEND_OP_COND         1  // CMD1  = 0x41

#define CMD_SEND_CSD             9  // CMD9  = 0x49
#define CMD_SEND_CID            10  // CMD10 = 0x4A
#define CMD_STOP_TRANSMISSION   12  // CMD12 = 0x4C

#define CMD_SEND_STATUS         13  // CMD13 = 0x4D
#define CMD_SET_BLOCKLEN        16  // CMD16 = 0x50

#define CMD_READ_SINGLE_BLOCK   17  // CMD17 = 0x51
#define CMD_READ_MULT_BLOCK     18  // CMD18 = 0x52

#define CMD_SET_BLOCK_COUNT     23  // CMD23 = 0x57
#define CMD_WRITE_SINGLE_BLOCK  24  // CMD24 = 0x58
#define CMD_WRITE_MULT_BLOCK    25  // CMD25 = 0x59

#define CMD_PROG_CSD            27  // CMD27 = 0x5B
#define CMD_SET_WRITE_PROT      28  // CMD28 = 0x5C
#define CMD_CLR_WRITE_PROT      29  // CMD29 = 0x5D
#define CMD_SEND_WRITE_PROT     30  // CMD30 = 0x5E

#define CMD_SD_ERASE_GRP_START  32  // CMD32 = 0x60
#define CMD_SD_ERASE_GRP_END    33  // CMD33 = 0x61
#define CMD_UNTAG_SECTOR        34  // CMD34 = 0x62
#define CMD_ERASE_GRP_START     35  // CMD35 = 0x63
#define CMD_ERASE_GRP_END       36  // CMD36 = 0x64
#define CMD_UNTAG_ERASE_GROUP   37  // CMD37 = 0x65
#define CMD_ERASE               38  // CMD38 = 0x66

#define SD_DUMMY_BYTE            0xFF

// Tokens (necessary because at nop/idle (and CS active) only 0xff is on the data/command line)
#define START_DATA_SINGLE_BLOCK_READ    0xFE
#define START_DATA_MULTIPLE_BLOCK_READ  0xFE

#define START_DATA_SINGLE_BLOCK_WRITE   0xFE
#define START_DATA_MULTIPLE_BLOCK_WRITE 0xFD
#define STOP_DATA_MULTIPLE_BLOCK_WRITE  0xFD

// response
#define SD_IN_IDLE_STATE          0x01
#define SD_ERASE_RESET            0x02
#define SD_ILLEGAL_COMMAND        0x04
#define SD_COM_CRC_ERROR          0x08
#define SD_ERASE_SEQUENCE_ERROR   0x10
#define SD_ADDRESS_ERROR          0x20
#define SD_PARAMETER_ERROR        0x40
#define SD_RESPONSE_FAILURE       0xFF

// Data response error
#define SD_DATA_OK                0x05
#define SD_DATA_CRC_ERROR         0x0B
#define SD_DATA_WRITE_ERROR       0x0D
#define SD_DATA_OTHER_ERROR       0xFF
//}}}

// private
//{{{
static uint8_t sdReadByte() {

  SPI->DR = SD_DUMMY_BYTE;
  while (!(SPI->SR & SPI_I2S_FLAG_RXNE));

  return SPI->DR;
  }
//}}}
//{{{
static void sdReadBlockData (uint8_t* buf, uint32_t bytes) {

  for (int i = 0; i < bytes; i++) {
    SPI->DR = SD_DUMMY_BYTE;
    while (!(SPI->SR & SPI_I2S_FLAG_RXNE));
    *buf++ = SPI->DR;
    }
  }
//}}}

//{{{
static void sdWriteByte (uint8_t data) {

  SPI->DR = data;
  while (!(SPI->SR & SPI_I2S_FLAG_TXE));

  while (SPI->SR & SPI_I2S_FLAG_BSY);
  }
//}}}
//{{{
static void sdWriteCommand (uint8_t command, uint32_t arg, uint8_t crc) {

  SPI->DR = command | 0x40;
  while (!(SPI->SR & SPI_I2S_FLAG_TXE));

  SPI->DR = arg >> 24;
  while (!(SPI->SR & SPI_I2S_FLAG_TXE));

  SPI->DR = arg >> 16;
  while (!(SPI->SR & SPI_I2S_FLAG_TXE));

  SPI->DR = arg >> 8;
  while (!(SPI->SR & SPI_I2S_FLAG_TXE));

  SPI->DR = arg;
  while (!(SPI->SR & SPI_I2S_FLAG_TXE));

  SPI->DR = crc;
  while (!(SPI->SR & SPI_I2S_FLAG_TXE));

  while (SPI->SR & SPI_I2S_FLAG_BSY);
  }
//}}}

//{{{
static bool getResponse (uint8_t response) {

  // Check for response or timeout
  for (uint16_t i = 0; i < 0xFFF; i++)
    if (sdReadByte() == response)
      return true;

  return false;
  }
//}}}
//{{{
static bool getDataResponse() {

  bool ok = (sdReadByte() & 0x1F) == SD_DATA_OK;
  while (sdReadByte() == 0);

  return ok;
  }
//}}}

//{{{
static bool getCID (SD_CID* SD_cid) {

  bool ok = false;

  // CS lo
  GPIO_CS->BSRRH = CS_PIN;

  sdWriteCommand (CMD_SEND_CID, 0, 0xFF);

  // Wait for response in the R1 format (0x00 is no errors)
  uint8_t CID_Tab[16];
  if (getResponse (0x00))
    if (getResponse (START_DATA_SINGLE_BLOCK_READ)) {
      for (uint32_t i = 0; i < 16; i++)
        CID_Tab[i] = sdReadByte();

      // dump CRC bytes
      sdWriteByte (SD_DUMMY_BYTE);
      sdWriteByte (SD_DUMMY_BYTE);

      SD_cid->ManufacturerID = CID_Tab[0];
      SD_cid->OEM_AppliID = CID_Tab[1] << 8;
      SD_cid->OEM_AppliID |= CID_Tab[2];
      SD_cid->ProdName1 = CID_Tab[3] << 24;
      SD_cid->ProdName1 |= CID_Tab[4] << 16;
      SD_cid->ProdName1 |= CID_Tab[5] << 8;
      SD_cid->ProdName1 |= CID_Tab[6];
      SD_cid->ProdName2 = CID_Tab[7];
      SD_cid->ProdRev = CID_Tab[8];
      SD_cid->ProdSN = CID_Tab[9] << 24;
      SD_cid->ProdSN |= CID_Tab[10] << 16;
      SD_cid->ProdSN |= CID_Tab[11] << 8;
      SD_cid->ProdSN |= CID_Tab[12];
      SD_cid->Reserved1 |= (CID_Tab[13] & 0xF0) >> 4;
      SD_cid->ManufactDate = (CID_Tab[13] & 0x0F) << 8;
      SD_cid->ManufactDate |= CID_Tab[14];
      SD_cid->CID_CRC = (CID_Tab[15] & 0xFE) >> 1;
      SD_cid->Reserved2 = 1;
      ok = true;
      }

  // CS hi
  GPIO_CS->BSRRL = CS_PIN;

  // Send dummy byte: 8 Clock pulses of delay
  sdWriteByte (SD_DUMMY_BYTE);

  return ok;
  }
//}}}
//{{{
static bool getCSD (SD_CSD* SD_csd) {

  bool ok = false;

  // CS lo
  GPIO_CS->BSRRH = CS_PIN;

  // Send CMD9 - CMD_SEND_CSD
  sdWriteCommand (CMD_SEND_CSD, 0, 0xFF);

  // Wait for response in the R1 format (0x00 is no errors)
  if (getResponse (0x00))
    if (getResponse (START_DATA_SINGLE_BLOCK_READ)) {
      uint8_t CSD_Tab[16];
      for (uint32_t i = 0; i < 16; i++)
        CSD_Tab[i] = sdReadByte();

      // dump CRC bytes
      sdWriteByte (SD_DUMMY_BYTE);
      sdWriteByte (SD_DUMMY_BYTE);

      SD_csd->CSDStruct = (CSD_Tab[0] & 0xC0) >> 6;
      SD_csd->SysSpecVersion = (CSD_Tab[0] & 0x3C) >> 2;
      SD_csd->Reserved1 = CSD_Tab[0] & 0x03;
      SD_csd->TAAC = CSD_Tab[1];
      SD_csd->NSAC = CSD_Tab[2];
      SD_csd->MaxBusClkFrec = CSD_Tab[3];
      SD_csd->CardComdClasses = CSD_Tab[4] << 4;
      SD_csd->CardComdClasses |= (CSD_Tab[5] & 0xF0) >> 4;
      SD_csd->RdBlockLen = CSD_Tab[5] & 0x0F;
      SD_csd->PartBlockRead = (CSD_Tab[6] & 0x80) >> 7;
      SD_csd->WrBlockMisalign = (CSD_Tab[6] & 0x40) >> 6;
      SD_csd->RdBlockMisalign = (CSD_Tab[6] & 0x20) >> 5;
      SD_csd->DSRImpl = (CSD_Tab[6] & 0x10) >> 4;
      SD_csd->Reserved2 = 0; /*!< Reserved */
      SD_csd->DeviceSize = (CSD_Tab[6] & 0x03) << 10;
      SD_csd->DeviceSize |= (CSD_Tab[7]) << 2;
      SD_csd->DeviceSize |= (CSD_Tab[8] & 0xC0) >> 6;
      SD_csd->MaxRdCurrentVDDMin = (CSD_Tab[8] & 0x38) >> 3;
      SD_csd->MaxRdCurrentVDDMax = (CSD_Tab[8] & 0x07);
      SD_csd->MaxWrCurrentVDDMin = (CSD_Tab[9] & 0xE0) >> 5;
      SD_csd->MaxWrCurrentVDDMax = (CSD_Tab[9] & 0x1C) >> 2;
      SD_csd->DeviceSizeMul = (CSD_Tab[9] & 0x03) << 1;
      SD_csd->DeviceSizeMul |= (CSD_Tab[10] & 0x80) >> 7;
      SD_csd->EraseGrSize = (CSD_Tab[10] & 0x40) >> 6;
      SD_csd->EraseGrMul = (CSD_Tab[10] & 0x3F) << 1;
      SD_csd->EraseGrMul |= (CSD_Tab[11] & 0x80) >> 7;
      SD_csd->WrProtectGrSize = (CSD_Tab[11] & 0x7F);
      SD_csd->WrProtectGrEnable = (CSD_Tab[12] & 0x80) >> 7;
      SD_csd->ManDeflECC = (CSD_Tab[12] & 0x60) >> 5;
      SD_csd->WrSpeedFact = (CSD_Tab[12] & 0x1C) >> 2;
      SD_csd->MaxWrBlockLen = (CSD_Tab[12] & 0x03) << 2;
      SD_csd->MaxWrBlockLen |= (CSD_Tab[13] & 0xC0) >> 6;
      SD_csd->WriteBlockPaPartial = (CSD_Tab[13] & 0x20) >> 5;
      SD_csd->Reserved3 = 0;
      SD_csd->ContentProtectAppli = (CSD_Tab[13] & 0x01);
      SD_csd->FileFormatGrouop = (CSD_Tab[14] & 0x80) >> 7;
      SD_csd->CopyFlag = (CSD_Tab[14] & 0x40) >> 6;
      SD_csd->PermWrProtect = (CSD_Tab[14] & 0x20) >> 5;
      SD_csd->TempWrProtect = (CSD_Tab[14] & 0x10) >> 4;
      SD_csd->FileFormat = (CSD_Tab[14] & 0x0C) >> 2;
      SD_csd->ECC = (CSD_Tab[14] & 0x03);
      SD_csd->CSD_CRC = (CSD_Tab[15] & 0xFE) >> 1;
      SD_csd->Reserved4 = 1;

      ok = true;
      }

  // CS hi
  GPIO_CS->BSRRL = CS_PIN;

  sdWriteByte (SD_DUMMY_BYTE);

  return ok;
  }
//}}}

// public
//{{{
uint16_t SD_getStatus() {

  // CS lo
  GPIO_CS->BSRRH = CS_PIN;

  // Send CMD13 CMD_SEND_STATUS)
  sdWriteCommand (CMD_SEND_STATUS, 0, 0xFF);

  uint16_t status = sdReadByte();
  status |= (uint16_t)(sdReadByte() << 8);

  // CS hi
  GPIO_CS->BSRRL = CS_PIN;

  sdWriteByte (SD_DUMMY_BYTE);

  return status;
  }
//}}}
//{{{
SD_Error SD_getCardInfo (SD_CardInfo* cardinfo) {

  if (getCSD (&(cardinfo->SD_csd)))
    if (getCID (&(cardinfo->SD_cid))) {

    cardinfo->CardBlockSize = 1 << (cardinfo->SD_csd.RdBlockLen);

    cardinfo->CardCapacity = cardinfo->SD_csd.DeviceSize + 1 ;
    cardinfo->CardCapacity *= (1 << (cardinfo->SD_csd.DeviceSizeMul + 2));
    cardinfo->CardCapacity *= cardinfo->CardBlockSize;

    return SD_OK;
    }

  return SD_RESPONSE_FAILURE;
  }
//}}}

//{{{
SD_Error SD_readBlocks (uint8_t* buf, uint32_t sector, uint32_t blockSize, uint32_t blocks) {

  SD_Error error = SD_RESPONSE_FAILURE;

  // CS lo
  GPIO_CS->BSRRH = CS_PIN;

  sector *= blockSize;

  // Data transfer
  uint32_t offset = 0;
  while (blocks--) {
    // Send CMD17 - CMD_READ_SINGLE_BLOCK to read one block
    sdWriteCommand (CMD_READ_SINGLE_BLOCK, sector + offset, 0xFF);

    // Check if the SD acknowledged the read block command: R1 response (0x00: no errors)
    if (!getResponse (0x00))
      return SD_RESPONSE_FAILURE;

    // Now look for the data token to signify the start of the data
    if (getResponse (START_DATA_SINGLE_BLOCK_READ)) {
      sdReadBlockData (buf, blockSize);

      buf += blockSize;
      offset += blockSize;

      // dump CRC bytes
      sdReadByte();
      sdReadByte();

      // Set response value to success */
      error = SD_OK;
      }
    else // Set response value to failure */
      error = SD_RESPONSE_FAILURE;
    }

  // CS hi
  GPIO_CS->BSRRL = CS_PIN;

  sdWriteByte (SD_DUMMY_BYTE);

  return error;
  }
//}}}
//{{{
SD_Error SD_writeBlocks (uint8_t* buf, uint32_t sector, uint32_t blockSize, uint32_t blocks) {

  SD_Error error = SD_RESPONSE_FAILURE;

  #ifdef DEBUG
  printf ("SD_writeBlocks %d %d %d\n", sector, blockSize, blocks);
  #endif

  // CS lo
  GPIO_CS->BSRRH = CS_PIN;

  sector *= blockSize;

  // Data transfer
  uint32_t Offset = 0;
  while (blocks--) {
    // Send CMD24 (CMD_WRITE_SINGLE_BLOCK) to write blocks
    sdWriteCommand (CMD_WRITE_SINGLE_BLOCK, sector + Offset, 0xFF);

    // Check if the SD acknowledged the write block command: R1 response (0x00: no errors)
    if (!getResponse (0x00))
      return SD_RESPONSE_FAILURE;

    sdWriteByte (SD_DUMMY_BYTE);

    // Send the data token to signify the start of the data
    sdWriteByte (START_DATA_SINGLE_BLOCK_WRITE);

    uint16_t crc = 0;
    for (uint32_t i = 0; i < blockSize; i++) {
      sdWriteByte (*buf);
      crc  = (uint8_t)(crc >> 8) | (crc << 8);
      crc ^= *buf;
      crc ^= (uint8_t)(crc & 0xff) >> 4;
      crc ^= (crc << 8) << 4;
      crc ^= ((crc & 0xff) << 4) << 1;
      buf++;
      }

    // Set next write address
    Offset += blockSize;

    sdWriteByte (crc>>8);
    sdWriteByte (crc);

    // Read data response
    if (getDataResponse())
      error = SD_OK;
    else
      error = SD_RESPONSE_FAILURE;
    }

  // CS hi
  GPIO_CS->BSRRL = CS_PIN;

  sdWriteByte (SD_DUMMY_BYTE);

  return error;
  }
//}}}

//{{{
SD_Error SD_init (bool wideBus) {

  // clocks
  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIO | RCC_AHB1Periph_SPI_GPIO, ENABLE);
  //RCC_APB1PeriphClockCmd (RCC_APB1Periph_SPI, ENABLE);
  RCC_APB2PeriphClockCmd (RCC_APB2Periph_SPI, ENABLE);

  // enable CS GPIO pin
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.GPIO_Pin = CS_PIN;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init (GPIO_CS, &GPIO_InitStruct);
  // CS hi
  GPIO_CS->BSRRL = CS_PIN;

  // enable SPI pins
  GPIO_InitStruct.GPIO_Pin = SPI_SCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_Init (GPIO_SPI, &GPIO_InitStruct);

  // set SPI pin to alternate function
  GPIO_PinAFConfig (GPIO_SPI, SPI_SCK_SOURCE, GPIO_AF_SPI);
  GPIO_PinAFConfig (GPIO_SPI, SPI_MISO_SOURCE, GPIO_AF_SPI);
  GPIO_PinAFConfig (GPIO_SPI, SPI_MOSI_SOURCE, GPIO_AF_SPI);

  // master, mode3, 2 wire duplex, 8bit, MSBfirst, NSS pin high - 350khz
  SPI_InitTypeDef SPI_InitStructure;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft | SPI_NSSInternalSoft_Set;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_Init (SPI, &SPI_InitStructure);
  SPI_Cmd (SPI, ENABLE);

  //  CS hi, MOSI hi for 80 clocks - 10x 0xFF, CS lo
  GPIO_CS->BSRRL = CS_PIN;
  for (int i = 0; i <= 9; i++)
    sdWriteByte (SD_DUMMY_BYTE);
  GPIO_CS->BSRRH = CS_PIN;

  // CMD0 - CMD_GO_IDLE_STATE put SD in SPI mode
  sdWriteCommand (CMD_GO_IDLE_STATE, 0, 0x95);

  // wait for IN_IDLE_STATE response (R1 Format) equal to 0x01
  if (!getResponse (SD_IN_IDLE_STATE))
    return SD_RESPONSE_FAILURE;

  // card initialization process
  do {
    // CS hi, dummy byte, CS lo
    GPIO_CS->BSRRL = CS_PIN;
    sdWriteByte (SD_DUMMY_BYTE);
    GPIO_CS->BSRRH = CS_PIN;

    // Send CMD1 (Activates the card process) until response equal to 0x0
    sdWriteCommand (CMD_SEND_OP_COND, 0, 0xFF);
    // Wait for no error Response (R1 Format) equal to 0x00
    } while (!getResponse (0x00));

  // CS hi, dummy byte
  GPIO_CS->BSRRL = CS_PIN;
  sdWriteByte (SD_DUMMY_BYTE);

  // SPI fast speed - 22.5Mhz
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
  SPI_Init (SPI, &SPI_InitStructure);

  return SD_OK;
  }
//}}}
