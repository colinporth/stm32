// main.c
//{{{  includes
#include <stdio.h>
#include <string.h>

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_dcmi.h"
#include "stm32f4xx_ltdc.h"

#include "display.h"
#include "touch.h"

#include "../files/ff.h"
#include "../files/tjpgd.h"
#include "../files/sd.h"

#include "stm32f429i_discovery_l3gd20.h"
//}}}
//{{{  externals
int freeSansBold_len;
const uint8_t freeSansBold[64228];
//}}}
//{{{  dcmi defines
#define SLAVE_ADDRESS   0x42

#define REG_GAIN  0x00  /* Gain lower 8 bits (rest in vref) */
#define REG_BLUE  0x01  /* blue gain */
#define REG_RED   0x02  /* red gain */
#define REG_VREF  0x03  /* Pieces of GAIN, VSTART, VSTOP */

#define REG_COM1  0x04  /* Control 1 */
#define  COM1_CCIR656   0x40  /* CCIR656 enable */

#define REG_BAVE  0x05  /* U/B Average level */
#define REG_GbAVE 0x06  /* Y/Gb Average level */
#define REG_AECHH 0x07  /* AEC MS 5 bits */
#define REG_RAVE  0x08  /* V/R Average level */

#define REG_COM2  0x09  /* Control 2 */
#define  COM2_SSLEEP    0x10  /* Soft sleep mode */

#define REG_PID   0x0a  /* Product ID MSB */
#define REG_VER   0x0b  /* Product ID LSB */

#define REG_COM3  0x0c  /* Control 3 */
#define  COM3_SWAP    0x40    /* Byte swap */
#define  COM3_SCALEEN   0x08    /* Enable scaling */
#define  COM3_DCWEN   0x04    /* Enable downsamp/crop/window */

#define REG_COM4  0x0d  /* Control 4 */
#define REG_COM5  0x0e  /* All "reserved" */
#define REG_COM6  0x0f  /* Control 6 */
#define REG_AECH  0x10  /* More bits of AEC value */

#define REG_CLKRC 0x11  /* Clock control */
  #define CLK_EXT   0x40      /* Use external clock directly */
  #define CLK_SCALE 0x3f      /* Mask for internal clock scale */
  #define CLOCK_SCALE_15fps 3 // 15fps
  #define CLOCK_SCALE_20fps 2 // 20fps
  #define CLOCK_SCALE_30fps 1 // 30fps

#define REG_COM7  0x12  // Control 7
  #define   COM7_RESET     0x80    // b7 = regs reset

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

#define REG_COM8  0x13  /* Control 8 */
#define   COM8_FASTAEC   0x80    /* Enable fast AGC/AEC */
#define   COM8_AECSTEP   0x40    /* Unlimited AEC step size */
#define   COM8_BFILT     0x20    /* Band filter enable */
#define   COM8_AGC       0x04    /* Auto gain enable */
#define   COM8_AWB       0x02    /* White balance enable */
#define   COM8_AEC       0x01    /* Auto exposure enable */

#define REG_COM9  0x14  /* Control 9  - gain ceiling */

#define REG_COM10 0x15  /* Control 10 */
#define   COM10_HSYNC     0x40    /* HSYNC instead of HREF */
#define   COM10_PCLK_HB   0x20    /* Suppress PCLK on horiz blank */
#define   COM10_HREF_REV  0x08    /* Reverse HREF */
#define   COM10_VS_LEAD   0x04    /* VSYNC on clock leading edge */
#define   COM10_VS_NEG    0x02    /* VSYNC negative */
#define   COM10_HS_NEG    0x01    /* HSYNC negative */

#define REG_HSTART 0x17  /* Horiz start high bits */
#define REG_HSTOP  0x18  /* Horiz stop high bits */
#define REG_VSTART 0x19  /* Vert start high bits */
#define REG_VSTOP  0x1a  /* Vert stop high bits */
#define REG_PSHFT  0x1b  /* Pixel delay after HREF */

#define REG_MIDH   0x1c  /* Manuf. ID high */
#define REG_MIDL   0x1d  /* Manuf. ID low */

#define REG_MVFP  0x1e  /* Mirror / vflip */
#define   MVFP_MIRROR   0x20    /* Mirror image */
#define   MVFP_FLIP   0x10    /* Vertical flip */

#define REG_AEW   0x24  /* AGC upper limit */
#define REG_AEB   0x25  /* AGC lower limit */
#define REG_VPT   0x26  /* AGC/AEC fast mode op region */
#define REG_HSYST 0x30  /* HSYNC rising edge delay */
#define REG_HSYEN 0x31  /* HSYNC falling edge delay */
#define REG_HREF  0x32  /* HREF pieces */

#define REG_TSLB  0x3a  /* lots of stuff */
#define   TSLB_YLAST    0x04    /* UYVY or VYUY - see com13 */

