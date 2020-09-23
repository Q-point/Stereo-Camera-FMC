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
//						- PMBus
//						- FMC IPMI EEPROM
//
//----------------------------------------------------------------

#include <stdio.h>
#include <math.h>
#include "xparameters.h"
#include "xstatus.h"
#include "sleep.h"
#include "zc702_i2c_utils.h"


// FMC IMAGEON Mux Selections
#define FMC_IMAGEON_I2C_SELECT_DDCEDID    0x01
#define FMC_IMAGEON_I2C_SELECT_HDMI_OUT   0x02
#define FMC_IMAGEON_I2C_SELECT_HDMI_IN    0x04
#define FMC_IMAGEON_I2C_SELECT_IO_EXP     0x08
#define FMC_IMAGEON_I2C_SELECT_VID_CLK    0x08

// FMC IMAGEON I2C Addresses
#define FMC_IMAGEON_I2C_MUX_ADDR   0x70 // (PCA9546A)
#define FMC_IMAGEON_DDCEDID_ADDR   0x50
#define FMC_IMAGEON_HDMI_OUT_ADDR  0x39 // (ADV7511)
#define FMC_IMAGEON_HDMI_IN_ADDR   0x4C // (ADV7611)
#define FMC_IMAGEON_IO_EXP_ADDR    0x20 // (PCA9534)
#define FMC_IMAGEON_VID_CLK_ADDR   0x65 // (CDCE913)

// FMC IMAGEON Programmable ADV7611 I2C addresses
#define FMC_IMAGEON_ADV7611_IO_ADDR          FMC_IMAGEON_HDMI_IN_ADDR
#define FMC_IMAGEON_ADV7611_CEC_ADDR         0x40
#define FMC_IMAGEON_ADV7611_INFOFRAME_ADDR   0x35
#define FMC_IMAGEON_ADV7611_DPLL_ADDR        0x26
#define FMC_IMAGEON_ADV7611_KSV_ADDR         0x32
#define FMC_IMAGEON_ADV7611_EDID_ADDR        0x36
#define FMC_IMAGEON_ADV7611_HDMI_ADDR        0x34
#define FMC_IMAGEON_ADV7611_CP_ADDR          0x22

// ZC702 Mux Selections
#define ZC702_I2C_SELECT_USRCLK    0x01
#define ZC702_I2C_SELECT_HDMI      0x02
#define ZC702_I2C_SELECT_EEPROM    0x04
#define ZC702_I2C_SELECT_EXPANDER  0x08
#define ZC702_I2C_SELECT_RTC       0x10
#define ZC702_I2C_SELECT_FMC1      0x20
#define ZC702_I2C_SELECT_FMC2      0x40
#define ZC702_I2C_SELECT_PMBUS     0x80

// ZC702 I2C Addresses
#define ZC702_I2C_MUX_ADDR   0x74 // (PCA9458)
#define ZC702_USRCLK_ADDR    0x5D // (SI570)
#define ZC702_HDMI_ADDR      0x39 // (ADV7511)
#define ZC702_PMBUS0_ADDR    0x34 // (UCD9248)
#define ZC702_PMBUS1_ADDR    0x35 // (UCD9248)
#define ZC702_PMBUS2_ADDR    0x36 // (UCD9248)
#define ZC702_FMC_IPMI_ADDR  0x50 // (24LC02)

// PMBus Commands
#define PMBUS_CMD_PAGE       0x00
#define PMBUS_CMD_READ_VOUT  0x8B
#define PMBUS_CMD_READ_IOUT  0x8C


// Constants
static const unsigned long long default_freq = 156250000; // 156.25  MHz
static const unsigned long long max_dco_freq = 945000000; // 945     MHz
static const unsigned long long min_dco_freq =  10000000; //  10     MHz

const unsigned long long freq_HD1080P = 148500000; // VESA, 1920 x 1080, 148.5   MHz at 60 fps
const unsigned long long freq_WSXGA   = 147140000; // VESA, 1680 x 1050, 147.14  MHz at 60 fps
const unsigned long long freq_SXGA    = 108000000; // VESA, 1280 x 1024, 108     MHz at 60 fps
const unsigned long long freq_HD720P  =  74250000; // VESA, 1280 x  720,  74.25  MHz at 60 fps
const unsigned long long freq_XGA     =  65000000; // VESA, 1024 x  768,  65     MHz at 60 fps
const unsigned long long freq_SVGA    =  40000000; // VESA,  800 x  600,  40     MHz at 60 fps
const unsigned long long freq_VGA     =  25175000; // VESA,  640 x  480,  25.175 MHz at 60 fps

