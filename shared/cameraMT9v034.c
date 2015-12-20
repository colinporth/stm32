// cameraMT9v034.c
//{{{  includes
#include "stdio.h"

#include "display.h"
#include "camera.h"
#include "delay.h"
#include "sdram.h"

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_dcmi.h"
//}}}

#define MT9V034_SLAVE_ADDRESS  0x90   // SA_CTRL 0,1 = 0
#define SENSOR_WIDTH    752
#define SENSOR_HEIGHT   480

#define FRAME_BUFFER    SDRAM

//#define CONTEXT_A
#ifdef CONTEXT_A
  #define PIC_WIDTH     (SENSOR_WIDTH/2)
  #define PIC_HEIGHT    (SENSOR_HEIGHT/2)
  #define NUM_BUFFERS   1
  #define BUFFER_SIZE   (PIC_WIDTH*PIC_HEIGHT)
#else
  #define PIC_WIDTH     SENSOR_WIDTH
  #define PIC_HEIGHT    SENSOR_HEIGHT
  #define NUM_BUFFERS   2
  #define BUFFER_SIZE   (PIC_WIDTH*PIC_HEIGHT/2)
#endif

volatile static uint16_t id = 0;
volatile static uint32_t frame = 0;
volatile static uint32_t framePhase = 0;
volatile static uint32_t frameDone = false;

//{{{
static bool I2C_WriteReg8 (uint8_t reg, uint8_t data) {

  uint32_t timeout = 10000;

  I2C_GenerateSTART(I2C2, ENABLE);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_MODE_SELECT))
    if ((timeout--) == 0) return false;

  I2C_Send7bitAddress (I2C2, MT9V034_SLAVE_ADDRESS, I2C_Direction_Transmitter);
  while(!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    if ((timeout--) == 0) return false;

  I2C_SendData (I2C2, reg);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    if ((timeout--) == 0) return false;

  I2C_SendData (I2C2, data);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    if ((timeout--) == 0) return false;

  I2C_GenerateSTOP (I2C2, ENABLE);

  return true;
  }
//}}}
//{{{
static bool I2C_WriteReg16 (uint8_t reg, uint16_t data) {

  return I2C_WriteReg8 (reg, data>>8) && I2C_WriteReg8 (0xF0, data);
  }
//}}}
//{{{
static uint8_t I2C_ReadReg8 (uint8_t reg) {

  I2C_GenerateSTART (I2C2, ENABLE);

  uint32_t timeout = 10000;
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_MODE_SELECT))
    if ((timeout--) == 0) return 0xFF;

  I2C_Send7bitAddress (I2C2, MT9V034_SLAVE_ADDRESS+1, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    if ((timeout--) == 0) return 0xFF;

  I2C_SendData (I2C2, (uint8_t)(reg));
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    if ((timeout--) == 0) return 0xFF;

  I2C2->SR1 |= (uint16_t)0x0400;

  I2C_GenerateSTART (I2C2, ENABLE);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_MODE_SELECT))
    if ((timeout--) == 0) return 0xFF;

  I2C_Send7bitAddress (I2C2, MT9V034_SLAVE_ADDRESS+1, I2C_Direction_Receiver);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
    if ((timeout--) == 0) return 0xFF;

  I2C_AcknowledgeConfig (I2C2, DISABLE);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED))
    if ((timeout--) == 0) return 0xFF;

  I2C_GenerateSTOP (I2C2, ENABLE);

  return I2C_ReceiveData (I2C2);
  }
//}}}
//{{{
static uint16_t I2C_ReadReg16 (uint8_t reg) {

  return (I2C_ReadReg8 (reg)<<8) | I2C_ReadReg8 (0xF0);
  }
//}}}

