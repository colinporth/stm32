// F4i2c2.c
// PB10 = SCL
// PB11 = SDA
//{{{  includes
#include "i2c.h"

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_i2c.h"
//}}}

static uint8_t SlaveAddr;

//{{{
void i2cInit (uint8_t slaveAddr) {

  SlaveAddr = slaveAddr;

  // I2C Periph clock enable
  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_APB1PeriphClockCmd (RCC_APB1Periph_I2C2, ENABLE);

  // Configure I2C pins: SCL
  GPIO_InitTypeDef  GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_10 |GPIO_Pin_11; // PB10 = SCL, PB11 = SDA
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_Init (GPIOB, &GPIO_InitStructure);

  GPIO_PinAFConfig (GPIOB, GPIO_PinSource10, GPIO_AF_I2C2);
  GPIO_PinAFConfig (GPIOB, GPIO_PinSource11, GPIO_AF_I2C2);

  // I2C Init
  I2C_InitTypeDef I2C_InitStructure;
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 = 0x00;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = 100000;
  I2C_Init (I2C2, &I2C_InitStructure);

  // I2C Init
  I2C_Cmd (I2C2, ENABLE);
  }
//}}}

//{{{
bool i2cGetStatus() {

  int i2cCountDown = 100000;

  // Clear the I2C AF flag
  I2C_ClearFlag (I2C2, I2C_FLAG_AF);

  // Enable I2C acknowledgement if it is already disabled by other function
  I2C_AcknowledgeConfig (I2C2, ENABLE);

  // Transmission Phase - Send I2C START condition
  I2C_GenerateSTART (I2C2, ENABLE);

  // test I2C EV5 and clear it
  while ((!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_MODE_SELECT)) && i2cCountDown)
    i2cCountDown--;

  if (i2cCountDown == 0)
    return false;

  // Send slave address for write
  I2C_Send7bitAddress (I2C2, SlaveAddr, I2C_Direction_Transmitter);

  while ((!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) && i2cCountDown)
    i2cCountDown--;

  return (I2C_GetFlagStatus (I2C2, I2C_FLAG_AF) == 0x00) && i2cCountDown;
  }
//}}}

//{{{
uint8_t i2cReadReg (uint8_t regName) {

  // Enable I2C acknowledgement if it is already disabled by other function
  I2C_AcknowledgeConfig (I2C2, ENABLE);

  // Transmission Phase - Send I2C START condition
  I2C_GenerateSTART (I2C2, ENABLE);

  // test I2C EV5 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_MODE_SELECT)) {}

  // Send slave address for write
  I2C_Send7bitAddress (I2C2, SlaveAddr, I2C_Direction_Transmitter);

  // test I2C EV6 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {}

  // Send the specified register data pointer
  I2C_SendData (I2C2, regName);

  // test I2C EV8 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {}

  // Reception Phase - Send Restart condition
  I2C_GenerateSTART (I2C2, ENABLE);

  // test EV5 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_MODE_SELECT)) {}

  // Send slave address for read
  I2C_Send7bitAddress (I2C2, SlaveAddr, I2C_Direction_Receiver);

  // test EV6 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {}

  // test EV7 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED)) {}

  // Store I2C received data
  uint8_t regValue = I2C_ReceiveData (I2C2);

  // Disable I2C acknowledgement
  I2C_AcknowledgeConfig (I2C2, DISABLE);

  // Send I2C STOP Condition
  I2C_GenerateSTOP (I2C2, ENABLE);

  return regValue;
  }
//}}}
//{{{
uint16_t i2cReadRegWord (uint8_t regName) {

  // Enable I2C acknowledgement if it is already disabled by other function
  I2C_AcknowledgeConfig (I2C2, ENABLE);

  // Transmission Phase - Send I2C START condition
  I2C_GenerateSTART (I2C2, ENABLE);

  // test I2C EV5 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_MODE_SELECT)) {}

  // Send slave address for write
  I2C_Send7bitAddress (I2C2, SlaveAddr, I2C_Direction_Transmitter);

  // test I2C EV6 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {}

  // Send the specified register data pointer
  I2C_SendData (I2C2, regName);

  // test I2C EV8 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {}

  // Reception Phase - Send Restart condition
  I2C_GenerateSTART (I2C2, ENABLE);

  // test EV5 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_MODE_SELECT)) {}

  // Send slave address for read
  I2C_Send7bitAddress (I2C2, SlaveAddr, I2C_Direction_Receiver);

  // test EV6 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {}

  // test EV7 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED)) {}

  // Store I2C received data
  uint16_t regValue = (uint16_t)(I2C_ReceiveData (I2C2) << 8);

  // Disable I2C acknowledgement
  I2C_AcknowledgeConfig (I2C2, DISABLE);

  // Send I2C STOP Condition
  I2C_GenerateSTOP (I2C2, ENABLE);

  // test RXNE flag
  while (I2C_GetFlagStatus(I2C2, I2C_FLAG_RXNE) == RESET) {}

  // Store I2C received data
  regValue |= I2C_ReceiveData (I2C2);

  return regValue;
  }
//}}}

//{{{
void i2cWriteReg (uint8_t regName, uint8_t value) {

  // Transmission Phase - Send I2C START condition
  I2C_GenerateSTART (I2C2, ENABLE);

  // test I2C EV5 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_MODE_SELECT)) {}

  // Send slave address for write
  I2C_Send7bitAddress (I2C2, SlaveAddr, I2C_Direction_Transmitter);

  // test I2C EV6 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {}

  // Send the specified register data pointer
  I2C_SendData (I2C2, regName);

  // test I2C EV8 and clear it
  while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {}

  // Send I2C data
  I2C_SendData (I2C2, value);

  // test I2C EV8 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {}

  // Send I2C STOP Condition
  I2C_GenerateSTOP (I2C2, ENABLE);
  }
//}}}
//{{{
void i2cWriteRegWord (uint8_t regName, uint16_t value) {

  // Transmission Phase - Send I2C START condition
  I2C_GenerateSTART (I2C2, ENABLE);

  // test I2C EV5 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_MODE_SELECT)) {}

  // Send slave address for write
  I2C_Send7bitAddress (I2C2, SlaveAddr, I2C_Direction_Transmitter);

  // test I2C EV6 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {}

  // Send the specified register data pointer
  I2C_SendData (I2C2, regName);

  // test I2C EV8 and clear it
  while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {}

  // Send I2C data
  I2C_SendData (I2C2, (uint8_t)(value >> 8));

  // test I2C EV8 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {}

  // Send I2C data
  I2C_SendData (I2C2, (uint8_t)value);

  // test I2C EV8 and clear it
  while (!I2C_CheckEvent (I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {}

  // Send I2C STOP Condition
  I2C_GenerateSTOP (I2C2, ENABLE);
  }
//}}}
