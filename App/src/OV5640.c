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
 
#include "xparameters.h"
#include "sleep.h"
#include "xiicps.h"
#include "xil_printf.h"
#include "OV5640.h"
#include "Delay.h"
#include "ov5640af.h"

int OV5640_WriteReg(XIicPs* IicPs, uint16_t addr, uint8_t data)
{
    u8 SendBuffer[3];
    SendBuffer[0] = (addr >> 8) & 0xff;
    SendBuffer[1] = addr & 0xff;
    SendBuffer[2] = data & 0xff;
    XIicPs_MasterSendPolled(IicPs, SendBuffer, 3, OV5640_ADDR);
    return XST_SUCCESS;
}

int OV5640_ReadReg(XIicPs* IicPs,uint16_t reg)
{
	u8 buff[2];

	buff[0] = reg;
	XIicPs_MasterSendPolled(IicPs, buff, 1, OV5640_ADDR);
	while(XIicPs_BusIsBusy(IicPs)){
		/* NOP */
	}

	XIicPs_MasterRecvPolled(IicPs, buff, 1, OV5640_ADDR);
	while(XIicPs_BusIsBusy(IicPs)){
		/* NOP */
	}

	return buff[0];
}


int OV5640_Init(XIicPs* IicPs, u16 device_id, u32 sclk_rate) {
    int status = 0;
    XIicPs_Config *config;
    config = XIicPs_LookupConfig(device_id);
    if (NULL == config) {
    	xil_printf("Configuration lookup failed %d\r\n", status);
        return XST_FAILURE;
    }
    status = XIicPs_CfgInitialize(IicPs, config, config->BaseAddress);
    if (status != XST_SUCCESS) {
    	xil_printf("Initialization failed %d\r\n", status);
        return XST_FAILURE;
    }
    /*
     * Perform a self-test to ensure that the hardware was built correctly.
     */
    status = XIicPs_SelfTest(IicPs);
    if (status != XST_SUCCESS) {
    	xil_printf("Self-test failed %d\r\n", status);
        return XST_FAILURE;
    }
    /*
     * Set the IIC serial clock rate.
     */
    XIicPs_SetSClk(IicPs, sclk_rate);
    return XST_SUCCESS;
}

int OV5640_Configuration(XIicPs* IicPs, u8 camera_id)
{
    if (camera_id == CONFIG_OV5640) {
        int length = sizeof(ov5640_config2 ) / sizeof(camera_config_word_t);
        xil_printf("*I*     Camera id: OV5640, configuration data length: %d\n", length);
        for(int i = 0; i < length; i++)
        	OV5640_WriteReg(IicPs, ov5640_config2 [i].addr, ov5640_config2 [i].data);
        return XST_SUCCESS;
    }
    else {
        xil_printf("*E*     Unknown camera id specified: %d\n", camera_id);
        return XST_FAILURE;
    }
}



uint8_t OV5640_Setup(XIicPs* IicPs)
{
	uint16_t i=0;
	//uint16_t reg;


	delay_ms(30);

//	reg=OV5640_ReadReg(IicPs,OV5640_CHIPIDH);
//	reg<<=8;
//	reg|=OV5640_ReadReg(IicPs,OV5640_CHIPIDL);
//	if(reg!=OV5640_ID)
//	{
//		xil_printf("ID: %d \r\n",reg);
//		return 1;
//	}
//	OV5640_WriteReg(IicPs,0x3103,0X11);	//system clock from pad, bit[1]
//	OV5640_WriteReg(IicPs,0X3008,0X82);
//	delay_ms(10);

	for(i=0;i<sizeof(ov5640_init_reg_tbl)/4;i++)
	{
		OV5640_WriteReg(IicPs,ov5640_init_reg_tbl[i][0],ov5640_init_reg_tbl[i][1]);
	}

	return 0x00; 	//ok
}



//void Camera_Set_Reg(XIicPs *InstancePtr, struct reginfo *regarray) {
//    int i = 0;
//    int length = sizeof(regarray) / sizeof(reginfo);
//
//    while (regarray[i].reg != SEQUENCE_END) {
//    	OV5640_WriteReg(regarray[i].reg, regarray[i].val);
//		i++;
//	}
//}

//int Camera_Init(XIicPs *InstancePtr)
//{
//	Camera_Set_Reg(InstancePtr,sensor_OV5640_VGA_init_data);
//	return 0;
//}