#define REG_COM11 0x3b  /* Control 11 */
#define   COM11_NIGHT   0x80    /* NIght mode enable */
#define   COM11_NMFR    0x60    /* Two bit NM frame rate */
#define   COM11_HZAUTO    0x10    /* Auto detect 50/60 Hz */
#define   COM11_50HZ    0x08    /* Manual 50Hz select */
#define   COM11_EXP   0x02

#define REG_COM12 0x3c  /* Control 12 */
#define   COM12_HREF    0x80    /* HREF always */

#define REG_COM13 0x3d  /* Control 13 */
#define   COM13_GAMMA   0x80    /* Gamma enable */
#define   COM13_UVSAT   0x40    /* UV saturation auto adjustment */
#define   COM13_UVSWAP    0x01    /* V before U - w/TSLB */

#define REG_COM14 0x3e  /* Control 14 */
#define   COM14_DCWEN   0x10    /* DCW/PCLK-scale enable */

#define REG_EDGE  0x3f  /* Edge enhancement factor */

#define REG_COM15 0x40  /* Control 15 */
#define   COM15_R10F0   0x00    /* Data range 10 to F0 */
#define   COM15_R01FE   0x80    /*            01 to FE */
#define   COM15_R00FF   0xc0    /*            00 to FF */
#define   COM15_RGB565  0x10    /* RGB565 output */
#define   COM15_RGB555  0x30    /* RGB555 output */

#define REG_COM16 0x41  /* Control 16 */
#define   COM16_AWBGAIN   0x08    /* AWB gain enable */

#define REG_COM17 0x42  /* Control 17 */
#define   COM17_AECWIN    0xc0    /* AEC window - must match COM4 */
#define   COM17_CBAR    0x08    /* DSP Color bar */

//This matrix defines how the colors are generated, must be
// * tweaked to adjust hue and saturation.
// * Order: v-red, v-green, v-blue, u-red, u-green, u-blue
// * They are nine-bit signed quantities, with the sign bit
// * stored in 0x58.  Sign for v-red is bit 0, and up from there.
#define REG_CMATRIX_BASE 0x4f
#define REG_CMATRIX_SIGN 0x58

#define REG_BRIGHT  0x55  /* Brightness */
#define REG_CONTRAS 0x56  /* Contrast control */

#define REG_GFIX  0x69  /* Fix gain control */

#define REG_DBLV  0x6b  /* PLL control an debugging */
#define   DBLV_BYPASS 0x00    /* Bypass PLL */
#define   DBLV_X4     0x01    /* clock x4 */
#define   DBLV_X6     0x10    /* clock x6 */
#define   DBLV_X8     0x11    /* clock x8 */

#define REG_REG76 0x76  /* OV's name */
#define   R76_BLKPCOR   0x80    /* Black pixel correction enable */
#define   R76_WHTPCOR   0x40    /* White pixel correction enable */

#define REG_RGB444  0x8c  /* RGB 444 control */
#define   R444_ENABLE   0x02    /* Turn on RGB444, overrides 5x5 */
#define   R444_RGBX   0x01    /* Empty nibble at end */

#define REG_HAECC1  0x9f  /* Hist AEC/AGC control 1 */
#define REG_HAECC2  0xa0  /* Hist AEC/AGC control 2 */
#define REG_BD50MAX 0xa5  /* 50hz banding step limit */
#define REG_HAECC3  0xa6  /* Hist AEC/AGC control 3 */
#define REG_HAECC4  0xa7  /* Hist AEC/AGC control 4 */
#define REG_HAECC5  0xa8  /* Hist AEC/AGC control 5 */
#define REG_HAECC6  0xa9  /* Hist AEC/AGC control 6 */
#define REG_HAECC7  0xaa  /* Hist AEC/AGC control 7 */
#define REG_BD60MAX 0xab  /* 60hz banding step limit */
//}}}

static __IO uint32_t DelayCount;
//{{{
void delayMs (__IO uint32_t ms) {

  DelayCount = ms;
  while (DelayCount != 0) {}
  }
//}}}
//{{{
void SysTick_Handler() {

  if (DelayCount != 0x00)
    DelayCount--;
  }
//}}}

float GyroCalibration[3];
//{{{
uint32_t L3GD20_TIMEOUT_UserCallback() {

  return 0;
  }