//{{{
static void initGPIO() {
// PB04 - gpio PWRDWN
// PB10 - i2c2 scl   - 10k pullup only 100khz
// PB11 - i2c2 sda   - 10k pullup only 100khz
// PA06 - dcmi clk
// PA04 - dcmi hs
// PB07 - dcmi vs
// PC06 - dcmi d0
// PC07 - dcmi d1
// PG10 - dcmi d2
// PG11 - dcmi d3
// PE04 - dcmi d4
// PD03 - dcmi d5
// PB08 - dcmi d6
// PB09 - dcmi d7
  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB |
                          RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD |
                          RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOG, ENABLE);
  RCC_APB1PeriphClockCmd (RCC_APB1Periph_I2C2, ENABLE);
  RCC_AHB2PeriphClockCmd (RCC_AHB2Periph_DCMI, ENABLE);

  // config PB04 - PWRDN lo
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_25MHz;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init (GPIOB, &GPIO_InitStruct);
  GPIOB->BSRRH = GPIO_Pin_4;

  // config I2C2 AF SCL,SDA
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource10, GPIO_AF_I2C2);
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource11, GPIO_AF_I2C2);

  // init I2C2 GPIO
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;
  GPIO_Init (GPIOB, &GPIO_InitStruct);

  // config I2C2
  I2C_InitTypeDef I2C_InitStruct;
  I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStruct.I2C_ClockSpeed = 400000;  // 400kHz with 2.7k pullups
  I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStruct.I2C_OwnAddress1 = 0xFE;
  I2C_InitStruct.I2C_Ack = I2C_Ack_Disable;
  I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_Init (I2C2, &I2C_InitStruct);
  I2C_Cmd (I2C2, ENABLE);

  // setup DCMI pins to AF13
  GPIO_PinAFConfig (GPIOA, GPIO_PinSource4, GPIO_AF_DCMI);  // hs
  GPIO_PinAFConfig (GPIOA, GPIO_PinSource6, GPIO_AF_DCMI);  // pclk
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource7, GPIO_AF_DCMI);  // vs
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource8, GPIO_AF_DCMI);  // D6
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource9, GPIO_AF_DCMI);  // D7
  GPIO_PinAFConfig (GPIOC, GPIO_PinSource6, GPIO_AF_DCMI);  // D0
  GPIO_PinAFConfig (GPIOC, GPIO_PinSource7, GPIO_AF_DCMI);  // D1
  GPIO_PinAFConfig (GPIOD, GPIO_PinSource3, GPIO_AF_DCMI);  // D5
  GPIO_PinAFConfig (GPIOE, GPIO_PinSource4, GPIO_AF_DCMI);  // D4
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource10, GPIO_AF_DCMI); // D2
  GPIO_PinAFConfig (GPIOG, GPIO_PinSource11, GPIO_AF_DCMI); // D3

  // GPIOA hs, pixclk
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_6;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init (GPIOA, &GPIO_InitStruct);

  // GPIOB vs,D6,7
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
  GPIO_Init (GPIOB, &GPIO_InitStruct);

  // GPIOC D0:1
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_Init (GPIOC, &GPIO_InitStruct);

  // GPIOD D5
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3;
  GPIO_Init (GPIOD, &GPIO_InitStruct);

  // GPIOE D4
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
  GPIO_Init (GPIOE, &GPIO_InitStruct);

  // GPIOG vs,D2,3
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
  GPIO_Init (GPIOG, &GPIO_InitStruct);
  }
