// cameraMT9D112.c
//{{{  unused settings
  // 1600x1200
  //I2C_WriteReg16 (0x338c, 0xa120);
  //I2C_WriteReg16 (0x3390, 0x0002);

  //I2C_WriteReg16 (0x338c, 0xa103);
  //I2C_WriteReg16 (0x3390, 0x0002);

//static struct regval_list sensor_ev_neg4_regs[] = {
//  {{0x33,0x8c},{0x22,0x44}},
//  {{0x33,0x90},{0x00,0x10}},
//static struct regval_list sensor_ev_zero_regs[] = {
//  {{0x33,0x8c},{0x22,0x44}},
//  {{0x33,0x90},{0x00,0x3d}},
//static struct regval_list sensor_ev_pos4_regs[] = {
//  {{0x33,0x8c},{0x22,0x44}},
//  {{0x33,0x90},{0x00,0x7d}},
//static struct regval_list sensor_wb_auto_regs[] = {
//{{0x33,0x8C}, {0xA1,0x02}},
//{{0x33,0x90}, {0x00,0x0F}},
//{{0x33,0x8C}, {0xA3,0x67}},
//{{0x33,0x90}, {0x00,0x80}},
//{{0x33,0x8C}, {0xA3,0x68}},
//{{0x33,0x90}, {0x00,0x80}},
//{{0x33,0x8C}, {0xA3,0x69}},
//{{0x33,0x90}, {0x00,0x80}},
//}}}
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

#define MT9D112_SLAVE_ADDRESS  0x78

#define FRAME_BUFFER    SDRAM

//#define QVGA
#ifdef QVGA
  #define PIC_WIDTH     320
  #define PIC_HEIGHT    240
#else
  #define PIC_WIDTH     640
  #define PIC_HEIGHT    480
#endif

#define NUM_BUFFERS     (((PIC_WIDTH*PIC_HEIGHT*2/4)/0x10000) + 1)
#define BUFFER_SIZE     (PIC_WIDTH*PIC_HEIGHT*2/NUM_BUFFERS)


volatile static uint32_t DelayCount;
volatile static uint32_t frame = 0;
volatile static uint32_t frameDone = false;
volatile static uint32_t framePhase = 0;

//{{{
static uint16_t I2C_WriteReg16 (uint16_t reg, uint16_t data) {

  DelayCount = 0x1000;

  // Send I2C1 START condition
  I2C_GenerateSTART (I2C2, ENABLE);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_MODE_SELECT))
    if ((DelayCount--) == 0)
      return false;

  // Send slave Address for write
  I2C_Send7bitAddress (I2C2, MT9D112_SLAVE_ADDRESS, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    if ((DelayCount--) == 0)
      return false;

  I2C_SendData (I2C2, reg >> 8);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    if ((DelayCount--) == 0)
      return false;
  I2C_SendData (I2C2, reg & 0xFF);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    if ((DelayCount--) == 0)
      return false;

  I2C_SendData (I2C2, data>>8);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    if ((DelayCount--) == 0)
      return false;
  I2C_SendData (I2C2, data & 0xFF);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    if ((DelayCount--) == 0)
      return false;

  I2C_GenerateSTOP (I2C2, ENABLE);
  return true;
  }
//}}}

