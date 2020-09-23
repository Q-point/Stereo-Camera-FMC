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
 

#ifndef SRC_PCA9548_C_
#define SRC_PCA9548_C_

#include "PCA9548.h"

extern XIicPs Iic_inst0;

int PCA9548_readRegMux(void)
{
  u8 recvBuffer[1] = {0 };
  u8 result = 0x0;
  int Status = 0;
  while (XIicPs_BusIsBusy( & Iic_inst0));
  Status = XIicPs_MasterRecvPolled( & Iic_inst0, recvBuffer, 1, PCA9548_ADDRESS);
  //wait a while
  if (Status != XST_SUCCESS) {
    print("FAILED RECV\n");
    return XST_FAILURE;
  }

  result = (u8)(recvBuffer[0]);
  xil_printf("Data: %x\n", result);

  return result; //this thing return the data

}

int PCA9548_writeRegMux(u8 val)
{
  u8 buffer[1] = {
    (u8)(val)
  };
  int Status = 0;
  while (XIicPs_BusIsBusy( & Iic_inst0));
  Status = XIicPs_MasterSendPolled( & Iic_inst0, buffer, 1, PCA9548_ADDRESS);
  //wait a while
  if (Status != XST_SUCCESS) {
    print("FAILED SUMTING\n");
    return XST_FAILURE;
  }
  //	while (XIicPs_BusIsBusy(&mux_instance)) {
  /* NOP */
  //}
  return 0; //no useful return here
}

#endif /* SRC_PCA9548_C_ */
