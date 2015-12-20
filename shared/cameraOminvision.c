// cameraOmnivision.c
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
#include "stm32f4xx_ltdc.h"
//}}}
//{{{  camera common defines
#define OV7670_DEVICE_ADDRESS  0x42
#define OV9655_DEVICE_ADDRESS  0x60

#define REG_PID   0x0a  // Product ID MSB
#define REG_VER   0x0b  // Product ID LSB

#define REG_MIDH  0x1c  // Manuf. ID high
#define REG_MIDL  0x1d  // Manuf. ID low
//}}}

#define FRAME_BUFFER  SDRAM

#define QVGA

#ifdef QVGA
  #define PIC_WIDTH     320
  #define PIC_HEIGHT    240
#else
  #define PIC_WIDTH     640
  #define PIC_HEIGHT    480
#endif

#define NUM_BUFFERS     (((PIC_WIDTH*PIC_HEIGHT*2/4)/0x10000) + 1)
#define BUFFER_SIZE     (PIC_WIDTH*PIC_HEIGHT*2/NUM_BUFFERS)

static uint8_t deviceAddress;
static uint16_t manf_id = 0xFAFF;
static uint16_t prod_id = 0xFAFF;

volatile static uint32_t DelayCount;
volatile static uint32_t frame = 0;
volatile static uint32_t frameDone = false;
volatile static uint32_t framePhase = 0;

//{{{
static uint8_t I2C_ReadReg (uint8_t reg) {

  DelayCount = 0x4000;

  // Send I2C1 START condition
  I2C_GenerateSTART (I2C2, ENABLE);

  // wait for I2C1 EV5 --> Slave has acknowledged start condition
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_MODE_SELECT))
    if ((DelayCount--) == 0)
      return 0xFF;

  // Send slave Address for write
  I2C_Send7bitAddress (I2C2, deviceAddress, I2C_Direction_Transmitter);

  // wait for I2C1 EV6
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    if ((DelayCount--) == 0)
      return 0xFF;

  I2C_SendData (I2C2, reg);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    if ((DelayCount--) == 0)
      return 0xFF;

  I2C_GenerateSTOP (I2C2, ENABLE);
  for (int i = 0xBFF; i > 0; i--)
    DelayCount++;

  // wait until I2C1 is not busy anymore
  while (I2C_GetFlagStatus (I2C2, I2C_FLAG_BUSY));
    if ((DelayCount--) == 0)
      return 0xFF;

  // Send I2C1 START condition
  I2C_GenerateSTART (I2C2, ENABLE);

  // wait for I2C1 EV5 --> Slave has acknowledged start condition
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_MODE_SELECT))
    if ((DelayCount--) == 0)
      return 0xFF;

  // Send slave Address for write
  I2C_Send7bitAddress (I2C2, deviceAddress, I2C_Direction_Receiver);

  // wait for I2C1 EV6
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
    if ((DelayCount--) == 0)
      return 0xFF;

  // disable acknowledge of received data
  I2C_AcknowledgeConfig (I2C2, DISABLE);

  // wait until one byte has been received
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED))
    if ((DelayCount--) == 0)
      return 0xFF;

  uint8_t data = I2C_ReceiveData (I2C2);

  I2C_GenerateSTOP (I2C2, ENABLE);

  while (DelayCount--);

  return data;
  }
//}}}
//{{{
static uint8_t I2C_WriteReg (uint8_t reg, uint8_t data) {

  DelayCount = 0x4000;

  // Send I2C1 START condition
  I2C_GenerateSTART (I2C2, ENABLE);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_MODE_SELECT))
    if ((DelayCount--) == 0)
      return false;

  // Send slave Address for write
  I2C_Send7bitAddress (I2C2, deviceAddress, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    if ((DelayCount--) == 0)
      return false;

  I2C_SendData (I2C2, reg);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    if ((DelayCount--) == 0)
      return false;

  I2C_SendData (I2C2, data);
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    if ((DelayCount--) == 0)
      return false;

  I2C_GenerateSTOP (I2C2, ENABLE);
  return true;
  }
//}}}

//{{{
static void initTimerClock() {
// PB00 - tim3 clock -> dcmi xclk

  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_APB1PeriphClockCmd (RCC_APB1Periph_TIM3, ENABLE);

  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_25MHz;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init (GPIOB, &GPIO_InitStruct);

  GPIO_PinAFConfig (GPIOB, GPIO_PinSource0, GPIO_AF_TIM3);

  // TIM3 Time base configuration
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_TimeBaseStructure.TIM_Period = 3;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit (TIM3, &TIM_TimeBaseStructure);

  // TIM3 PWM1 Mode configuration: Channel3
  TIM_OCInitTypeDef TIM_OCInitStructure;
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = 2; // TIM_TimeBaseStructure.TIM_Period/2;
  TIM_OC3Init (TIM3, &TIM_OCInitStructure);

  TIM_OC3PreloadConfig (TIM3, TIM_OCPreload_Enable);
  TIM_ARRPreloadConfig (TIM3, ENABLE);
  TIM_Cmd (TIM3, ENABLE);
  }