//{{{
static void initGPIO() {
// PB04 - gpio PWRDWN
// PB10 - i2c2 scl
// PB11 - i2c2 sda
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
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_25MHz;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;
  GPIO_Init (GPIOB, &GPIO_InitStruct);

  // config I2C2
  I2C_InitTypeDef I2C_InitStruct;
  I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStruct.I2C_ClockSpeed = 100000;  // 100kHz
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
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
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
static void initMT9D112() {
  //  MCUBootMode - pulse reset
  I2C_WriteReg16 (0x3386, 0x2501);
  I2C_WriteReg16 (0x3386, 0x2500);
  delayMs (100);

  // reset_register = parallel enable
  I2C_WriteReg16 (0x301A, 0x0ACC);

  // standby_control
  I2C_WriteReg16 (0x3202, 0x0008);
  delayMs (100);

  // input powerDown, outputSlew = 5 (of 0:7)
  I2C_WriteReg16 (0x3214, 0x0585);
  //{{{  PLL control
  //I2C_WriteReg16 (0x341E, 0x8F0B); // bypass PLL

  I2C_WriteReg16 (0x341E, 0x8F09); // PLL,clk_in control BYPASS PLL
  I2C_WriteReg16 (0x341C, 0x0140); // PLL N = 1, M = 64 - clk = (10Mhz / 2) * (64 / 8)  = 40 mhz
  delayMs (5);

  I2C_WriteReg16 (0x341E, 0x8F09); // PLL,clk_in control: PLL ON, bypass PLL
  I2C_WriteReg16 (0x341E, 0x8F08); // PLL,clk_in control: USE PLL
  //}}}

  //{{{  A output width = 640, height = 320
  I2C_WriteReg16 (0x338C, 0x2703); // Output Width (A)
  I2C_WriteReg16 (0x3390, 640);
  I2C_WriteReg16 (0x338C, 0x2705); // Output Height (A)
  I2C_WriteReg16 (0x3390, 480);
  //}}}
  //{{{  A row,column start,end = 120,160,1101,1461
  I2C_WriteReg16 (0x338C, 0x270D);
  I2C_WriteReg16 (0x3390, 0x0078);

  I2C_WriteReg16 (0x338C, 0x270F);
  I2C_WriteReg16 (0x3390, 0x00A0);

  I2C_WriteReg16 (0x338C, 0x2711);
  I2C_WriteReg16 (0x3390, 0x044d);

  I2C_WriteReg16 (0x338C, 0x2713);
  I2C_WriteReg16 (0x3390, 0x05b5);
  //}}}
  //{{{  A sensor_col_delay_A = 175
  I2C_WriteReg16 (0x338C, 0x2715);
  I2C_WriteReg16 (0x3390, 0x00AF);
  //}}}
  //{{{  A sensor_row_speed_A = 0x2111
  I2C_WriteReg16 (0x338C, 0x2717);
  I2C_WriteReg16 (0x3390, 0x2111);
  //}}}
  //{{{  A read mode = 0x046c default, x, xy binning, x,y odd increment
  I2C_WriteReg16 (0x338C, 0x2719);
  I2C_WriteReg16 (0x3390, 0x046c);
  //}}}
  //{{{  A sensor_x
  I2C_WriteReg16 (0x338C, 0x271B); // sensor_sample_time_pck (A)
  I2C_WriteReg16 (0x3390, 0x024F); // = 591

  I2C_WriteReg16 (0x338C, 0x271D); // sensor_fine_correction (A)
  I2C_WriteReg16 (0x3390, 0x0102); // = 258

  I2C_WriteReg16 (0x338C, 0x271F); // sensor_fine_IT_min (A)
  I2C_WriteReg16 (0x3390, 0x0279); // = 633

  I2C_WriteReg16 (0x338C, 0x2721); // sensor_fine_IT_max_margin (A)
  I2C_WriteReg16 (0x3390, 0x0155); // = 341
  //}}}
  //{{{  A frame lines = 575
  I2C_WriteReg16 (0x338C, 0x2723);
  I2C_WriteReg16 (0x3390, 0x0205);
  //}}}
  //{{{  A line length = 1391
  I2C_WriteReg16 (0x338C, 0x2725);
  I2C_WriteReg16 (0x3390, 0x056F);
  //}}}
  //{{{  A sensor_dac_id_x
  I2C_WriteReg16 (0x338C, 0x2727); // sensor_dac_id_4_5 (A)
  I2C_WriteReg16 (0x3390, 0x2020); // = 8224

  I2C_WriteReg16 (0x338C, 0x2729); // sensor_dac_id_6_7 (A)
  I2C_WriteReg16 (0x3390, 0x2020); // = 8224

  I2C_WriteReg16 (0x338C, 0x272B); // sensor_dac_id_8_9 (A)
  I2C_WriteReg16 (0x3390, 0x1020); // = 4128

  I2C_WriteReg16 (0x338C, 0x272D); // sensor_dac_id_10_11 (A)
  I2C_WriteReg16 (0x3390, 0x2007); // = 8199
  //}}}
  //{{{  A crop = 0,0,640,480
  I2C_WriteReg16 (0x338C, 0x2751); // Crop_X0 (A)
  I2C_WriteReg16 (0x3390, 0x0000); // = 0

  I2C_WriteReg16 (0x338C, 0x2753); // Crop_X1 (A)
  I2C_WriteReg16 (0x3390, 0x0280); // = 640

  I2C_WriteReg16 (0x338C, 0x2755); // Crop_Y0 (A)
  I2C_WriteReg16 (0x3390, 0x0000); // = 0

  I2C_WriteReg16 (0x338C, 0x2757); // Crop_Y1 (A)
  I2C_WriteReg16 (0x3390, 0x01E0); // = 480
  //}}}
  //{{{  A output_format = rgb565
  I2C_WriteReg16 (0x338c, 0x2795); // Natural, Swaps chrominance byte
  I2C_WriteReg16 (0x3390, 0x0022); // select RGB565 - 0002 = yuv
  //}}}

  //{{{  B output width = 1600, height = 1200
  I2C_WriteReg16 (0x338C, 0x2707);
  I2C_WriteReg16 (0x3390, 0x0640);

  I2C_WriteReg16 (0x338C, 0x2709);
  I2C_WriteReg16 (0x3390, 0x04B0);
  //}}}
  //{{{  B row,column start,end = 4,4,1211,1611
  I2C_WriteReg16 (0x338C, 0x272F); // Row Start (B)
  I2C_WriteReg16 (0x3390, 0x0004); // = 4

  I2C_WriteReg16 (0x338C, 0x2731); // Column Start (B)
  I2C_WriteReg16 (0x3390, 0x0004); // = 4

  I2C_WriteReg16 (0x338C, 0x2733); // Row End (B)
  I2C_WriteReg16 (0x3390, 0x04BB); // = 1211

  I2C_WriteReg16 (0x338C, 0x2735); // Column End (B)
  I2C_WriteReg16 (0x3390, 0x064B); // = 1611
  //}}}
  //{{{  B extra delay = 124
  I2C_WriteReg16 (0x338C, 0x2737);
  I2C_WriteReg16 (0x3390, 0x007C);
  //}}}
  //{{{  B row speed  = 8465
  I2C_WriteReg16 (0x338C, 0x2739);
  I2C_WriteReg16 (0x3390, 0x2111);
  //}}}
  //{{{  B read mode = 0x0024 default, no binning, normal readout
  I2C_WriteReg16 (0x338C, 0x273B);
  I2C_WriteReg16 (0x3390, 0x0024);
  //}}}
  //{{{  B sensor_x
  I2C_WriteReg16 (0x338C, 0x273D); // sensor_sample_time_pck (B)
  I2C_WriteReg16 (0x3390, 0x0120); // = 288

  I2C_WriteReg16 (0x338C, 0x2741); // sensor_fine_IT_min (B)
  I2C_WriteReg16 (0x3390, 0x0169); // = 361
  //}}}
  //{{{  B frame lines = 1276
  I2C_WriteReg16 (0x338C, 0x2745);
  I2C_WriteReg16 (0x3390, 0x04FC);
  //}}}
  //{{{  B line length = 2351
  I2C_WriteReg16 (0x338C, 0x2747);
  I2C_WriteReg16 (0x3390, 0x092F);
  //}}}
  //{{{  B crop = 0,0,1600,1200
  I2C_WriteReg16 (0x338C, 0x275F); // Crop_X0 (B)
  I2C_WriteReg16 (0x3390, 0x0000); // = 0

  I2C_WriteReg16 (0x338C, 0x2761); // Crop_X1 (B)
  I2C_WriteReg16 (0x3390, 0x0640); // = 1600

  I2C_WriteReg16 (0x338C, 0x2763); // Crop_Y0 (B)
  I2C_WriteReg16 (0x3390, 0x0000); // = 0

  I2C_WriteReg16 (0x338C, 0x2765); // Crop_Y1 (B)
  I2C_WriteReg16 (0x3390, 0x04B0); // = 1200
  //}}}

  //{{{  AE
  I2C_WriteReg16 (0x338C, 0xA215); // AE maxADChi
  I2C_WriteReg16 (0x3390, 0x0006); // gain_thd

  I2C_WriteReg16 (0x338C, 0xA206); // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0036); // AE_TARGET

  I2C_WriteReg16 (0x338C, 0xA207); // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0004); // AE_GATE

  I2C_WriteReg16 (0x338C, 0xA20c); // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0008); // AE_GAT
  //}}}
  //{{{  black level, gain
  I2C_WriteReg16 (0x3278, 0x0050); // first black level
  I2C_WriteReg16 (0x327a, 0x0050); // first black level,red
  I2C_WriteReg16 (0x327c, 0x0050); // green_1
  I2C_WriteReg16 (0x327e, 0x0050); // green_2
  I2C_WriteReg16 (0x3280, 0x0050); // blue

  delayMs (10);
  //}}}
  I2C_WriteReg16 (0x337e, 0x2000); // Y,RGB offset
  //{{{  AWB
  //{{{  AWB_GAIN_x
  I2C_WriteReg16 (0x338C, 0xA34A);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0059);     // AWB_GAIN_MIN
  I2C_WriteReg16 (0x338C, 0xA34B);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x00A6);     // AWB_GAIN_MAX
  //}}}
  //{{{  AWB_CNT_PXL_TH
  I2C_WriteReg16 (0x338C, 0x235F);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0040);     // AWB_CNT_PXL_TH
  //}}}
  //{{{  AWB_TG_x
  I2C_WriteReg16 (0x338C, 0xA361);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x00D2);     // AWB_TG_MIN0
  I2C_WriteReg16 (0x338C, 0xA362);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x00E6);     // AWB_TG_MAX0
  //}}}
  //{{{  AWB_X0
  I2C_WriteReg16 (0x338C, 0xA363);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0010);     // AWB_X0
  //}}}
  //{{{  AWB_Kx
  I2C_WriteReg16 (0x338C, 0xA364);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x00A0);     // AWB_KR_L
  I2C_WriteReg16 (0x338C, 0xA365);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0096);     // AWB_KG_L
  I2C_WriteReg16 (0x338C, 0xA366);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0080);     // AWB_KB_L
  I2C_WriteReg16 (0x338C, 0xA367);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0080);     // AWB_KR_R
  I2C_WriteReg16 (0x338C, 0xA368);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0080);     // AWB_KG_R
  I2C_WriteReg16 (0x338C, 0xA369);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0080);     // AWB_KB_R
  //}}}
  I2C_WriteReg16 (0x32A2, 0x3640);     // RESERVED_SOC1_32A2  // fine tune color setting
  //{{{  AWB_CCM_L_x
  I2C_WriteReg16 (0x338C, 0x2306);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x02FF);     // AWB_CCM_L_0
  I2C_WriteReg16 (0x338C, 0x2308);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0xFE6E);     // AWB_CCM_L_1
  I2C_WriteReg16 (0x338C, 0x230A);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0xFFC2);     // AWB_CCM_L_2
  I2C_WriteReg16 (0x338C, 0x230C);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0xFF4A);     // AWB_CCM_L_3
  I2C_WriteReg16 (0x338C, 0x230E);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x02D7);     // AWB_CCM_L_4
  I2C_WriteReg16 (0x338C, 0x2310);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0xFF30);     // AWB_CCM_L_5
  I2C_WriteReg16 (0x338C, 0x2312);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0xFF6E);     // AWB_CCM_L_6
  I2C_WriteReg16 (0x338C, 0x2314);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0xFDEE);     // AWB_CCM_L_7
  I2C_WriteReg16 (0x338C, 0x2316);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x03CF);     // AWB_CCM_L_8
  I2C_WriteReg16 (0x338C, 0x2318);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0020);     // AWB_CCM_L_9
  I2C_WriteReg16 (0x338C, 0x231A);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x003C);     // AWB_CCM_L_10
  //}}}
  //{{{  AWB_CCM_RL_x
  I2C_WriteReg16 (0x338C, 0x231C);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x002C);     // AWB_CCM_RL_0
  I2C_WriteReg16 (0x338C, 0x231E);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0xFFBC);     // AWB_CCM_RL_1
  I2C_WriteReg16 (0x338C, 0x2320);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0016);     // AWB_CCM_RL_2
  I2C_WriteReg16 (0x338C, 0x2322);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0037);     // AWB_CCM_RL_3
  I2C_WriteReg16 (0x338C, 0x2324);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0xFFCD);     // AWB_CCM_RL_4
  I2C_WriteReg16 (0x338C, 0x2326);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0xFFF3);     // AWB_CCM_RL_5
  I2C_WriteReg16 (0x338C, 0x2328);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0077);     // AWB_CCM_RL_6
  I2C_WriteReg16 (0x338C, 0x232A);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x00F4);     // AWB_CCM_RL_7
  I2C_WriteReg16 (0x338C, 0x232C);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0xFE95);     // AWB_CCM_RL_8
  I2C_WriteReg16 (0x338C, 0x232E);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0014);     // AWB_CCM_RL_9
  I2C_WriteReg16 (0x338C, 0x2330);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0xFFE8);     // AWB_CCM_RL_10  //end
  //}}}
  //{{{  AWB_GAIN_BUFFER_SPEED
  I2C_WriteReg16 (0x338C, 0xA348);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0008);     // AWB_GAIN_BUFFER_SPEED
  //}}}
  //{{{  AWB_JUMP_DIVISOR
  I2C_WriteReg16 (0x338C, 0xA349);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0002);     // AWB_JUMP_DIVISOR
  //}}}
  //{{{  AWB_GAIN_x
  I2C_WriteReg16 (0x338C, 0xA34A);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0059);     // AWB_GAIN_MIN
  I2C_WriteReg16 (0x338C, 0xA34B);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x00A6);     // AWB_GAIN_MAX
  //}}}
  //{{{  AWB_CCM_POSITION_x
  I2C_WriteReg16 (0x338C, 0xA34F);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0000);     // AWB_CCM_POSITION_MIN
  I2C_WriteReg16 (0x338C, 0xA350);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x007F);     // AWB_CCM_POSITION_MAX
  //}}}
  //{{{  AWB SATURATION
  I2C_WriteReg16 (0x338C, 0xA352);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x001E);     // AWB_SATURATION
  //}}}
  //{{{  AWB_MODE
  I2C_WriteReg16 (0x338C, 0xA353);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0002);     // AWB_MODE
  //}}}
  //{{{  AWB_STEADY_BGAIN_x
  I2C_WriteReg16 (0x338C, 0xA35B);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x007E);     // AWB_STEADY_BGAIN_OUT_MIN
  I2C_WriteReg16 (0x338C, 0xA35C);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0086);     // AWB_STEADY_BGAIN_OUT_MAX
  I2C_WriteReg16 (0x338C, 0xA35D);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x007F);     // AWB_STEADY_BGAIN_IN_MIN
  I2C_WriteReg16 (0x338C, 0xA35E);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0082);     // AWB_STEADY_BGAIN_IN_MAX
  //}}}
  //{{{  AWB_WINDOW_x
  I2C_WriteReg16 (0x338C, 0xA302);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0000);     // AWB_WINDOW_POS
  I2C_WriteReg16 (0x338C, 0xA303);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x00EF);     // AWB_WINDOW_SIZE
  //}}}
  //{{{  HG_PERCENT
  I2C_WriteReg16 (0x338C, 0xAB05);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0000);     // HG_PERCENT
  //}}}
  //}}}
  I2C_WriteReg16 (0x35A4, 0x0596); // BRIGHT_COLOR_KILL_CONTROLS
  //{{{  SEQ_LLSATx
  I2C_WriteReg16 (0x338C, 0xA118);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x001E);     // SEQ_LLSAT1

  I2C_WriteReg16 (0x338C, 0xA119);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0004);     // SEQ_LLSAT2
  //}}}
  //{{{  SEQ_LLINTERPTHRESHx
  I2C_WriteReg16 (0x338C, 0xA11A);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x000A);     // SEQ_LLINTERPTHRESH1

  I2C_WriteReg16 (0x338C, 0xA11B);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0020);     // SEQ_LLINTERPTHRESH2
  //}}}
  //{{{  SEQ_NR_THx
  I2C_WriteReg16 (0x338C, 0xA13E);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0004);     // SEQ_NR_TH1_R

  I2C_WriteReg16 (0x338C, 0xA13F);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x000E);     // SEQ_NR_TH1_G

  I2C_WriteReg16 (0x338C, 0xA140);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0004);     // SEQ_NR_TH1_B

  I2C_WriteReg16 (0x338C, 0xA141);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0004);     // SEQ_NR_TH1_OL

  I2C_WriteReg16 (0x338C, 0xA142);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0032);     // SEQ_NR_TH2_R

  I2C_WriteReg16 (0x338C, 0xA143);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x000F);     // SEQ_NR_TH2_G

  I2C_WriteReg16 (0x338C, 0xA144);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0032);     // SEQ_NR_TH2_B

  I2C_WriteReg16 (0x338C, 0xA145);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0032);     // SEQ_NR_TH2_OL
  //}}}
  //{{{  SEQ_NR_GAINTHx
  I2C_WriteReg16 (0x338C, 0xA146);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0005);     // SEQ_NR_GAINTH1

  I2C_WriteReg16 (0x338C, 0xA147);     // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x003A);     // SEQ_NR_GAINTH2
  //}}}
  //{{{  R9
  I2C_WriteReg16 (0x338C, 0x222E); // R9 Step
  I2C_WriteReg16 (0x3390, 0x0090); // = 144

  I2C_WriteReg16 (0x338C, 0xA408); // search_f1_50
  I2C_WriteReg16 (0x3390, 0x001A); // = 26

  I2C_WriteReg16 (0x338C, 0xA409); // search_f2_50
  I2C_WriteReg16 (0x3390, 0x001D); // = 29


  I2C_WriteReg16 (0x338C, 0xA40A); // search_f1_60
  I2C_WriteReg16 (0x3390, 0x0020); // = 32

  I2C_WriteReg16 (0x338C, 0xA40B); // search_f2_60
  I2C_WriteReg16 (0x3390, 0x0023); // = 35


  I2C_WriteReg16 (0x338C, 0x2411); // R9_Step_60_A
  I2C_WriteReg16 (0x3390, 0x0090); // = 144

  I2C_WriteReg16 (0x338C, 0x2413); // R9_Step_50_A
  I2C_WriteReg16 (0x3390, 0x00AD); // = 173


  I2C_WriteReg16 (0x338C, 0x2415); // R9_Step_60_B
  I2C_WriteReg16 (0x3390, 0x0055); // = 85

  I2C_WriteReg16 (0x338C, 0x2417); // R9_Step_50_B
  I2C_WriteReg16 (0x3390, 0x0066); // = 102
  //}}}
  //{{{  AE sensitivity  = 40
  I2C_WriteReg16 (0x338c, 0xa207);
  I2C_WriteReg16 (0x3390, 0x0040);
  //}}}
  //{{{  Stat_min  = 2
  I2C_WriteReg16 (0x338C, 0xA40D);
  I2C_WriteReg16 (0x3390, 0x0002);
  //}}}
  //{{{  Min_amplitude = 1
  I2C_WriteReg16 (0x338C, 0xA410);
  I2C_WriteReg16 (0x3390, 0x0001);
  //}}}

  //{{{  Refresh Sequencer Mode = 6
  I2C_WriteReg16 (0x338C, 0xA103);
  I2C_WriteReg16 (0x3390, 0x0006);
  delayMs (100);
  //}}}
  //{{{  Refresh Sequencer = 5
  I2C_WriteReg16 (0x338C, 0xA103);
  I2C_WriteReg16 (0x3390, 0x0005);
  delayMs (100);
  //}}}

  I2C_WriteReg16 (0x33f4, 0x031d); // defect - undocumented
  //{{{  saturation
  I2C_WriteReg16 (0x338c, 0xa118);
  I2C_WriteReg16 (0x3390, 0x0026);
  //}}}

  //{{{  SEQ_CAP_MODE = 0
  I2C_WriteReg16 (0x338C, 0xA120); // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0000); // SEQ_CAP_MODE
  //}}}
  //{{{  SEQ_CM = 1
  I2C_WriteReg16 (0x338C, 0xA103); // MCU_ADDRESS
  I2C_WriteReg16 (0x3390, 0x0001); // SEQ_CM
  delayMs (100);
  //}}}
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

  // transfer half completed, setup next buffer in cycle of 3
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

//{{{
uint32_t cameraId() {
  return 0xD112;
  }
//}}}
//{{{
uint32_t cameraFrame() {
  return frame;
  }
//}}}

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

  lcdSpiDisplay1 (FRAME_BUFFER,
                  (PIC_WIDTH - getWidth())/2, (PIC_HEIGHT - getHeight())/2,
                  PIC_WIDTH);
  }
//}}}

//{{{
bool initCamera (bool continuous) {

  initGPIO();
  initMT9D112();
  initDCMI (continuous);

  return true;
  }
//}}}
