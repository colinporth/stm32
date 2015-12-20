// touchF4i2c3.c - stmp811 touch
// PA08 - SCL
// PC09 - SDA
// PA15 - TPINT
//{{{  includes
#include "stm32f4xx.h"

#include "touch.h"

void delayMs (__IO uint32_t ms);
//}}}
//{{{  i2c3 defines
#define IOE_I2C                    I2C3
#define IOE_I2C_CLK                RCC_APB1Periph_I2C3

#define IOE_I2C_SCL_PIN            GPIO_Pin_8
#define IOE_I2C_SCL_GPIO_PORT      GPIOA
#define IOE_I2C_SCL_GPIO_CLK       RCC_AHB1Periph_GPIOA
#define IOE_I2C_SCL_SOURCE         GPIO_PinSource8
#define IOE_I2C_SCL_AF             GPIO_AF_I2C3

#define IOE_I2C_SDA_PIN            GPIO_Pin_9
#define IOE_I2C_SDA_GPIO_PORT      GPIOC
#define IOE_I2C_SDA_GPIO_CLK       RCC_AHB1Periph_GPIOC
#define IOE_I2C_SDA_SOURCE         GPIO_PinSource9
#define IOE_I2C_SDA_AF             GPIO_AF_I2C3
//}}}
//{{{  Expander defines
// The 7 bits IO Expanders addresses and chip IDs
#define IOE_ADDR                  0x82
#define STMPE811_ID               0x0811

// IO Expander Functionalities definitions
#define IOE_ADC_FCT               0x01
#define IOE_TP_FCT                0x02
#define IOE_IO_FCT                0x04

// Eval Board IO Exapander Pins definition
#define IO1_IN_ALL_PINS           (uint32_t)(MEMS_INT1_PIN | MEMS_INT2_PIN)
#define IO2_IN_ALL_PINS           (uint32_t)0x00
#define IO1_OUT_ALL_PINS          (uint32_t)(VBAT_DIV_PIN)
#define IO2_OUT_ALL_PINS          (uint32_t)(AUDIO_RESET_PIN | MII_INT_PIN)

// Interrupt source configuration definitons
#define IOE_ITSRC_TP              0x01

// Glaobal Interrupts definitions
#define IOE_GIT_GPIO              0x80
#define IOE_GIT_ADC               0x40
#define IOE_GIT_TEMP              0x20
#define IOE_GIT_FE                0x10
#define IOE_GIT_FF                0x08
#define IOE_GIT_FOV               0x04
#define IOE_GIT_FTH               0x02
#define IOE_GIT_TOUCH             0x01

// Interrupt Line output parameters
#define Polarity_Low              0x00
#define Polarity_High             0x04
#define Type_Level                0x00
#define Type_Edge                 0x02

// IO Interrupts
#define IO_IT_0                   0x01
#define IO_IT_1                   0x02
#define IO_IT_2                   0x04
#define IO_IT_3                   0x08
#define IO_IT_4                   0x10
#define IO_IT_5                   0x20
#define IO_IT_6                   0x40
#define IO_IT_7                   0x80
#define ALL_IT                    0xFF
#define IOE_TP_IT                 (uint8_t)(IO_IT_0 | IO_IT_1 | IO_IT_2)
#define IOE_INMEMS_IT             (uint8_t)(IO_IT_2 | IO_IT_3)

// Edge detection value
#define EDGE_FALLING              0x01
#define EDGE_RISING               0x02

// @brief  Global interrupt Enable bit
#define IOE_GIT_EN                0x01
//}}}
//{{{  reg defines
//  Identification registers
#define IOE_REG_CHP_ID             0x00
#define IOE_REG_ID_VER             0x02

// General Control Registers
#define IOE_REG_SYS_CTRL1          0x03
#define IOE_REG_SYS_CTRL2          0x04
#define IOE_REG_SPI_CFG            0x08

// Interrupt Control register
#define IOE_REG_INT_CTRL           0x09
#define IOE_REG_INT_EN             0x0A
#define IOE_REG_INT_STA            0x0B
#define IOE_REG_GPIO_INT_EN        0x0C
#define IOE_REG_GPIO_INT_STA       0x0D

// GPIO Registers
#define IOE_REG_GPIO_SET_PIN       0x10
#define IOE_REG_GPIO_CLR_PIN       0x11
#define IOE_REG_GPIO_MP_STA        0x12
#define IOE_REG_GPIO_DIR           0x13
#define IOE_REG_GPIO_ED            0x14
#define IOE_REG_GPIO_RE            0x15
#define IOE_REG_GPIO_FE            0x16
#define IOE_REG_GPIO_AF            0x17

