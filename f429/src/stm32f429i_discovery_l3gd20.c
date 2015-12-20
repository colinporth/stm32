//{{{  includes
#include "stm32f429i_discovery_l3gd20.h"

#include "stm32f4xx.h"
#include "stm32f4xx_spi.h"
#include "stm32f4xx_rcc.h"
//}}}
//{{{  L3GD20 SPI Interface pins
#define L3GD20_SPI                       SPI5
#define L3GD20_SPI_CLK                   RCC_APB2Periph_SPI5

#define L3GD20_SPI_SCK_PIN               GPIO_Pin_7                  /* PF.07 */
#define L3GD20_SPI_SCK_GPIO_PORT         GPIOF                       /* GPIOF */
#define L3GD20_SPI_SCK_GPIO_CLK          RCC_AHB1Periph_GPIOF
#define L3GD20_SPI_SCK_SOURCE            GPIO_PinSource7
#define L3GD20_SPI_SCK_AF                GPIO_AF_SPI5

#define L3GD20_SPI_MISO_PIN              GPIO_Pin_8                  /* PF.08 */
#define L3GD20_SPI_MISO_GPIO_PORT        GPIOF                       /* GPIOF */
#define L3GD20_SPI_MISO_GPIO_CLK         RCC_AHB1Periph_GPIOF
#define L3GD20_SPI_MISO_SOURCE           GPIO_PinSource8
#define L3GD20_SPI_MISO_AF               GPIO_AF_SPI5

#define L3GD20_SPI_MOSI_PIN              GPIO_Pin_9                  /* PF.09 */
#define L3GD20_SPI_MOSI_GPIO_PORT        GPIOF                       /* GPIOF */
#define L3GD20_SPI_MOSI_GPIO_CLK         RCC_AHB1Periph_GPIOF
#define L3GD20_SPI_MOSI_SOURCE           GPIO_PinSource9
#define L3GD20_SPI_MOSI_AF               GPIO_AF_SPI5

#define L3GD20_SPI_CS_PIN                GPIO_Pin_1                  /* PC.01 */
#define L3GD20_SPI_CS_GPIO_PORT          GPIOC                       /* GPIOC */
#define L3GD20_SPI_CS_GPIO_CLK           RCC_AHB1Periph_GPIOC

#define L3GD20_SPI_INT1_PIN              GPIO_Pin_1                  /* PA.01 */
#define L3GD20_SPI_INT1_GPIO_PORT        GPIOA                       /* GPIOA */
#define L3GD20_SPI_INT1_GPIO_CLK         RCC_AHB1Periph_GPIOA
#define L3GD20_SPI_INT1_EXTI_LINE        EXTI_Line1
#define L3GD20_SPI_INT1_EXTI_PORT_SOURCE EXTI_PortSourceGPIOA
#define L3GD20_SPI_INT1_EXTI_PIN_SOURCE  EXTI_PinSource1
#define L3GD20_SPI_INT1_EXTI_IRQn        EXTI1_IRQn

#define L3GD20_SPI_INT2_PIN              GPIO_Pin_2                  /* PA.02 */
#define L3GD20_SPI_INT2_GPIO_PORT        GPIOA                       /* GPIOA */
#define L3GD20_SPI_INT2_GPIO_CLK         RCC_AHB1Periph_GPIOA
#define L3GD20_SPI_INT2_EXTI_LINE        EXTI_Line2
#define L3GD20_SPI_INT2_EXTI_PORT_SOURCE EXTI_PortSourceGPIOA
#define L3GD20_SPI_INT2_EXTI_PIN_SOURCE  EXTI_PinSource2
#define L3GD20_SPI_INT2_EXTI_IRQn        EXTI2_IRQn
//}}}
//{{{  register defines
#define L3GD20_WHO_AM_I_ADDR          0x0F  /* device identification register */

#define L3GD20_CTRL_REG1_ADDR         0x20  /* Control register 1 */
#define L3GD20_CTRL_REG2_ADDR         0x21  /* Control register 2 */
#define L3GD20_CTRL_REG3_ADDR         0x22  /* Control register 3 */
#define L3GD20_CTRL_REG4_ADDR         0x23  /* Control register 4 */
#define L3GD20_CTRL_REG5_ADDR         0x24  /* Control register 5 */