// Global variables
static unsigned long long xtal_freq;

// Function Prototypes
static int iic_read1( XIicPs *IicPs, u8 Address, u8 *Data );
static int iic_read2( XIicPs *IicPs, u8 Address, u8 Register, u8 *Data, int ByteCount );

// I2C Config Struct
typedef struct {
	u8 Reg;
	u8 Data;
	u8 Init;
} ZC702_I2C_CONFIG;

// I2C Config Struct
typedef struct {
	u8 Addr;
	u8 Reg;
	u8 Data;
	u8 Init;
} ZC702_I2C_CONFIG2;

// PMBus Data Struct
typedef struct {
	char  Name[16];
	u8    Address;
	u8    Page;
	float Voltage;
	float Current;
	float Power;
} ZC702_PMBUS_DATA;

// SI570 Lookup Struct
typedef struct {
	u8 hs_div;
	u8 n1;
	unsigned long long freq;
} ZC702_SI570_LOOKUP;

// Common FRU Header
struct fru_header {
	u8 version;
	struct	{
		u8 internal;
		u8 chassis;
		u8 board;
		u8 product;
		u8 multi;
	} offset;
	u8 pad;
	u8 checksum;
};


#if (ADV7511_VIDEO_OUT_FORMAT == RGB444)

/*
 * The video input format of the ADV7511 is set to YCbCr, 16-bit, 4:2:2,
 * ID 1 (separate syncs), Style 1. The video output format is set to
 * RGB, 24-bit, 4:4:4, DVI mode.
 *
 * CSC coefficients (registers 0x18 - 0x2F) are taken from tables 40/57
 * of the ADV7511 programmer's guide i.e. converting HDTV YCrCb
 * (16 to 235 or limited range) to RGB (0 to 255 or full range).
 */