void OV5640_YUV_Setup(XIicPs* IicPs)
{
	//YCbCr/Compression Reference Settings

	//15fps VGA YUV output
	// 24MHz input clock, 24MHz PCLK
	OV5640_WriteReg(IicPs,0x3103, 0x11); // system clock from pad, bit[1]
	OV5640_WriteReg(IicPs,0x3008, 0x82); // software reset, bit[7]
	// delay 5ms
	OV5640_WriteReg(IicPs,0x3008, 0x42); // software power down, bit[6]
	OV5640_WriteReg(IicPs,0x3103, 0x03); // system clock from PLL, bit[1]
	OV5640_WriteReg(IicPs,0x3017, 0xff); // FREX, Vsync, HREF, PCLK, D[9:6] output enable
	OV5640_WriteReg(IicPs,0x3018, 0xff); // D[5:0], GPIO[1:0] output enable
	OV5640_WriteReg(IicPs,0x3034, 0x1a); // MIPI 10-bit
	OV5640_WriteReg(IicPs,0x3037, 0x13); // PLL root divider, bit[4], PLL pre-divider, bit[3:0]
	OV5640_WriteReg(IicPs,0x3108, 0x01); // PCLK root divider, bit[5:4], SCLK2x root divider, bit[3:2]
	// SCLK root divider, bit[1:0]
	OV5640_WriteReg(IicPs,0x3630, 0x36);
	OV5640_WriteReg(IicPs,0x3631, 0x0e);
	OV5640_WriteReg(IicPs,0x3632, 0xe2);
	OV5640_WriteReg(IicPs,0x3633, 0x12);
	OV5640_WriteReg(IicPs,0x3621, 0xe0);
	OV5640_WriteReg(IicPs,0x3704, 0xa0);
	OV5640_WriteReg(IicPs,0x3703, 0x5a);
	OV5640_WriteReg(IicPs,0x3715, 0x78);
	OV5640_WriteReg(IicPs,0x3717, 0x01);
	OV5640_WriteReg(IicPs,0x370b, 0x60);
	OV5640_WriteReg(IicPs,0x3705, 0x1a);
	OV5640_WriteReg(IicPs,0x3905, 0x02);
	OV5640_WriteReg(IicPs,0x3906, 0x10);
	OV5640_WriteReg(IicPs,0x3901, 0x0a);
	OV5640_WriteReg(IicPs,0x3731, 0x12);
	OV5640_WriteReg(IicPs,0x3600, 0x08); // VCM control
	OV5640_WriteReg(IicPs,0x3601, 0x33); // VCM control
	OV5640_WriteReg(IicPs,0x302d, 0x60); // system control
	OV5640_WriteReg(IicPs,0x3620, 0x52);
	OV5640_WriteReg(IicPs,0x371b, 0x20);
	OV5640_WriteReg(IicPs,0x471c, 0x50);
	OV5640_WriteReg(IicPs,0x3a13, 0x43); // pre-gain = 1.047x
	OV5640_WriteReg(IicPs,0x3a18, 0x00); // gain ceiling
	OV5640_WriteReg(IicPs,0x3a19, 0xf8); // gain ceiling = 15.5x
	OV5640_WriteReg(IicPs,0x3635, 0x13);
	OV5640_WriteReg(IicPs,0x3636, 0x03);
	OV5640_WriteReg(IicPs,0x3634, 0x40);
	OV5640_WriteReg(IicPs,0x3622, 0x01);
	// 50/60Hz detection
	OV5640_WriteReg(IicPs,0x3c01, 0x34); // Band auto, bit[7]
	OV5640_WriteReg(IicPs,0x3c04, 0x28); // threshold low sum
	OV5640_WriteReg(IicPs,0x3c05, 0x98); // threshold high sum
	OV5640_WriteReg(IicPs,0x3c06, 0x00); // light meter 1 threshold[15:8]
	OV5640_WriteReg(IicPs,0x3c07, 0x08); // light meter 1 threshold[7:0]
	OV5640_WriteReg(IicPs,0x3c08, 0x00); // light meter 2 threshold[15:8]
	OV5640_WriteReg(IicPs,0x3c09, 0x1c); // light meter 2 threshold[7:0]
	OV5640_WriteReg(IicPs,0x3c0a, 0x9c); // sample number[15:8]
	OV5640_WriteReg(IicPs,0x3c0b, 0x40); // sample number[7:0]

	OV5640_WriteReg(IicPs,0x3810, 0x00); // Timing Hoffset[11:8]
	OV5640_WriteReg(IicPs,0x3811, 0x10); // Timing Hoffset[7:0]
	OV5640_WriteReg(IicPs,0x3812, 0x00); // Timing Voffset[10:8]
	OV5640_WriteReg(IicPs,0x3708, 0x64);
	OV5640_WriteReg(IicPs,0x4001, 0x02); // BLC start from line 2
	OV5640_WriteReg(IicPs,0x4005, 0x1a); // BLC always update
	OV5640_WriteReg(IicPs,0x3000, 0x00); // enable blocks
	OV5640_WriteReg(IicPs,0x3004, 0xff); // enable clocks
	OV5640_WriteReg(IicPs,0x300e, 0x58); // MIPI power down, DVP enable
	OV5640_WriteReg(IicPs,0x302e, 0x00);
	OV5640_WriteReg(IicPs,0x4300, 0x30); // YUV 422, YUYV
	OV5640_WriteReg(IicPs,0x501f, 0x00); // YUV 422
	OV5640_WriteReg(IicPs,0x440e, 0x00);
	OV5640_WriteReg(IicPs,0x5000, 0xa7); // Lenc on, raw gamma on, BPC on, WPC on, CIP on

	// AEC target
	OV5640_WriteReg(IicPs,0x3a0f, 0x30); // stable range in high
	OV5640_WriteReg(IicPs,0x3a10, 0x28); // stable range in low
	OV5640_WriteReg(IicPs,0x3a1b, 0x30); // stable range out high
	OV5640_WriteReg(IicPs,0x3a1e, 0x26); // stable range out low
	OV5640_WriteReg(IicPs,0x3a11, 0x60); // fast zone high
	OV5640_WriteReg(IicPs,0x3a1f, 0x14); // fast zone low

	// Lens correction for ?
	OV5640_WriteReg(IicPs,0x5800, 0x23);
	OV5640_WriteReg(IicPs,0x5801, 0x14);
	OV5640_WriteReg(IicPs,0x5802, 0x0f);
	OV5640_WriteReg(IicPs,0x5803, 0x0f);
	OV5640_WriteReg(IicPs,0x5804, 0x12);
	OV5640_WriteReg(IicPs,0x5805, 0x26);
	OV5640_WriteReg(IicPs,0x5806, 0x0c);
	OV5640_WriteReg(IicPs,0x5807, 0x08);
	OV5640_WriteReg(IicPs,0x5808, 0x05);
	OV5640_WriteReg(IicPs,0x5809, 0x05);
	OV5640_WriteReg(IicPs,0x580a, 0x08);
	OV5640_WriteReg(IicPs,0x580b, 0x0d);

	OV5640_WriteReg(IicPs,0x580c, 0x08);
	OV5640_WriteReg(IicPs,0x580d, 0x03);
	OV5640_WriteReg(IicPs,0x580e, 0x00);
	OV5640_WriteReg(IicPs,0x580f, 0x00);
	OV5640_WriteReg(IicPs,0x5810, 0x03);
	OV5640_WriteReg(IicPs,0x5811, 0x09);
	OV5640_WriteReg(IicPs,0x5812, 0x07);
	OV5640_WriteReg(IicPs,0x5813, 0x03);
	OV5640_WriteReg(IicPs,0x5814, 0x00);
	OV5640_WriteReg(IicPs,0x5815, 0x01);
	OV5640_WriteReg(IicPs,0x5816, 0x03);
	OV5640_WriteReg(IicPs,0x5817, 0x08);
	OV5640_WriteReg(IicPs,0x5818, 0x0d);
	OV5640_WriteReg(IicPs,0x5819, 0x08);
	OV5640_WriteReg(IicPs,0x581a, 0x05);
	OV5640_WriteReg(IicPs,0x581b, 0x06);
	OV5640_WriteReg(IicPs,0x581c, 0x08);
	OV5640_WriteReg(IicPs,0x581d, 0x0e);
	OV5640_WriteReg(IicPs,0x581e, 0x29);
	OV5640_WriteReg(IicPs,0x581f, 0x17);
	OV5640_WriteReg(IicPs,0x5820, 0x11);
	OV5640_WriteReg(IicPs,0x5821, 0x11);
	OV5640_WriteReg(IicPs,0x5822, 0x15);
	OV5640_WriteReg(IicPs,0x5823, 0x28);
	OV5640_WriteReg(IicPs,0x5824, 0x46);
	OV5640_WriteReg(IicPs,0x5825, 0x26);
	OV5640_WriteReg(IicPs,0x5826, 0x08);
	OV5640_WriteReg(IicPs,0x5827, 0x26);
	OV5640_WriteReg(IicPs,0x5828, 0x64);
	OV5640_WriteReg(IicPs,0x5829, 0x26);
	OV5640_WriteReg(IicPs,0x582a, 0x24);
	OV5640_WriteReg(IicPs,0x582b, 0x22);
	OV5640_WriteReg(IicPs,0x582c, 0x24);
	OV5640_WriteReg(IicPs,0x582d, 0x24);
	OV5640_WriteReg(IicPs,0x582e, 0x06);
	OV5640_WriteReg(IicPs,0x582f, 0x22);
	OV5640_WriteReg(IicPs,0x5830, 0x40);
	OV5640_WriteReg(IicPs,0x5831, 0x42);
	OV5640_WriteReg(IicPs,0x5832, 0x24);
	OV5640_WriteReg(IicPs,0x5833, 0x26);
	OV5640_WriteReg(IicPs,0x5834, 0x24);
	OV5640_WriteReg(IicPs,0x5835, 0x22);
	OV5640_WriteReg(IicPs,0x5836, 0x22);
	OV5640_WriteReg(IicPs,0x5837, 0x26);
	OV5640_WriteReg(IicPs,0x5838, 0x44);
	OV5640_WriteReg(IicPs,0x5839, 0x24);
	OV5640_WriteReg(IicPs,0x583a, 0x26);
	OV5640_WriteReg(IicPs,0x583b, 0x28);
	OV5640_WriteReg(IicPs,0x583c, 0x42);
	OV5640_WriteReg(IicPs,0x583d, 0xce); // lenc BR offset
	// AWB
	OV5640_WriteReg(IicPs,0x5180, 0xff); // AWB B block
	OV5640_WriteReg(IicPs,0x5181, 0xf2); // AWB control
	OV5640_WriteReg(IicPs,0x5182, 0x00); // [7:4] max local counter, [3:0] max fast counter
	OV5640_WriteReg(IicPs,0x5183, 0x14); // AWB advanced
	OV5640_WriteReg(IicPs,0x5184, 0x25);
	OV5640_WriteReg(IicPs,0x5185, 0x24);
	OV5640_WriteReg(IicPs,0x5186, 0x09);
	OV5640_WriteReg(IicPs,0x5187, 0x09);
	OV5640_WriteReg(IicPs,0x5188, 0x09);
	OV5640_WriteReg(IicPs,0x5189, 0x75);
	OV5640_WriteReg(IicPs,0x518a, 0x54);
	OV5640_WriteReg(IicPs,0x518b, 0xe0);
	OV5640_WriteReg(IicPs,0x518c, 0xb2);
	OV5640_WriteReg(IicPs,0x518d, 0x42);
	OV5640_WriteReg(IicPs,0x518e, 0x3d);
	OV5640_WriteReg(IicPs,0x518f, 0x56);
	OV5640_WriteReg(IicPs,0x5190, 0x46);
	OV5640_WriteReg(IicPs,0x5191, 0xf8); // AWB top limit
	OV5640_WriteReg(IicPs,0x5192, 0x04); // AWB bottom limit
	OV5640_WriteReg(IicPs,0x5193, 0x70); // red limit
	OV5640_WriteReg(IicPs,0x5194, 0xf0); // green limit
	OV5640_WriteReg(IicPs,0x5195, 0xf0); // blue limit
	OV5640_WriteReg(IicPs,0x5196, 0x03); // AWB control
	OV5640_WriteReg(IicPs,0x5197, 0x01); // local limit
	OV5640_WriteReg(IicPs,0x5198, 0x04);
	OV5640_WriteReg(IicPs,0x5199, 0x12);
	OV5640_WriteReg(IicPs,0x519a, 0x04);
	OV5640_WriteReg(IicPs,0x519b, 0x00);
	OV5640_WriteReg(IicPs,0x519c, 0x06);
	OV5640_WriteReg(IicPs,0x519d, 0x82);
	OV5640_WriteReg(IicPs,0x519e, 0x38); // AWB control
	// Gamma
	OV5640_WriteReg(IicPs,0x5480, 0x01); // Gamma bias plus on, bit[0]
	OV5640_WriteReg(IicPs,0x5481, 0x08);
	OV5640_WriteReg(IicPs,0x5482, 0x14);
	OV5640_WriteReg(IicPs,0x5483, 0x28);
	OV5640_WriteReg(IicPs,0x5484, 0x51);
	OV5640_WriteReg(IicPs,0x5485, 0x65);
	OV5640_WriteReg(IicPs,0x5486, 0x71);
	OV5640_WriteReg(IicPs,0x5487, 0x7d);
	OV5640_WriteReg(IicPs,0x5488, 0x87);
	OV5640_WriteReg(IicPs,0x5489, 0x91);
	OV5640_WriteReg(IicPs,0x548a, 0x9a);

	OV5640_WriteReg(IicPs,0x548b, 0xaa);
	OV5640_WriteReg(IicPs,0x548c, 0xb8);
	OV5640_WriteReg(IicPs,0x548d, 0xcd);
	OV5640_WriteReg(IicPs,0x548e, 0xdd);
	OV5640_WriteReg(IicPs,0x548f, 0xea);
	OV5640_WriteReg(IicPs,0x5490, 0x1d);

	// color matrix
	OV5640_WriteReg(IicPs,0x5381, 0x1e); // CMX1 for Y
	OV5640_WriteReg(IicPs,0x5382, 0x5b); // CMX2 for Y
	OV5640_WriteReg(IicPs,0x5383, 0x08); // CMX3 for Y
	OV5640_WriteReg(IicPs,0x5384, 0x0a); // CMX4 for U
	OV5640_WriteReg(IicPs,0x5385, 0x7e); // CMX5 for U
	OV5640_WriteReg(IicPs,0x5386, 0x88); // CMX6 for U
	OV5640_WriteReg(IicPs,0x5387, 0x7c); // CMX7 for V
	OV5640_WriteReg(IicPs,0x5388, 0x6c); // CMX8 for V
	OV5640_WriteReg(IicPs,0x5389, 0x10); // CMX9 for V
	OV5640_WriteReg(IicPs,0x538a, 0x01); // sign[9]
	OV5640_WriteReg(IicPs,0x538b, 0x98); // sign[8:1]

	// UV adjsut
	OV5640_WriteReg(IicPs,0x5580, 0x06); // saturation on, bit[1]
	OV5640_WriteReg(IicPs,0x5583, 0x40);
	OV5640_WriteReg(IicPs,0x5584, 0x10);
	OV5640_WriteReg(IicPs,0x5589, 0x10);
	OV5640_WriteReg(IicPs,0x558a, 0x00);
	OV5640_WriteReg(IicPs,0x558b, 0xf8);
	OV5640_WriteReg(IicPs,0x501d, 0x40); // enable manual offset of contras

	// CIP
	OV5640_WriteReg(IicPs,0x5300, 0x08); // CIP sharpen MT threshold 1
	OV5640_WriteReg(IicPs,0x5301, 0x30); // CIP sharpen MT threshold 2
	OV5640_WriteReg(IicPs,0x5302, 0x10); // CIP sharpen MT offset 1
	OV5640_WriteReg(IicPs,0x5303, 0x00); // CIP sharpen MT offset 2
	OV5640_WriteReg(IicPs,0x5304, 0x08); // CIP DNS threshold 1
	OV5640_WriteReg(IicPs,0x5305, 0x30); // CIP DNS threshold 2
	OV5640_WriteReg(IicPs,0x5306, 0x08); // CIP DNS offset 1
	OV5640_WriteReg(IicPs,0x5307, 0x16); // CIP DNS offset 2
	OV5640_WriteReg(IicPs,0x5309, 0x08); // CIP sharpen TH threshold 1
	OV5640_WriteReg(IicPs,0x530a, 0x30); // CIP sharpen TH threshold 2
	OV5640_WriteReg(IicPs,0x530b, 0x04); // CIP sharpen TH offset 1
	OV5640_WriteReg(IicPs,0x530c, 0x06); // CIP sharpen TH offset 2

	OV5640_WriteReg(IicPs,0x5025, 0x00);
	OV5640_WriteReg(IicPs,0x3008, 0x02); // wake up from standby, bit[6]

}