//}}}
//{{{
static void Demo_GyroConfig() {

  // Configure Mems L3GD20
  L3GD20_InitTypeDef L3GD20_InitStructure;
  L3GD20_InitStructure.Power_Mode = L3GD20_MODE_ACTIVE;
  L3GD20_InitStructure.Output_DataRate = L3GD20_OUTPUT_DATARATE_1;
  L3GD20_InitStructure.Axes_Enable = L3GD20_AXES_ENABLE;
  L3GD20_InitStructure.Band_Width = L3GD20_BANDWIDTH_4;
  L3GD20_InitStructure.BlockData_Update = L3GD20_BlockDataUpdate_Continous;
  L3GD20_InitStructure.Endianness = L3GD20_BLE_LSB;
  L3GD20_InitStructure.Full_Scale = L3GD20_FULLSCALE_500;
  L3GD20_Init (&L3GD20_InitStructure);

  L3GD20_FilterConfigTypeDef L3GD20_FilterStructure;
  L3GD20_FilterStructure.HighPassFilter_Mode_Selection = L3GD20_HPM_NORMAL_MODE_RES;
  L3GD20_FilterStructure.HighPassFilter_CutOff_Frequency = L3GD20_HPFCF_0;
  L3GD20_FilterConfig (&L3GD20_FilterStructure) ;

  L3GD20_FilterCmd (L3GD20_HIGHPASSFILTER_ENABLE);
}
//}}}
//{{{
static void Demo_GyroReadAngRate (float* pfData) {

  #define L3GD20_CTRL_REG4_ADDR 0x23  /* Control register 4 */
  #define L3GD20_OUT_X_L_ADDR   0x28  /* Output Register X */

  uint8_t tmpbuffer[6] ={0};
  int16_t RawData[3] = {0};
  uint8_t tmpreg = 0;
  float sensitivity = 0;
  int i = 0;

  L3GD20_Read (&tmpreg, L3GD20_CTRL_REG4_ADDR,1);
  L3GD20_Read (tmpbuffer, L3GD20_OUT_X_L_ADDR,6);

  // check in the control register 4 the data alignment (Big Endian or Little Endian)
  if (!(tmpreg & 0x40))
    for (i = 0; i < 3; i++)
      RawData[i] = (int16_t)(((uint16_t)tmpbuffer[2*i+1] << 8) + tmpbuffer[2*i]);
  else
    for (i = 0; i < 3; i++)
      RawData[i] = (int16_t)(((uint16_t)tmpbuffer[2*i] << 8) + tmpbuffer[2*i+1]);

  // Switch the sensitivity value set in the CRTL4
  switch (tmpreg & 0x30) {
    case 0x00:
      sensitivity = (float)114.285f; // 250 dps full scale [LSB/dps]
      break;

    case 0x10:
      sensitivity = (float)57.1429f; // 500 dps full scale [LSB/dps]
      break;

    case 0x20:
      sensitivity = (float)14.285f;  // 2000 dps full scale [LSB/dps]
      break;
    }

  // divide by sensitivity
  for (i = 0; i < 3; i++)
    pfData[i] = (float)RawData[i] / sensitivity;
  }
//}}}
//{{{
static void Gyro_SimpleCalibration (float* GyroData) {

  float X_BiasError, Y_BiasError, Z_BiasError = 0.0;

  uint32_t BiasErrorSplNbr = 500;
  for (int i = 0; i < BiasErrorSplNbr; i++) {
    Demo_GyroReadAngRate (GyroData);
    X_BiasError += GyroData[0];
    Y_BiasError += GyroData[1];
    Z_BiasError += GyroData[2];
    }

  // Set bias errors
  X_BiasError /= BiasErrorSplNbr;
  Y_BiasError /= BiasErrorSplNbr;
  Z_BiasError /= BiasErrorSplNbr;

  // Get offset value on X, Y and Z
  GyroData[0] = X_BiasError;
  GyroData[1] = Y_BiasError;
  GyroData[2] = Z_BiasError;
  }
//}}}

//{{{
void I2C_init() {

  // enable clock for SCL and SDA pins
  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB, ENABLE);

  // enable APB1 peripheral clock for I2C2
  RCC_APB1PeriphClockCmd (RCC_APB1Periph_I2C2, ENABLE);

  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11; // we are going to use PB10 and PB11
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;             // set pins to alternate function
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;        // set GPIO speed
  GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;           // set output to open drain --> the line has to be only pulled low, not driven high
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;             // enable pull up resistors
  GPIO_Init (GPIOB, &GPIO_InitStruct);                  // init GPIOB

  // Connect I2C2 pins to AF
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource10, GPIO_AF_I2C2);  // SCL
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource11, GPIO_AF_I2C2); // SDA

  // configure I2C2
  I2C_InitTypeDef I2C_InitStruct;
  I2C_InitStruct.I2C_ClockSpeed = 30000;         // 100kHz
  I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;         // I2C mode
  I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2; // 50% duty cycle --> standard
  I2C_InitStruct.I2C_OwnAddress1 = 0x00;          // own address, not relevant in master mode
  I2C_InitStruct.I2C_Ack = I2C_Ack_Disable;       // disable acknowledge when reading (can be changed later on)
  I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; // set address length to 7 bit addresses
  I2C_Init (I2C2, &I2C_InitStruct);                // init I2C1

  // enable I2C2
  I2C_Cmd (I2C2, ENABLE);
}
//}}}
//{{{
void I2C_start (I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction) {

  // wait until I2C1 is not busy anymore
  while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY));

  // Send I2C1 START condition
  I2C_GenerateSTART(I2Cx, ENABLE);

  // wait for I2C1 EV5 --> Slave has acknowledged start condition
  while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT));

  // Send slave Address for write
  I2C_Send7bitAddress(I2Cx, address, direction);

  /* wait for I2C1 EV6, check if
   * either Slave has acknowledged Master transmitter or
   * Master receiver mode, depending on the transmission
   * direction
   */
  if (direction == I2C_Direction_Transmitter){
    while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    }
  else if (direction == I2C_Direction_Receiver){
    while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));
    }
  }