#define ZC702_HDMI_CONFIG_LEN  41
ZC702_I2C_CONFIG zc702_hdmi_config[ZC702_HDMI_CONFIG_LEN] =
{
	{0x41, 0x00, 0x10}, // Power Down Control
						//    R0x41[  6] = PowerDown = 0 (power-up)
	{0xD6, 0x00, 0xC0}, // HPD Control
						//    R0xD6[7:6] = HPD Control = 11 (always high)
    {0x15, 0x00, 0x01}, // Input YCbCr 4:2:2 with separate syncs
    {0x16, 0x00, 0x38}, // Output format 444, Input Color Depth = 8
                        //    R0x16[  7] = Output Video Format = 0 (444)
                        //    R0x16[5:4] = Input Video Color Depth = 11 (8 bits/color)
                        //    R0x16[3:2] = Input Video Style = 10 (style 1)
                        //    R0x16[  1] = DDR Input Edge = 0 (falling edge)
                        //    R0x16[  0] = Output Color Space = 0 (RGB)
    {0x18, 0x00, 0xE7}, // Color Space Conversion
                        //    R0x18[  7] = CSC enable = 1 (CSC enabled)
                        //    R0x18[6:5] = CSC Scaling Factor = 11 (+/- 4.0, -16384 - 16380)
                        //    R0x18[4:0] = CSC coefficient A1[12:8] = 00111
    {0x19, 0x00, 0x34}, //    R0x19[7:0] = CSC coefficient A1[ 7:0] =      00110100
    {0x1A, 0x00, 0x04}, //    R0x1A[  5] = CSC coefficient update
                        //    R0x1A[4:0] = CSC coefficient A2[12:8] = 00100
    {0x1B, 0x00, 0xAD}, //    R0x1B[7:0] = CSC coefficient A2[ 7:0] =      10101101
    {0x1C, 0x00, 0x00}, //    R0x1C[4:0] = CSC coefficient A3[12:8] = 00000
    {0x1D, 0x00, 0x00}, //    R0x1D[7:0] = CSC coefficient A3[ 7:0] =      00000000
    {0x1E, 0x00, 0x1C}, //    R0x1E[4:0] = CSC coefficient A4[12:8] = 11100
    {0x1F, 0x00, 0x1B}, //    R0x1F[7:0] = CSC coefficient A4[ 7:0] =      00011011
    {0x20, 0x00, 0x1D}, //    R0x20[4:0] = CSC coefficient B1[12:8] = 11101
    {0x21, 0x00, 0xDC}, //    R0x21[7:0] = CSC coefficient B1[ 7:0] =      11011100
    {0x22, 0x00, 0x04}, //    R0x22[4:0] = CSC coefficient B2[12:8] = 00100
    {0x23, 0x00, 0xAD}, //    R0x23[7:0] = CSC coefficient B2[ 7:0] =      10101101
    {0x24, 0x00, 0x1F}, //    R0x24[4:0] = CSC coefficient B3[12:8] = 11111
    {0x25, 0x00, 0x24}, //    R0x25[7:0] = CSC coefficient B3[ 7:0] =      00100100
    {0x26, 0x00, 0x01}, //    R0x26[4:0] = CSC coefficient B4[12:8] = 00001
    {0x27, 0x00, 0x35}, //    R0x27[7:0] = CSC coefficient B4[ 7:0] =      00110101
    {0x28, 0x00, 0x00}, //    R0x28[4:0] = CSC coefficient C1[12:8] = 00000
    {0x29, 0x00, 0x00}, //    R0x29[7:0] = CSC coefficient C1[ 7:0] =      00000000
    {0x2A, 0x00, 0x04}, //    R0x2A[4:0] = CSC coefficient C2[12:8] = 00100
    {0x2B, 0x00, 0xAD}, //    R0x2B[7:0] = CSC coefficient C2[ 7:0] =      10101101
    {0x2C, 0x00, 0x08}, //    R0x2C[4:0] = CSC coefficient C3[12:8] = 01000
    {0x2D, 0x00, 0x7C}, //    R0x2D[7:0] = CSC coefficient C3[ 7:0] =      01111100
    {0x2E, 0x00, 0x1B}, //    R0x2E[4:0] = CSC coefficient C4[12:8] = 11011
    {0x2F, 0x00, 0x77}, //    R0x2F[7:0] = CSC coefficient C4[ 7:0] =      01110111
    {0x48, 0x00, 0x08}, // Video Input Justification
                        //    R0x48[8:7] = Video Input Justification = 01 (right justified)
    {0x55, 0x00, 0x00}, // Set RGB in AVinfo Frame
                        //    R0x55[6:5] = Output Format = 00 (RGB)
    {0x56, 0x00, 0x28}, // Aspect Ratio
                        //    R0x56[5:4] = Picture Aspect Ratio = 10 (16:9)
                        //    R0x56[3:0] = Active Format Aspect Ratio = 1000 (Same as Aspect Ratio)
    {0x98, 0x00, 0x03}, // ADI Recommended Write
    {0x9A, 0x00, 0xE0}, // ADI Recommended Write
    {0x9C, 0x00, 0x30}, // PLL Filter R1 Value
    {0x9D, 0x00, 0x61}, // Set clock divide
    {0xA2, 0x00, 0xA4}, // ADI Recommended Write
    {0xA3, 0x00, 0xA4}, // ADI Recommended Write
    {0xAF, 0x00, 0x04}, // HDMI/DVI Modes
                        //    R0xAF[  7] = HDCP Enable = 0 (HDCP disabled)
                        //    R0xAF[  4] = Frame Encryption = 0 (Current frame NOT HDCP encrypted)
                        //    R0xAF[3:2] = 01 (fixed)
                        //    R0xAF[  1] = HDMI/DVI Mode Select = 0 (DVI Mode)
    {0xE0, 0x00, 0xD0}, // Must be set to 0xD0 for proper operation
    {0xF9, 0x00, 0x00}, // Fixed I2C Address (This should be set to a non-conflicting I2C address)
	{0xBA, 0x00, 0xA0}  // Adjust clock-to-data delay
};

#else // YCBCR422

/*
 * The video input format of the ADV7511 is set to YCbCr, 16-bit, 4:2:2,
 * ID 1 (separate syncs), Style 1. The video output format is set to
 * YCbCr, 16-bit, 4:2:2, HDMI mode.
 */
