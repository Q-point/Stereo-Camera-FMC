//----------------------------------------------------------------
//
// Design Name:         ZC702 I2C Utilities
// Target Devices:      Zynq-7
//
// Tool versions:       ISE 14.1
//
// Description:         ZC702 I2C Utilities.
//                      - I2C Multiplexer
//                      - HDMI
//						- Clock Synthesizer
//                      - PMBus
//						- FMC IPMI EEPROM
//
//----------------------------------------------------------------

#ifndef __ZC702_I2C_UTILS_H__
#define __ZC702_I2C_UTILS_H__

#include "xiicps.h"


// Turn on/off Debug messages
#ifdef I2C_DEBUG
#define  debug_printf  printf
#else
#define  debug_printf(msg, args...) do {  } while (0)
#endif


// ADV7511 video output format
#define RGB444   0
#define YCRCB422 1
#define ADV7511_VIDEO_OUT_FORMAT  RGB444

// PS I2C0 Init Parameters -> ZC702 via MIO
#define ZC702_IIC_DEVICE_ID  XPAR_XIICPS_0_DEVICE_ID
#define ZC702_IIC_SCLK_RATE  100000

// PS I2C1 Init Parameters -> FMC via EMIO
#define FMC_IIC_DEVICE_ID  XPAR_XIICPS_1_DEVICE_ID
#define FMC_IIC_SCLK_RATE  100000

// FMC slots
#define ZC702_FMC_SLOT1 1
#define ZC702_FMC_SLOT2 2


// Constants
const unsigned long long freq_HD1080P;
const unsigned long long freq_WSXGA;
const unsigned long long freq_SXGA;
const unsigned long long freq_HD720P;
const unsigned long long freq_XGA;
const unsigned long long freq_SVGA;
const unsigned long long freq_VGA;


// I2C Init Function
int iic_init( XIicPs *IicPs, u16 DeviceId, u32 ClkRate );

// HDMI Functions
int zc702_hdmi_init( XIicPs *IicPs );

void zc702_iic_mux( XIicPs *IicPs, u8 MuxSelect );


#endif // __ZC702_I2C_UTILS_H__