//}}}
//{{{
/* This function transmits one byte to the slave device
 * Parameters:
 *    I2Cx --> the I2C peripheral e.g. I2C1
 *    data --> the data byte to be transmitted
 */
void I2C_write (I2C_TypeDef* I2Cx, uint8_t data) {

  I2C_SendData(I2Cx, data);

  // wait for I2C1 EV8_2 --> byte has been transmitted
  while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  }
//}}}
//{{{
/* This function reads one byte from the slave device
 * and acknowledges the byte (requests another byte)
 */
uint8_t I2C_read_ack (I2C_TypeDef* I2Cx) {

  uint8_t data;

  // enable acknowledge of recieved data
  I2C_AcknowledgeConfig (I2Cx, ENABLE);

  // wait until one byte has been received
  while (!I2C_CheckEvent (I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED) );

  // read data from I2C data register and return data byte
  data = I2C_ReceiveData (I2Cx);
  return data;
  }
//}}}
//{{{
/* This function reads one byte from the slave device
 * and doesn't acknowledge the recieved data
 */
uint8_t I2C_read_nack (I2C_TypeDef* I2Cx) {

  uint8_t data;

  // disable acknowledge of received data
  I2C_AcknowledgeConfig (I2Cx, DISABLE);

  // wait until one byte has been received
  while( !I2C_CheckEvent (I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED) );

  // read data from I2C data register and return data byte
  data = I2C_ReceiveData (I2Cx);

  return data;
  }
//}}}
//{{{
void I2C_stop (I2C_TypeDef* I2Cx) {

  // Send I2C1 STOP Condition
  I2C_GenerateSTOP (I2Cx, ENABLE);
  }
//}}}
//{{{
uint8_t I2C_readreg (uint8_t reg) {

  // start a transmission in Master transmitter mode
  I2C_start (I2C2, SLAVE_ADDRESS, I2C_Direction_Transmitter);
  I2C_write (I2C2, reg);
  I2C_stop (I2C2);

  delayMs (1);

  // start a transmission in Master receiver mode
  I2C_start (I2C2, SLAVE_ADDRESS, I2C_Direction_Receiver);
  uint8_t tmp = I2C_read_nack (I2C2);
  I2C_stop (I2C2);

  delayMs (1);

  return tmp;
  }
//}}}
//{{{
uint8_t I2C_writereg (uint8_t reg, uint8_t data) {

  // start a transmission in Master transmitter mode
  I2C_start (I2C2, SLAVE_ADDRESS, I2C_Direction_Transmitter);
  I2C_write (I2C2, reg);
  delayMs (1);

  I2C_write (I2C2, data);
  I2C_stop (I2C2);
  delayMs (1);

  // start a transmission in Master receiver mode
  I2C_start (I2C2, SLAVE_ADDRESS, I2C_Direction_Receiver);
  uint8_t tmp = I2C_read_nack (I2C2);
  I2C_stop (I2C2); // stop the transmission

  delayMs (1);

  return tmp;
}
//}}}

//{{{
void initMCO() {

  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);

  // Connect to AF0
  GPIO_PinAFConfig (GPIOA, GPIO_PinSource8, GPIO_AF_MCO);

  // Configure MCO1 pin(PA8) in alternate function
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOA, &GPIO_InitStruct);

  // HSI clock selected to output on MCO1 pin(PA8)
  RCC_MCO1Config (RCC_MCO1Source_PLLCLK, RCC_MCO1Div_5);
  }