int OV5640_init_setting(XIicPs *IicPs)
{
// initialize OV5640
int regInit[] =
{
//
//OV5640 setting Version History
//dated 04/08/2010 A02
//--Based on v08 release
//
//dated 04/20/2010 A03
//--Based on V10 release
//
//dated 04/22/2010 A04
//--Based on V10 release
//--updated ccr & awb setting
//
//dated 04/22/2010 A06
//--Based on A05 release
//--Add pg setting
//
//dated 05/19/2011 A09
//--changed pchg 3708 setting
0x3008, 0x42, // software power down
0x3103, 0x03, // sysclk from pll
0x3017, 0xff, // Frex, Vsync, Href, PCLK, D
0x3018, 0xff, // D[5:0], GPIO[1:0] output
0x3034, 0x1a, // PLL, MIPI 10-bit
0x3037, 0x13, // PLL
0x3108, 0x01, // clock divider
0x3630, 0x36,
0x3631, 0x0e,
0x3632, 0xe2,
0x3633, 0x12,
0x3621, 0xe0,
0x3704, 0xa0,
0x3703, 0x5a,
0x3715, 0x78,
0x3717, 0x01,
0x370b, 0x60,
0x3705, 0x1a,
0x3905, 0x02,
0x3906, 0x10,
0x3901, 0x0a,
0x3731, 0x12,
0x3600, 0x08, // VCM debug
0x3601, 0x33, // VCM debug
0x302d, 0x60, // system control
0x3620, 0x52,
0x371b, 0x20,
0x471c, 0x50,


0x3a13, 0x43, // pre-gain = 1.05x
0x3a18, 0x00, // AEC gain ceiling = 7.75x
0x3a19, 0x7c, // AEC gain ceiling
0x3635, 0x13,
0x3636, 0x03,
0x3634, 0x40,
0x3622, 0x01,
//50/60Hz detection
0x3c01, 0x34, // sum auto, band counter enable, threshold = 4
0x3c04, 0x28, // threshold low sum
0x3c05, 0x98, // threshold high sum
0x3c06, 0x00, // light meter 1 threshold H
0x3c07, 0x07, // light meter 1 threshold L
0x3c08, 0x00, // light meter 2 threshold H
0x3c09, 0x1c, // light meter 2 threshold L
0x3c0a, 0x9c, // sample number H
0x3c0b, 0x40, // sample number L
0x3810, 0x00, // X offset
0x3811, 0x10, // X offset
0x3812, 0x00, // Y offset
0x3708, 0x64,
0x4001, 0x02, // BLC start line
0x4005, 0x1a, // BLC always update
0x3000, 0x00, // enable MCU, OTP
0x3004, 0xff, // enable BIST, MCU memory, MCU, OTP, STROBE, D5060, timing, array clock
0x300e, 0x58, // MIPI 2 lane? power down PHY HS TX, PHY LP RX, DVP enable
0x302e, 0x00,
0x4300, 0x30, // YUV 422, YUYV
0x501f, 0x00, // ISP YUV 422
0x440e, 0x00,
0x5000, 0xa7, // LENC on, raw gamma on, BPC on, WPC on, CIP on
//AEC target
0x3a0f, 0x30, // stable in high
0x3a10, 0x28, // stable in low
0x3a1b, 0x30, // stable out high
0x3a1e, 0x26, // stable out low
0x3a11, 0x60, // fast zone high
0x3a1f, 0x14, // fast zone low
//LENC
0x5800, 0x23,
0x5801, 0x14,
0x5802, 0x0f,
0x5803, 0x0f,
0x5804, 0x12,
0x5805, 0x26,
0x5806, 0x0c,
0x5807, 0x08,
0x5808, 0x05,
0x5809, 0x05,
0x580a, 0x08,
0x580b, 0x0d,
0x580c, 0x08,
0x580d, 0x03,
0x580e, 0x00,
0x580f, 0x00,
0x5810, 0x03,
0x5811, 0x09,
0x5812, 0x07,
0x5813, 0x03,
0x5814, 0x00,
0x5815, 0x01,



0x5816, 0x03,
0x5817, 0x08,
0x5818, 0x0d,
0x5819, 0x08,
0x581a, 0x05,
0x581b, 0x06,
0x581c, 0x08,
0x581d, 0x0e,
0x581e, 0x29,
0x581f, 0x17,
0x5820, 0x11,
0x5821, 0x11,
0x5822, 0x15,
0x5823, 0x28,
0x5824, 0x46,
0x5825, 0x26,
0x5826, 0x08,
0x5827, 0x26,
0x5828, 0x64,
0x5829, 0x26,
0x582a, 0x24,
0x582b, 0x22,
0x582c, 0x24,
0x582d, 0x24,
0x582e, 0x06,
0x582f, 0x22,
0x5830, 0x40,
0x5831, 0x42,
0x5832, 0x24,
0x5833, 0x26,
0x5834, 0x24,
0x5835, 0x22,
0x5836, 0x22,
0x5837, 0x26,
0x5838, 0x44,
0x5839, 0x24,
0x583a, 0x26,
0x583b, 0x28,
0x583c, 0x42,
0x583d, 0xce, // LENC BR offset
//AWB
0x5180, 0xff, // AWB B block
0x5181, 0xf2, // AWB control
0x5182, 0x00, // [7:4] max local counter, [3:0] max fast counter
0x5183, 0x14, // AWB advance
0x5184, 0x25,
0x5185, 0x24,
0x5186, 0x09,
0x5187, 0x09,
0x5188, 0x09,
0x5189, 0x75,
0x518a, 0x54,
0x518b, 0xe0,
0x518c, 0xb2,
0x518d, 0x42,
0x518e, 0x3d,
0x518f, 0x56,
0x5190, 0x46,
0x5191, 0xf8, // AWB top limit
0x5192, 0x04, // AWB botton limit
0x5193, 0x70, // Red limit
0x5194, 0xf0, // Green Limit
0x5195, 0xf0, // Blue limit

0x5196, 0x03, // AWB control
0x5197, 0x01, // local limit
0x5198, 0x04,
0x5199, 0x12,
0x519a, 0x04,
0x519b, 0x00,
0x519c, 0x06,
0x519d, 0x82,
0x519e, 0x38, // AWB control
//Gamma
0x5480, 0x01, // BIAS plus on
0x5481, 0x08,
0x5482, 0x14,
0x5483, 0x28,
0x5484, 0x51,
0x5485, 0x65,
0x5486, 0x71,
0x5487, 0x7d,
0x5488, 0x87,
0x5489, 0x91,
0x548a, 0x9a,
0x548b, 0xaa,
0x548c, 0xb8,
0x548d, 0xcd,
0x548e, 0xdd,
0x548f, 0xea,
0x5490, 0x1d,
//color matrix
0x5381, 0x1e, // CMX1 for Y
0x5382, 0x5b, // CMX2 for Y
0x5383, 0x08, // CMX3 for Y
0x5384, 0x0a, // CMX4 for U
0x5385, 0x7e, // CMX5 for U
0x5386, 0x88, // CMX6 for U
0x5387, 0x7c, // CMX7 for V
0x5388, 0x6c, // CMX8 for V
0x5389, 0x10, // CMX9 for V
0x538a, 0x01, // sign[9]
0x538b, 0x98, // sign[8:1]
//UV adjust
0x5580, 0x06, // brightness on, saturation on
0x5583, 0x40, // Sat U
0x5584, 0x10, // Sat V
0x5589, 0x10, // UV adjust th1
0x558a, 0x00, // UV adjust th2[8]
0x558b, 0xf8, // UV adjust th2[7:0]
0x501d, 0x40, // enable manual offset in contrast
//CIP
0x5300, 0x08, // sharpen-MT th1
0x5301, 0x30, // sharpen-MT th2
0x5302, 0x10, // sharpen-MT off1
0x5303, 0x00, // sharpen-MT off2
0x5304, 0x08, // De-noise th1
0x5305, 0x30, // De-noise th2
0x5306, 0x08, // De-noise off1
0x5307, 0x16, // De-noise off2
0x5309, 0x08, // sharpen-TH th1
0x530a, 0x30, // sharpen-TH th2
0x530b, 0x04, // sharpen-TH off1
0x530c, 0x06, // sharpen-TH off2

0x5025, 0x00,
0x3008, 0x02, // wake up from software power down
};
OV5640_WriteReg(IicPs,0x3103, 0x11); // sysclk from pad
OV5640_WriteReg(IicPs,0x3008, 0x82); // software reset
// delay 5ms
delay_ms(5);
// Write initialization table
for (int i=0; i<sizeof(regInit)/sizeof(int); i+=2)
{
	OV5640_WriteReg(IicPs,regInit[i], regInit[i+1]);
}
return 0;
}


