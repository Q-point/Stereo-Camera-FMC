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
 
#include "AXI_GPIO.h"



int  gpio_setup(XGpio* Gpio, uint16_t device_id)
{
	int Status;

	/* Initialize the GPIO 1 driver */
	Status = XGpio_Initialize(Gpio, device_id);
	if (Status != XST_SUCCESS) {
		xil_printf("Gpio Initialization Failed\r\n");
		return XST_FAILURE;
	}
	return XST_SUCCESS;
}



void gpio_write(XGpio* Gpio, uint8_t pin,uint8_t value)
{
	XGpio_DiscreteWrite(Gpio, pin, value);
}


void gpio_dir(XGpio* Gpio, uint8_t pin,uint8_t value)
{
	XGpio_SetDataDirection(Gpio, pin, value);	// Set as output 0x00, Set as input 0x01
}


uint8_t gpio_read(XGpio* Gpio, uint8_t pin)
{
	uint32_t DataRead = XGpio_DiscreteRead(Gpio, pin);
	
	return (uint8_t) DataRead;
}



/**
 * @brief Write a logic level to the pin. The pin should be confgured as output.
 * @param pin Pin to be configured, Available pin definitons are defined in the Utitlies.h
 * @para level Logic level can be HIGH or LOW.
 * @return none
 */
//void digitalWrite(PIN_t pin, unsigned char level)
//{
//	/*
//	 * Set the direction for the pin to be output and
//	 * Enable the Output enable for the LED Pin.
//	 */
//	XGpioPs_SetDirectionPin(&Gpio, Output_Pin, 1);
//	XGpioPs_SetOutputEnablePin(&Gpio, Output_Pin, 1);
//
//	switch(level)
//	{
//		case LOW:
//			/* Set the GPIO output to be low. */
//			XGpioPs_WritePin(&Gpio, Output_Pin, 0x0);
//		case HIGH:
//			XGpioPs_WritePin(&Gpio, Output_Pin, 0x1);
//		default:
//
//	}
//}