//}}}
//{{{
void initSCCB() {
// If we're running RGB565, we must rewrite clkrc after setting
// the other parameters or the image looks poor.  If we're *not*
// doing RGB565, we must not rewrite clkrc or the image looks really poor.
// (Update) Now that we retain clkrc state, we should be able
// to write it unconditionally, and that will make the frame rate persistent too.
  #define hstart    168  // Empirically determined
  #define hstop      24
  #define vstart     12
  #define vstop     492
  // VGA hstart   = 158, /.hstop    =  14, .vstart   =  10 .vstop    = 490,

  I2C_writereg (REG_COM7, COM7_RESET);
  delayMs (100);

  I2C_writereg (REG_TSLB,  0x04);

  // Horizontal 11 bits, top 8 hstart and hstop.
  // Bottom 3 of hstart are in href[2:0], bottom 3 of hstop in href[5:3]
  // There is a mystery "edge offset" value in the top two bits of href.
  I2C_writereg (REG_HSTART, (hstart >> 3) & 0xff);
  I2C_writereg (REG_HSTOP, (hstop >> 3) & 0xff);
  I2C_writereg (REG_HREF, 0x80 | ((hstop & 0x7) << 3) | (hstart & 0x7));

  // Vertical 10 bits.
  I2C_writereg (REG_VSTART, (vstart >> 2) & 0xff);
  I2C_writereg (REG_VSTOP, (vstop >> 2) & 0xff);
  I2C_writereg (REG_VREF, ((vstop & 0x3) << 2) | (vstart & 0x3));

  // rgb565
  I2C_writereg (REG_RGB444, 0);            // No RGB444 please
  I2C_writereg (REG_COM1,   0x0);          // CCIR601
  I2C_writereg (REG_COM7,   COM7_FMT_QVGA | COM7_RGB);
  I2C_writereg (REG_COM15,  COM15_RGB565);
  I2C_writereg (REG_COM9,   0x38);         // 16x gain ceiling; 0x8 is reserved bit
  I2C_writereg (REG_COM13,  COM13_GAMMA | COM13_UVSAT);

  // cmatrix = {179,-179,0,-61,-176,228 },
  I2C_writereg (REG_CMATRIX_BASE,   179);  // unsigned Matrix coefficients
  I2C_writereg (REG_CMATRIX_BASE+1, 179);
  I2C_writereg (REG_CMATRIX_BASE+2, 0);
  I2C_writereg (REG_CMATRIX_BASE+3, 61);
  I2C_writereg (REG_CMATRIX_BASE+4, 176);
  I2C_writereg (REG_CMATRIX_BASE+5, 228);
  I2C_writereg (REG_CMATRIX_SIGN,   0x9a); // lower 6 bits coefficient signs bits + upper 2 bits crap

  I2C_writereg (REG_CLKRC, CLOCK_SCALE_30fps);

  I2C_writereg (REG_COM3, 0);
  I2C_writereg (REG_COM14, 0);

  // Mystery scaling numbers
  I2C_writereg (0x70, 0x3a);
  I2C_writereg (0x71, 0x35);
  I2C_writereg (0x72, 0x11);
  I2C_writereg (0x73, 0xf0);
  I2C_writereg (0xa2, 0x02);
  I2C_writereg (REG_COM10, 0x0);

  // Gamma curve values
  I2C_writereg (0x7a, 0x20);
  I2C_writereg (0x7b, 0x10);
  I2C_writereg (0x7c, 0x1e);
  I2C_writereg (0x7d, 0x35);
  I2C_writereg (0x7e, 0x5a);
  I2C_writereg (0x7f, 0x69);
  I2C_writereg (0x80, 0x76);
  I2C_writereg (0x81, 0x80);
  I2C_writereg (0x82, 0x88);
  I2C_writereg (0x83, 0x8f);
  I2C_writereg (0x84, 0x96);
  I2C_writereg (0x85, 0xa3);
  I2C_writereg (0x86, 0xaf);
  I2C_writereg (0x87, 0xc4);
  I2C_writereg (0x88, 0xd7);
  I2C_writereg (0x89, 0xe8);

  // AGC and AEC parameters, disable features, tweak values, enable features
  I2C_writereg (REG_COM8, COM8_FASTAEC | COM8_AECSTEP | COM8_BFILT);
  I2C_writereg (REG_GAIN, 0);
  I2C_writereg (REG_AECH, 0);
  I2C_writereg (REG_COM4, 0x40); /* magic reserved bit */
  I2C_writereg (REG_COM9, 0x18); /* 4x gain + magic rsvd bit */
  I2C_writereg (REG_BD50MAX, 0x05);
  I2C_writereg (REG_BD60MAX, 0x07);
  I2C_writereg (REG_AEW, 0x95);
  I2C_writereg (REG_AEB, 0x33);
  I2C_writereg (REG_VPT, 0xe3);
  I2C_writereg (REG_HAECC1, 0x78);
  I2C_writereg (REG_HAECC2, 0x68);
  I2C_writereg (0xa1, 0x03); /* magic */
  I2C_writereg (REG_HAECC3, 0xd8);
  I2C_writereg (REG_HAECC4, 0xd8);
  I2C_writereg (REG_HAECC5, 0xf0);
  I2C_writereg (REG_HAECC6, 0x90);
  I2C_writereg (REG_HAECC7, 0x94);
  I2C_writereg (REG_COM8, COM8_FASTAEC | COM8_AECSTEP | COM8_BFILT | COM8_AGC | COM8_AEC);

  // Almost all of these are magic "reserved" values
  I2C_writereg (REG_COM5, 0x61);
  I2C_writereg (REG_COM6, 0x4b);
  I2C_writereg (0x16, 0x02);
  I2C_writereg (REG_MVFP, 0x07);
  I2C_writereg (0x21, 0x02);
  I2C_writereg (0x22, 0x91);
  I2C_writereg (0x29, 0x07);
  I2C_writereg (0x33, 0x0b);
  I2C_writereg (0x35, 0x0b);
  I2C_writereg (0x37, 0x1d);
  I2C_writereg (0x38, 0x71);
  I2C_writereg (0x39, 0x2a);
  I2C_writereg (REG_COM12, 0x78);
  I2C_writereg (0x4d, 0x40);
  I2C_writereg (0x4e, 0x20);
  I2C_writereg (REG_GFIX, 0);
  I2C_writereg (0x6b, 0x4a);
  I2C_writereg (0x74, 0x10);
  I2C_writereg (0x8d, 0x4f);
  I2C_writereg (0x8e, 0);
  I2C_writereg (0x8f, 0);
  I2C_writereg (0x90, 0);
  I2C_writereg (0x91, 0);
  I2C_writereg (0x96, 0);
  I2C_writereg (0x9a, 0);
  I2C_writereg (0xb0, 0x84);
  I2C_writereg (0xb1, 0x0c);
  I2C_writereg (0xb2, 0x0e);
  I2C_writereg (0xb3, 0x82);
  I2C_writereg (0xb8, 0x0a);

  // More reserved magic, some of which tweaks white balance
  I2C_writereg (0x43, 0x0a);
  I2C_writereg (0x44, 0xf0);
  I2C_writereg (0x45, 0x34);
  I2C_writereg (0x46, 0x58);
  I2C_writereg (0x47, 0x28);
  I2C_writereg (0x48, 0x3a);
  I2C_writereg (0x59, 0x88);
  I2C_writereg (0x5a, 0x88);
  I2C_writereg (0x5b, 0x44);
  I2C_writereg (0x5c, 0x67);
  I2C_writereg (0x5d, 0x49);
  I2C_writereg (0x5e, 0x0e);
  I2C_writereg (0x6c, 0x0a);
  I2C_writereg (0x6d, 0x55);
  I2C_writereg (0x6e, 0x11);
  I2C_writereg (0x6f, 0x9f); /* "9e for advance AWB" */
  I2C_writereg (0x6a, 0x40);
  I2C_writereg (REG_BLUE, 0x40);
  I2C_writereg (REG_RED, 0x60);
  I2C_writereg (REG_COM8, COM8_FASTAEC | COM8_AECSTEP | COM8_BFILT | COM8_AGC | COM8_AEC | COM8_AWB);

  I2C_writereg (REG_COM16, COM16_AWBGAIN);
  I2C_writereg (REG_EDGE, 0);
  I2C_writereg (0x75, 0x05);
  I2C_writereg (0x76, 0xe1);
  I2C_writereg (0x4c, 0);
  I2C_writereg (0x77, 0x01);
  I2C_writereg (REG_COM13, 0xc3);
  I2C_writereg (0x4b, 0x09);
  I2C_writereg (0xc9, 0x60);
  I2C_writereg (REG_COM16, 0x38);
  I2C_writereg (0x56, 0x40);

  I2C_writereg (0x34, 0x11);
  I2C_writereg (REG_COM11, COM11_EXP | COM11_HZAUTO);
  I2C_writereg (0xa4, 0x88);
  I2C_writereg (0x96, 0);
  I2C_writereg (0x97, 0x30);
  I2C_writereg (0x98, 0x20);
  I2C_writereg (0x99, 0x30);
  I2C_writereg (0x9a, 0x84);
  I2C_writereg (0x9b, 0x29);
  I2C_writereg (0x9c, 0x03);
  I2C_writereg (0x9d, 0x4c);
  I2C_writereg (0x9e, 0x3f);
  I2C_writereg (0x78, 0x04);

  // Extra-weird stuff
  I2C_writereg (0x79, 0x01);
  I2C_writereg (0xc8, 0xf0);
  I2C_writereg (0x79, 0x0f);
  I2C_writereg (0xc8, 0x00);
  I2C_writereg (0x79, 0x10);
  I2C_writereg (0xc8, 0x7e);
  I2C_writereg (0x79, 0x0a);
  I2C_writereg (0xc8, 0x80);
  I2C_writereg (0x79, 0x0b);
  I2C_writereg (0xc8, 0x01);
  I2C_writereg (0x79, 0x0c);
  I2C_writereg (0xc8, 0x0f);
  I2C_writereg (0x79, 0x0d);
  I2C_writereg (0xc8, 0x20);
  I2C_writereg (0x79, 0x09);
  I2C_writereg (0xc8, 0x80);
  I2C_writereg (0x79, 0x02);
  I2C_writereg (0xc8, 0xc0);
  I2C_writereg (0x79, 0x03);
  I2C_writereg (0xc8, 0x40);
  I2C_writereg (0x79, 0x05);
  I2C_writereg (0xc8, 0x30);
  I2C_writereg (0x79, 0x26);
  }