int OV5640_ColoBar_test(XIicPs *IicPs)
{
	OV5640_WriteReg(IicPs,0x0c,0xd1);
	return 0;
}








void OV5640_JPEG_Mode(XIicPs* IicPs)
{
	uint16_t i=0;

	for(i=0;i<(sizeof(OV5640_jpeg_reg_tbl)/4);i++)
	{
		OV5640_WriteReg(IicPs,OV5640_jpeg_reg_tbl[i][0],OV5640_jpeg_reg_tbl[i][1]);
	}
}

void OV5640_RGB565_Mode(XIicPs* IicPs)
{
	uint16_t i=0;

	for(i=0;i<(sizeof(ov5640_init_reg_tbl)/4);i++)
	{
		OV5640_WriteReg(IicPs, ov5640_init_reg_tbl[i][0],ov5640_init_reg_tbl[i][1]);
	}
}

const static uint8_t OV5640_SATURATION_TBL[7][6]=
{
	0X0C,0x30,0X3D,0X3E,0X3D,0X01,//-3
	0X10,0x3D,0X4D,0X4E,0X4D,0X01,//-2
	0X15,0x52,0X66,0X68,0X66,0X02,//-1
	0X1A,0x66,0X80,0X82,0X80,0X02,//+0
	0X1F,0x7A,0X9A,0X9C,0X9A,0X02,//+1
	0X24,0x8F,0XB3,0XB6,0XB3,0X03,//+2
	0X2B,0xAB,0XD6,0XDA,0XD6,0X04,//+3
};