//}}}
//{{{
static bool initMT9V034() {
  //{{{  register defines
  #define MTV_CHIP_VERSION        0x00
  #define MTV_CHIP_CONTROL        0x07
  #define MTV_SOFT_RESET          0x0C

  #define MTV_HDR_ENABLE          0x0F
  #define MTV_ADC_RES_CTRL        0x1C

  #define MTV_ROW_NOISE_CORR_CTRL 0x70
  #define MTV_DIGITAL_TEST        0x7F

  #define MTV_TILED_DIGITAL_GAIN  0x80

  #define MTV_AGC_AEC_DESIRED_BIN 0xA5
  #define MTV_MAX_GAIN            0xAB

  #define MTV_MIN_EXPOSURE        0xAC  // datasheet min coarse shutter width
  #define MTV_MAX_EXPOSURE        0xAD  // datasheet max coarse shutter width

  #define MTV_AEC_AGC_ENABLE      0xAF
  #define MTV_AGC_AEC_PIXEL_COUNT 0xB0
  #define MTV_AEC_UPDATE          0xA6
  #define MTV_AEC_LOWPASS         0xA8
  #define MTV_AGC_UPDATE          0xA9
  #define MTV_AGC_LOWPASS         0xAA
  //}}}
  //{{{  context A register defines
  #define MTV_COLUMN_START_A      0x01
  #define MTV_ROW_START_A         0x02
  #define MTV_WINDOW_HEIGHT_A     0x03
  #define MTV_WINDOW_WIDTH_A      0x04
  #define MTV_HOR_BLANKING_A      0x05
  #define MTV_VER_BLANKING_A      0x06

  #define MTV_COARSE_SW_1_A       0x08
  #define MTV_COARSE_SW_2_A       0x09
  #define MTV_COARSE_SW_CTRL_A    0x0A
  #define MTV_COARSE_SW_TOTAL_A   0x0B

  #define MTV_READ_MODE_A         0x0D

  #define MTV_V1_CTRL_A           0x31
  #define MTV_V2_CTRL_A           0x32
  #define MTV_V3_CTRL_A           0x33
  #define MTV_V4_CTRL_A           0x34

  #define MTV_ANALOG_GAIN_CTRL_A  0x35

  #define MTV_FINE_SW_1_A         0xD3
  #define MTV_FINE_SW_2_A         0xD4
  #define MTV_FINE_SW_TOTAL_A     0xD5
  //}}}
  //{{{  context B register defines
  #define MTV_READ_MODE_B         0x0E

  #define MTV_ANALOG_GAIN_CTRL_B  0x36

  #define MTV_V1_CTRL_B           0x39
  #define MTV_V2_CTRL_B           0x3A
  #define MTV_V3_CTRL_B           0x3B
  #define MTV_V4_CTRL_B           0x3C

  #define MTV_COLUMN_START_B      0xC9
  #define MTV_ROW_START_B         0xCA
  #define MTV_WINDOW_HEIGHT_B     0xCB
  #define MTV_WINDOW_WIDTH_B      0xCC
  #define MTV_HOR_BLANKING_B      0xCD
  #define MTV_VER_BLANKING_B      0xCE

  #define MTV_COARSE_SW_1_B       0xCF
  #define MTV_COARSE_SW_2_B       0xD0
  #define MTV_COARSE_SW_CTRL_B    0xD1
  #define MTV_COARSE_SW_TOTAL_B   0xD2

  #define MTV_FINE_SW_1_B         0xD6
  #define MTV_FINE_SW_2_B         0xD7
  #define MTV_FINE_SW_TOTAL_B     0xD8
  //}}}

  id = I2C_ReadReg16 (MTV_CHIP_VERSION);
  if (id != 0x1324)
    return false;

  //{{{  Context A - 376x240 bin2
  I2C_WriteReg16 (MTV_READ_MODE_A, 0x305);   // b3:0 = row,col bin2
  I2C_WriteReg16 (MTV_COLUMN_START_A, 1);
  I2C_WriteReg16 (MTV_WINDOW_WIDTH_A, SENSOR_WIDTH);
  I2C_WriteReg16 (MTV_HOR_BLANKING_A, 71);   // min for bin2

  I2C_WriteReg16 (MTV_ROW_START_A, 4);
  I2C_WriteReg16 (MTV_WINDOW_HEIGHT_A, SENSOR_HEIGHT);
  I2C_WriteReg16 (MTV_VER_BLANKING_A, 10);

  I2C_WriteReg16 (MTV_COARSE_SW_1_A, 0x01BB);
  I2C_WriteReg16 (MTV_COARSE_SW_2_A, 0x01D9);
  I2C_WriteReg16 (MTV_COARSE_SW_CTRL_A, 0x0164);

  I2C_WriteReg16 (MTV_V2_CTRL_A, 480);
  //}}}
  //{{{  Context B - 752x480 no bin
  I2C_WriteReg16 (MTV_READ_MODE_B, 0x300);

  I2C_WriteReg16 (MTV_COLUMN_START_B, 1);
  I2C_WriteReg16 (MTV_WINDOW_WIDTH_B, SENSOR_WIDTH);
  I2C_WriteReg16 (MTV_HOR_BLANKING_B, 61);          // min for no bin

  I2C_WriteReg16 (MTV_ROW_START_B, 4);
  I2C_WriteReg16 (MTV_WINDOW_HEIGHT_B, SENSOR_HEIGHT);
  I2C_WriteReg16 (MTV_VER_BLANKING_B, 10);

  I2C_WriteReg16 (MTV_COARSE_SW_1_B, 0x01BB);
  I2C_WriteReg16 (MTV_COARSE_SW_2_B, 0x01D9);
  I2C_WriteReg16 (MTV_COARSE_SW_CTRL_B, 0x0164);

  I2C_WriteReg16 (MTV_V2_CTRL_B, 480);
  //}}}

  #ifdef CONTEXT_A
  I2C_WriteReg16 (MTV_CHIP_CONTROL, 0x0188);        // b15 = A, b8 = sim, b7 = par, b3 = master
  #else
  I2C_WriteReg16 (MTV_CHIP_CONTROL, 0x8188);        // b15 = B, b8 = sim, b7 = par, b3 = master
  #endif

  I2C_WriteReg16 (MTV_ADC_RES_CTRL, 0x0202);        // AB 10bit
  I2C_WriteReg16 (MTV_HDR_ENABLE, 0x0000);          // AB disable HDR
  I2C_WriteReg16 (MTV_ROW_NOISE_CORR_CTRL, 0x0101); // AB enable row noise cancellation
  //I2C_WriteReg16 (MTV_DIGITAL_TEST, 0x2800);

  // AEC,AGC
  I2C_WriteReg16 (MTV_AEC_AGC_ENABLE, 0x0303);      // AB AEC,AGC enabled
  I2C_WriteReg16 (MTV_MIN_EXPOSURE, 0x0001);
  I2C_WriteReg16 (MTV_MAX_EXPOSURE, 480);
  I2C_WriteReg16 (MTV_MAX_GAIN, 64);                // range 16 to 64
  I2C_WriteReg16 (MTV_AGC_AEC_DESIRED_BIN, 58);     // desired brightness 58 max 64
  I2C_WriteReg16 (MTV_AGC_AEC_PIXEL_COUNT, 4096);   // pixels for AEC/AGC histogram
  I2C_WriteReg16 (MTV_AEC_UPDATE, 0x02);            // AEC frames skip before updating exposure
  I2C_WriteReg16 (MTV_AEC_LOWPASS, 0x01);
  I2C_WriteReg16 (MTV_AGC_UPDATE, 0x02);            // AGC frames skip before updating gain
  I2C_WriteReg16 (MTV_AGC_LOWPASS, 0x02);

  I2C_WriteReg16 (MTV_SOFT_RESET, 0x01);

  return true;
  }