// ADC Registers
#define IOE_REG_ADC_INT_EN         0x0E
#define IOE_REG_ADC_INT_STA        0x0F
#define IOE_REG_ADC_CTRL1          0x20
#define IOE_REG_ADC_CTRL2          0x21
#define IOE_REG_ADC_CAPT           0x22

#define IOE_REG_ADC_DATA_CH0       0x30
#define IOE_REG_ADC_DATA_CH1       0x32
#define IOE_REG_ADC_DATA_CH2       0x34
#define IOE_REG_ADC_DATA_CH3       0x36
#define IOE_REG_ADC_DATA_CH4       0x38
#define IOE_REG_ADC_DATA_CH5       0x3A
#define IOE_REG_ADC_DATA_CH6       0x3B
#define IOE_REG_ADC_DATA_CH7       0x3C

// TouchPanel Registers
#define IOE_REG_TP_CTRL            0x40
#define IOE_REG_TP_CFG             0x41

#define IOE_REG_WDM_TR_X           0x42
#define IOE_REG_WDM_TR_Y           0x44
#define IOE_REG_WDM_BL_X           0x46
#define IOE_REG_WDM_BL_Y           0x48

#define IOE_REG_FIFO_TH            0x4A
#define IOE_REG_FIFO_STA           0x4B
#define IOE_REG_FIFO_SIZE          0x4C

#define IOE_REG_TP_DATA_X          0x4D
#define IOE_REG_TP_DATA_Y          0x4F
#define IOE_REG_TP_DATA_Z          0x51
#define IOE_REG_TP_DATA_XYZ        0x52
#define IOE_REG_TP_FRACT_XYZ       0x56
#define IOE_REG_TP_DATA            0x57
#define IOE_REG_TP_I_DRIVE         0x58
#define IOE_REG_TP_SHIELD          0x59
//}}}
//{{{  IO pin defines
// Touch Panel Pins definition
#define TOUCH_YD                   IO_Pin_1
#define TOUCH_XD                   IO_Pin_2
#define TOUCH_YU                   IO_Pin_3
#define TOUCH_XU                   IO_Pin_4

#define TOUCH_IO_ALL               (uint32_t)(IO_Pin_1 | IO_Pin_2 | IO_Pin_3 | IO_Pin_4)

// IO Pins
#define IO_Pin_0                   0x01
#define IO_Pin_1                   0x02
#define IO_Pin_2                   0x04
#define IO_Pin_3                   0x08
#define IO_Pin_4                   0x10
#define IO_Pin_5                   0x20
#define IO_Pin_6                   0x40
#define IO_Pin_7                   0x80
#define IO_Pin_ALL                 0xFF

// IO Pin directions
#define Direction_IN               0x00
#define Direction_OUT              0x01
//}}}

static uint16_t _x = 0, _y = 0;

#define TIMEOUT_MAX  0x3000
uint32_t IOE_TimeOut = TIMEOUT_MAX;

//{{{
static uint8_t I2C_WriteDeviceReg (uint8_t regAddr, uint8_t regValue) {

  // Begin the configuration sequence
  I2C_GenerateSTART (IOE_I2C, ENABLE);

  // Test on EV5 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_SB))
    if (IOE_TimeOut-- == 0) return false;

  // Transmit the slave address and enable writing operation
  I2C_Send7bitAddress (IOE_I2C, IOE_ADDR, I2C_Direction_Transmitter);

  // Test on EV6 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_ADDR))
    if (IOE_TimeOut-- == 0)  return false;

  // Read status reg 2 to clear ADDR flag
  IOE_I2C->SR2;

  // Test on EV8_1 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_TXE))
    if (IOE_TimeOut-- == 0) return false;

  // Transmit the first address for r/w operations
  I2C_SendData (IOE_I2C, regAddr);

  // Test on EV8 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_TXE))
    if (IOE_TimeOut-- == 0) return false;

  // Prepare the reg value to be sent
  I2C_SendData(IOE_I2C, regValue);

  // Test on EV8_2 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while ((!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_TXE)) || (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_BTF)))
    if (IOE_TimeOut-- == 0) return false;

  // End the configuration sequence
  I2C_GenerateSTOP (IOE_I2C, ENABLE);

  return 0;
  }