const static uint8_t OV5640_EXPOSURE_TBL[7][6]= {
    0x10,0x08,0x10,0x08,0x20,0x10,//-3
    0x20,0x18,0x41,0x20,0x18,0x10,//-2
    0x30,0x28,0x61,0x30,0x28,0x10,//-1
    0x38,0x30,0x61,0x38,0x30,0x10,//0
    0x40,0x38,0x71,0x40,0x38,0x10,//+1
    0x50,0x48,0x90,0x50,0x48,0x20,//+2
    0x60,0x58,0xa0,0x60,0x58,0x20,//+3
};

const static uint8_t OV5640_EFFECTS_TBL[7][3]=
{
		0X06,0x40,0X10, // normal
		0X1E,0xA0,0X40,
		0X1E,0x80,0XC0,
		0X1E,0x80,0X80,
		0X1E,0x40,0XA0,
		0X40,0x40,0X10,
		0X1E,0x60,0X60,
};

const static uint8_t OV5640_LIGHTMODE_TBL[5][7]=
{
	0x04,0X00,0X04,0X00,0X04,0X00,0X00,//Auto
	0x06,0X1C,0X04,0X00,0X04,0XF3,0X01,//Sunny
	0x05,0X48,0X04,0X00,0X07,0XCF,0X01,//Office
	0x06,0X48,0X04,0X00,0X04,0XD3,0X01,//Cloudy
	0x04,0X10,0X04,0X00,0X08,0X40,0X01,//Home
};