//}}}
//{{{
void initDCMI() {
// PA08 - mco1
// PB10 - i2c2 scl
// PB11 - i2c2 sda
// PA06 - clk
// PA04 - hs
// PG09 - vs   or PB07
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
  RCC_AHB2PeriphClockCmd (RCC_AHB2Periph_DCMI, ENABLE);

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
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init (GPIOA, &GPIO_InitStructure);

  // GPIOB vs,D6,7
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
  GPIO_Init (GPIOB, &GPIO_InitStructure);

  // GPIOC D0:1
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_Init (GPIOC, &GPIO_InitStructure);

  // GPIOD D5
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_Init (GPIOD, &GPIO_InitStructure);

  // GPIOE D4
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_Init (GPIOG, &GPIO_InitStructure);

  // GPIOG vs,D2,3
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
  GPIO_Init (GPIOG, &GPIO_InitStructure);

  DCMI_DeInit();

  // DCMI configuration
  DCMI_InitTypeDef DCMI_InitStructure;
  DCMI_InitStructure.DCMI_CaptureMode = DCMI_CaptureMode_SnapShot;
  //DCMI_InitStructure.DCMI_CaptureMode = DCMI_CaptureMode_Continuous;
  DCMI_InitStructure.DCMI_SynchroMode = DCMI_SynchroMode_Hardware;
  //DCMI_InitStructure.DCMI_PCKPolarity = DCMI_PCKPolarity_Falling;
  DCMI_InitStructure.DCMI_PCKPolarity = DCMI_PCKPolarity_Rising;
  DCMI_InitStructure.DCMI_VSPolarity = DCMI_VSPolarity_High;
  DCMI_InitStructure.DCMI_HSPolarity = DCMI_HSPolarity_Low;
  DCMI_InitStructure.DCMI_CaptureRate = DCMI_CaptureRate_All_Frame;
  DCMI_InitStructure.DCMI_ExtendedDataMode = DCMI_ExtendedDataMode_8b;
  DCMI_Init (&DCMI_InitStructure);

  // config DMA2 to transfer Data from DCMI - Enable DMA2 clock
  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_DMA2, ENABLE);

  // DMA2 Stream1 Configuration
  DMA_DeInit (DMA2_Stream1);

  DMA_InitTypeDef DMA_InitStructure;
  DMA_InitStructure.DMA_Channel = DMA_Channel_1;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(DCMI_BASE + 0x28);
  DMA_InitStructure.DMA_Memory0BaseAddr = 0xD0000000;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = 320*240/2;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  //DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

  // DMA2 IRQ channel Configuration
  DMA_Init (DMA2_Stream1, &DMA_InitStructure);
  }
