/*
AUTHOR: dhq     

DESCRIPTION:

CHANGE HISTORY:
23 Aug 2020		dhq
	Initial version

MIT License

Copyright (c) 2020 dhq 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */
 
#ifndef SRC_PCA9548_H_
#define SRC_PCA9548_H_

#include <stdint.h>

#include "platform.h"
#include "xil_printf.h"

#include "xiicps.h"

#define PCA9548_ADDRESS  	0x74

// ZC702 Mux Selections
#define ZC702_I2C_SELECT_USRCLK    0x01
#define ZC702_I2C_SELECT_HDMI      0x02
#define ZC702_I2C_SELECT_EEPROM    0x04
#define ZC702_I2C_SELECT_EXPANDER  0x08
#define ZC702_I2C_SELECT_RTC       0x10
#define ZC702_I2C_SELECT_FMC1      0x20
#define ZC702_I2C_SELECT_FMC2      0x40
#define ZC702_I2C_SELECT_PMBUS     0x80

int PCA9548_readRegMux(void);
int PCA9548_writeRegMux(u8 val);


#endif /* SRC_PCA9548_H_ */