//exposure: 0 - 6,
void OV5640_Exposure(XIicPs* IicPs,uint8_t exposure)
{
        OV5640_WriteReg(IicPs,0x3212,0x03);	//start group 3
        OV5640_WriteReg(IicPs,0x3a0f,OV5640_EXPOSURE_TBL[exposure][0]);
        OV5640_WriteReg(IicPs,0x3a10,OV5640_EXPOSURE_TBL[exposure][1]);
        OV5640_WriteReg(IicPs,0x3a1b,OV5640_EXPOSURE_TBL[exposure][2]);
        OV5640_WriteReg(IicPs,0x3a1e,OV5640_EXPOSURE_TBL[exposure][3]);
        OV5640_WriteReg(IicPs,0x3a11,OV5640_EXPOSURE_TBL[exposure][4]);
        OV5640_WriteReg(IicPs,0x3a1f,OV5640_EXPOSURE_TBL[exposure][5]);
        OV5640_WriteReg(IicPs,0x3212,0x13); //end group 3
        OV5640_WriteReg(IicPs,0x3212,0xa3); //launch group 3
}



// light mode:
//      0: auto
//      1: sunny
//      2: office
//      3: cloudy
//      4: home
void OV5640_Light_Mode(XIicPs* IicPs,uint8_t mode)
{
	uint8_t i;
	OV5640_WriteReg(IicPs,0x3212,0x03);	//start group 3
	for(i=0;i<7;i++)OV5640_WriteReg(IicPs,0x3400+i,OV5640_LIGHTMODE_TBL[mode][i]);
	OV5640_WriteReg(IicPs,0x3212,0x13); //end group 3
	OV5640_WriteReg(IicPs,0x3212,0xa3); //launch group 3
}