//}}}

//{{{  jpeg
static FATFS fatfs;
static DWORD serialNo;
static char path[128] = "/";
static char label[128];
static char fileStr[256];
static char longFileName[_MAX_LFN + 1];
static int dirs = 0;
static int file = 0;
static int files = 0;
//{{{
static UINT inFunc (JDEC* jdec, BYTE* buf, UINT bytes) {

  //printf ("input_func %d\n", bytes);
  if (buf)
    f_read ((FIL*)jdec->device, buf, bytes, &bytes);
  else
    f_lseek ((FIL*)jdec->device, f_tell ((FIL*)jdec->device) + bytes);

  return bytes;
}
//}}}
//{{{
static UINT outFunc (JDEC* jdec, void* buf, JRECT* rect) {

  //printf ("output_func %d %d %d %d\n", rect->left, rect->top, rect->width, rect->height);
  image_t image;
  image.width = rect->width;
  image.height = rect->height;
  image.pixels = buf;

  drawImage (&image, rect->left, 24+rect->top);
  return JDR_OK;
  }
//}}}
//{{{
static int decodeJpgFile (const char* fileName) {

  JDEC jdec;
  BYTE jdecWork[4096];

  FIL file;
  f_open (&file, fileName, FA_OPEN_EXISTING | FA_READ);

  jd_prepare (&jdec, inFunc, jdecWork, 4096, &file);

  int scale = 0;
  int scaleRatio = 1;
  while (scale < 3 && ((jdec.width/scaleRatio) > getWidth() || (jdec.height/scaleRatio) > getHeight())) {
     scale++;
     scaleRatio *= 2;
     }

  jd_decomp (&jdec, outFunc, scale);

  char str[50];
  sprintf (str, "%dx%d scaled 1/%d", jdec.width, jdec.height, scaleRatio);
  //drawRect (Black, 0, getHeight()-32, getWidth(), 16);
  drawTTString (LightGrey, 12, str, 0, getHeight()-32, getWidth(), 16);

  return 1;
  }
//}}}
//{{{
static int decodeFiles (char* path) {

  FILINFO filinfo;
  filinfo.lfname = longFileName;
  filinfo.lfsize = sizeof longFileName;

  DIR dir;
  int ok = f_opendir (&dir, path) == FR_OK;
  if (ok) {
    int i = strlen (path);
    while (true) {
      ok = (f_readdir (&dir, &filinfo) == FR_OK);
      if (!ok || filinfo.fname[0] == 0) // Break on error or end of dir
        break;
      if (filinfo.fname[0] == '.') // Ignore dot entry
        continue;

      char* fileName = *filinfo.lfname ? filinfo.lfname : filinfo.fname;
      if (filinfo.fattrib & AM_DIR) {
        // directory
        sprintf (&path[i], "/%s", fileName);
        ok = decodeFiles (path);
        if (!ok)
          break;
        path[i] = 0;
        }
      else {
        // file
        file++;
        int len = strlen (fileName);
        bool jpg = ((len > 4) &&
                    (fileName[len-4] == '.') &&
                    ((fileName[len-3] == 'j') || (fileName[len-3] == 'J')) &&
                    ((fileName[len-2] == 'p') || (fileName[len-2] == 'P')) &&
                    ((fileName[len-1] == 'g') || (fileName[len-1] == 'G')));
        if (jpg) {
          sprintf (fileStr, "%s/%s", path, fileName);
          decodeJpgFile (fileStr);
          }

        sprintf (fileStr, "%d:%d %s", file, files, fileName);
        drawRect (jpg ? Black : Yellow, 0, getHeight()-18, getWidth(), 18);
        drawTTString (White, 12, fileStr, 0, getHeight()-18, getWidth(), 17);
        }
      }
    }

  return ok;
  }