//}}}
//{{{
static uint8_t I2C_ReadDeviceReg (uint8_t regAddr) {

  uint8_t tmp = 0;

  // Enable the I2C peripheral
  I2C_GenerateSTART (IOE_I2C, ENABLE);

   // Test on EV5 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_SB))
    if (IOE_TimeOut-- == 0) return 0xFF;
  // Disable Acknowledgement
  I2C_AcknowledgeConfig (IOE_I2C, DISABLE);

  // Transmit the slave address and enable writing operation
  I2C_Send7bitAddress (IOE_I2C, IOE_ADDR, I2C_Direction_Transmitter);

  // Test on EV6 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_ADDR))
    if (IOE_TimeOut-- == 0) return 0xFF;

  // Read status reg 2 to clear ADDR flag
  IOE_I2C->SR2;

  // Test on EV8 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_TXE))
    if (IOE_TimeOut-- == 0) return 0xFF;

  // Transmit the first address for r/w operations
  I2C_SendData (IOE_I2C, regAddr);

  // Test on EV8 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while ((!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_TXE)) || 
         (!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_BTF)))
    if (IOE_TimeOut-- == 0) return 0xFF;

  // Regenerate a start condition
  I2C_GenerateSTART (IOE_I2C, ENABLE);

  // Test on EV5 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_SB))
    if (IOE_TimeOut-- == 0) return 0xFF;

  // Transmit the slave address and enable writing operation
  I2C_Send7bitAddress (IOE_I2C, IOE_ADDR, I2C_Direction_Receiver);

  // Test on EV6 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_ADDR))
    if (IOE_TimeOut-- == 0) return 0xFF;

    // Read status reg 2 to clear ADDR flag
  IOE_I2C->SR2;

  // Test on EV7 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_RXNE))
    if (IOE_TimeOut-- == 0) return 0xFF;

  // End the configuration sequence
  I2C_GenerateSTOP (IOE_I2C, ENABLE);

  // Load the reg value
  tmp = I2C_ReceiveData (IOE_I2C);

  // Enable Acknowledgement
  I2C_AcknowledgeConfig (IOE_I2C, ENABLE);

  // Return the read value
  return tmp;
  }
//}}}
//{{{
static uint16_t I2C_ReadDataBuffer (uint8_t regAddr) {

  uint8_t IOE_BufferRX[2] = {0x00, 0x00};

  // Enable the I2C peripheral
  I2C_GenerateSTART (IOE_I2C, ENABLE);

  // Test on EV5 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_SB))
    if (IOE_TimeOut-- == 0) return 0xFF;;

  // Send device address for write
  I2C_Send7bitAddress (IOE_I2C, IOE_ADDR, I2C_Direction_Transmitter);

  // Test on EV6 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_ADDR))
    if (IOE_TimeOut-- == 0) return 0xFF;
  // Read status reg 2 to clear ADDR flag
  IOE_I2C->SR2;

  // Test on EV8 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_TXE))
    if (IOE_TimeOut-- == 0) return 0xFF;

  // Send the device's internal address to write to
  I2C_SendData (IOE_I2C, regAddr);

  // Send START condition a second time
  I2C_GenerateSTART (IOE_I2C, ENABLE);

  // Test on EV5 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_SB))
    if (IOE_TimeOut-- == 0) return 0xFF;

  // Send IO Expander address for read
  I2C_Send7bitAddress (IOE_I2C, IOE_ADDR, I2C_Direction_Receiver);

  // Test on EV6 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_ADDR))
    if (IOE_TimeOut-- == 0) return 0xFF;

  // Disable Acknowledgement and set Pos bit
  I2C_AcknowledgeConfig (IOE_I2C, DISABLE);
  I2C_NACKPositionConfig (IOE_I2C, I2C_NACKPosition_Next);

  // Read status reg 2 to clear ADDR flag
  IOE_I2C->SR2;

  // Test on EV7 and clear it
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus (IOE_I2C, I2C_FLAG_BTF))
    if (IOE_TimeOut-- == 0) return 0xFF;

  // Send STOP Condition
  I2C_GenerateSTOP (IOE_I2C, ENABLE);

  // Read the first byte from the IO Expander
  IOE_BufferRX[1] = I2C_ReceiveData (IOE_I2C);

  // Read the second byte from the IO Expander
  IOE_BufferRX[0] = I2C_ReceiveData (IOE_I2C);

  // Enable Acknowledgement and reset POS bit to be ready for another reception
  I2C_AcknowledgeConfig (IOE_I2C, ENABLE);
  I2C_NACKPositionConfig (IOE_I2C, I2C_NACKPosition_Current);

  // return the data
  return ((uint16_t) IOE_BufferRX[0] | ((uint16_t)IOE_BufferRX[1]<< 8));
}
//}}}