//}}}
//{{{
static void initGPIO() {
// PB00 - TIM3 -> dcmi xclk
// PB04 - PWRDWN
// PB10 - i2c2 scl
// PB11 - i2c2 sda
// PA06 - pixclk
// PA04 - hs
// PB07 - vs
// PC06 - d0
// PC07 - d1
// PG10 - d2
// PG11 - d3
// PE04 - d4
// PD03 - d5
// PB08 - d6
// PB09 - d7

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

  // config I2C2 AF SCL, SDA
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
  I2C_InitStruct.I2C_ClockSpeed = 100000;  // 100kHz
  I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStruct.I2C_OwnAddress1 = 0xFE;
  I2C_InitStruct.I2C_Ack = I2C_Ack_Disable;
  I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_Init (I2C2, &I2C_InitStruct);
  I2C_Cmd (I2C2, ENABLE);
  //I2C_AcknowledgeConfig (I2C2, ENABLE);

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
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
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
static void initOV7670() {
//#define OV7670_BAVE  0x05  // U/B Average level
//#define OV7670_AECHH 0x07  // AEC MS 5 bits
//#define REG_RAVE     0x08  // V/R Average level
//#define OV7670_PSHFT 0x1b  // Pixel delay after HREF
//#define OV7670_HSYST 0x30  // HSYNC rising edge delay
//#define OV7670_HSYEN 0x31  // HSYNC falling edge delay
//#define OV7670_COM17 0x42  // Control 17
//  #define  COM17_AECWIN  0xc0    // AEC window - must match COM4
//  #define  COM17_CBAR    0x08    // DSP Color bar

  //{{{  COM7 reset
  #define OV7670_COM7  0x12
    #define   COM7_RESET     0x80    // b7 = regs reset

  I2C_WriteReg (OV7670_COM7, COM7_RESET);

  delayMs (1);
  //}}}
  //{{{  COM2 - softSleep, output drive
  #define OV7670_COM2  0x09  // Control 2

    #define COM2_SSLEEP    0x10  // Soft sleep mode
  //}}}

  //{{{  TSLB - line buffer test, uv swap
  #define OV7670_TSLB  0x3a  // lots of stuff
    #define  TSLB_YLAST    0x04    // UYVY or VYUY - see com13

  I2C_WriteReg (OV7670_TSLB,  0x04);
  //}}}
  //{{{  HSTART,HSTOP,HREF - VSTART,VSTOP,VREF
  #define hstart  172
  #define hstop    28
  #define vstart   12
  #define vstop   492

  // VGA hstart = 158, hstop =  14, vstart =  10, vstop = 490

  #define OV7670_HSTART 0x17  // Horiz start high bits
  #define OV7670_HSTOP  0x18  // Horiz stop high bits

  #define OV7670_VSTOP  0x1a  // Vert stop high bits
  #define OV7670_VSTART 0x19  // Vert start high bits

  #define OV7670_HREF  0x32  // HREF pieces
  #define OV7670_VREF  0x03  // Pieces of GAIN, VSTART, VSTOP

  // Horizontal 11 bits, top 8 hstart and hstop
  // - bottom 3 of hstart are in href[2:0],
  // - bottom 3 of hstop in href[5:3]
  // mystery "edge offset" value in the top two bits of href.
  I2C_WriteReg (OV7670_HSTART, (hstart >> 3) & 0xff);
  I2C_WriteReg (OV7670_HSTOP,  (hstop  >> 3) & 0xff);
  I2C_WriteReg (OV7670_HREF, 0x80 | ((hstop & 0x7) << 3) | (hstart & 0x7));

  // Vertical 10 bits.
  I2C_WriteReg (OV7670_VSTART, (vstart >> 2) & 0xff);
  I2C_WriteReg (OV7670_VSTOP,  (vstop  >> 2) & 0xff);
  I2C_WriteReg (OV7670_VREF,  ((vstop & 0x3) << 2) | (vstart & 0x3));
  //}}}
  //{{{  RGB444, COM1, COM7 COM15  - format, colourSpace
  // rgb565
  #define OV7670_RGB444  0x8c  // RGB 444 control
    #define  R444_ENABLE   0x02   // Turn on RGB444, overrides 5x5
    #define  R444_RGBX     0x01   // Empty nibble at end

  #define OV7670_COM1  0x04  // Control 1
    #define COM1_CCIR656   0x40  // CCIR656 enable

  // more COM7 flags
    #define   COM7_FMT_MASK  0x38    // b5,b4,b3
    #define   COM7_FMT_CIF   0x20    // CIF format
    #define   COM7_FMT_QVGA  0x10    // QVGA format
    #define   COM7_FMT_QCIF  0x08    // QCIF format
    #define   COM7_FMT_VGA   0x00    // VGA format
    #define   COM7_BARS      0x02    // b1 - colour bars
    #define   COM7_PBAYER    0x05    // b2,b0 - Processed bayer
    #define   COM7_RGB       0x04    // RGB format
    #define   COM7_BAYER     0x01    // Bayer format
    #define   COM7_YUV       0x00    // YUV

  #define OV7670_COM15 0x40  // Control 15
    #define  COM15_R10F0   0x00    // Data range 10 to F0
    #define  COM15_R01FE   0x80    //            01 to FE
    #define  COM15_R00FF   0xc0    //            00 to FF
    #define  COM15_RGB565  0x10    // RGB565 output
    #define  COM15_RGB555  0x30    // RGB555 output


  I2C_WriteReg (OV7670_RGB444, 0);            // No RGB444
  I2C_WriteReg (OV7670_COM1,   0x0);          // CCIR601

  #ifdef QVGA
  I2C_WriteReg (OV7670_COM7,   COM7_FMT_QVGA | COM7_RGB);
  #else
  I2C_WriteReg (OV7670_COM7,   COM7_FMT_VGA | COM7_RGB);
  #endif

  I2C_WriteReg (OV7670_COM15,  COM15_RGB565 | COM15_R00FF);
  //}}}
  //{{{  CLKRC - frame rate
  #define OV7670_CLKRC 0x11  // Clock control
    #define CLK_EXT   0x40      // Use external clock directly
    #define CLK_SCALE 0x3f      // Mask for internal clock scale
    #define CLOCK_SCALE_15fps 3 // 15fps
    #define CLOCK_SCALE_20fps 2 // 20fps
    #define CLOCK_SCALE_30fps 1 // 30fps

  #ifdef QVGA
    I2C_WriteReg (OV7670_CLKRC, CLOCK_SCALE_30fps);
  #else
    I2C_WriteReg (OV7670_CLKRC, CLOCK_SCALE_15fps);
  #endif
  //}}}

  //{{{  COM9 - gain ceiling
  #define OV7670_COM9  0x14

  I2C_WriteReg (OV7670_COM9,  0x38); // 16x gain ceiling, 0x8 is reserved bit
  //}}}
  //{{{  matrix
  //This matrix defines how the colors are generated, must be tweaked to adjust hue and saturation.
  // Order: v-red, v-green, v-blue, u-red, u-green, u-blue
  // nine-bit signed, sign bit in 0x58.  Sign for v-red is bit 0, and up from there.
  #define OV7670_CMATRIX_BASE 0x4f
  #define OV7670_CMATRIX_SIGN 0x58

  // matrix = {179,-179,0,-61,-176,228 },
  I2C_WriteReg (OV7670_CMATRIX_BASE,   179);  // unsigned coeffs
  I2C_WriteReg (OV7670_CMATRIX_BASE+1, 179);
  I2C_WriteReg (OV7670_CMATRIX_BASE+2, 0);
  I2C_WriteReg (OV7670_CMATRIX_BASE+3, 61);
  I2C_WriteReg (OV7670_CMATRIX_BASE+4, 176);
  I2C_WriteReg (OV7670_CMATRIX_BASE+5, 228);
  I2C_WriteReg (OV7670_CMATRIX_SIGN,   0x9a); // lo 6 bits coeff sign bits + hi 2 bits crap
  //}}}

  //{{{  COM3 - tristate, scale enable, DCW enable
  #define OV7670_COM3  0x0c
    #define COM3_SWAP    0x40    // Byte swap
    #define COM3_SCALEEN 0x08    // Enable scaling
    #define COM3_DCWEN   0x04    // Enable downsamp / crop / window

  I2C_WriteReg (OV7670_COM3, 0);
  //}}}
  //{{{  COM14 - dcw and scaling pclk
  #define OV7670_COM14 0x3e  // Control 14
    #define  COM14_DCWEN   0x10    // DCW/PCLK-scale enable

  I2C_WriteReg (OV7670_COM14, 0);
  //}}}

  //{{{  70-73,a2 - scaling
  I2C_WriteReg (0x70, 0x3a);
  I2C_WriteReg (0x71, 0x35);
  I2C_WriteReg (0x72, 0x11);
  I2C_WriteReg (0x73, 0xf0);
  I2C_WriteReg (0xa2, 0x02);
  //}}}
  //{{{  COM10 hsync, vref, pclk control
  #define OV7670_COM10 0x15  // Control 10
    #define  COM10_HSYNC     0x40    // HSYNC instead of HREF
    #define  COM10_PCLK_HB   0x20    // Suppress PCLK on horiz blank
    #define  COM10_HREF_REV  0x08    // Reverse HREF
    #define  COM10_VS_LEAD   0x04    // VSYNC on clock leading edge
    #define  COM10_VS_NEG    0x02    // VSYNC negative
    #define  COM10_HS_NEG    0x01    // HSYNC negative

  I2C_WriteReg (OV7670_COM10, 0x0);
  //}}}

  //{{{  COM13 - gamma enable, uv swap
  #define OV7670_COM13 0x3d
    #define  COM13_GAMMA   0x80  // Gamma enable
    #define  COM13_UVSAT   0x40  // UV saturation auto adjustment
    #define  COM13_UVSWAP  0x01  // V before U - w/TSLB

  I2C_WriteReg (OV7670_COM13, COM13_GAMMA | COM13_UVSAT);
  //}}}
  //{{{  7a-89 - Gamma curve values
  I2C_WriteReg (0x7a, 0x20);  // SLOP
  I2C_WriteReg (0x7b, 0x10);  // GAM1
  I2C_WriteReg (0x7c, 0x1e);  // GAM2
  I2C_WriteReg (0x7d, 0x35);  // GAM3
  I2C_WriteReg (0x7e, 0x5a);  // GAM4
  I2C_WriteReg (0x7f, 0x69);  // GAM5
  I2C_WriteReg (0x80, 0x76);  // GAM6
  I2C_WriteReg (0x81, 0x80);  // GAM7
  I2C_WriteReg (0x82, 0x88);  // GAM8
  I2C_WriteReg (0x83, 0x8f);  // GAM9
  I2C_WriteReg (0x84, 0x96);  // GAM10
  I2C_WriteReg (0x85, 0xa3);  // GAM11
  I2C_WriteReg (0x86, 0xaf);  // GAM12
  I2C_WriteReg (0x87, 0xc4);  // GAM13
  I2C_WriteReg (0x88, 0xd7);  // GAM14
  I2C_WriteReg (0x89, 0xe8);  // GAM15
  //}}}

  //{{{  aec/agc
  #define OV7670_GAIN  0x00  // Gain lower 8 bits (rest in vref)
  #define OV7670_AECH  0x10  // More bits of AEC value
  #define OV7670_COM4  0x0d  // Control 4

  #define OV7670_AEW   0x24  // AGC upper limit
  #define OV7670_AEB   0x25  // AGC lower limit
  #define OV7670_VPT   0x26  // AGC/AEC fast mode op region

  #define OV7670_HAECC1  0x9f  // Hist AEC/AGC control 1
  #define OV7670_HAECC2  0xa0  // Hist AEC/AGC control 2
  #define OV7670_BD50MAX 0xa5  // 50hz banding step limit
  #define OV7670_HAECC3  0xa6  // Hist AEC/AGC control 3
  #define OV7670_HAECC4  0xa7  // Hist AEC/AGC control 4
  #define OV7670_HAECC5  0xa8  // Hist AEC/AGC control 5
  #define OV7670_HAECC6  0xa9  // Hist AEC/AGC control 6
  #define OV7670_HAECC7  0xaa  // Hist AEC/AGC control 7
  #define OV7670_BD60MAX 0xab  // 60hz banding step limit

  // AGC and AEC parameters, disable features, tweak values, enable features
  #define OV7670_COM8  0x13  // Control 8
    #define  COM8_FASTAEC   0x80    // Enable fast AGC/AEC
    #define  COM8_AECSTEP   0x40    // Unlimited AEC step size
    #define  COM8_BFILT     0x20    // Band filter enable
    #define  COM8_AGC       0x04    // Auto gain enable
    #define  COM8_AWB       0x02    // White balance enable
    #define  COM8_AEC       0x01    // Auto exposure enable


  I2C_WriteReg (OV7670_COM8, COM8_FASTAEC | COM8_AECSTEP | COM8_BFILT);
  I2C_WriteReg (OV7670_GAIN, 0);
  I2C_WriteReg (OV7670_AECH, 0);
  I2C_WriteReg (OV7670_COM4, 0x40); /* magic reserved bit */
  I2C_WriteReg (OV7670_COM9, 0x18); /* 4x gain + magic rsvd bit */
  I2C_WriteReg (OV7670_BD50MAX, 0x05);
  I2C_WriteReg (OV7670_BD60MAX, 0x07);

  I2C_WriteReg (OV7670_AEW, 0x95);
  I2C_WriteReg (OV7670_AEB, 0x33);
  I2C_WriteReg (OV7670_VPT, 0xe3);

  I2C_WriteReg (OV7670_HAECC1, 0x78);
  I2C_WriteReg (OV7670_HAECC2, 0x68);
  I2C_WriteReg (0xa1, 0x03); /* magic */
  I2C_WriteReg (OV7670_HAECC3, 0xd8);
  I2C_WriteReg (OV7670_HAECC4, 0xd8);
  I2C_WriteReg (OV7670_HAECC5, 0xf0);
  I2C_WriteReg (OV7670_HAECC6, 0x90);
  I2C_WriteReg (OV7670_HAECC7, 0x94);

  I2C_WriteReg (OV7670_COM8, COM8_FASTAEC | COM8_AECSTEP | COM8_BFILT | COM8_AGC | COM8_AEC);
  //}}}
  //{{{  COM5 - reserved
  #define OV7670_COM5  0x0e  // All "reserved"

  I2C_WriteReg (OV7670_COM5, 0x61);
  //}}}
  //{{{  COM6 - optical black row
  #define OV7670_COM6  0x0f

  I2C_WriteReg (OV7670_COM6, 0x4b);
  //}}}
  I2C_WriteReg (0x16, 0x02);
  //{{{  MVFP - hv flip, black sun
  #define OV7670_MVFP  0x1e  // Mirror / vflip
    #define  MVFP_MIRROR  0x20   // Mirror image
    #define  MVFP_FLIP    0x10    // Vertical flip

  I2C_WriteReg (OV7670_MVFP, 0x07); // no flip, black sun enable
  //}}}
  //{{{  21,22,29,33,35,37-39
  I2C_WriteReg (0x21, 0x02);
  I2C_WriteReg (0x22, 0x91);
  I2C_WriteReg (0x29, 0x07);
  I2C_WriteReg (0x33, 0x0b);
  I2C_WriteReg (0x35, 0x0b);
  I2C_WriteReg (0x37, 0x1d);
  I2C_WriteReg (0x38, 0x71);
  I2C_WriteReg (0x39, 0x2a);
  //}}}
  //{{{  COM12
  #define OV7670_COM12 0x3c  // Control 12
    #define  COM12_HREF    0x80    // HREF always

  I2C_WriteReg (OV7670_COM12, 0x78);
  //}}}
  I2C_WriteReg (0x4d, 0x40);
  I2C_WriteReg (0x4e, 0x20);
  //{{{  GFIX
  #define OV7670_GFIX  0x69  // Fix gain control

  I2C_WriteReg (OV7670_GFIX, 0);
  //}}}
  //{{{  DBLV - PLL control, regulator control
  #define OV7670_DBLV  0x6b  // PLL control and debugging
    #define  DBLV_BYPASS 0x00    // Bypass PLL
    #define  DBLV_X4     0x01    // clock x4
    #define  DBLV_X6     0x10    // clock x6
    #define  DBLV_X8     0x11    // clock x8

  I2C_WriteReg (OV7670_DBLV, 0x4a); // bypass PLL, bypass internal regulator, clock divider = 10
  //}}}
  //{{{  74,8d-91,96.9a,b0-b3,b8
  I2C_WriteReg (0x74, 0x10); // REG74 - digital gain control

  I2C_WriteReg (0x8d, 0x4f); // reserved
  I2C_WriteReg (0x8e, 0);    // reserved
  I2C_WriteReg (0x8f, 0);    // reserved
  I2C_WriteReg (0x90, 0);    // reserved
  I2C_WriteReg (0x91, 0);    // reserved
  I2C_WriteReg (0x96, 0);    // reserved
  I2C_WriteReg (0x9a, 0);    // reserved
  I2C_WriteReg (0xb0, 0x84); // reserved

  I2C_WriteReg (0xb1, 0x0c); // ABLC1 - ABLC enable
  I2C_WriteReg (0xb2, 0x0e); // reserved
  I2C_WriteReg (0xb3, 0x82); // THL_DLT - ALBC target

  I2C_WriteReg (0xb8, 0x0a); // reserved
  //}}}
  //{{{  AWB - 43-48,59-5e,6c-6f,6a
  #define OV7670_BLUE  0x01  // blue gain
  #define OV7670_RED   0x02  // red gain

  #define OV7670_COM16 0x41  // Control 16
    #define  COM16_AWBGAIN   0x08    // AWB gain enable

  // More reserved magic, some of which tweaks white balance
  I2C_WriteReg (0x43, 0x0a);
  I2C_WriteReg (0x44, 0xf0);
  I2C_WriteReg (0x45, 0x34);
  I2C_WriteReg (0x46, 0x58);
  I2C_WriteReg (0x47, 0x28);
  I2C_WriteReg (0x48, 0x3a);
  I2C_WriteReg (0x59, 0x88);
  I2C_WriteReg (0x5a, 0x88);
  I2C_WriteReg (0x5b, 0x44);
  I2C_WriteReg (0x5c, 0x67);
  I2C_WriteReg (0x5d, 0x49);
  I2C_WriteReg (0x5e, 0x0e);
  I2C_WriteReg (0x6c, 0x0a);
  I2C_WriteReg (0x6d, 0x55);
  I2C_WriteReg (0x6e, 0x11);
  I2C_WriteReg (0x6f, 0x9f); /* "9e for advance AWB" */
  I2C_WriteReg (0x6a, 0x40);

  I2C_WriteReg (OV7670_BLUE, 0x40);
  I2C_WriteReg (OV7670_RED, 0x60);
  I2C_WriteReg (OV7670_COM8, COM8_FASTAEC | COM8_AECSTEP | COM8_BFILT | COM8_AGC | COM8_AEC | COM8_AWB);

  I2C_WriteReg (OV7670_COM16, COM16_AWBGAIN);
  //}}}
  //{{{  edge enhancement
  #define OV7670_EDGE  0x3f  // Edge enhancement factor

  #define OV7670_REG76 0x76  // OV's name */
    #define  R76_BLKPCOR   0x80    // Black pixel correction enable
    #define  R76_WHTPCOR   0x40    // White pixel correction enable

  I2C_WriteReg (OV7670_EDGE, 0);
  I2C_WriteReg (0x75, 0x05);         // REG75 - edge enhance lower limit
  I2C_WriteReg (OV7670_REG76, 0xe1); // REG76 - black white picel correction, edge enhance upper limit
  //}}}

  I2C_WriteReg (0x4c, 0);    // DNSTH - denoise strength
  I2C_WriteReg (0x77, 0x01); // REG77 - offset denoise range control
  //{{{  more COM13
  I2C_WriteReg (OV7670_COM13, 0xc3);
  //}}}

  I2C_WriteReg (0x4b, 0x09); // REG4B - uv average enable
  I2C_WriteReg (0xc9, 0x60); // SATCTR - UV saturation control
  //{{{  more COM16
  I2C_WriteReg (OV7670_COM16, 0x38);
  //}}}

  //{{{  BRIGHT
  #define OV7670_BRIGHT  0x55  // Brightness

  I2C_WriteReg (OV7670_BRIGHT, 0x00);
  //}}}
  //{{{  CONTRAS - contrast
  #define OV7670_CONTRAS 0x56  // Contrast control

  I2C_WriteReg (OV7670_CONTRAS, 0x50);
  //}}}
  I2C_WriteReg (0x34, 0x11); // ARBLM - array reference control
  //{{{  COM11 - night Mode, flicker detection
  #define OV7670_COM11 0x3b  // Control 11
    #define  COM11_NIGHT   0x80  // NIght mode enable
    #define  COM11_NMFR    0x60  // Two bit NM frame rate
    #define  COM11_HZAUTO  0x10  // Auto detect 50/60 Hz
    #define  COM11_50HZ    0x08  // Manual 50Hz select
    #define  COM11_EXP     0x02

  I2C_WriteReg (OV7670_COM11, COM11_EXP | COM11_HZAUTO);
  //}}}

  //{{{  a4,96-9e,78
  I2C_WriteReg (0xa4, 0x88);
  I2C_WriteReg (0x96, 0);
  I2C_WriteReg (0x97, 0x30);
  I2C_WriteReg (0x98, 0x20);
  I2C_WriteReg (0x99, 0x30);
  I2C_WriteReg (0x9a, 0x84);
  I2C_WriteReg (0x9b, 0x29);
  I2C_WriteReg (0x9c, 0x03);
  I2C_WriteReg (0x9d, 0x4c);
  I2C_WriteReg (0x9e, 0x3f);
  I2C_WriteReg (0x78, 0x04);
  //}}}
  //{{{  79,C8 - extra-weird stuff
  I2C_WriteReg (0x79, 0x01);
  I2C_WriteReg (0xc8, 0xf0);
  I2C_WriteReg (0x79, 0x0f);
  I2C_WriteReg (0xc8, 0x00);
  I2C_WriteReg (0x79, 0x10);
  I2C_WriteReg (0xc8, 0x7e);
  I2C_WriteReg (0x79, 0x0a);
  I2C_WriteReg (0xc8, 0x80);
  I2C_WriteReg (0x79, 0x0b);
  I2C_WriteReg (0xc8, 0x01);
  I2C_WriteReg (0x79, 0x0c);
  I2C_WriteReg (0xc8, 0x0f);
  I2C_WriteReg (0x79, 0x0d);
  I2C_WriteReg (0xc8, 0x20);
  I2C_WriteReg (0x79, 0x09);
  I2C_WriteReg (0xc8, 0x80);
  I2C_WriteReg (0x79, 0x02);
  I2C_WriteReg (0xc8, 0xc0);
  I2C_WriteReg (0x79, 0x03);
  I2C_WriteReg (0xc8, 0x40);
  I2C_WriteReg (0x79, 0x05);
  I2C_WriteReg (0xc8, 0x30);
  I2C_WriteReg (0x79, 0x26);
  //}}}
  }
//}}}
//{{{
static void initOV9655() {

#define NUM_FMT_REGS 14
// COM7  COM3  COM4  HSTA  HSTOP HREF  VSTAR VSTOP VREF  EXHCH EXHCL ADC,  OCOM, OFON
// 0x12, 0x0c, 0x0d, 0x17, 0x18, 0x32, 0x19, 0x1a, 0x03, 0x2a, 0x2b, 0x37, 0x38, 0x39,
// 0x10, 0x04, 0x80, 0x25, 0xc5, 0xbf, 0x00, 0x80, 0x12, 0x10, 0x40, 0x91, 0x12, 0x43,  // qvga
// 0x40, 0x04, 0x80, 0x26, 0xc6, 0xed, 0x01, 0x3d, 0x00, 0x10, 0x40, 0x91, 0x12, 0x43,  // vga
// 0x00, 0x00, 0x00, 0x1e, 0xbe, 0xbf, 0x01, 0x81, 0x12, 0x10, 0x34, 0x81, 0x93, 0x51,  // sxga

#define OV9655_COM7       0x12
  #define OV9655_SCCB_REG_RESET                     0x80
  #define OV9655_FORMAT_CTRL_15fpsVGA               0x00
  #define OV9655_FORMAT_CTRL_30fpsVGA_NoVArioPixel  0x50
  #define OV9655_FORMAT_CTRL_30fpsVGA_VArioPixel    0x60
  #define OV9655_OUTPUT_FORMAT_RAWRGB               0x00
  #define OV9655_OUTPUT_FORMAT_RAWRGB_DATA          0x00
  #define OV9655_OUTPUT_FORMAT_RAWRGB_INTERP        0x01
  #define OV9655_OUTPUT_FORMAT_YUV                  0x02
  #define OV9655_OUTPUT_FORMAT_RGB                  0x03

#define OV9655_HSTART     0x17
#define OV9655_HSTOP      0x18
#define OV9655_HREF       0x32

#define OV9655_VSTART     0x19
#define OV9655_VSTOP      0x1A
#define OV9655_VREF       0x03

  I2C_WriteReg (OV9655_COM7, OV9655_SCCB_REG_RESET);
  delayMs (1);

  I2C_WriteReg (OV9655_HSTART, 0x18);     // hstart 10:3
  I2C_WriteReg (OV9655_HSTOP,  0x04);     // hstop  10:3
  I2C_WriteReg (OV9655_HREF,   0x12);     // 5:3 = hstop 2:0, 2:0 = hstart 2:0

  I2C_WriteReg (OV9655_VSTART, 0x01);     // vstart 10:3
  I2C_WriteReg (OV9655_VSTOP,  0x80);     // vstop  10:3
  I2C_WriteReg (OV9655_VREF,   0x02);     // 5:3 = vstop 2:0, 2:0 = vstart 2:0

  I2C_WriteReg (OV9655_COM7, OV9655_FORMAT_CTRL_30fpsVGA_VArioPixel | OV9655_OUTPUT_FORMAT_RGB);

  //{{{  0x0x
  #define OV9655_GAIN       0x00
  #define OV9655_BLUE       0x01
  #define OV9655_RED        0x02

  #define OV9655_COM1       0x04
    #define OV9655_CCIR656_FORMAT  0x40
    #define OV9655_HREF_SKIP_0     0x00
    #define OV9655_HREF_SKIP_1     0x04
    #define OV9655_HREF_SKIP_3     0x08

  #define OV9655_BAVE       0x05
  #define OV9655_GbAVE      0x06
  #define OV9655_GrAVE      0x07
  #define OV9655_RAVE       0x08

  #define OV9655_COM2       0x09
    #define OV9655_SOFT_SLEEP_MODE  0x10
    #define OV9655_ODCAP_1x         0x00
    #define OV9655_ODCAP_2x         0x01
    #define OV9655_ODCAP_3x         0x02
    #define OV9655_ODCAP_4x         0x03

  #define OV9655_COM3       0x0C
    #define OV9655_COLOR_BAR_OUTPUT         0x80
    #define OV9655_OUTPUT_MSB_LAS_SWAP      0x40
    #define OV9655_PIN_REMAP_RESETB_EXPST   0x08
    #define OV9655_RGB565_FORMAT            0x00
    #define OV9655_RGB_OUTPUT_AVERAGE       0x04
    #define OV9655_SINGLE_FRAME             0x01

  #define OV9655_COM4       0x0D

  #define OV9655_COM5       0x0E
    #define OV9655_SLAM_MODE_ENABLE      0x40
    #define OV9655_EXPOSURE_NORMAL_MODE  0x01

  #define OV9655_COM6       0x0F

  I2C_WriteReg (OV9655_GAIN, 0x00);
  I2C_WriteReg (OV9655_BLUE, 0x80);
  I2C_WriteReg (OV9655_RED,  0x80);

  I2C_WriteReg (OV9655_COM1, 0x00);
  I2C_WriteReg (OV9655_COM2, 0x03);
  I2C_WriteReg (OV9655_COM5, OV9655_EXPOSURE_NORMAL_MODE);
  I2C_WriteReg (OV9655_COM6, 0x40); // BLC input optical black
  //}}}
  //{{{  0x1x
  #define OV9655_AEC        0x10
  #define OV9655_CLKRC      0x11
  #define OV9655_COM8       0x13

  #define OV9655_COM9       0x14
    #define OV9655_GAIN_2x         0x00
    #define OV9655_GAIN_4x         0x10
    #define OV9655_GAIN_8x         0x20
    #define OV9655_GAIN_16x        0x30
    #define OV9655_GAIN_32x        0x40
    #define OV9655_GAIN_64x        0x50
    #define OV9655_GAIN_128x       0x60
    #define OV9655_DROP_VSYNC      0x04
    #define OV9655_DROP_HREF       0x02

  #define OV9655_COM10      0x15
    #define OV9655_RESETb_REMAP_SLHS    0x80
    #define OV9655_HREF_CHANGE_HSYNC    0x40
    #define OV9655_PCLK_ON              0x00
    #define OV9655_PCLK_OFF             0x20
    #define OV9655_PCLK_POLARITY_REV    0x10
    #define OV9655_HREF_POLARITY_REV    0x08
    #define OV9655_RESET_ENDPOINT       0x04
    #define OV9655_VSYNC_NEG            0x02
    #define OV9655_HSYNC_NEG            0x01

  #define OV9655_REG16      0x16

  #define OV9655_PSHFT      0x1B
  #define OV9655_MVFP       0x1E


  I2C_WriteReg (OV9655_AEC,   0x50);
  I2C_WriteReg (OV9655_CLKRC, 0x80);
  I2C_WriteReg (OV9655_COM8,  0xef);
  I2C_WriteReg (OV9655_COM9,  0x3a);
  I2C_WriteReg (OV9655_COM10, 0);
  I2C_WriteReg (OV9655_REG16, 0x24);

  I2C_WriteReg (OV9655_MVFP,  0x00); /*0x20*/
  //}}}
  //{{{  0x2x
  #define OV9655_BOS        0x20
  #define OV9655_GBOS       0x21
  #define OV9655_GROS       0x22
  #define OV9655_ROS        0x23
  #define OV9655_AEW        0x24
  #define OV9655_AEB        0x25
  #define OV9655_VPT        0x26
  #define OV9655_BBIAS      0x27
  #define OV9655_GbBIAS     0x28
  #define OV9655_PREGAIN    0x29
  #define OV9655_EXHCH      0x2A
  #define OV9655_EXHCL      0x2B
  #define OV9655_RBIAS      0x2C
  #define OV9655_ADVFL      0x2D
  #define OV9655_ADVFH      0x2E
  #define OV9655_YAVE       0x2F


  I2C_WriteReg (OV9655_AEW,    0x3c);
  I2C_WriteReg (OV9655_AEB,    0x36);
  I2C_WriteReg (OV9655_VPT,    0x72);
  I2C_WriteReg (OV9655_BBIAS,  0x08);
  I2C_WriteReg (OV9655_GbBIAS, 0x08);
  I2C_WriteReg (OV9655_PREGAIN,0x15);
  I2C_WriteReg (OV9655_EXHCH,  0x00);
  I2C_WriteReg (OV9655_EXHCL,  0x00);
  I2C_WriteReg (OV9655_RBIAS,  0x08);
  //}}}
  //{{{  0x3x
  #define OV9655_HSYST      0x30
  #define OV9655_HSYEN      0x31
  #define OV9655_CHLF       0x33
  #define OV9655_AREF1      0x34
  #define OV9655_AREF2      0x35
  #define OV9655_AREF3      0x36
  #define OV9655_ADC1       0x37
  #define OV9655_ADC2       0x38
  #define OV9655_AREF4      0x39

  #define OV9655_TSLB       0x3A
    #define OV9655_PCLK_DELAY_0         0x00
    #define OV9655_PCLK_DELAY_2         0x40
    #define OV9655_PCLK_DELAY_4         0x80
    #define OV9655_PCLK_DELAY_6         0xC0
    #define OV9655_OUTPUT_BITWISE_REV   0x20
    #define OV9655_UV_NORMAL            0x00
    #define OV9655_UV_FIXED             0x10
    #define OV9655_YUV_SEQ_YUYV         0x00    // wrong bits
    #define OV9655_YUV_SEQ_YVYU         0x02
    #define OV9655_YUV_SEQ_VYUY         0x04
    #define OV9655_YUV_SEQ_UYVY         0x06
    #define OV9655_BANDING_FREQ_50      0x02

  #define OV9655_COM11      0x3B
  #define OV9655_COM12      0x3C
  #define OV9655_COM13      0x3D
  #define OV9655_COM14      0x3E
  #define OV9655_EDGE       0x3F


  I2C_WriteReg (OV9655_CHLF, 0x00);

  I2C_WriteReg (OV9655_AREF1, 0x3f);
  I2C_WriteReg (OV9655_AREF2, 0x00);
  I2C_WriteReg (OV9655_AREF3, 0x3a);
  I2C_WriteReg (OV9655_ADC2, 0x72);
  I2C_WriteReg (OV9655_AREF4, 0x57);

  I2C_WriteReg (OV9655_TSLB, 0xca);

  I2C_WriteReg (OV9655_COM11, 0x04);
  I2C_WriteReg (OV9655_COM13, 0x99);
  I2C_WriteReg (OV9655_COM14, 0x02);
  I2C_WriteReg (OV9655_EDGE, 0xc1);
  //}}}
  //{{{  0x4x,5x
  #define OV9655_COM15      0x40
    #define OV9655_RGB_555      0x30
    #define OV9655_RGB_565      0x10
    #define OV9655_RGB_NORMAL   0x00

  #define OV9655_COM16      0x41
  #define OV9655_COM17      0x42

  #define OV9655_MTX1       0x4F
  #define OV9655_MTX2       0x50
  #define OV9655_MTX3       0x51
  #define OV9655_MTX4       0x52
  #define OV9655_MTX5       0x53
  #define OV9655_MTX6       0x54
  #define OV9655_BRTN       0x55
  #define OV9655_CNST1      0x56
  #define OV9655_CNST2      0x57
  #define OV9655_MTXS       0x58

  #define OV9655_AWBOP1     0x59
  #define OV9655_AWBOP2     0x5A
  #define OV9655_AWBOP3     0x5B
  #define OV9655_AWBOP4     0x5C
  #define OV9655_AWBOP5     0x5D
  #define OV9655_AWBOP6     0x5E
  #define OV9655_BLMT       0x5F


  I2C_WriteReg (OV9655_COM15, 0xd0); // 5:4 = RGB565, 7:6= output range 00:FF, (3 = no RB swap ?)
  I2C_WriteReg (OV9655_COM16, 0x41); // 1 = color matrix coeff double
  I2C_WriteReg (OV9655_COM17, 0xc0); // 7 = de noise auto, 6 = edge enhance auto 4 = auto digital gain

  I2C_WriteReg (0x43, 0x0a);  // reserved
  I2C_WriteReg (0x44, 0xf0);  // reserved
  I2C_WriteReg (0x45, 0x46);  // reserved
  I2C_WriteReg (0x46, 0x62);  // reserved
  I2C_WriteReg (0x47, 0x2a);  // reserved
  I2C_WriteReg (0x48, 0x3c);  // reserved
  I2C_WriteReg (0x4a, 0xfc);  // reserved
  I2C_WriteReg (0x4b, 0xfc);  // reserved
  I2C_WriteReg (0x4c, 0x7f);  // reserved
  I2C_WriteReg (0x4d, 0x7f);  // reserved
  I2C_WriteReg (0x4e, 0x7f);  // reserved

  I2C_WriteReg (OV9655_MTX1, 0x98);
  I2C_WriteReg (OV9655_MTX2, 0x98);
  I2C_WriteReg (OV9655_MTX3, 0x00);
  I2C_WriteReg (OV9655_MTX4, 0x28);
  I2C_WriteReg (OV9655_MTX5, 0x70);
  I2C_WriteReg (OV9655_MTX6, 0x98);
  I2C_WriteReg (OV9655_MTXS, 0x1a);    // matrix sign bits

  I2C_WriteReg (OV9655_AWBOP1, 0x85);
  I2C_WriteReg (OV9655_AWBOP2, 0xa9);
  I2C_WriteReg (OV9655_AWBOP3, 0x64);
  I2C_WriteReg (OV9655_AWBOP4, 0x84);
  I2C_WriteReg (OV9655_AWBOP5, 0x53);
  I2C_WriteReg (OV9655_AWBOP6, 0x0e);
  I2C_WriteReg (OV9655_BLMT, 0xf0);
  //}}}
  //{{{  0x6x
  #define OV9655_RLMT       0x60          // reserved
  #define OV9655_GLMT       0x61          // reserved

  #define OV9655_LCC1       0x62
  #define OV9655_LCC2       0x63
  #define OV9655_LCC3       0x64
  #define OV9655_LCC4       0x65

  #define OV9655_MANU       0x66           // lcc5 ?
  #define OV9655_MANV       0x67           // manu ?
  #define OV9655_MANY       0x68           // manv ?
  #define OV9655_VARO       0x69
  #define OV9655_BD50MAX    0x6A
  #define OV9655_DBLV       0x6B


  I2C_WriteReg (OV9655_RLMT, 0xf0);
  I2C_WriteReg (OV9655_GLMT, 0xf0);

  I2C_WriteReg (OV9655_LCC1, 0x00);     // lens correction
  I2C_WriteReg (OV9655_LCC2, 0x00);
  I2C_WriteReg (OV9655_LCC3, 0x02);
  I2C_WriteReg (OV9655_LCC4, 0x20);

  I2C_WriteReg (OV9655_MANU, 0x00);
  I2C_WriteReg (OV9655_VARO, 0x0a);
  I2C_WriteReg (OV9655_DBLV, 0x0a);

  I2C_WriteReg (0x6c, 0x04);
  I2C_WriteReg (0x6d, 0x55);
  I2C_WriteReg (0x6e, 0x00);
  I2C_WriteReg (0x6f, 0x9d);
  //}}}
  //{{{  0x7x,8x
  #define OV9655_DNSTH      0x70
  #define OV9655_POIDX      0x72
  #define OV9655_PCKDV      0x73
  #define OV9655_XINDX      0x74
  #define OV9655_YINDX      0x75
  #define OV9655_SLOP       0x7A

  #define OV9655_GAM1       0x7B
  #define OV9655_GAM2       0x7C
  #define OV9655_GAM3       0x7D
  #define OV9655_GAM4       0x7E
  #define OV9655_GAM5       0x7F
  #define OV9655_GAM6       0x80
  #define OV9655_GAM7       0x81
  #define OV9655_GAM8       0x82
  #define OV9655_GAM9       0x83
  #define OV9655_GAM10      0x84
  #define OV9655_GAM11      0x85
  #define OV9655_GAM12      0x86
  #define OV9655_GAM13      0x87
  #define OV9655_GAM14      0x88
  #define OV9655_GAM15      0x89

  #define OV9655_COM18      0x8B
  #define OV9655_COM19      0x8C
  #define OV9655_COM20      0x8D


  I2C_WriteReg (OV9655_DNSTH, 0x21);
  I2C_WriteReg (0x71, 0x78);

  I2C_WriteReg (OV9655_POIDX, 0x11);
  I2C_WriteReg (OV9655_PCKDV, 0x01);
  I2C_WriteReg (OV9655_XINDX, 0x10);
  I2C_WriteReg (OV9655_YINDX, 0x10);

  I2C_WriteReg (0x76, 0x01);
  I2C_WriteReg (0x77, 0x02);

  I2C_WriteReg (OV9655_SLOP, 0x12);
  I2C_WriteReg (OV9655_GAM1, 0x08);
  I2C_WriteReg (OV9655_GAM2, 0x16);
  I2C_WriteReg (OV9655_GAM3, 0x30);
  I2C_WriteReg (OV9655_GAM4, 0x5e);
  I2C_WriteReg (OV9655_GAM5, 0x72);
  I2C_WriteReg (OV9655_GAM6, 0x82);
  I2C_WriteReg (OV9655_GAM7, 0x8e);
  I2C_WriteReg (OV9655_GAM8, 0x9a);
  I2C_WriteReg (OV9655_GAM9, 0xa4);
  I2C_WriteReg (OV9655_GAM10, 0xac);
  I2C_WriteReg (OV9655_GAM11, 0xb8);
  I2C_WriteReg (OV9655_GAM12, 0xc3);
  I2C_WriteReg (OV9655_GAM13, 0xd6);
  I2C_WriteReg (OV9655_GAM14, 0xe6);
  I2C_WriteReg (OV9655_GAM15, 0xf2);

  I2C_WriteReg (0x8a, 0x24);
  I2C_WriteReg (OV9655_COM19, 0x80);
  //}}}
  //{{{  0x9x,Ax,Bx,Cx
  #define OV9655_DMLNL      0x92
  #define OV9655_DMLNH      0x93
  #define OV9655_LCC6       0x9D
  #define OV9655_LCC7       0x9E
  #define OV9655_AECH       0xA1
  #define OV9655_BD50       0xA2
  #define OV9655_BD60       0xA3
  #define OV9655_COM21      0xA4
  #define OV9655_GREEN      0xA6
  #define OV9655_VZST       0xA7
  #define OV9655_REFA8      0xA8
  #define OV9655_REFA9      0xA9
  #define OV9655_BLC1       0xAC
  #define OV9655_BLC2       0xAD
  #define OV9655_BLC3       0xAE
  #define OV9655_BLC4       0xAF
  #define OV9655_BLC5       0xB0
  #define OV9655_BLC6       0xB1
  #define OV9655_BLC7       0xB2
  #define OV9655_BLC8       0xB3
  #define OV9655_CTRLB4     0xB4
  #define OV9655_FRSTL      0xB7
  #define OV9655_FRSTH      0xB8
  #define OV9655_ADBOFF     0xBC
  #define OV9655_ADROFF     0xBD
  #define OV9655_ADGbOFF    0xBE
  #define OV9655_ADGrOFF    0xBF
  #define OV9655_COM23      0xC4
  #define OV9655_BD60MAX    0xC5
  #define OV9655_COM24      0xC7

  I2C_WriteReg (0x90, 0x7d);
  I2C_WriteReg (0x91, 0x7b);

  I2C_WriteReg (OV9655_LCC6, 0x02);
  I2C_WriteReg (OV9655_LCC7, 0x02);
  I2C_WriteReg (0x9f, 0x7a);
  I2C_WriteReg (0xa0, 0x79);

  I2C_WriteReg (OV9655_AECH, 0x1f);
  I2C_WriteReg (OV9655_COM21, 0x50);
  I2C_WriteReg (0xa5, 0x68);
  I2C_WriteReg (OV9655_GREEN, 0x4a);
  I2C_WriteReg (OV9655_REFA8, 0xc1);
  I2C_WriteReg (OV9655_REFA9, 0xef);
  I2C_WriteReg (0xaa, 0x92);
  I2C_WriteReg (0xab, 0x04);

  I2C_WriteReg (OV9655_BLC1, 0x80);
  I2C_WriteReg (OV9655_BLC2, 0x80);
  I2C_WriteReg (OV9655_BLC3, 0x80);
  I2C_WriteReg (OV9655_BLC4, 0x80);
  I2C_WriteReg (OV9655_BLC7, 0xf2);
  I2C_WriteReg (OV9655_BLC8, 0x20);

  I2C_WriteReg (OV9655_CTRLB4, 0x20);
  I2C_WriteReg (0xb5, 0x00);
  I2C_WriteReg (0xb6, 0xaf);
  I2C_WriteReg (0xb6, 0xaf);
  I2C_WriteReg (0xbb, 0xae);

  I2C_WriteReg (OV9655_ADBOFF, 0x7f);
  I2C_WriteReg (OV9655_ADROFF, 0x7f);
  I2C_WriteReg (OV9655_ADGbOFF, 0x7f);
  I2C_WriteReg (OV9655_ADGrOFF, 0x7f);

  I2C_WriteReg (0xbf, 0x7f);
  I2C_WriteReg (0xc0, 0xaa);
  I2C_WriteReg (0xc1, 0xc0);
  I2C_WriteReg (0xc2, 0x01);
  I2C_WriteReg (0xc3, 0x4e);
  I2C_WriteReg (0xc6, 0x05);

  I2C_WriteReg (OV9655_COM24, 0x81);
  I2C_WriteReg (0xc9, 0xe0);
  I2C_WriteReg (0xca, 0xe8);
  I2C_WriteReg (0xcb, 0xf0);
  I2C_WriteReg (0xcc, 0xd8);
  I2C_WriteReg (0xcd, 0x93);
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
  DCMI_InitStructure.DCMI_VSPolarity = DCMI_VSPolarity_High;
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
  DMA_InitStructure.DMA_BufferSize = BUFFER_SIZE / 4;
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
                            FRAME_BUFFER + (framePhase * BUFFER_SIZE),
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
  return manf_id << 16 | prod_id;
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
  if (prod_id == 0x9657)
    lcdSpiDisplay1 (FRAME_BUFFER, 0,0, 320);
  else
    lcdSpiDisplay2 (FRAME_BUFFER, 0,0, 320);
  }
//}}}

//{{{
bool initCamera (bool continuous) {

  initTimerClock();
  initGPIO();

  deviceAddress = OV7670_DEVICE_ADDRESS;
  manf_id = I2C_ReadReg (REG_MIDH);
  if (manf_id == 0xFF) {
    deviceAddress = OV9655_DEVICE_ADDRESS;
    manf_id = I2C_ReadReg (REG_MIDH);
    }
  manf_id = (manf_id <<8) | I2C_ReadReg (REG_MIDL);
  prod_id = I2C_ReadReg (REG_PID);
  prod_id = (prod_id<<8) | I2C_ReadReg (REG_VER);

  #ifdef DEBUG
  printf ("id %x %x\n", manf_id, prod_id);
  #endif

  if (prod_id == 0x9657)
    initOV9655();
  else if (prod_id == 0x7673)
    initOV7670();
  else
    return false;

  initDCMI (continuous);

  return true;
  }
//}}}