// Color Saturation:
//   sat:  0 - 6
void OV5640_Color_Saturation(XIicPs* IicPs,uint8_t sat)
{
	uint8_t i;
	OV5640_WriteReg(IicPs,0x3212,0x03);	//start group 3
	OV5640_WriteReg(IicPs,0x5381,0x1c);
	OV5640_WriteReg(IicPs,0x5382,0x5a);
	OV5640_WriteReg(IicPs,0x5383,0x06);
	for(i=0;i<6;i++)  OV5640_WriteReg(IicPs,0x5384+i,OV5640_SATURATION_TBL[sat][i]);
	OV5640_WriteReg(IicPs,0x538b, 0x98);
	OV5640_WriteReg(IicPs,0x538a, 0x01);
	OV5640_WriteReg(IicPs,0x3212, 0x13); //end group 3
	OV5640_WriteReg(IicPs,0x3212, 0xa3); //launch group 3
}

//Brightness
//     bright:  0 - 8
void OV5640_Brightness(XIicPs* IicPs,uint8_t bright)
{
	uint8_t brtval;
	if(bright<4)brtval=4-bright;
	else brtval=bright-4;
	OV5640_WriteReg(IicPs,0x3212,0x03);	//start group 3
	OV5640_WriteReg(IicPs,0x5587,brtval<<4);
	if(bright<4)OV5640_WriteReg(IicPs,0x5588,0x09);
	else OV5640_WriteReg(IicPs,0x5588,0x01);
	OV5640_WriteReg(IicPs,0x3212,0x13); //end group 3
	OV5640_WriteReg(IicPs,0x3212,0xa3); //launch group 3
}

//Contrast:
//     contrast:  0 - 6
void OV5640_Contrast(XIicPs* IicPs,uint8_t contrast)
{
	uint8_t reg0val=0X00;
	uint8_t reg1val=0X20;
	switch(contrast)
	{
		case 0://-3
			reg1val=reg0val=0X14;
			break;
		case 1://-2
			reg1val=reg0val=0X18;
			break;
		case 2://-1
			reg1val=reg0val=0X1C;
			break;
		case 4://1
			reg0val=0X10;
			reg1val=0X24;
			break;
		case 5://2
			reg0val=0X18;
			reg1val=0X28;
			break;
		case 6://3
			reg0val=0X1C;
			reg1val=0X2C;
			break;
	}
	OV5640_WriteReg(IicPs, 0x3212,0x03); //start group 3
	OV5640_WriteReg(IicPs, 0x5585,reg0val);
	OV5640_WriteReg(IicPs, 0x5586,reg1val);
	OV5640_WriteReg(IicPs, 0x3212,0x13); //end group 3
	OV5640_WriteReg(IicPs, 0x3212,0xa3); //launch group 3
}
// Sharpness:
//    sharp: 0 - 33   (0: close , 33: auto , other: Sharpness)