//}}}
//{{{
static void initDCMI (bool continuous) {

  // config DMA2 clock
  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_DMA2, ENABLE);

  // config DCMI
  DCMI_InitTypeDef DCMI_InitStructure;
  DCMI_InitStructure.DCMI_CaptureMode =
    continuous ? DCMI_CaptureMode_Continuous : DCMI_CaptureMode_SnapShot;
  DCMI_InitStructure.DCMI_SynchroMode = DCMI_SynchroMode_Hardware;
  DCMI_InitStructure.DCMI_PCKPolarity = DCMI_PCKPolarity_Rising;
  DCMI_InitStructure.DCMI_VSPolarity = DCMI_VSPolarity_Low;
  DCMI_InitStructure.DCMI_HSPolarity = DCMI_HSPolarity_Low;
  DCMI_InitStructure.DCMI_CaptureRate = DCMI_CaptureRate_All_Frame;
  DCMI_InitStructure.DCMI_ExtendedDataMode = DCMI_ExtendedDataMode_8b;
  DCMI_Init (&DCMI_InitStructure);

  // DMA2 Stream1 Configuration
  DMA_DeInit (DMA2_Stream1);

  DMA_InitTypeDef DMA_InitStructure;
  DMA_InitStructure.DMA_Channel = DMA_Channel_1;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(DCMI_BASE + 0x28);
  DMA_InitStructure.DMA_Memory0BaseAddr = FRAME_BUFFER;
  DMA_InitStructure.DMA_BufferSize = BUFFER_SIZE/4;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init (DMA2_Stream1, &DMA_InitStructure);

  DMA_DoubleBufferModeConfig (DMA2_Stream1, FRAME_BUFFER, DMA_Memory_0);
  DMA_DoubleBufferModeCmd (DMA2_Stream1, ENABLE);

  // config DMA global Interrupt, half transfer, transfer complete
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init (&NVIC_InitStructure);
  DMA_ITConfig (DMA2_Stream1, DMA_IT_HT, ENABLE);
  DMA_ITConfig (DMA2_Stream1, DMA_IT_TC, ENABLE);

  // config dcmi global Interrupt, frame complete
  NVIC_InitStructure.NVIC_IRQChannel = DCMI_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init (&NVIC_InitStructure);
  DCMI_ITConfig (DCMI_IT_FRAME, ENABLE);
  }