//{{{
static void fnctCmd (uint8_t Fct, FunctionalState NewState) {

  // Get the reg value
  uint8_t tmp = I2C_ReadDeviceReg (IOE_REG_SYS_CTRL2);

  if (NewState != DISABLE)
    // Set the Functionalities to be Enabled
    tmp &= ~(uint8_t)Fct;
  else
    // Set the Functionalities to be Disabled
    tmp |= (uint8_t)Fct;

  // Set the register value
  I2C_WriteDeviceReg (IOE_REG_SYS_CTRL2, tmp);
  }
//}}}
//{{{
static void tpConfig() {
  // touch Panel functionality
  fnctCmd (IOE_TP_FCT, ENABLE);

  // Sample Time, bit number and ADC Reference
  I2C_WriteDeviceReg (IOE_REG_ADC_CTRL1, 0x49);

  delayMs(20);

  //  ADC clock speed: 3.25 MHz
  I2C_WriteDeviceReg (IOE_REG_ADC_CTRL2, 0x01);

  // Select TSC pins in non default mode
  // Disable the selected pins alternate function
  uint8_t tmp = I2C_ReadDeviceReg(IOE_REG_GPIO_AF);
  tmp &= ~(uint8_t)TOUCH_IO_ALL;
  I2C_WriteDeviceReg (IOE_REG_GPIO_AF, tmp);

  // 2nF filter capacitor
  I2C_WriteDeviceReg (IOE_REG_TP_CFG, 0x9A);

  // single point reading
  I2C_WriteDeviceReg (IOE_REG_FIFO_TH, 0x01);

  // Write 0x01 to clear the FIFO memory content.
  I2C_WriteDeviceReg (IOE_REG_FIFO_STA, 0x01);

  // Write 0x00 to put the FIFO back into operation mode
  I2C_WriteDeviceReg (IOE_REG_FIFO_STA, 0x00);

  //  Z value: 7 fractional part and 1 whole part
  I2C_WriteDeviceReg (IOE_REG_TP_FRACT_XYZ, 0x01);

  // driving capability for TSC pins: 50mA
  I2C_WriteDeviceReg (IOE_REG_TP_I_DRIVE, 0x01);

  // Use no tracking index, touch-panel controller operation mode (XYZ) and enable the TSC
  I2C_WriteDeviceReg (IOE_REG_TP_CTRL, 0x03);

  //  Clear all the status pending bits
  I2C_WriteDeviceReg (IOE_REG_INT_STA, 0xFF);
  }
//}}}

#ifdef IT
//{{{
static void tpitConfig() {
// Enable the Global interrupt

  // Set the global interrupts to be Enabled
  uint8_t tmp = I2C_ReadDeviceReg (IOE_REG_INT_CTRL);
  tmp |= (uint8_t)IOE_GIT_EN;
  I2C_WriteDeviceReg (IOE_REG_INT_CTRL, tmp);

  // Get the current value of the INT_EN Reg
  tmp = I2C_ReadDeviceReg (IOE_REG_INT_EN);
  tmp |= (uint8_t)IOE_GIT_TOUCH | IOE_GIT_FTH | IOE_GIT_FOV;
  I2C_WriteDeviceReg (IOE_REG_INT_EN, tmp);

  // Read the GPIO_IT_STA to clear all pending bits if any
  I2C_ReadDeviceReg (IOE_REG_GPIO_INT_STA);
  }
//}}}
//{{{
static FlagStatus getGITStatus (uint8_t Global_IT) {

  // Get the Interrupt status
  __IO uint8_t tmp = I2C_ReadDeviceReg(IOE_REG_INT_STA);

  if ((tmp & (uint8_t)Global_IT) != 0)
    return SET;
  else
    return RESET;
  }
//}}}
//{{{
static void clearGITPending (uint8_t Global_IT) {

  // Write 1 to the bits that have to be cleared
  I2C_WriteDeviceReg (IOE_REG_INT_STA, Global_IT);
  }
//}}}
#endif

//{{{
static uint16_t TP_Read_X() {

  // Read x value from DATA_X Reg
  int32_t x = I2C_ReadDataBuffer (IOE_REG_TP_DATA_X);

  // x value first correction
  if(x <= 3000)
    x = 3870 - x;
  else
    x = 3800 - x;

  // x value second correction
  int32_t xr = x / 15;

  // return x position value
  if (xr <= 0)
    xr = 0;
  else if (xr > 240)
    xr = 239;

  return (uint16_t)(xr);
  }