#define L3GD20_REFERENCE_REG_ADDR     0x25  /* Reference register */

#define L3GD20_OUT_TEMP_ADDR          0x26  /* Out temp register */

#define L3GD20_STATUS_REG_ADDR        0x27  /* Status register */

#define L3GD20_OUT_X_L_ADDR           0x28  /* Output Register X */
#define L3GD20_OUT_X_H_ADDR           0x29  /* Output Register X */
#define L3GD20_OUT_Y_L_ADDR           0x2A  /* Output Register Y */
#define L3GD20_OUT_Y_H_ADDR           0x2B  /* Output Register Y */
#define L3GD20_OUT_Z_L_ADDR           0x2C  /* Output Register Z */
#define L3GD20_OUT_Z_H_ADDR           0x2D  /* Output Register Z */

#define L3GD20_FIFO_CTRL_REG_ADDR     0x2E  /* Fifo control Register */
#define L3GD20_FIFO_SRC_REG_ADDR      0x2F  /* Fifo src Register */

#define L3GD20_INT1_CFG_ADDR          0x30  /* Interrupt 1 configuration Register */
#define L3GD20_INT1_SRC_ADDR          0x31  /* Interrupt 1 source Register */
#define L3GD20_INT1_TSH_XH_ADDR       0x32  /* Interrupt 1 Threshold X register */
#define L3GD20_INT1_TSH_XL_ADDR       0x33  /* Interrupt 1 Threshold X register */
#define L3GD20_INT1_TSH_YH_ADDR       0x34  /* Interrupt 1 Threshold Y register */
#define L3GD20_INT1_TSH_YL_ADDR       0x35  /* Interrupt 1 Threshold Y register */
#define L3GD20_INT1_TSH_ZH_ADDR       0x36  /* Interrupt 1 Threshold Z register */
#define L3GD20_INT1_TSH_ZL_ADDR       0x37  /* Interrupt 1 Threshold Z register */
#define L3GD20_INT1_DURATION_ADDR     0x38  /* Interrupt 1 DURATION register */
//}}}
#define READWRITE_CMD              ((uint8_t)0x80)
#define MULTIPLEBYTE_CMD           ((uint8_t)0x40)
#define DUMMY_BYTE                 ((uint8_t)0x00) // Dummy Byte Send by SPI Master to generate Clock to slave

/* Uncomment the following line to use the default L3GD20_TIMEOUT_UserCallback()
   function implemented in stm32f429i_discovery_lgd20.c file.
   L3GD20_TIMEOUT_UserCallback() function is called whenever a timeout condition
   occure during communication (waiting transmit data register empty flag(TXE)
   or waiting receive data register is not empty flag (RXNE)). */
/* #define USE_DEFAULT_TIMEOUT_CALLBACK */

/* Maximum Timeout values for flags waiting loops. These timeouts are not based
   on accurate values, they just guarantee that the application will not remain
   stuck if the SPI communication is corrupted.
   You may modify these timeout values depending on CPU frequency and application
   conditions (interrupts routines ...). */
#define L3GD20_FLAG_TIMEOUT ((uint32_t)0x1000)
#define I_AM_L3GD20 ((uint8_t)0xD4)

// STM32F429I-DISCO_L3GD20_Exported_Macros
#define L3GD20_CS_LOW() GPIO_ResetBits (L3GD20_SPI_CS_GPIO_PORT, L3GD20_SPI_CS_PIN)
#define L3GD20_CS_HIGH() GPIO_SetBits (L3GD20_SPI_CS_GPIO_PORT, L3GD20_SPI_CS_PIN)

__IO uint32_t  L3GD20Timeout = L3GD20_FLAG_TIMEOUT;

static uint8_t L3GD20_SendByte(uint8_t byte);
static void L3GD20_LowLevel_Init();

#ifdef USE_DEFAULT_TIMEOUT_CALLBACK
//{{{
uint32_t L3GD20_TIMEOUT_UserCallback()
{
  /* Block communication and all processes */
  while (1)
  {
  }
}
//}}}
#endif

