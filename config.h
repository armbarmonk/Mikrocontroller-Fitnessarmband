//*******************************************************************
/*!
\file   config.h
\author Thomas Breuer
\date   23.02.20232
\brief  Board specific configuration
*/

//*******************************************************************
/*
Board:    STM32-Nucleo32-L432

\see STM32-Nucleo32-L432/board_pinout.txt
*/

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
//
// SETUP:
// ======


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

//*******************************************************************
using namespace EmbSysLib::Hw;
using namespace EmbSysLib::Dev;
using namespace EmbSysLib::Ctrl;
using namespace EmbSysLib::Mod;

//*******************************************************************
PinConfig::MAP PinConfig::table[] =
{
  // UART
  USART2_TX_PA2,
  USART2_RX_PA15,

  I2C1_SCL_PB6,
  I2C1_SDA_PB7,

  END_OF_TABLE
};

//-------------------------------------------------------------------
// Port
//-------------------------------------------------------------------
Port_Mcu   portB( Port_Mcu::PB );

//-------------------------------------------------------------------
// UART
//-------------------------------------------------------------------
Uart_Mcu   uart ( Uart_Mcu::USART_2, 9600, 255, 255 );

Terminal   terminal( uart, 255,255,"# +" );

I2Cmaster_Mcu i2cBus( I2Cmaster_Mcu::I2C_1, 400/*kHz*/ );

//*******************************************************************
#include "Hardware/Peripheral/Display/DisplayGraphic_SSD1306i2c.cpp"

#include "Resource/Font/Font_8x12.h"

DisplayGraphic_SSD1306i2c disp( i2cBus,
                                false,
                                fontFont_8x12,
                                1 );

ScreenGraphic screen( disp );

//-------------------------------------------------------------------
// Digital
//-------------------------------------------------------------------
Digital    led_A( portB, 3, Digital::Out,  0 ); // LD3 (green)

Digital    btn_A( portB, 4, Digital::InPU, 1 ); // BTN (ext)
Digital    btn_B( portB, 5, Digital::InPU, 1 ); // <<< NEU >>> zweite Taste (PB5)

