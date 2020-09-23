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
 
 #include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xv_tpg.h"
#include "zc702_hw.h"
#include "xvidc.h"
#include "xgpio.h"
#include "Delay.h"
#include "xv_mix.h"
#include "xv_mix_l2.h"
#include "vga_modes.h"
#include "AXI_GPIO.h"
#include "xil_io.h"
#include "iic_utils.h"
#include "OV5640.h"
#include "xiicps.h"
#include "Delay.h"
#include "PCA9548.h"
#include "zc702_i2c_utils.h"

XV_tpg 				tpg_inst;
XV_tpg_Config 		*tpg_config;

XV_mix 				xv_mix;
XV_mix_Config 		*xv_config;
XV_Mix_l2 			xv_mixl2;

XGpio gpio0;
XGpio gpio1;
XGpio gpio2;

#define HEIGHT                                  768
#define WIDTH                                   1024

XIicPs Iic_inst0;
XIicPs Iic_inst1;


int main()
{
	int Status;
    init_platform();

    xil_printf("Mixer application on ZC702 using FMC VGA\n\r");

    xil_printf("1. Setup GPIO reset!\r\n");

	Status = XGpio_Initialize(&gpio0, XPAR_AXI_GPIO_0_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Gpio 0 Initialization Failed\r\n");
		return XST_FAILURE;
	}

	XGpio_SetDataDirection(&gpio0, 1, 0);	// Set as output 0x00

    gpio_setup(&gpio1,XPAR_AXI_GPIO_1_DEVICE_ID);
    gpio_setup(&gpio2,XPAR_AXI_GPIO_2_DEVICE_ID);
    gpio_dir(&gpio1,RST, 0x00);
    gpio_dir(&gpio2,RST, 0x00);
    gpio_write(&gpio1,RST, 0x00);
    gpio_write(&gpio2,RST, 0x00);
    del_sleep(1);
    gpio_write(&gpio1,RST, 0x01);
    gpio_write(&gpio2,RST, 0x01);
    del_sleep(1);


    xil_printf("1A. Setup IIC1 AND IIC0. \n\r");

	ps_iic_init(XPAR_XIICPS_0_DEVICE_ID, &Iic_inst0);
    ps_iic_init(XPAR_XIICPS_1_DEVICE_ID, &Iic_inst1);
    set_iic_mux(&Iic_inst0, ZC702_I2C_SELECT_FMC1, ZC702_I2C_MUX_ADDR);


    OV5640_Configuration(&Iic_inst0, CONFIG_OV5640);
    OV5640_Configuration(&Iic_inst1, CONFIG_OV5640);


    xil_printf("2. Start of VDMA Configuration!\r\n");
	Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x30, 0x8B);

    Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0xAC, 0x10000000);
    Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0xB0, 0x10280000);
    Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0xB4, 0x10600000);

    Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0xA8, WIDTH*3);
    Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0xA4, WIDTH*3);

    Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0xA0, HEIGHT);

    Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x00, 0x8B);
    Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x5C, 0x10000000);
    Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x60, 0x10280000);
    Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x64, 0x10600000);
    Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x58, WIDTH*3);
    Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x54, WIDTH*3);
    Xil_Out32(XPAR_AXI_VDMA_0_BASEADDR + 0x50, HEIGHT);

    xil_printf("2B. VDMA was Configured. Now configuring cam\n");

    xil_printf("1D. Starting camera 0 and 1 of Stereo Cape\n\r");



	// Select FMC1
	zc702_iic_mux(&Iic_inst0,ZC702_I2C_SELECT_FMC1);
	xil_printf("1D. I2C Crossbar selected FMC 1\r\n");

	xil_printf("I2C Configured\n\r");
	uint8_t controlReg = PCA9548_readRegMux();
	xil_printf("Control register: %x\n", controlReg);
	delay_ms(500);
    xil_printf("\r\n Selecting FMC1 and FMC2");

    PCA9548_writeRegMux(ZC702_I2C_SELECT_FMC1);
    controlReg = PCA9548_readRegMux();
    xil_printf("Control register: %x\n", controlReg);

    int result;


    xil_printf("2. TPG source setup!\r\n");

    /*************************************************************************************/
    /* Insert the code for the TPG here */
    Status = XV_tpg_Initialize(&tpg_inst, XPAR_V_TPG_0_DEVICE_ID);
    if(Status!= XST_SUCCESS)
    {
    	xil_printf("TPG configuration failed\r\n");
        	return(XST_FAILURE);
    }
    XV_tpg_Set_height(&tpg_inst, 768);		    // Set Resolution to 800x600
    XV_tpg_Set_width(&tpg_inst, 1024);
    XV_tpg_Set_colorFormat(&tpg_inst, XVIDC_CSF_RGB);		    // Set Color Space to RGB
    XV_tpg_EnableAutoRestart(&tpg_inst);			    //Start the TPG
    XV_tpg_Start(&tpg_inst);
    XV_tpg_Set_bckgndId(&tpg_inst, XTPG_BKGND_SOLID_BLACK);		    // Change the pattern to color bar

    xil_printf("3. TPG started!\r\n");

	/***********************************************************************************************/

	XGpio_DiscreteClear(&gpio0, 1, 1);		// Clear output (LOW)
	del_sleep(1);
	XGpio_DiscreteSet(&gpio0, 1, 1);
   	xil_printf("Mixer reset. \n\r");

   	xil_printf("4. Mixer Configuration. \n\r");
   	Status = XVMix_Initialize(&xv_mixl2, XPAR_XV_MIX_0_DEVICE_ID);

   		if(Status != XST_SUCCESS) {
   			xil_printf("ERR:: Mixer device not found\r\n");
   			return(XST_FAILURE);
   			}

	XVidC_VideoStream 	VidStrm;
	XVidC_ColorFormat Cfmt;
	XVidC_VideoTiming const *TimingPtr;

	u32 				width, height;

	XVidC_VideoWindow Win_Layer1;
	XVidC_VideoWindow Win_Layer2;

	u32 Stride_Layer1;
	u32 Stride_Layer2;


	width = 1024;
	height = 768;
	//Stop mixer
	XVMix_Stop(&xv_mixl2);
	XVMix_GetLayerColorFormat(&xv_mixl2, XVMIX_LAYER_MASTER, &Cfmt);


	VidStrm.VmId = XVIDC_VM_XGA_60_P;
	VidStrm.ColorFormatId = XVIDC_CSF_RGB;
	VidStrm.FrameRate = XVIDC_FR_60HZ;
	VidStrm.IsInterlaced = FALSE;
	VidStrm.Is3D = FALSE;
	VidStrm.ColorDepth = XVIDC_BPC_8;
	VidStrm.PixPerClk = XVIDC_PPC_1;
	VidStrm.Timing.HActive = width;
	VidStrm.Timing.VActive = height;
	XVMix_SetVidStream(&xv_mixl2, &VidStrm);

	TimingPtr 				= XVidC_GetTimingInfo(VidStrm.VmId);
	VidStrm.Timing 		 	= *TimingPtr;
	VidStrm.FrameRate 	 	= XVidC_GetFrameRate(VidStrm.VmId);

	xil_printf("\r\n********************************************\r\n");
	xil_printf("V-Mixer Input Video Stream: %s (%s)\r\n", XVidC_GetVideoModeStr(VidStrm.VmId), XVidC_GetColorFormatStr(VidStrm.ColorFormatId));
	xil_printf("Frame Rate: %s\r\n",  XVidC_GetFrameRateStr(VidStrm.VmId));
	xil_printf("Pixel Clock: %d\r\n", XVidC_GetPixelClockHzByVmId(VidStrm.VmId));
	xil_printf("Color Depth: %d\r\n", VidStrm.ColorDepth);

	xil_printf("HSYNC Timing: hav=%04d, hfp=%02d, hsw=%02d(hsp=%d), "
			"hbp=%03d, htot=%04d \r\n", 
			VidStrm.Timing.HActive,
			VidStrm.Timing.HFrontPorch, 
			VidStrm.Timing.HSyncWidth,
			VidStrm.Timing.HSyncPolarity,
			VidStrm.Timing.HBackPorch, 
			VidStrm.Timing.HTotal);
	xil_printf("VSYNC Timing: vav=%04d, vfp=%02d, "
		"vsw=%02d(vsp=%d), vbp=%03d, vtot=%04d\r\n",
			VidStrm.Timing.VActive, 
			VidStrm.Timing.F0PVFrontPorch,
			VidStrm.Timing.F0PVSyncWidth, 
			VidStrm.Timing.VSyncPolarity,
			VidStrm.Timing.F0PVBackPorch, 
			VidStrm.Timing.F0PVTotal);
	 xil_printf("********************************************\r\n");

	/* Set Memory Layer Base Addresses */
	int NumLayers = XVMix_GetNumLayers(&xv_mixl2);
	xil_printf("\nNumLayers: %d\n", NumLayers);

	Win_Layer1.StartX = 0;			//layer 1
	Win_Layer1.StartY = 0;
	Win_Layer1.Width = 640;
	Win_Layer1.Height = 480;
	Stride_Layer1 = 0;

	Win_Layer2.StartX = 320;			//layer 2
	Win_Layer2.StartY = 0;
	Win_Layer2.Width = 640;
	Win_Layer2.Height = 480;
	Stride_Layer2 = 0;

	xil_printf("Layer 1 window %d \r\n ", XVMix_SetLayerWindow(&xv_mixl2, XVMIX_LAYER_1, &Win_Layer1, Stride_Layer1));
	xil_printf("Layer 2 window %d \r\n ", XVMix_SetLayerWindow(&xv_mixl2, XVMIX_LAYER_2, &Win_Layer2, Stride_Layer2));

	xil_printf("5. Layer window setup\r\n");

	XVMix_SetBackgndColor(&xv_mixl2, XVMIX_BKGND_WHITE, VidStrm.ColorDepth);

  	XVMix_SetLayerAlpha(&xv_mixl2, XVMIX_LAYER_1, 255);
   	XVMix_SetLayerAlpha(&xv_mixl2, XVMIX_LAYER_2, 255);

   	XVMix_LayerEnable(&xv_mixl2, XVMIX_LAYER_MASTER);
	XVMix_LayerEnable(&xv_mixl2, XVMIX_LAYER_1);
	XVMix_LayerEnable(&xv_mixl2, XVMIX_LAYER_2);

	xil_printf("INFO: Mixer configured\r\n");

	del_sleep(1);

	xil_printf("6. Mixer Debug. \n\r");
	XVMix_InterruptDisable(&xv_mixl2);
	XVMix_Start(&xv_mixl2);

	xil_printf("All layers configured\r\n") ;

	xil_printf("Enabled %d \r\n",  XVMix_IsLayerEnabled(&xv_mixl2,XVMIX_LAYER_MASTER));

	XVMix_DbgReportStatus(&xv_mixl2);
	XVMix_DbgLayerInfo(&xv_mixl2,XVMIX_LAYER_MASTER);
	XVMix_DbgLayerInfo(&xv_mixl2,XVMIX_LAYER_1);
	XVMix_DbgLayerInfo(&xv_mixl2,XVMIX_LAYER_2);

	xil_printf("7. Mixer started. \n\r");

   	while(1);

	cleanup_platform();
	return 0;
}