#define ZC702_HDMI_CONFIG_LEN  16
ZC702_I2C_CONFIG zc702_hdmi_config[ZC702_HDMI_CONFIG_LEN] =
{
	{0x41, 0x00, 0x10}, // Power Down Control
						//    R0x41[  6] = PowerDown = 0 (power-up)
	{0xD6, 0x00, 0xC0}, // HPD Control
						//    R0xD6[7:6] = HPD Control = 11 (always high)
    {0x15, 0x00, 0x01}, // Input YCbCr 4:2:2 with separate syncs
    {0x16, 0x00, 0xB9}, // Output format 4:2:2, Input Color Depth = 8
                        //    R0x16[  7] = Output Video Format = 1 (4:2:2)
                        //    R0x16[5:4] = Input Video Color Depth = 11 (8 bits/color)
                        //    R0x16[3:2] = Input Video Style = 10 (style 1)
                        //    R0x16[  1] = DDR Input Edge = 0 (falling edge)
                        //    R0x16[  0] = Output Color Space = 1 (YCbCr)
    {0x48, 0x00, 0x08}, // Video Input Justification
                        //    R0x48[8:7] = Video Input Justification = 01 (right justified)
    {0x55, 0x00, 0x20}, // Set RGB in AVinfo Frame
                        //    R0x55[6:5] = Output Format = 01 (YCbCr)
    {0x56, 0x00, 0x28}, // Aspect Ratio
                        //    R0x56[5:4] = Picture Aspect Ratio = 10 (16:9)
                        //    R0x56[3:0] = Active Format Aspect Ratio = 1000 (Same as Aspect Ratio)
    {0x98, 0x00, 0x03}, // ADI Recommended Write
    {0x9A, 0x00, 0xE0}, // ADI Recommended Write
    {0x9C, 0x00, 0x30}, // PLL Filter R1 Value
    {0x9D, 0x00, 0x61}, // Set clock divide
    {0xA2, 0x00, 0xA4}, // ADI Recommended Write
    {0xA3, 0x00, 0xA4}, // ADI Recommended Write
    {0xAF, 0x00, 0x06}, // HDMI/DVI Modes
                        //    R0xAF[  7] = HDCP Enable = 0 (HDCP disabled)
                        //    R0xAF[  4] = Frame Encryption = 0 (Current frame NOT HDCP encrypted)
                        //    R0xAF[3:2] = 01 (fixed)
                        //    R0xAF[  1] = HDMI/DVI Mode Select = 2 (HDMI Mode)
    {0xE0, 0x00, 0xD0}, // Must be set to 0xD0 for proper operation
    {0xF9, 0x00, 0x00}  // Fixed I2C Address (This should be set to a non-conflicting I2C address)
};

#endif


/*
 * The Data field of the SI570 struct will hold the default register values
 * for registers 0x7 to 0xC of a specific SI570 device on a ZC702 board.
 * On ZC702, the frequency of the SI570 defaults to 156.25 MHz.
 *
 * The Init field of the SI570 struct will hold the derived register values
 * for generating a 148.5 MHz output clock which is our default video clock.
 *
 * The Data and Init fields are populated by calling the zc702_si570_init
 * function.
 */

#define ZC702_USRCLK_CONFIG_LEN  6
static ZC702_I2C_CONFIG zc702_usrclk_config[ZC702_USRCLK_CONFIG_LEN] =
{
	{0x07, 0x00, 0x00}, // High Speed / N1 Dividers
						//    R0x07[7:5] = HS_DIV[2:0]
						//	  R0x07[4:0] = N1[6:2]
	{0x08, 0x00, 0x00}, // N1 Divider / Reference Frequency
						//    R0x08[7:6] = N1[1:0]
						//    R0x08[5:0] = RFREQ[37:32]
	{0x09, 0x00, 0x00}, //    R0x09[7:0] = RFREQ[31:24]
	{0x0A, 0x00, 0x00}, //    R0x0A[7:0] = RFREQ[23:16]
	{0x0B, 0x00, 0x00}, //    R0x0B[7:0] = RFREQ[15: 8]
	{0x0C, 0x00, 0x00}  //    R0x0C[7:0] = RFREQ[ 7: 0]
//	{0x87, 0x00, 0x40}  // Reset / Freeze / Memory Control
						//    R0x87[7] = RST_REG      = 0
						//    R0x87[6] = NewFreq      = 1
						//    R0x87[5] = Freeze M     = 0
						//    R0x87[4] = Freeze VCADC = 0
						//    R0x87[0] = RECALL       = 0
};


/*
 * The HS_DIV[2:0] value maps to 6 possible DCO High Speed Divider settings.
 * A table is used to define this mapping and enable the 3-bit encoded HS_DIV[2:0] value
 * to be converted into an absolute value.
 *
 *    HS_DIV[2:0]      'HS_DIV' value
 *
 *        000               4
 *        001               5
 *        010               6
 *        011               7
 *        100               -
 *        101               9
 *        110               -
 *        111              11
 */

static const u8 HS_DIV_lookup[8] =
{
	4, 5, 6, 7, 0, 9, 0, 11
};


/*
 * This table maps each of the valid HS_DIV values to a byte value which encodes the
 * HS_DIV[2:0] into bits [7:5] of a byte.
 *
 *  value of    HS_DIV[2:0]       Byte
 *   HS_DIV      encoding       encoding
 *     4            000         00000000 = 00
 *     5            001         00100000 = 20
 *     6            010         01000000 = 40
 *     7            011         01100000 = 60
 *     9            101         10100000 = A0
 *    11            111         11100000 = E0
 */

