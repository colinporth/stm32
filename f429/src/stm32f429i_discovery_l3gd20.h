#pragma once

#include "stm32f4xx.h"

#ifdef __cplusplus
 extern "C" {
#endif

//{{{  defines
#define L3GD20_MODE_POWERDOWN       ((uint8_t)0x00)
#define L3GD20_MODE_ACTIVE          ((uint8_t)0x08)

// OutPut_DataRate_Selection
#define L3GD20_OUTPUT_DATARATE_1    ((uint8_t)0x00)
#define L3GD20_OUTPUT_DATARATE_2    ((uint8_t)0x40)
#define L3GD20_OUTPUT_DATARATE_3    ((uint8_t)0x80)
#define L3GD20_OUTPUT_DATARATE_4    ((uint8_t)0xC0)

// Axes_Selection
#define L3GD20_X_ENABLE            ((uint8_t)0x02)
#define L3GD20_Y_ENABLE            ((uint8_t)0x01)
#define L3GD20_Z_ENABLE            ((uint8_t)0x04)
#define L3GD20_AXES_ENABLE         ((uint8_t)0x07)
#define L3GD20_AXES_DISABLE        ((uint8_t)0x00)

// BandWidth_Selection
#define L3GD20_BANDWIDTH_1         ((uint8_t)0x00)
#define L3GD20_BANDWIDTH_2         ((uint8_t)0x10)
#define L3GD20_BANDWIDTH_3         ((uint8_t)0x20)
#define L3GD20_BANDWIDTH_4         ((uint8_t)0x30)

// Full_Scale_Selection
#define L3GD20_FULLSCALE_250               ((uint8_t)0x00)
#define L3GD20_FULLSCALE_500               ((uint8_t)0x10)
#define L3GD20_FULLSCALE_2000              ((uint8_t)0x20)

// Block_Data_Update
#define L3GD20_BlockDataUpdate_Continous   ((uint8_t)0x00)
#define L3GD20_BlockDataUpdate_Single      ((uint8_t)0x80)

// Endian_Data_selection
#define L3GD20_BLE_LSB                     ((uint8_t)0x00)
#define L3GD20_BLE_MSB                     ((uint8_t)0x40)

// High_Pass_Filter_status
#define L3GD20_HIGHPASSFILTER_DISABLE      ((uint8_t)0x00)
#define L3GD20_HIGHPASSFILTER_ENABLE       ((uint8_t)0x10)

// INT1_Interrupt_status
#define L3GD20_INT1INTERRUPT_DISABLE       ((uint8_t)0x00)
#define L3GD20_INT1INTERRUPT_ENABLE    ((uint8_t)0x80)

// INT2_Interrupt_status
#define L3GD20_INT2INTERRUPT_DISABLE       ((uint8_t)0x00)
#define L3GD20_INT2INTERRUPT_ENABLE    ((uint8_t)0x08)

// INT1_Interrupt_ActiveEdge
#define L3GD20_INT1INTERRUPT_LOW_EDGE      ((uint8_t)0x20)
#define L3GD20_INT1INTERRUPT_HIGH_EDGE     ((uint8_t)0x00)

// Boot_Mode_selection
#define L3GD20_BOOT_NORMALMODE             ((uint8_t)0x00)
#define L3GD20_BOOT_REBOOTMEMORY           ((uint8_t)0x80)

// High_Pass_Filter_Mode
#define L3GD20_HPM_NORMAL_MODE_RES         ((uint8_t)0x00)
#define L3GD20_HPM_REF_SIGNAL              ((uint8_t)0x10)
#define L3GD20_HPM_NORMAL_MODE             ((uint8_t)0x20)
#define L3GD20_HPM_AUTORESET_INT           ((uint8_t)0x30)

// High_Pass_CUT OFF_Frequency
#define L3GD20_HPFCF_0              0x00
#define L3GD20_HPFCF_1              0x01
#define L3GD20_HPFCF_2              0x02
#define L3GD20_HPFCF_3              0x03
#define L3GD20_HPFCF_4              0x04
#define L3GD20_HPFCF_5              0x05
#define L3GD20_HPFCF_6              0x06
#define L3GD20_HPFCF_7              0x07
#define L3GD20_HPFCF_8              0x08
#define L3GD20_HPFCF_9              0x09
//}}}

//{{{  L3GD20_InitTypeDef
typedef struct {
  uint8_t Power_Mode;                         /* Power-down/Sleep/Normal Mode */
  uint8_t Output_DataRate;                    /* OUT data rate */
  uint8_t Axes_Enable;                        /* Axes enable */
  uint8_t Band_Width;                         /* Bandwidth selection */
  uint8_t BlockData_Update;                   /* Block Data Update */
  uint8_t Endianness;                         /* Endian Data selection */
  uint8_t Full_Scale;                         /* Full Scale selection */
}L3GD20_InitTypeDef;
//}}}
//{{{  L3GD20_FilterConfigTypeDef
typedef struct {
  uint8_t HighPassFilter_Mode_Selection;      /* Internal filter mode */
  uint8_t HighPassFilter_CutOff_Frequency;    /* High pass filter cut-off frequency */
}L3GD20_FilterConfigTypeDef;
//}}}
//{{{  L3GD20_InterruptConfigTypeDef
typedef struct {
  uint8_t Latch_Request;                      /* Latch interrupt request into CLICK_SRC register */
  uint8_t Interrupt_Axes;                     /* X, Y, Z Axes Interrupts */
  uint8_t Interrupt_ActiveEdge;               /*  Interrupt Active edge */
}L3GD20_InterruptConfigTypeDef;
//}}}

// STM32F429I-DISCO_L3GD20_Exported_Functions
// Sensor Configuration Functions */
void L3GD20_Init(L3GD20_InitTypeDef *L3GD20_InitStruct);
void L3GD20_RebootCmd();

//INT1 Interrupt Configuration Functions */
void L3GD20_INT1InterruptCmd(uint8_t InterruptState);
void L3GD20_INT2InterruptCmd(uint8_t InterruptState);
void L3GD20_INT1InterruptConfig(L3GD20_InterruptConfigTypeDef *L3GD20_IntConfigStruct);
uint8_t L3GD20_GetDataStatus();

// High Pass Filter Configuration Functions */
void L3GD20_FilterConfig(L3GD20_FilterConfigTypeDef *L3GD20_FilterStruct);
void L3GD20_FilterCmd(uint8_t HighPassFilterState);
void L3GD20_Write(uint8_t* pBuffer, uint8_t WriteAddr, uint16_t NumByteToWrite);
void L3GD20_Read(uint8_t* pBuffer, uint8_t ReadAddr, uint16_t NumByteToRead);

/* USER Callbacks: This is function for which prototype only is declared in
   MEMS accelerometre driver and that should be implemented into user applicaiton. */
/* L3GD20_TIMEOUT_UserCallback() function is called whenever a timeout condition
   occure during communication (waiting transmit data register empty flag(TXE)
   or waiting receive data register is not empty flag (RXNE)).
   You can use the default timeout callback implementation by uncommenting the
   define USE_DEFAULT_TIMEOUT_CALLBACK in stm32f429i_discovery_l3gd20.h file.
   Typically the user implementation of this callback should reset MEMS peripheral
   and re-initialize communication or in worst case reset all the application. */
uint32_t L3GD20_TIMEOUT_UserCallback();

#ifdef __cplusplus
}
#endif