//}}}
//{{{
static int countFiles (char* path) {

  FILINFO filinfo;
  filinfo.lfname = longFileName;
  filinfo.lfsize = sizeof longFileName;

  DIR dir;
  int ok = f_opendir (&dir, path) == FR_OK;
  if (ok) {
    int i = strlen (path);
    while (true) {
      ok = (f_readdir (&dir, &filinfo) == FR_OK);
      if (!ok || filinfo.fname[0] == 0) // Break on error or end of dir
        break;
      if (filinfo.fname[0] == '.') // Ignore dot entry
        continue;

      char* fileName = *filinfo.lfname ? filinfo.lfname : filinfo.fname;
      if (filinfo.fattrib & AM_DIR) {
        // directory
        sprintf (&path[i], "/%s", fileName);
        ok = countFiles (path);
        if (!ok)
          break;
        path[i] = 0;
        dirs++;
        }
      else
        files++;
      }
    }

  return ok;
  }
//}}}
//}}}

int main() {
  //{{{  sysTick init
  SysTick->VAL = 0;                       // config sysTick
  SysTick->LOAD = SystemCoreClock / 1000; // - countdown value
  SysTick->CTRL = 0x7;                    // - 0x4 = sysTick HCLK, 0x2 = intEnable, 0x1 = enable
  //}}}

  displayInit();
  clearScreen (Blue);
  setTTFont (freeSansBold, freeSansBold_len);

  char str[80];
  sprintf (str, "Ver " __TIME__" "__DATE__"");
  drawTTString (Yellow, 18, str, 0, 0, getWidth(), 24);

  initMCO();
  I2C_init();
  initSCCB();
  //printf ("SCCB %x,%x,%x,%x\n", I2C_readreg (0x1c), I2C_readreg (0x1d), I2C_readreg (0x0a), I2C_readreg (0x0b));

  initDCMI();
  DMA_Cmd (DMA2_Stream1, ENABLE);
  DCMI_Cmd (ENABLE);

  while (true) {
    DCMI_CaptureCmd (ENABLE);
    delayMs (36);

    uint16_t* ptr = (uint16_t*)0xD0000000;
    for (int i = 0; i < 320*240; i++) {
      uint16_t value = *ptr;
      *ptr++ = (value << 8) | (value >> 8);
      }

    lcdSpiDisplay (0, 0, getWidth(), getHeight());
    }

  sprintf (str, "f429 display %dx%d", getWidth(), getHeight());
  drawTTString (Green, 24, str, 0, getHeight()-32, getWidth(), 32);

  if (SD_init (true) == SD_OK) {
    f_mount (0, &fatfs);
    f_getlabel (path, label, &serialNo);
    countFiles (path);
    decodeFiles (path);
    }

  #define count
  #ifdef count
  touchInit();

  int i = 0;
  while (true) {
    int tenths = i % 10;

    drawRect (Blue, 0, 26, getWidth(), 24);
    drawRect (Green, tenths * (getWidth()/10), 26, (getWidth()/10)-1, (2*tenths)+4);

    if (getTouchDown()) {
      int16_t x,y,z;
      getTouchPos (&x, &y, &z, getWidth(), getHeight());
      sprintf (str, "x:%d y:%d z:%d", x, y, z);
      drawRect (Blue, 0, getHeight()-32, getWidth(), 32);
      drawTTString (White, 24, str, 0, getHeight()-32, getWidth(), 32);
      }

    sprintf (str, "%d:%02d:%1d", (i/600) % 60, (i/10) % 60, tenths);
    if (!getMono())
      drawRect (Blue, 0, 80, getWidth(), getWidth()*5/16);
    drawTTString (Black, getWidth()/4, str, 2, 82, getWidth(), getWidth()*5/16);
    drawTTString (White, getWidth()/4, str, 0, 80, getWidth(), getWidth()*5/16);

    delayMs (100);
    i++;
    }
  #endif

  Demo_GyroConfig();
  Gyro_SimpleCalibration (GyroCalibration);

  while (true) {
    float Buffer[3];
    Demo_GyroReadAngRate (Buffer);

    Buffer[0] = Buffer[0] - GyroCalibration[0];
    Buffer[1] = Buffer[1] - GyroCalibration[1];

    sprintf (str, "%f %f", Buffer[0], Buffer[1]);
    drawTTString (White, 24, str, 0, getHeight()-32, getWidth(), 32);

    delayMs(50);
    }
  }