//{{{
static void L3GD20_LowLevel_Init()
{
  GPIO_InitTypeDef GPIO_InitStructure;
  SPI_InitTypeDef  SPI_InitStructure;

  /* Enable the SPI periph */
  RCC_APB2PeriphClockCmd(L3GD20_SPI_CLK, ENABLE);

  /* Enable SCK, MOSI and MISO GPIO clocks */
  RCC_AHB1PeriphClockCmd(L3GD20_SPI_SCK_GPIO_CLK | L3GD20_SPI_MISO_GPIO_CLK | L3GD20_SPI_MOSI_GPIO_CLK, ENABLE);

  /* Enable CS  GPIO clock */
  RCC_AHB1PeriphClockCmd(L3GD20_SPI_CS_GPIO_CLK, ENABLE);

  /* Enable INT1 GPIO clock */
  RCC_AHB1PeriphClockCmd(L3GD20_SPI_INT1_GPIO_CLK, ENABLE);

  /* Enable INT2 GPIO clock */
  RCC_AHB1PeriphClockCmd(L3GD20_SPI_INT2_GPIO_CLK, ENABLE);

  GPIO_PinAFConfig(L3GD20_SPI_SCK_GPIO_PORT, L3GD20_SPI_SCK_SOURCE, L3GD20_SPI_SCK_AF);
  GPIO_PinAFConfig(L3GD20_SPI_MISO_GPIO_PORT, L3GD20_SPI_MISO_SOURCE, L3GD20_SPI_MISO_AF);
  GPIO_PinAFConfig(L3GD20_SPI_MOSI_GPIO_PORT, L3GD20_SPI_MOSI_SOURCE, L3GD20_SPI_MOSI_AF);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;//GPIO_PuPd_DOWN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  /* SPI SCK pin configuration */
  GPIO_InitStructure.GPIO_Pin = L3GD20_SPI_SCK_PIN;
  GPIO_Init(L3GD20_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);

  /* SPI  MOSI pin configuration */
  GPIO_InitStructure.GPIO_Pin =  L3GD20_SPI_MOSI_PIN;
  GPIO_Init(L3GD20_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

  /* SPI MISO pin configuration */
  GPIO_InitStructure.GPIO_Pin = L3GD20_SPI_MISO_PIN;
  GPIO_Init(L3GD20_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);

  /* SPI configuration -------------------------------------------------------*/
  SPI_I2S_DeInit(L3GD20_SPI);
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_Init(L3GD20_SPI, &SPI_InitStructure);

  /* Enable SPI1  */
  SPI_Cmd(L3GD20_SPI, ENABLE);

  /* Configure GPIO PIN for Lis Chip select */
  GPIO_InitStructure.GPIO_Pin = L3GD20_SPI_CS_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(L3GD20_SPI_CS_GPIO_PORT, &GPIO_InitStructure);

  /* Deselect : Chip Select high */
  GPIO_SetBits(L3GD20_SPI_CS_GPIO_PORT, L3GD20_SPI_CS_PIN);

  /* Configure GPIO PINs to detect Interrupts */
  GPIO_InitStructure.GPIO_Pin = L3GD20_SPI_INT1_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(L3GD20_SPI_INT1_GPIO_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = L3GD20_SPI_INT2_PIN;
  GPIO_Init(L3GD20_SPI_INT2_GPIO_PORT, &GPIO_InitStructure);
}
//}}}
//{{{
static uint8_t L3GD20_SendByte (uint8_t byte)
{
  /* Loop while DR register in not empty */
  L3GD20Timeout = L3GD20_FLAG_TIMEOUT;
  while (SPI_I2S_GetFlagStatus(L3GD20_SPI, SPI_I2S_FLAG_TXE) == RESET)
  {
    if((L3GD20Timeout--) == 0) return L3GD20_TIMEOUT_UserCallback();
  }

  /* Send a Byte through the SPI peripheral */
  SPI_I2S_SendData(L3GD20_SPI, (uint16_t)byte);
  /* Wait to receive a Byte */
  L3GD20Timeout = L3GD20_FLAG_TIMEOUT;
  while (SPI_I2S_GetFlagStatus(L3GD20_SPI, SPI_I2S_FLAG_RXNE) == RESET)
  {
    if((L3GD20Timeout--) == 0) return L3GD20_TIMEOUT_UserCallback();
  }

  /* Return the Byte read from the SPI bus */
  return (uint8_t)SPI_I2S_ReceiveData(L3GD20_SPI);
}
//}}}

//{{{
void L3GD20_RebootCmd()
{
  uint8_t tmpreg;

  /* Read CTRL_REG5 register */
  L3GD20_Read(&tmpreg, L3GD20_CTRL_REG5_ADDR, 1);

  /* Enable or Disable the reboot memory */
  tmpreg |= L3GD20_BOOT_REBOOTMEMORY;

  /* Write value to MEMS CTRL_REG5 regsister */
  L3GD20_Write(&tmpreg, L3GD20_CTRL_REG5_ADDR, 1);
}
//}}}

//{{{
void L3GD20_INT1InterruptConfig (L3GD20_InterruptConfigTypeDef *L3GD20_IntConfigStruct)
{
  uint8_t ctrl_cfr = 0x00, ctrl3 = 0x00;

  /* Read INT1_CFG register */
  L3GD20_Read(&ctrl_cfr, L3GD20_INT1_CFG_ADDR, 1);

  /* Read CTRL_REG3 register */
  L3GD20_Read(&ctrl3, L3GD20_CTRL_REG3_ADDR, 1);

  ctrl_cfr &= 0x80;

  ctrl3 &= 0xDF;

  /* Configure latch Interrupt request and axe interrupts */
  ctrl_cfr |= (uint8_t)(L3GD20_IntConfigStruct->Latch_Request| \
                   L3GD20_IntConfigStruct->Interrupt_Axes);

  ctrl3 |= (uint8_t)(L3GD20_IntConfigStruct->Interrupt_ActiveEdge);

  /* Write value to MEMS INT1_CFG register */
  L3GD20_Write(&ctrl_cfr, L3GD20_INT1_CFG_ADDR, 1);

  /* Write value to MEMS CTRL_REG3 register */
  L3GD20_Write(&ctrl3, L3GD20_CTRL_REG3_ADDR, 1);
}
//}}}
//{{{
void L3GD20_INT1InterruptCmd (uint8_t InterruptState)
{
  uint8_t tmpreg;

  /* Read CTRL_REG3 register */
  L3GD20_Read(&tmpreg, L3GD20_CTRL_REG3_ADDR, 1);

  tmpreg &= 0x7F;
  tmpreg |= InterruptState;

  /* Write value to MEMS CTRL_REG3 regsister */
  L3GD20_Write(&tmpreg, L3GD20_CTRL_REG3_ADDR, 1);
}
//}}}
//{{{
void L3GD20_INT2InterruptCmd (uint8_t InterruptState)
{
  uint8_t tmpreg;

  /* Read CTRL_REG3 register */
  L3GD20_Read(&tmpreg, L3GD20_CTRL_REG3_ADDR, 1);

  tmpreg &= 0xF7;
  tmpreg |= InterruptState;

  /* Write value to MEMS CTRL_REG3 regsister */
  L3GD20_Write(&tmpreg, L3GD20_CTRL_REG3_ADDR, 1);
}
//}}}

//{{{
void L3GD20_FilterConfig (L3GD20_FilterConfigTypeDef *L3GD20_FilterStruct)
{
  uint8_t tmpreg;

  /* Read CTRL_REG2 register */
  L3GD20_Read(&tmpreg, L3GD20_CTRL_REG2_ADDR, 1);

  tmpreg &= 0xC0;

  /* Configure MEMS: mode and cutoff frquency */
  tmpreg |= (uint8_t) (L3GD20_FilterStruct->HighPassFilter_Mode_Selection |\
                      L3GD20_FilterStruct->HighPassFilter_CutOff_Frequency);

  /* Write value to MEMS CTRL_REG2 regsister */
  L3GD20_Write(&tmpreg, L3GD20_CTRL_REG2_ADDR, 1);
}
//}}}
//{{{
void L3GD20_FilterCmd (uint8_t HighPassFilterState)
 {
  uint8_t tmpreg;

  /* Read CTRL_REG5 register */
  L3GD20_Read(&tmpreg, L3GD20_CTRL_REG5_ADDR, 1);

  tmpreg &= 0xEF;

  tmpreg |= HighPassFilterState;

  /* Write value to MEMS CTRL_REG5 regsister */
  L3GD20_Write(&tmpreg, L3GD20_CTRL_REG5_ADDR, 1);
}
//}}}

//{{{
uint8_t L3GD20_GetDataStatus()
{
  uint8_t tmpreg;

  /* Read STATUS_REG register */
  L3GD20_Read(&tmpreg, L3GD20_STATUS_REG_ADDR, 1);

  return tmpreg;
}
//}}}

//{{{
void L3GD20_Write (uint8_t* pBuffer, uint8_t WriteAddr, uint16_t NumByteToWrite)
{
  /* Configure the MS bit:
       - When 0, the address will remain unchanged in multiple read/write commands.
       - When 1, the address will be auto incremented in multiple read/write commands.
  */
  if(NumByteToWrite > 0x01)
  {
    WriteAddr |= (uint8_t)MULTIPLEBYTE_CMD;
  }
  /* Set chip select Low at the start of the transmission */
  L3GD20_CS_LOW();

  /* Send the Address of the indexed register */
  L3GD20_SendByte(WriteAddr);
  /* Send the data that will be written into the device (MSB First) */
  while(NumByteToWrite >= 0x01)
  {
    L3GD20_SendByte(*pBuffer);
    NumByteToWrite--;
    pBuffer++;
  }

  /* Set chip select High at the end of the transmission */
  L3GD20_CS_HIGH();
}
//}}}
//{{{
void L3GD20_Read (uint8_t* pBuffer, uint8_t ReadAddr, uint16_t NumByteToRead)
{
  if(NumByteToRead > 0x01)
  {
    ReadAddr |= (uint8_t)(READWRITE_CMD | MULTIPLEBYTE_CMD);
  }
  else
  {
    ReadAddr |= (uint8_t)READWRITE_CMD;
  }
  /* Set chip select Low at the start of the transmission */
  L3GD20_CS_LOW();

  /* Send the Address of the indexed register */
  L3GD20_SendByte(ReadAddr);

  /* Receive the data that will be read from the device (MSB First) */
  while(NumByteToRead > 0x00)
  {
    /* Send dummy byte (0x00) to generate the SPI clock to L3GD20 (Slave device) */
    *pBuffer = L3GD20_SendByte(DUMMY_BYTE);
    NumByteToRead--;
    pBuffer++;
  }

  /* Set chip select High at the end of the transmission */
  L3GD20_CS_HIGH();
}
//}}}

//{{{
void L3GD20_Init (L3GD20_InitTypeDef *L3GD20_InitStruct)
{
  uint8_t ctrl1 = 0x00, ctrl4 = 0x00;

  /* Configure the low level interface ---------------------------------------*/
  L3GD20_LowLevel_Init();

  /* Configure MEMS: data rate, power mode, full scale and axes */
  ctrl1 |= (uint8_t) (L3GD20_InitStruct->Power_Mode | L3GD20_InitStruct->Output_DataRate | \
                    L3GD20_InitStruct->Axes_Enable | L3GD20_InitStruct->Band_Width);

  ctrl4 |= (uint8_t) (L3GD20_InitStruct->BlockData_Update | L3GD20_InitStruct->Endianness | \
                    L3GD20_InitStruct->Full_Scale);

  /* Write value to MEMS CTRL_REG1 regsister */
  L3GD20_Write(&ctrl1, L3GD20_CTRL_REG1_ADDR, 1);

  /* Write value to MEMS CTRL_REG4 regsister */
  L3GD20_Write(&ctrl4, L3GD20_CTRL_REG4_ADDR, 1);
}
//}}}