void OV5640_Sharpness(XIicPs* IicPs,uint8_t sharp)
{
	if(sharp<33)
	{
		OV5640_WriteReg(IicPs, 0x5308,0x65);
		OV5640_WriteReg(IicPs,0x5302,sharp);
	}else	// auto
	{
		OV5640_WriteReg(IicPs, 0x5308,0x25);
		OV5640_WriteReg(IicPs, 0x5300,0x08);
		OV5640_WriteReg(IicPs, 0x5301,0x30);
		OV5640_WriteReg(IicPs, 0x5302,0x10);
		OV5640_WriteReg(IicPs, 0x5303,0x00);
		OV5640_WriteReg(IicPs, 0x5309,0x08);
		OV5640_WriteReg(IicPs, 0x530a,0x30);
		OV5640_WriteReg(IicPs, 0x530b,0x04);
		OV5640_WriteReg(IicPs, 0x530c,0x06);
	}

}


void OV5640_Special_Effects(XIicPs* IicPs,uint8_t eft)
{
	OV5640_WriteReg(IicPs,0x3212,0x03); //start group 3
	OV5640_WriteReg(IicPs,0x5580,OV5640_EFFECTS_TBL[eft][0]);
	OV5640_WriteReg(IicPs,0x5583,OV5640_EFFECTS_TBL[eft][1]);// sat U
	OV5640_WriteReg(IicPs,0x5584,OV5640_EFFECTS_TBL[eft][2]);// sat V
	OV5640_WriteReg(IicPs,0x5003,0x08);
	OV5640_WriteReg(IicPs,0x3212,0x13); //end group 3
	OV5640_WriteReg(IicPs,0x3212,0xa3); //launch group 3
}

// Flash Lamp
//  sw:  0: off
//       1:  on
void OV5640_Flash_Lamp(XIicPs* IicPs,uint8_t sw)
{
	OV5640_WriteReg(IicPs,0x3016,0X02);
	OV5640_WriteReg(IicPs,0x301C,0X02);
	if(sw)OV5640_WriteReg(IicPs,0X3019,0X02);
	else OV5640_WriteReg(IicPs,0X3019,0X00);
}

// set the output size
uint8_t OV5640_OutSize_Set(XIicPs* IicPs,uint16_t offx,uint16_t offy,uint16_t width,uint16_t height)
{
        OV5640_WriteReg(IicPs,0X3212,0X03);

        OV5640_WriteReg(IicPs,0x3808,width>>8);
        OV5640_WriteReg(IicPs,0x3809,width&0xff);
        OV5640_WriteReg(IicPs,0x380a,height>>8);
        OV5640_WriteReg(IicPs,0x380b,height&0xff);

        OV5640_WriteReg(IicPs,0x3810,offx>>8);
        OV5640_WriteReg(IicPs,0x3811,offx&0xff);

        OV5640_WriteReg(IicPs,0x3812,offy>>8);
        OV5640_WriteReg(IicPs,0x3813,offy&0xff);

        OV5640_WriteReg(IicPs,0X3212,0X13);
        OV5640_WriteReg(IicPs,0X3212,0Xa3);

        return 0;
}


uint8_t OV5640_Focus_Init(XIicPs* IicPs)
{
	uint16_t i;
	uint16_t addr=0x8000;
	uint8_t state=0x8F;
	OV5640_WriteReg(IicPs,0x3000, 0x20);	//reset
	for(i=0;i<sizeof(OV5640_AF_Config);i++)
	{
		OV5640_WriteReg(IicPs,addr,OV5640_AF_Config[i]);
		addr++;
	}
	OV5640_WriteReg(IicPs,0x3022,0x00);
	OV5640_WriteReg(IicPs,0x3023,0x00);
	OV5640_WriteReg(IicPs,0x3024,0x00);
	OV5640_WriteReg(IicPs,0x3025,0x00);
	OV5640_WriteReg(IicPs,0x3026,0x00);
	OV5640_WriteReg(IicPs,0x3027,0x00);
	OV5640_WriteReg(IicPs,0x3028,0x00);
	OV5640_WriteReg(IicPs,0x3029,0x7f);
	OV5640_WriteReg(IicPs,0x3000,0x00);
	i=0;
	do
	{
		state=OV5640_ReadReg(IicPs,0x3029);
		delay_ms(5);
		i++;
		if(i>1000)return 1;
	}while(state!=0x70);
	return 0;
}

uint8_t OV5640_Auto_Focus(XIicPs* IicPs)
{
	uint8_t temp=0;
	uint16_t retry=0;
	OV5640_WriteReg(IicPs,0x3023,0x01);
	OV5640_WriteReg(IicPs,0x3022,0x08);
	do
	{
		temp=OV5640_ReadReg(IicPs,0x3023);
		retry++;
		if(retry>1000)return 2;
		delay_ms(5);
	} while(temp!=0x00);
	OV5640_WriteReg(IicPs,0x3023,0x01);
	OV5640_WriteReg(IicPs,0x3022,0x04);
	retry=0;
	do
	{
		temp=OV5640_ReadReg(IicPs,0x3023);
		retry++;
		if(retry>1000)return 2;
		delay_ms(5);
	}while(temp!=0x00);
	return 0;
}