static const u8 HS_DIV_lookup_reverse[8] =
{
	0x00, 0x20, 0x40, 0x60, 0x00, 0xA0, 0x00, 0xE0
};


/*
 * The DCO must operate in the frequency range 4.85GHz to 5.67GHz. To achieve this an
 * appropriate combination of HS_DIV and N1 divider values must be selected to correspond
 * with the desired output frequency of the Si570.
 *
 *     HS_DIV can only have values 4, 5, 6, 7, 9 or 11.
 *     N1 can only have values 1 and any even number up to 128.
 *
 * The following table was prepared provides suitable pairs of valid HS_DIV and N1 values
 * that cover ranges of the full output frequency spectrum.
 */

#define ZC702_SI570_LOOKUP_LEN 38
static const ZC702_SI570_LOOKUP SI570_lookup[ZC702_SI570_LOOKUP_LEN] =
{
//   HS_DIV  N1    Start of Frequency Range  hex / MHz
	 {7,    70,    0x00989680}, //  10
	 {6,    74,    0x00A7D8C0}, //  11
	 {7,    58,    0x00B71B00}, //  12
	 {7,    50,    0x00D59F80}, //  14
	 {9,    34,    0x00F42400}, //  16
	 {4,    68,    0x0112A880}, //  18
	 {4,    62,    0x01312D00}, //  20
	 {7,    32,    0x014FB180}, //  22
	 {7,    28,    0x017D7840}, //  25
	{11,    16,    0x01AB3F00}, //  28
	 {4,    38,    0x01E84800}, //  32
	{11,    12,    0x02349340}, //  37
	 {6,    20,    0x02719C40}, //  41
	 {4,    26,    0x02CD29C0}, //  47
	 {9,    10,    0x0337F980}, //  54
	 {5,    16,    0x03A2C940}, //  61
	 {7,    10,    0x042C1D80}, //  70
	 {4,    16,    0x0487AB00}, //  76
	 {7,     8,    0x052F83C0}, //  87
	 {5,    10,    0x05D75C80}, //  98
	{11,     4,    0x069DB9C0}, // 111
	 {5,     8,    0x07459280}, // 122
	 {9,     4,    0x080BEFC0}, // 135
	 {4,     8,	   0x090F5600}, // 152
	 {7,     4,    0x0A5F0780}, // 174
	 {6,     4,    0x0C0EDA60}, // 202.3
	{11,     2,	   0x0D2C3140}, // 221
	 {5,     4,    0x0E7BE2C0}, // 243
	 {9,     2,    0x1017DF80}, // 270
	 {4,     4,    0x121EAC00}, // 304
	 {7,     2,	   0x14AECCC0}, // 347
	 {6,     2,    0x1823CF40}, // 405
	{11,     1,    0x1A492040}, // 441
	 {5,     2,    0x1CF7C580}, // 486
	 {9,     1,    0x20207CC0}, // 539
	 {4,     2,    0x242E15C0}, // 607
	 {7,     1,    0x295D9980}, // 694
	 {6,     1,    0x30385C40}  // 809
};


/*
 * TI Power Regulator Voltage, Current, and Power Readings.
 */

#define ZC702_PMBUS_DATA_LEN 10
static ZC702_PMBUS_DATA zc702_pmbus_data[ZC702_PMBUS_DATA_LEN] =
{
	{"VCCINT",    ZC702_PMBUS0_ADDR, 0, 0.0, 0.0, 0.0},
	{"VCCPINT",   ZC702_PMBUS0_ADDR, 1, 0.0, 0.0, 0.0},
	{"VCCAUX",    ZC702_PMBUS0_ADDR, 2, 0.0, 0.0, 0.0},
	{"VCCPAUX",   ZC702_PMBUS0_ADDR, 3, 0.0, 0.0, 0.0},
	{"VADJ",      ZC702_PMBUS1_ADDR, 0, 0.0, 0.0, 0.0},
	{"VCC1V5",    ZC702_PMBUS1_ADDR, 1, 0.0, 0.0, 0.0},
	{"VCCMIO_PS", ZC702_PMBUS1_ADDR, 2, 0.0, 0.0, 0.0},
	{"VCCBRAM",   ZC702_PMBUS1_ADDR, 3, 0.0, 0.0, 0.0},
	{"VCC3V3",    ZC702_PMBUS2_ADDR, 0, 0.0, 0.0, 0.0},
	{"VCC2V5",    ZC702_PMBUS2_ADDR, 1, 0.0, 0.0, 0.0}
};