//}}}
//{{{
static uint16_t TP_Read_Y() {

  // Read y value from DATA_Y Reg
  int32_t y = I2C_ReadDataBuffer(IOE_REG_TP_DATA_Y);

  // y value first correction
  y -= 360;

  // y value second correction
  int32_t yr = y / 11;

  // return y position value
  if(yr <= 0)
    yr = 0;
  else if (yr > 320)
    yr = 319;

  return (uint16_t)(yr);
  }
//}}}
//{{{
static uint16_t TP_Read_Z() {

  // Read z value from DATA_Z Reg
  int32_t z = I2C_ReadDataBuffer (IOE_REG_TP_DATA_Z);

  // return z position value
  if (z <= 0)
    z = 0;

  return (uint16_t)(z);
  }
//}}}

//{{{
bool getTouchDown() {

  // Check if the Touch detect event happened
  return I2C_ReadDeviceReg (IOE_REG_TP_CTRL) & 0x80;
  }
//}}}
//{{{
void getTouchPos (int16_t* x, int16_t* y, int16_t* z, int16_t xlen, uint16_t ylen) {

  uint16_t tpx , tpy;
  uint16_t xDiff, yDiff;

  tpx = TP_Read_X();
  tpy = TP_Read_Y();

  xDiff = tpx > _x ? (tpx - _x): (_x - tpx);
  yDiff = tpy > _y ? (tpy - _y): (_y - tpy);
  if (xDiff + yDiff > 5) {
    _x = tpx;
    _y = tpy;
    }

  *x = _x;
  *y = _y;
  *z = TP_Read_Z();

  // Clear the interrupt pending bit and enable the FIFO again
  I2C_WriteDeviceReg (IOE_REG_FIFO_STA, 0x01);
  I2C_WriteDeviceReg (IOE_REG_FIFO_STA, 0x00);
  }
//}}}

//{{{
bool touchInit() {

  // Enable IOE_I2C and IOE_I2C_GPIO_PORT & Alternate Function clocks
  RCC_APB1PeriphClockCmd (IOE_I2C_CLK, ENABLE);
  RCC_AHB1PeriphClockCmd (IOE_I2C_SCL_GPIO_CLK | IOE_I2C_SDA_GPIO_CLK, ENABLE);

  //RCC_APB2PeriphClockCmd (RCC_APB2Periph_SYSCFG, ENABLE);

  // Reset IOE_I2C IP
  RCC_APB1PeriphResetCmd (IOE_I2C_CLK, ENABLE);
  // Release reset signal of IOE_I2C IP
  RCC_APB1PeriphResetCmd (IOE_I2C_CLK, DISABLE);

  GPIO_PinAFConfig (IOE_I2C_SCL_GPIO_PORT, IOE_I2C_SCL_SOURCE, IOE_I2C_SCL_AF);
  GPIO_PinAFConfig (IOE_I2C_SDA_GPIO_PORT, IOE_I2C_SDA_SOURCE, IOE_I2C_SDA_AF);

  // IOE_I2C SCL and SDA pins configuration
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = IOE_I2C_SCL_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init (IOE_I2C_SCL_GPIO_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = IOE_I2C_SDA_PIN;
  GPIO_Init (IOE_I2C_SDA_GPIO_PORT, &GPIO_InitStructure);

  I2C_InitTypeDef I2C_InitStructure;
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 = 0x00;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = 100000;
  I2C_Init (IOE_I2C, &I2C_InitStructure);

  // Enable the I2C peripheral
  I2C_Cmd (IOE_I2C, ENABLE);

  if ((I2C_ReadDeviceReg(0)<<8 | I2C_ReadDeviceReg(1)) != STMPE811_ID)
    return false;

  // Power Down the IO_Expander
  I2C_WriteDeviceReg (IOE_REG_SYS_CTRL1, 0x02);

  // wait for a delay to insure Regs erasing
  delayMs (20);

  // Power On the Codec after the power off => all registers are reinitialized
  I2C_WriteDeviceReg (IOE_REG_SYS_CTRL1, 0x00);

  // IO Expander configuration - Touch Panel controller and ADC configuration
  fnctCmd (IOE_ADC_FCT, ENABLE);
  tpConfig();

  return true;
  }
//}}}