//}}}

//{{{
void DMA2_Stream1_IRQHandler() {

  // transfer completed
  if (DMA_GetITStatus (DMA2_Stream1, DMA_IT_TCIF1) != RESET) {
    DMA_ClearITPendingBit (DMA2_Stream1, DMA_IT_TCIF1);
    return;
    }

  // transfer half completed, setup next buffer in cycle of NUM_BUFFERS
  if (DMA_GetITStatus (DMA2_Stream1, DMA_IT_HTIF1) != RESET) {
    DMA_ClearITPendingBit (DMA2_Stream1, DMA_IT_HTIF1);

    framePhase++;
    if (framePhase == NUM_BUFFERS)
      framePhase = 0;

    DMA_MemoryTargetConfig (DMA2_Stream1,
                            FRAME_BUFFER + (framePhase*BUFFER_SIZE),
                            DMA_GetCurrentMemoryTarget (DMA2_Stream1) ? DMA_Memory_0 : DMA_Memory_1);
    }
  }
//}}}
//{{{
void DCMI_IRQHandler() {

  if (DCMI_GetITStatus (DCMI_IT_FRAME) != RESET) {
    DCMI_ClearITPendingBit (DCMI_IT_FRAME);
    frame++;
    frameDone = true;
    }

  return;
  }
//}}}

uint32_t cameraId() { return id; }
uint32_t cameraFrame() { return frame; }

//{{{
void startCamera() {

  DMA_Cmd (DMA2_Stream1, ENABLE);
  DCMI_Cmd (ENABLE);
  DCMI_CaptureCmd (ENABLE);
  }
//}}}
//{{{
void waitCamera() {

  frameDone = false;
  while (!frameDone) {}
  }
//}}}
//{{{
void showCamera() {
  lcdSpiDisplayMono (FRAME_BUFFER,
                     (PIC_WIDTH - getWidth()) / 2, (PIC_HEIGHT - getHeight()) / 2,
                     PIC_WIDTH);
  }
//}}}

//{{{
bool initCamera (bool continuous) {

  initGPIO();

  if (!initMT9V034())
    return false;

  initDCMI (continuous);

  return true;
  }
//}}}