#define FMC_IMAGEON_HDMI_IN_MAPPING_LEN  7
static ZC702_I2C_CONFIG2 fmc_imageon_hdmi_in_mapping[FMC_IMAGEON_HDMI_IN_MAPPING_LEN] =
{
	{FMC_IMAGEON_ADV7611_IO_ADDR, 0xF4, 0x00, FMC_IMAGEON_ADV7611_CEC_ADDR<<1},       // CEC
	{FMC_IMAGEON_ADV7611_IO_ADDR, 0xF5, 0x00, FMC_IMAGEON_ADV7611_INFOFRAME_ADDR<<1}, // INFOFRAME
	{FMC_IMAGEON_ADV7611_IO_ADDR, 0xF8, 0x00, FMC_IMAGEON_ADV7611_DPLL_ADDR<<1},      // DPLL
	{FMC_IMAGEON_ADV7611_IO_ADDR, 0xF9, 0x00, FMC_IMAGEON_ADV7611_KSV_ADDR<<1},       // KSV (Repeater)
	{FMC_IMAGEON_ADV7611_IO_ADDR, 0xFA, 0x00, FMC_IMAGEON_ADV7611_EDID_ADDR<<1},      // EDID
	{FMC_IMAGEON_ADV7611_IO_ADDR, 0xFB, 0x00, FMC_IMAGEON_ADV7611_HDMI_ADDR<<1},      // HDMI
	{FMC_IMAGEON_ADV7611_IO_ADDR, 0xFD, 0x00, FMC_IMAGEON_ADV7611_CP_ADDR<<1}         // CP
};


#define FMC_IMAGEON_HDMI_IN_EDID_POST_LEN  5
static ZC702_I2C_CONFIG2 fmc_imageon_hdmi_in_edid_post[FMC_IMAGEON_HDMI_IN_EDID_POST_LEN] =
{
	{FMC_IMAGEON_ADV7611_KSV_ADDR, 0x77, 0x00, 0x00}, // Set the Most Significant Bit of the SPA location to 0
	{FMC_IMAGEON_ADV7611_KSV_ADDR, 0x52, 0x00, 0x20}, // Set the SPA for port B.
	{FMC_IMAGEON_ADV7611_KSV_ADDR, 0x53, 0x00, 0x00}, // Set the SPA for port B.
	{FMC_IMAGEON_ADV7611_KSV_ADDR, 0x70, 0x00, 0x9E}, // Set the Least Significant Byte of the SPA location
	{FMC_IMAGEON_ADV7611_KSV_ADDR, 0x74, 0x00, 0x03}  // Enable the Internal EDID for Ports
};


#define FMC_IMAGEON_HDMI_IN_CONFIG_LEN  41
static ZC702_I2C_CONFIG2 fmc_imageon_hdmi_in_config[FMC_IMAGEON_HDMI_IN_CONFIG_LEN] =
{
	{FMC_IMAGEON_ADV7611_IO_ADDR,   0x01, 0x00, 0x06}, // Prim_Mode = 110b HDMI-GR
	{FMC_IMAGEON_ADV7611_IO_ADDR,   0x02, 0x00, 0xF5}, // Auto CSC, YCrCb out, Set op_656 bit
	{FMC_IMAGEON_ADV7611_IO_ADDR,   0x03, 0x00, 0x80}, // 16-Bit SDR ITU-R BT.656 4:2:2 Mode 0
	{FMC_IMAGEON_ADV7611_IO_ADDR,   0x04, 0x00, 0x62}, // OP_CH_SEL[2:0] = 011b - (P[15:8] Y, P[7:0] CrCb), XTAL_FREQ[1:0] = 01b (28.63636 MHz)
	{FMC_IMAGEON_ADV7611_IO_ADDR,   0x05, 0x00, 0x2C}, // AV Codes on

	{FMC_IMAGEON_ADV7611_CP_ADDR,   0x7B, 0x00, 0x05},

	{FMC_IMAGEON_ADV7611_IO_ADDR, 0x0B, 0x00, 0x44}, // Power up part
	{FMC_IMAGEON_ADV7611_IO_ADDR, 0x0C, 0x00, 0x42}, // Power up part
	{FMC_IMAGEON_ADV7611_IO_ADDR, 0x14, 0x00, 0x7F}, // Max Drive Strength
	{FMC_IMAGEON_ADV7611_IO_ADDR, 0x15, 0x00, 0x80}, // Disable Tristate of Pins
	{FMC_IMAGEON_ADV7611_IO_ADDR, 0x19, 0x00, 0x83}, // LLC DLL phase
	{FMC_IMAGEON_ADV7611_IO_ADDR, 0x33, 0x00, 0x40}, // LLC DLL enable

	{FMC_IMAGEON_ADV7611_CP_ADDR  , 0xBA, 0x00, 0x01}, // Set HDMI FreeRun

	{FMC_IMAGEON_ADV7611_KSV_ADDR , 0x40, 0x00, 0x81}, // Disable HDCP 1.1 features

	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0x9B, 0x00, 0x03}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0xC1, 0x00, 0x01}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0xC2, 0x00, 0x01}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0xC3, 0x00, 0x01}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0xC4, 0x00, 0x01}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0xC5, 0x00, 0x01}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0xC6, 0x00, 0x01}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0xC7, 0x00, 0x01}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0xC8, 0x00, 0x01}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0xC9, 0x00, 0x01}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0xCA, 0x00, 0x01}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0xCB, 0x00, 0x01}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0xCC, 0x00, 0x01}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0x00, 0x00, 0x08}, // Set HDMI Input Port A  (BG_MEAS_PORT_SEL = 001b)
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0x02, 0x00, 0x03}, // Enable Ports A & B in background mode
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0x83, 0x00, 0xFC}, // Enable clock terminators for port A & B
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0x6F, 0x00, 0x0C}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0x85, 0x00, 0x1F}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0x87, 0x00, 0x70}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0x8D, 0x00, 0x04}, // LFG Port A
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0x8E, 0x00, 0x1E}, // HFG Port A
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0x1A, 0x00, 0x8A}, // Unmute audio
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0x57, 0x00, 0xDA}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0x58, 0x00, 0x01}, // ADI recommended setting
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0x75, 0x00, 0x10}, // DDC drive strength
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0x90, 0x00, 0x04}, // LFG Port B
	{FMC_IMAGEON_ADV7611_HDMI_ADDR, 0x91, 0x00, 0x1E}  // HFG Port B
};


