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
 
#ifndef _AXI_GPIO_H
#define _AXI_GPIO_H

#include <stdio.h>
#include "xparameters.h"
#include "xgpio.h"

/************************** Constant Definitions *****************************/



#define GPIO_CAM1_RST_DEVICE_ID  XPAR_GPIO_0_DEVICE_ID
#define GPIO_CAM2_RST_DEVICE_ID  XPAR_GPIO_1_DEVICE_ID


#define RST   1

int  gpio_setup(XGpio* Gpio, uint16_t device_id);
void gpio_write(XGpio* Gpio, uint8_t pin,uint8_t value);
void gpio_dir(XGpio* Gpio, uint8_t pin,uint8_t value);
uint8_t gpio_read(XGpio* Gpio, uint8_t pin);



#endif