int iic_init( XIicPs *IicPs, u16 DeviceId, u32 ClkRate )
{
	int Status;
	XIicPs_Config *IicPs_Config;

	/*
	 * Initialize the IIC driver.
	 */
	IicPs_Config = XIicPs_LookupConfig(DeviceId);
	if (IicPs_Config == NULL) {
		debug_printf("No XIicPs instance found for ID %d\n\r", DeviceId);
		return XST_FAILURE;
	}

	Status = XIicPs_CfgInitialize(IicPs, IicPs_Config, IicPs_Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		debug_printf("XIicPs Initialization failed for ID %d\n\r", DeviceId);
		return XST_FAILURE;
	}

	/*
	 * Set the IIC serial clock rate.
	 */
	Status = XIicPs_SetSClk(IicPs, ClkRate);
	if (Status != XST_SUCCESS) {
		debug_printf("Setting XIicPs clock rate failed for ID %d\n\r", DeviceId);
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}


static int iic_write1( XIicPs *IicPs, u8 Address, u8 Data )
{
	int Status;

	/*
	 * Wait until bus is idle to start another transfer.
	 */
	while (XIicPs_BusIsBusy(IicPs)) {
		/* NOP */
	}

	/*
	 * Send the buffer using the IIC and check for errors.
	 */
	Status = XIicPs_MasterSendPolled(IicPs, &Data, 1, Address);
	if (Status != XST_SUCCESS) {
		debug_printf("XIicPs_MasterSendPolled error!\n\r");
		return XST_FAILURE;
	}

#ifdef I2C_DEBUG
	u8 read_data;
	iic_read1(IicPs, Address, &read_data);
#endif

	return XST_SUCCESS;
}


static int iic_write2( XIicPs *IicPs, u8 Address, u8 Register, u8 Data )
{
	u8 WriteBuffer[2];
	int Status;

	/*
	 * A temporary write buffer must be used which contains both the address
	 * and the data to be written, put the address in first
	 */
	WriteBuffer[0] = Register;
	WriteBuffer[1] = Data;

	/*
	 * Wait until bus is idle to start another transfer.
	 */
	while (XIicPs_BusIsBusy(IicPs)) {
		/* NOP */
	}

	/*
	 * Send the buffer using the IIC and check for errors.
	 */
	Status = XIicPs_MasterSendPolled(IicPs, WriteBuffer, 2, Address);
	if (Status != XST_SUCCESS) {
		debug_printf("XIicPs_MasterSendPolled error!\n\r");
		return XST_FAILURE;
	}

#ifdef I2C_DEBUG
	u8 read_data;
	iic_read2(IicPs, Address, Register, &read_data, 1);
#endif

	return XST_SUCCESS;
}


static void iic_writex( XIicPs *IicPs, u8 Address, ZC702_I2C_CONFIG Config[], u32 Length )
{
   int i;

   for ( i = 0; i < Length; i++ )
   {
      iic_write2(IicPs, Address, Config[i].Reg, Config[i].Init);
   }
}


static void iic_writex2( XIicPs *IicPs, ZC702_I2C_CONFIG2 Config[], u32 Length )
{
   int i;

   for ( i = 0; i < Length; i++ )
   {
      iic_write2(IicPs, Config[i].Addr, Config[i].Reg, Config[i].Init);
   }
}


static int iic_read1( XIicPs *IicPs, u8 Address, u8 *Data )
{
	int Status;

	/*
	* Wait until bus is idle to start another transfer.
	*/
	while (XIicPs_BusIsBusy(IicPs)) {
		/* NOP */
	}

	/*
	* Receive the data.
	*/
	Status = XIicPs_MasterRecvPolled(IicPs, Data, 1, Address);
	if (Status != XST_SUCCESS) {
		debug_printf("XIicPs_MasterRecvPolled error!\n\r");
		return XST_FAILURE;
	}

	debug_printf("[iic_read1] 0x%02X=0x%02X\n\r", Address, *Data);

	return XST_SUCCESS;
}


static int iic_read2( XIicPs *IicPs, u8 Address, u8 Register, u8 *Data, int ByteCount )
{
	int Status;

	/*
	 * Wait until bus is idle to start another transfer.
	 */
	while (XIicPs_BusIsBusy(IicPs)) {
		/* NOP */
	}

	/*
	 * Set the IIC Repeated Start option.
	 */
	Status = XIicPs_SetOptions(IicPs, XIICPS_REP_START_OPTION);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Send the buffer using the IIC and check for errors.
	 */
	Status = XIicPs_MasterSendPolled(IicPs, &Register, 1, Address);
	if (Status != XST_SUCCESS) {
		debug_printf("XIicPs_MasterSendPolled error!\n\r");
		return XST_FAILURE;
	}

	/*
	 * Receive the data.
	 */
	Status = XIicPs_MasterRecvPolled(IicPs, Data, ByteCount, Address);
	if (Status != XST_SUCCESS) {
		debug_printf("XIicPs_MasterRecvPolled error!\n\r");
		return XST_FAILURE;
	}

	/*
	 * Clear the IIC Repeated Start option.
	 */
	Status = XIicPs_ClearOptions(IicPs, XIICPS_REP_START_OPTION);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	debug_printf("[iic_read2] 0x%02X(0x%02X)=0x%02X\n\r", Address, Register, *Data);

	return XST_SUCCESS;
}


static void iic_readx( XIicPs *IicPs, u8 Address, ZC702_I2C_CONFIG Config[], u32 Length )
{
   int i;

   for ( i = 0; i < Length; i++ ) {
      iic_read2(IicPs, Address, Config[i].Reg, &Config[i].Data, 1);
   }
}


static void iic_readx2( XIicPs *IicPs, ZC702_I2C_CONFIG2 Config[], u32 Length )
{
   int i;

   for ( i = 0; i < Length; i++ ) {
      iic_read2(IicPs, Config[i].Addr, Config[i].Reg, &Config[i].Data, 1);
   }
}


void zc702_iic_mux( XIicPs *IicPs, u8 MuxSelect )
{
   iic_write1(IicPs, ZC702_I2C_MUX_ADDR, MuxSelect);
}


int zc702_hdmi_init( XIicPs *IicPs )
{
	u8 data = 0x00;
	u8 hpd_ctrl_mask = 0x40; // bit 6 = state of HPD

	// set IIC MUX
	zc702_iic_mux( IicPs, ZC702_I2C_SELECT_HDMI );

	// check HPD state
	iic_read2( IicPs, ZC702_HDMI_ADDR, 0x42, &data, 1);
	if((data & hpd_ctrl_mask) != hpd_ctrl_mask) {
		printf("Error: No monitor detected on HDMI input!\r\n");
		debug_printf("HPD state is 0x%02X!\r\n", data);
		return XST_FAILURE;
	}

	// write ADV7511 configuration
	iic_writex( IicPs, ZC702_HDMI_ADDR, zc702_hdmi_config, ZC702_HDMI_CONFIG_LEN );

	// read back video input and output configuration
	iic_readx( IicPs, ZC702_HDMI_ADDR, zc702_hdmi_config, ZC702_HDMI_CONFIG_LEN );

	return XST_SUCCESS;
}

