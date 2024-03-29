/******************************************************************************
 *  Copyright (c) 2016, Xilinx, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2.  Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *  3.  Neither the name of the copyright holder nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/******************************************************************************
 *
 *
 * @file arduino_lcd18.c
 *
 * This is the module for the Adafruit LCD18 on Arduino interface.
 * https://www.adafruit.com/product/802.
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who  Date     Changes
 * ----- --- -------- -----------------------------------------------
 * 1.00a pp  11/16/16 release
 *
 * </pre>
 *
 *****************************************************************************/
#include <stdint.h>
#include "xparameters.h"
#include "circular_buffer.h"
#include "gpio.h"
#include "spi_v2.h"
#include "uart_v2.h"
#include "timer.h"
#include "xio_switch.h"
#include "xil_cache.h"
#include "xsysmon.h"
#include "time.h"
//#include "bmp.h"

#define INIT				    0x1
#define DRAW 					0x2
#define DISPLAY					0x3
#define GPIO					0x4
#define PWM						0x5
#define FILL 					0x6
#define SLOW_FILL				0x7
#define DRAW_IMAGE				0x8
#define FILL_COLOR				0x9
#define SIMPLE					0xa
#define EIGHT					0xb
#define RECTANGLE				0xc
#define CIRCLE					0xd
#define	TRIANGLE				0xe
#define ELLIPSE					0xf
#define CURVE					0x10
#define DRAWLINE				0x11
#define FONT 					0x12
#define TEXTMODE				0x13
#define GRAPHICSMODE			0x14
#define LAYER					0x15
#define LAYERTRANS 				0x16
#define CAMERA1					0x17
#define CAMERA2					0x18
#define LETTER					0x19
#define LETTER_SETUP			0x20
#define FONT_SIZE 				0X21
#define TEXT_TRANSPARENT		0x22
#define BACKGROUND_COLOR		0x23
#define TEXT_CURSOR				0x24
#define TEXT_WRITE				0x25
#define LAYER_MODE				0x26
#define LAYER_EFFECT			0x27
#define CLEAR_WINDOW			0x28
#define CLEAR_MEM				0x29

/* Layer Effects */
#define LAYER1 					0
#define LAYER2					1
#define TRANSPARENT 			2
#define LIGHTEN					3
#define OR						4
#define AND						5
#define FLOATING 				6

/* Pin Assignments */
#define RX						0
#define TX						1
//cs - 3
//reset - 7
#define reset					0x7F
#define	start_transfer			0xFE
#define end_transfer			0xFF

/* Assignments */
#define width 					800
#define height 					480
// Colors (RGB565)
#define	RA8875_BLACK            0x0000
#define	RA8875_BLUE             0x001F
#define	RA8875_RED              0xF800
#define	RA8875_GREEN            0x07E0
#define RA8875_CYAN             0x07FF
#define RA8875_MAGENTA          0xF81F
#define RA8875_YELLOW           0xFFE0
#define RA8875_WHITE            0xFFFF

// Command/Data pins for SPI
#define RA8875_DATAWRITE        0x00
#define RA8875_DATAREAD         0x40
#define RA8875_CMDWRITE         0x80
#define RA8875_CMDREAD          0xC0

// Registers & bits
#define RA8875_PWRR             0x01
#define RA8875_PWRR_DISPON      0x80
#define RA8875_PWRR_DISPOFF     0x00
#define RA8875_PWRR_SLEEP       0x02
#define RA8875_PWRR_NORMAL      0x00
#define RA8875_PWRR_SOFTRESET   0x01

#define RA8875_MRWC             0x02

#define RA8875_GPIOX            0xC7

#define RA8875_PLLC1            0x88
#define RA8875_PLLC1_PLLDIV2    0x80
#define RA8875_PLLC1_PLLDIV1    0x00

#define RA8875_PLLC2            0x89
#define RA8875_PLLC2_DIV1       0x00
#define RA8875_PLLC2_DIV2       0x01
#define RA8875_PLLC2_DIV4       0x02
#define RA8875_PLLC2_DIV8       0x03
#define RA8875_PLLC2_DIV16      0x04
#define RA8875_PLLC2_DIV32      0x05
#define RA8875_PLLC2_DIV64      0x06
#define RA8875_PLLC2_DIV128     0x07

#define RA8875_SYSR             0x10
#define RA8875_SYSR_8BPP        0x00
#define RA8875_SYSR_16BPP       0x0C
#define RA8875_SYSR_MCU8        0x00
#define RA8875_SYSR_MCU16       0x03

#define RA8875_PCSR             0x04
#define RA8875_PCSR_PDATR       0x00
#define RA8875_PCSR_PDATL       0x80
#define RA8875_PCSR_CLK         0x00
#define RA8875_PCSR_2CLK        0x01
#define RA8875_PCSR_4CLK        0x02
#define RA8875_PCSR_8CLK        0x03

#define RA8875_HDWR             0x14

#define RA8875_HNDFTR           0x15
#define RA8875_HNDFTR_DE_HIGH   0x00
#define RA8875_HNDFTR_DE_LOW    0x80

#define RA8875_HNDR             0x16
#define RA8875_HSTR             0x17
#define RA8875_HPWR             0x18
#define RA8875_HPWR_LOW         0x00
#define RA8875_HPWR_HIGH        0x80

#define RA8875_VDHR0            0x19
#define RA8875_VDHR1            0x1A
#define RA8875_VNDR0            0x1B
#define RA8875_VNDR1            0x1C
#define RA8875_VSTR0            0x1D
#define RA8875_VSTR1            0x1E
#define RA8875_VPWR             0x1F
#define RA8875_VPWR_LOW         0x00
#define RA8875_VPWR_HIGH        0x80

#define RA8875_HSAW0            0x30
#define RA8875_HSAW1            0x31
#define RA8875_VSAW0            0x32
#define RA8875_VSAW1            0x33

#define RA8875_HEAW0            0x34
#define RA8875_HEAW1            0x35
#define RA8875_VEAW0            0x36
#define RA8875_VEAW1            0x37

#define RA8875_MCLR             0x8E
#define RA8875_MCLR_START       0x80
#define RA8875_MCLR_STOP        0x00
#define RA8875_MCLR_READSTATUS  0x80
#define RA8875_MCLR_FULL        0x00
#define RA8875_MCLR_ACTIVE      0x40

#define RA8875_DCR                    0x90
#define RA8875_DCR_LINESQUTRI_START   0x80
#define RA8875_DCR_LINESQUTRI_STOP    0x00
#define RA8875_DCR_LINESQUTRI_STATUS  0x80
#define RA8875_DCR_CIRCLE_START       0x40
#define RA8875_DCR_CIRCLE_STATUS      0x40
#define RA8875_DCR_CIRCLE_STOP        0x00
#define RA8875_DCR_FILL               0x20
#define RA8875_DCR_NOFILL             0x00
#define RA8875_DCR_DRAWLINE           0x00
#define RA8875_DCR_DRAWTRIANGLE       0x01
#define RA8875_DCR_DRAWSQUARE         0x10


#define RA8875_ELLIPSE                0xA0
#define RA8875_ELLIPSE_STATUS         0x80

#define RA8875_MWCR0            0x40
#define RA8875_MWCR0_GFXMODE    0x00
#define RA8875_MWCR0_TXTMODE    0x80

#define RA8875_CURH0            0x46
#define RA8875_CURH1            0x47
#define RA8875_CURV0            0x48
#define RA8875_CURV1            0x49

#define RA8875_P1CR             0x8A
#define RA8875_P1CR_ENABLE      0x80
#define RA8875_P1CR_DISABLE     0x00
#define RA8875_P1CR_CLKOUT      0x10
#define RA8875_P1CR_PWMOUT      0x00

#define RA8875_P1DCR            0x8B

#define RA8875_P2CR             0x8C
#define RA8875_P2CR_ENABLE      0x80
#define RA8875_P2CR_DISABLE     0x00
#define RA8875_P2CR_CLKOUT      0x10
#define RA8875_P2CR_PWMOUT      0x00

#define RA8875_P2DCR            0x8D

#define RA8875_PWM_CLK_DIV1     0x00
#define RA8875_PWM_CLK_DIV2     0x01
#define RA8875_PWM_CLK_DIV4     0x02
#define RA8875_PWM_CLK_DIV8     0x03
#define RA8875_PWM_CLK_DIV16    0x04
#define RA8875_PWM_CLK_DIV32    0x05
#define RA8875_PWM_CLK_DIV64    0x06
#define RA8875_PWM_CLK_DIV128   0x07
#define RA8875_PWM_CLK_DIV256   0x08
#define RA8875_PWM_CLK_DIV512   0x09
#define RA8875_PWM_CLK_DIV1024  0x0A
#define RA8875_PWM_CLK_DIV2048  0x0B
#define RA8875_PWM_CLK_DIV4096  0x0C
#define RA8875_PWM_CLK_DIV8192  0x0D
#define RA8875_PWM_CLK_DIV16384 0x0E
#define RA8875_PWM_CLK_DIV32768 0x0F

#define RA8875_TPCR0                  0x70
#define RA8875_TPCR0_ENABLE           0x80
#define RA8875_TPCR0_DISABLE          0x00
#define RA8875_TPCR0_WAIT_512CLK      0x00
#define RA8875_TPCR0_WAIT_1024CLK     0x10
#define RA8875_TPCR0_WAIT_2048CLK     0x20
#define RA8875_TPCR0_WAIT_4096CLK     0x30
#define RA8875_TPCR0_WAIT_8192CLK     0x40
#define RA8875_TPCR0_WAIT_16384CLK    0x50
#define RA8875_TPCR0_WAIT_32768CLK    0x60
#define RA8875_TPCR0_WAIT_65536CLK    0x70
#define RA8875_TPCR0_WAKEENABLE       0x08
#define RA8875_TPCR0_WAKEDISABLE      0x00
#define RA8875_TPCR0_ADCCLK_DIV1      0x00
#define RA8875_TPCR0_ADCCLK_DIV2      0x01
#define RA8875_TPCR0_ADCCLK_DIV4      0x02
#define RA8875_TPCR0_ADCCLK_DIV8      0x03
#define RA8875_TPCR0_ADCCLK_DIV16     0x04
#define RA8875_TPCR0_ADCCLK_DIV32     0x05
#define RA8875_TPCR0_ADCCLK_DIV64     0x06
#define RA8875_TPCR0_ADCCLK_DIV128    0x07

#define RA8875_TPCR1            0x71
#define RA8875_TPCR1_AUTO       0x00
#define RA8875_TPCR1_MANUAL     0x40
#define RA8875_TPCR1_VREFINT    0x00
#define RA8875_TPCR1_VREFEXT    0x20
#define RA8875_TPCR1_DEBOUNCE   0x04
#define RA8875_TPCR1_NODEBOUNCE 0x00
#define RA8875_TPCR1_IDLE       0x00
#define RA8875_TPCR1_WAIT       0x01
#define RA8875_TPCR1_LATCHX     0x02
#define RA8875_TPCR1_LATCHY     0x03

#define RA8875_TPXH             0x72
#define RA8875_TPYH             0x73
#define RA8875_TPXYL            0x74

#define RA8875_INTC1            0xF0
#define RA8875_INTC1_KEY        0x10
#define RA8875_INTC1_DMA        0x08
#define RA8875_INTC1_TP         0x04
#define RA8875_INTC1_BTE        0x02

#define RA8875_INTC2            0xF1
#define RA8875_INTC2_KEY        0x10
#define RA8875_INTC2_DMA        0x08
#define RA8875_INTC2_TP         0x04
#define RA8875_INTC2_BTE        0x02

#define RA8875_DPCR				0X20
#define RA8875_DPCR_ON			0x80 //0100 0000 |=
#define RA8875_DPCR_OFF			0xBF //1011 1111 &=

#define RA8875_MWCR0            0x40
#define RA8875_MWCR1			0x41

#define RA8875_LTPR0            0x52
#define RA8875_LTPR1			0x53

#define V_REF 3.33
#define SYSMON_DEVICE_ID XPAR_SYSMON_0_DEVICE_ID

gpio gpio_device;
spi spi_device;
uart uart_device;
u16 reset_dc_d7_to_d0;
static XSysMon SysMonInst;
XSysMon_Config *SysMonConfigPtr;
XSysMon *SysMonInstPtr = &SysMonInst;


uint8_t writeBuffer[2], readBuffer[2];

/* shared variables */
int textScale = 1;
int currentLayer = 0;

/* LOW LEVEL FUNCTIONS*/
void writeCommand(uint8_t cmd){
	writeBuffer[0] = RA8875_CMDWRITE;
	writeBuffer[1] = cmd;
	spi_transfer(spi_device, (const uint8_t*) writeBuffer, (uint8_t*) readBuffer, 2);
}

void writeData(uint8_t d){
	writeBuffer[0] = RA8875_DATAWRITE;
	writeBuffer[1] = d;
	spi_transfer(spi_device, (const uint8_t*) writeBuffer, (uint8_t*) readBuffer, 2);
}

void writeReg(uint8_t reg, uint8_t val){
	writeCommand(reg);
	writeData(val);
}

uint8_t readData(void)
{
	writeBuffer[0] = RA8875_DATAREAD;
	writeBuffer[1] = 0x0;
	spi_transfer(spi_device, writeBuffer, readBuffer, 2);
  return readBuffer[1];
}

uint8_t readReg(uint8_t reg)
{
  writeCommand(reg);
  return readData();
}

/* FUNCTIONS */

int waitPoll(uint8_t regname, uint8_t waitflag) {
  /* Wait for the command to finish */
  while (1)
  {
    uint8_t temp = readReg(regname);
    if (!(temp & waitflag))
      return 1;
  }
  return 0; // MEMEFIX: yeah i know, unreached! - add timeout?
}

void PLLinit(void){ //Fixed to 800x480
	//Initializes PLL on Chip
	writeReg(RA8875_PLLC1, RA8875_PLLC1_PLLDIV1 + 10);
	writeReg(RA8875_PLLC2, RA8875_PLLC2_DIV4);
}

//fast init
void initialize(void){ //Fixed to 800x480
	//Initializes the driver IC (clock setup, etc.)
	PLLinit();
	//Change RA8875_SYSR_16BPP to RA8875_SYSR_8BPP to change from 64k to 256 colors. Allows for layers to be used
	//RA8875_SYSR_MCU8 - 8 bit mode, RA8875_SYSR_MCU16 - 16 bit mode
//	writeReg(RA8875_SYSR, RA8875_SYSR_16BPP | RA8875_SYSR_MCU8);
	writeReg(RA8875_SYSR, RA8875_SYSR_8BPP | RA8875_SYSR_MCU8);

	/* Timing values */
	uint8_t pixclk;
	uint8_t hsync_start;
	uint8_t hsync_pw;
	uint8_t hsync_finetune;
	uint8_t hsync_nondisp;
	uint8_t vsync_pw;
	uint16_t vsync_nondisp;
	uint16_t vsync_start;

	pixclk          = RA8875_PCSR_PDATL | RA8875_PCSR_2CLK;
	hsync_nondisp   = 26;
	hsync_start     = 32;
	hsync_pw        = 96;
	hsync_finetune  = 0;
	vsync_nondisp   = 32;
	vsync_start     = 23;
	vsync_pw        = 2;

	writeReg(RA8875_PCSR, pixclk);

	/* Horizontal settings registers */
	writeReg(RA8875_HDWR, (width / 8) - 1);                          // H width: (HDWR + 1) * 8 = 480
	writeReg(RA8875_HNDFTR, RA8875_HNDFTR_DE_HIGH + hsync_finetune);
	writeReg(RA8875_HNDR, (hsync_nondisp - hsync_finetune - 2)/8);    // H non-display: HNDR * 8 + HNDFTR + 2 = 10
	writeReg(RA8875_HSTR, hsync_start/8 - 1);                         // Hsync start: (HSTR + 1)*8
	writeReg(RA8875_HPWR, RA8875_HPWR_LOW + (hsync_pw/8 - 1));        // HSync pulse width = (HPWR+1) * 8

	/* Vertical settings registers */
	writeReg(RA8875_VDHR0, (uint16_t)(height - 1) & 0xFF);
	writeReg(RA8875_VDHR1, (uint16_t)(height - 1) >> 8);
	writeReg(RA8875_VNDR0, vsync_nondisp-1);                          // V non-display period = VNDR + 1
	writeReg(RA8875_VNDR1, vsync_nondisp >> 8);
	writeReg(RA8875_VSTR0, vsync_start-1);                            // Vsync start position = VSTR + 1
	writeReg(RA8875_VSTR1, vsync_start >> 8);
	writeReg(RA8875_VPWR, RA8875_VPWR_LOW + vsync_pw - 1);            // Vsync pulse width = VPWR + 1

	/* Set active window X */
	writeReg(RA8875_HSAW0, 0);                                        // horizontal start point
	writeReg(RA8875_HSAW1, 0);
	writeReg(RA8875_HEAW0, (uint16_t)(width - 1) & 0xFF);            // horizontal end point
	writeReg(RA8875_HEAW1, (uint16_t)(width - 1) >> 8);

	/* Set active window Y */
	writeReg(RA8875_VSAW0, 0);                                        // vertical start point
	writeReg(RA8875_VSAW1, 0);
	writeReg(RA8875_VEAW0, (uint16_t)(height - 1) & 0xFF);           // horizontal end point
	writeReg(RA8875_VEAW1, (uint16_t)(height - 1) >> 8);

	 /* Clear the entire window */
	writeReg(RA8875_MCLR, RA8875_MCLR_START | RA8875_MCLR_FULL);
}
void displayOn(int on){
	if (on)
		writeReg(RA8875_PWRR, RA8875_PWRR_NORMAL | RA8875_PWRR_DISPON);
	else
		writeReg(RA8875_PWRR, RA8875_PWRR_NORMAL | RA8875_PWRR_DISPOFF);
}




/* DISPLAY MODES */

void textMode(void){
	writeCommand(RA8875_MWCR0);
	uint8_t temp = readData();
	temp |= RA8875_MWCR0_TXTMODE; // Set bit 7
	writeData(temp);

	/* Select the internal (ROM) font */
	writeCommand(0x21);
	temp = readData();
	temp &= ~((1<<7) | (1<<5)); // Clear bits 7 and 5
	writeData(temp);
}

void graphicsMode(void){
	writeCommand(RA8875_MWCR0);
	uint8_t temp = readData();
	temp &= ~RA8875_MWCR0_TXTMODE; // bit #7
	writeData(temp);
}

/* TEXT FUNCTIONS */

void textSetCursor(uint16_t x, uint16_t y)
{
  /* Set cursor location */
  writeCommand(0x2A);
  writeData(x & 0xFF);
  writeCommand(0x2B);
  writeData(x >> 8);
  writeCommand(0x2C);
  writeData(y & 0xFF);
  writeCommand(0x2D);
  writeData(y >> 8);
}

void textColor(uint16_t foreColor, uint16_t bgColor)
{
  /* Set Fore Color */
  writeCommand(0x63);
  writeData((foreColor & 0xf800) >> 11);
  writeCommand(0x64);
  writeData((foreColor & 0x07e0) >> 5);
  writeCommand(0x65);
  writeData((foreColor & 0x001f));

  /* Set Background Color */
  writeCommand(0x60);
  writeData((bgColor & 0xf800) >> 11);
  writeCommand(0x61);
  writeData((bgColor & 0x07e0) >> 5);
  writeCommand(0x62);
  writeData((bgColor & 0x001f));

  /* Clear transparency flag */
  writeCommand(0x22);
  uint8_t temp = readData();
  temp &= ~(1<<6); // Clear bit 6
  writeData(temp);
}

void textTransparent(uint16_t foreColor)
{
  /* Set Fore Color */
  writeCommand(0x63);
  writeData((foreColor & 0xf800) >> 11);
  writeCommand(0x64);
  writeData((foreColor & 0x07e0) >> 5);
  writeCommand(0x65);
  writeData((foreColor & 0x001f));

  /* Set transparency flag */
  writeCommand(0x22);
  uint8_t temp = readData();
  temp |= (1<<6); // Set bit 6
  writeData(temp);
}

void textEnlarge(uint8_t scale)
{
  if (scale > 3) scale = 3;

  /* Set font size flags */
  writeCommand(0x22);
  uint8_t temp = readData();
  temp &= ~(0xF); // Clears bits 0..3
  temp |= scale << 2;
  temp |= scale;
  writeData(temp);

  textScale = scale;
}

void textWrite(const char* buffer, uint16_t len)
{
	int counter = 0;
	if (len == 0) len = strlen(buffer);
		writeCommand(RA8875_MRWC);
	for (uint16_t i=0;i<len;i++){
		writeData(buffer[i]);
//		if(textScale > 1){ //if text enlarged, give more time to draw
	for(int i=0; i<1000; ++i){
		counter++;
	}
//		}

	}
}



/**************************************************************************/
/*!
      Change the beam scan direction on display
	  Parameters:
	  invertH: true(inverted),false(normal) horizontal
	  invertV: true(inverted),false(normal) vertical
*/
/**************************************************************************/
//void scanDirection(int invertH,int invertV)
//{
//	uint8_t DPCR_Reg = 0;
//	invertH == 1 ? DPCR_Reg |= (1 << 3) : DPCR_Reg &= ~(1 << 3);
//	invertV == 1 ? DPCR_Reg |= (1 << 2) : DPCR_Reg &= ~(1 << 2);
//	writeRegister(RA8875_DPCR,_DPCR_Reg);
//}

/* layer functions */

void clearActiveWindow(int full)
{
	uint8_t temp;
	temp = readReg(RA8875_MCLR);
	if(full == 1){
		temp &= ~(1 << 6);
	}
	else{
		temp |= (1 << 6);
	}
	writeData(temp);
}

void clearMemory(int stop)
{
	uint8_t temp;
	temp = readReg(RA8875_MCLR);
	if(stop == 1){
		temp &= ~(1 << 7);
	}
	else{
		temp |= (1 << 7);
	}
	writeData(temp);
}

void layerEffect(int effect){
	uint8_t	reg = 0b00000000;
	switch(effect){
		case LAYER1: //only layer 1 visible  [000]
			//do nothing
		break;
		case LAYER2: //only layer 2 visible  [001]
			reg |= (1 << 0);
		break;
		case TRANSPARENT: //transparent mode [011]
			reg |= (1 << 0); reg |= (1 << 1);
		break;
		case LIGHTEN: //lighten-overlay mode [010]
			reg |= (1 << 1);
		break;
		case OR: //boolean OR mode           [100]
			reg |= (1 << 2);
		break;
		case AND: //boolean AND mode         [101]
			reg |= (1 << 0); reg |= (1 << 2);
		break;
		case FLOATING: //floating windows    [110]
			reg |= (1 << 1); reg |= (1 << 2);
		break;
		default:
			//do nothing
		break;
	}
	writeReg(RA8875_LTPR0,reg);
}

void useLayers(int on){
//	if(on == 1){ //turn layers on
//		writeCommand(RA8875_DPCR);
//		uint8_t temp = readData();
//		temp |= RA8875_DPCR_ON; // Set bit 7
//		writeData(temp);
//	}
//	else{ //turn off layers
//		writeCommand(RA8875_DPCR);
//		uint8_t temp = readData();
//		temp &= RA8875_DPCR_OFF; // Set bit 7
//		writeData(temp);
//	}
	uint8_t _DPCR_Reg = 0;
	if(on == 1){ //turn layers on
		_DPCR_Reg |= (1 << 7);
		clearActiveWindow(1);
	}
	else{ //turn off layers
		_DPCR_Reg &= ~(1 << 7);
		clearActiveWindow(0);
	}
	writeReg(RA8875_DPCR, _DPCR_Reg);
}

void writeTo(int layer){
	uint8_t temp = readReg(RA8875_MWCR1);
	if(layer == 0){
		currentLayer = 0;
		temp &= ~((1<<3) | (1<<2));// Clear bits 3 and 2
		temp &= ~(1 << 0); //clear bit 0
	}
	else{
		currentLayer = 1;
		temp &= ~((1<<3) | (1<<2));// Clear bits 3 and 2
		temp |= (1 << 0); //bit set 0
	}
	writeData(temp);
}

void layerTransparency(uint8_t layer1,uint8_t layer2)
{
	if (layer1 > 8) layer1 = 8;
	if (layer2 > 8) layer2 = 8;
	writeReg(RA8875_LTPR1, ((layer2 & 0xF) << 4) | (layer1 & 0xF));
}

/**************************************************************************/
/*!
      Draws a single pixel at the specified location

      @args x[in]     The 0-based x location
      @args y[in]     The 0-base y location
      @args color[in] The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void drawPixel(int16_t x, int16_t y, uint16_t color){ //maybe change input to two color inputs (each one half of the color)
	writeReg(RA8875_CURH0, x);
	writeReg(RA8875_CURH1, x >> 8);
	writeReg(RA8875_CURV0, y);
	writeReg(RA8875_CURV1, y >> 8);
	writeCommand(RA8875_MRWC);
	uint8_t w_buffer[4];
	uint8_t r_buffer[4];
	w_buffer[0] = RA8875_DATAWRITE;
//	w_buffer[1] = color >> 8;
//	w_buffer[2] = color;
	w_buffer[1] = (color & 0xFF00) >> 8;
	w_buffer[2] = color & 0xFF; //mask this properly
	gpio_write(gpio_device, start_transfer);
	spi_transfer(spi_device, (const uint8_t*) w_buffer, (uint8_t*) r_buffer, 3);
	gpio_write(gpio_device, end_transfer);
}

void drawPixel8bit(int16_t x, int16_t y, uint8_t color){ //maybe change input to two color inputs (each one half of the color)
	writeReg(RA8875_CURH0, x);
	writeReg(RA8875_CURH1, x >> 8);
	writeReg(RA8875_CURV0, y);
	writeReg(RA8875_CURV1, y >> 8);
	writeCommand(RA8875_MRWC);
	uint8_t w_buffer[4];
	uint8_t r_buffer[4];
	w_buffer[0] = RA8875_DATAWRITE;
	w_buffer[1] = color;
	spi_transfer(spi_device, (const uint8_t*) w_buffer, (uint8_t*) r_buffer, 2);
}

void rectHelper(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, int filled){
  /* Set X */
  writeCommand(0x91);
  writeData(x);
  writeCommand(0x92);
  writeData(x >> 8);

  /* Set Y */
  writeCommand(0x93);
  writeData(y);
  writeCommand(0x94);
  writeData(y >> 8);

  /* Set X1 */
  writeCommand(0x95);
  writeData(w);
  writeCommand(0x96);
  writeData((w) >> 8);

  /* Set Y1 */
  writeCommand(0x97);
  writeData(h);
  writeCommand(0x98);
  writeData((h) >> 8);

  /* Set Color */
  writeCommand(0x63);
  writeData((color & 0xf800) >> 11);
  writeCommand(0x64);
  writeData((color & 0x07e0) >> 5);
  writeCommand(0x65);
  writeData((color & 0x001f));

  /* Draw! */
  writeCommand(RA8875_DCR);
  if (filled)
  {
    writeData(0xB0);
  }
  else
  {
    writeData(0x90);
  }

  /* Wait for the command to finish */
//  waitPoll(RA8875_DCR, RA8875_DCR_LINESQUTRI_STATUS);
}

void circleHelper(int16_t x0, int16_t y0, int16_t r, uint16_t color, int filled)
{
  /* Set X */
  writeCommand(0x99);
  writeData(x0);
  writeCommand(0x9a);
  writeData(x0 >> 8);

  /* Set Y */
  writeCommand(0x9b);
  writeData(y0);
  writeCommand(0x9c);
  writeData(y0 >> 8);

  /* Set Radius */
  writeCommand(0x9d);
  writeData(r);

  /* Set Color */
  writeCommand(0x63);
  writeData((color & 0xf800) >> 11);
  writeCommand(0x64);
  writeData((color & 0x07e0) >> 5);
  writeCommand(0x65);
  writeData((color & 0x001f));

  /* Draw! */
  writeCommand(RA8875_DCR);
  if (filled)
  {
    writeData(RA8875_DCR_CIRCLE_START | RA8875_DCR_FILL);
  }
  else
  {
    writeData(RA8875_DCR_CIRCLE_START | RA8875_DCR_NOFILL);
  }

  /* Wait for the command to finish */
//  waitPoll(RA8875_DCR, RA8875_DCR_CIRCLE_STATUS);
}

void triangleHelper(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color, int filled)
{
  /* Set Point 0 */
  writeCommand(0x91);
  writeData(x0);
  writeCommand(0x92);
  writeData(x0 >> 8);
  writeCommand(0x93);
  writeData(y0);
  writeCommand(0x94);
  writeData(y0 >> 8);

  /* Set Point 1 */
  writeCommand(0x95);
  writeData(x1);
  writeCommand(0x96);
  writeData(x1 >> 8);
  writeCommand(0x97);
  writeData(y1);
  writeCommand(0x98);
  writeData(y1 >> 8);

  /* Set Point 2 */
  writeCommand(0xA9);
  writeData(x2);
  writeCommand(0xAA);
  writeData(x2 >> 8);
  writeCommand(0xAB);
  writeData(y2);
  writeCommand(0xAC);
  writeData(y2 >> 8);

  /* Set Color */
  writeCommand(0x63);
  writeData((color & 0xf800) >> 11);
  writeCommand(0x64);
  writeData((color & 0x07e0) >> 5);
  writeCommand(0x65);
  writeData((color & 0x001f));

  /* Draw! */
  writeCommand(RA8875_DCR);
  if (filled)
  {
    writeData(0xA1);
  }
  else
  {
    writeData(0x81);
  }

  /* Wait for the command to finish */
//  waitPoll(RA8875_DCR, RA8875_DCR_LINESQUTRI_STATUS);
}

void ellipseHelper(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint16_t color, int filled)
{
  /* Set Center Point */
  writeCommand(0xA5);
  writeData(xCenter);
  writeCommand(0xA6);
  writeData(xCenter >> 8);
  writeCommand(0xA7);
  writeData(yCenter);
  writeCommand(0xA8);
  writeData(yCenter >> 8);

  /* Set Long and Short Axis */
  writeCommand(0xA1);
  writeData(longAxis);
  writeCommand(0xA2);
  writeData(longAxis >> 8);
  writeCommand(0xA3);
  writeData(shortAxis);
  writeCommand(0xA4);
  writeData(shortAxis >> 8);

  /* Set Color */
  writeCommand(0x63);
  writeData((color & 0xf800) >> 11);
  writeCommand(0x64);
  writeData((color & 0x07e0) >> 5);
  writeCommand(0x65);
  writeData((color & 0x001f));

  /* Draw! */
  writeCommand(0xA0);
  if (filled)
  {
    writeData(0xC0);
  }
  else
  {
    writeData(0x80);
  }

  /* Wait for the command to finish */
//  waitPoll(RA8875_ELLIPSE, RA8875_ELLIPSE_STATUS);
}

void curveHelper(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint8_t curvePart, uint16_t color, int filled)
{
  /* Set Center Point */
  writeCommand(0xA5);
  writeData(xCenter);
  writeCommand(0xA6);
  writeData(xCenter >> 8);
  writeCommand(0xA7);
  writeData(yCenter);
  writeCommand(0xA8);
  writeData(yCenter >> 8);

  /* Set Long and Short Axis */
  writeCommand(0xA1);
  writeData(longAxis);
  writeCommand(0xA2);
  writeData(longAxis >> 8);
  writeCommand(0xA3);
  writeData(shortAxis);
  writeCommand(0xA4);
  writeData(shortAxis >> 8);

  /* Set Color */
  writeCommand(0x63);
  writeData((color & 0xf800) >> 11);
  writeCommand(0x64);
  writeData((color & 0x07e0) >> 5);
  writeCommand(0x65);
  writeData((color & 0x001f));

  /* Draw! */
  writeCommand(0xA0);
  if (filled)
  {
    writeData(0xD0 | (curvePart & 0x03));
  }
  else
  {
    writeData(0x90 | (curvePart & 0x03));
  }

  /* Wait for the command to finish */
//  waitPoll(RA8875_ELLIPSE, RA8875_ELLIPSE_STATUS);
}

/**************************************************************************/
/*!
      Draws a HW accelerated line on the display

      @args x0[in]    The 0-based starting x location
      @args y0[in]    The 0-base starting y location
      @args x1[in]    The 0-based ending x location
      @args y1[in]    The 0-base ending y location
      @args color[in] The RGB565 color to use when drawing the pixel
*/
/**************************************************************************/
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color){
	writeCommand(0x91);
	writeData(x0);
	writeCommand(0x92);
	writeData(x0 >> 8);

	/* Set Y */
	writeCommand(0x93);
	writeData(y0);
	writeCommand(0x94);
	writeData(y0 >> 8);

	/* Set X1 */
	writeCommand(0x95);
	writeData(x1);
	writeCommand(0x96);
	writeData((x1) >> 8);

	/* Set Y1 */
	writeCommand(0x97);
	writeData(y1);
	writeCommand(0x98);
	writeData((y1) >> 8);

	/* Set Color */
	writeCommand(0x63);
	writeData((color & 0xf800) >> 11);
	writeCommand(0x64);
	writeData((color & 0x07e0) >> 5);
	writeCommand(0x65);
	writeData((color & 0x001f));

	/* Draw! */
	writeCommand(RA8875_DCR);
	writeData(0x80);
}

void drawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color){
	rectHelper(x, y, x+w, y+h, color, 0);
}

void fillRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color){
	rectHelper(x, y, x+w, y+h, color, 1);
}

void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color){
	circleHelper(x0, y0, r, color, 0);
}

void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color){
	circleHelper(x0, y0, r, color, 1);
}

void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color){
	triangleHelper(x0, y0, x1, y1, x2, y2, color, 0);
}

void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color){
	triangleHelper(x0, y0, x1, y1, x2, y2, color, 1);
}

void drawEllipse(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint16_t color){
	ellipseHelper(xCenter, yCenter, longAxis, shortAxis, color, 0);
}

void fillEllipse(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint16_t color){
	ellipseHelper(xCenter, yCenter, longAxis, shortAxis, color, 1);
}

void drawCurve(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint8_t curvePart, uint16_t color){
	curveHelper(xCenter, yCenter, longAxis, shortAxis, curvePart, color, 0);
}

void fillCurve(int16_t xCenter, int16_t yCenter, int16_t longAxis, int16_t shortAxis, uint8_t curvePart, uint16_t color){
	curveHelper(xCenter, yCenter, longAxis, shortAxis, curvePart, color, 1);
}

void fillScreen(uint16_t color)
{
  rectHelper(0, 0, width-1, height-1, color, 1);
}

/* TEXT FUNCTIONS */



//Backlight Functions
void GPIOX(int on){
	if (on)
		writeReg(RA8875_GPIOX, 1);
	else
		writeReg(RA8875_GPIOX, 0);
}
void PWM1config(int on, uint8_t clock){
	if (on) {
		writeReg(RA8875_P1CR, RA8875_P1CR_ENABLE | (clock & 0xF));
	}
	else {
		writeReg(RA8875_P1CR, RA8875_P1CR_DISABLE | (clock & 0xF));
	}
}
void PWM2config(int on, uint8_t clock){
	if (on) {
		writeReg(RA8875_P2CR, RA8875_P2CR_ENABLE | (clock & 0xF));
	}
	else {
		writeReg(RA8875_P2CR, RA8875_P2CR_DISABLE | (clock & 0xF));
	}
}
void PWM1out(uint8_t p){
	writeReg(RA8875_P1DCR, p);
}
void PWM2out(uint8_t p){
	writeReg(RA8875_P2DCR, p);
}



/*End Functions*/

int main(void)
{
    u32 cmd, xStatus;

    // disable DCache
    Xil_DCacheDisable();

    /*
     * initialize SPIs with clk_polarity and clk_phase as 0
     * Configure D10-D13 as Shared SPI (MISO is not used)
     */
    init_io_switch();
//    spi_open(unsigned int spiclk, unsigned int miso, unsigned int mosi, unsigned int ss)
//    spi_device = spi_open(13, 12, 11, 10);
    spi_device = spi_open_device(XPAR_SPI_0_DEVICE_ID);
    spi_device = spi_configure(spi_device, 1, 1);

    uart_device = uart_open(TX, RX);
//	set_pin(TX, UART0_TX);
//	set_pin(RX, UART0_RX);
//	uart_device = uart_open_device(0);

    // SysMon Initialize
    SysMonConfigPtr = XSysMon_LookupConfig(SYSMON_DEVICE_ID);
    if(SysMonConfigPtr == NULL)
        return -1;
    xStatus = XSysMon_CfgInitialize(SysMonInstPtr,SysMonConfigPtr,
                                    SysMonConfigPtr->BaseAddress);
    if(XST_SUCCESS != xStatus)
        return -1;

    // Clear the old status
    XSysMon_GetStatus(SysMonInstPtr);

    /*
     * Configure A0-A5 as GPIO, D0-D9 as GPIO
     * set the direction for all signals of channel 1 to be output
     */
    gpio_device = gpio_open_device(0);
    gpio_device = gpio_configure(gpio_device, 2, 9, 1);
    gpio_set_direction(gpio_device, 0);
    /*
     * write 0 to all data pins
     * reset = 1 -> not in reset state
     * dc = 0    -> in command mode
     */
//    reset_dc_d7_to_d0 = 0x000; //reset low (active low)
//    gpio_write(gpio_device, reset_dc_d7_to_d0);
//    reset_dc_d7_to_d0 = 0x200; //reset high
//    gpio_write(gpio_device, reset_dc_d7_to_d0);

    Xil_Out32(XPAR_IOP_ARDUINO_INTR_BASEADDR+4,0x0);
    Xil_Out32(XPAR_IOP_ARDUINO_INTR_BASEADDR,0x0);

    //Initialize Display

    int16_t x0 = 0;
    int16_t y0 = 0;
    int16_t x1 = 0;
	int16_t y1 = 0;
	int16_t x2 = 0;
	int16_t y2 = 0;

    int16_t x_pixel = 0;
    int16_t y_pixel = 0;
    int16_t x_start = 0;
    int16_t y_start = 0;
    uint16_t color_pixel = 0;
    uint16_t color = 0;
    uint16_t pixel = 0;
    uint8_t *image;
	int16_t x_size;
	int16_t y_size;
	uint8_t curve;
	uint8_t layer0 = 0;
	uint8_t layer1 = 0;
	int fill = 0;
    int index = 0;
    char* textBuffer = "Hey guys hwhats up?";
    char* text;
    uint16_t text_length = 0;

    uint16_t text_val = 0;

    uint8_t* picAddr;
    uint8_t* drawAddr;
    int read_count = 0;
    int pixel_count = 0;
    int stream_size = 153600;
    int num_read = 0;
    char stream_in[10] = {};
    uint8_t pixel8bit;
    int scale;
    uint16_t foreground_color;
    uint16_t background_color;

    char write_snap_cmd[1] = {1};

    while(1) {
        while(MAILBOX_CMD_ADDR==0);
        cmd = MAILBOX_CMD_ADDR;

        switch(cmd) {
            case INIT:
            	initialize();
				MAILBOX_DATA(0) = 1;
                // Assign default pin configurations - no operations needed
            	displayOn(1);
            	MAILBOX_DATA(0) = 2;
                MAILBOX_CMD_ADDR = 0x0;
            	break;
            case GPIO:
            	GPIOX(1);
            	MAILBOX_DATA(0) = 3;
            	MAILBOX_CMD_ADDR = 0x0;
            	break;
            case PWM:
            	PWM1config(1, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
				PWM1out(255);
				MAILBOX_DATA(0) = spi_device;
				MAILBOX_CMD_ADDR = 0x0;
				break;
            case DRAW:
            	//Communicates SPI commands
            	x_pixel = MAILBOX_DATA(0);
            	y_pixel = MAILBOX_DATA(1);
            	color_pixel = (uint16_t)MAILBOX_DATA(2);
            	drawPixel(x_pixel, y_pixel, color_pixel);
            	//drawLine(1,1,750, 450, RA8875_CYAN);
            	MAILBOX_DATA(0) = color_pixel;
            	MAILBOX_CMD_ADDR = 0x0;
            	break;
            case FILL:
            	fillScreen(0x07E0);
            	MAILBOX_DATA(0) = 1337;
            	MAILBOX_CMD_ADDR = 0x0;
            	break;
            case FILL_COLOR:
            	color_pixel = (uint16_t) MAILBOX_DATA(0);
            	fillScreen(color_pixel);
            	MAILBOX_DATA(0) = color_pixel;
            	MAILBOX_CMD_ADDR = 0x0;
            	break;
            case SLOW_FILL:
            	color_pixel = (uint16_t) MAILBOX_DATA(0);
            	for(x_pixel = 0; x_pixel < 800; x_pixel++){
            		for(y_pixel = 0; y_pixel < 480; y_pixel++){
            			drawPixel(x_pixel,y_pixel, color_pixel);
            		}
            	}
            	MAILBOX_CMD_ADDR = 0x0;
            	break;
            case DRAW_IMAGE:
            	x_start = MAILBOX_DATA(0);
            	y_start = MAILBOX_DATA(1);
            	image = MAILBOX_DATA(2) | 0x20000000;
            	x_size = MAILBOX_DATA(3);
            	y_size = MAILBOX_DATA(4);
            	index = 0;
            	for(x_pixel = x_start; x_pixel < x_size; x_pixel++){
            		for(y_pixel = y_start; y_pixel < y_size; y_pixel++){
            			pixel = ((uint16_t)image[index] << 8) | image[index+1];
            			drawPixel(x_pixel, y_pixel, pixel);
            			index += 2;
            		}
            	}
            	index = 0;
            	MAILBOX_DATA(0) = (image[0] << 8) | image[1];
            	MAILBOX_CMD_ADDR = 0x0;
            	break;

            case EIGHT:
            	x_start = MAILBOX_DATA(0);
            	y_start = MAILBOX_DATA(1);
            	image = MAILBOX_DATA(2) | 0x20000000;
            	x_size = MAILBOX_DATA(3);
            	y_size = MAILBOX_DATA(4);
            	index = 0;
            	for(x_pixel = x_start; x_pixel < x_size; x_pixel++){
            		for(y_pixel = y_start; y_pixel < y_size; y_pixel++){
            			pixel = image[index];
            			drawPixel8bit(x_pixel, y_pixel, pixel);
            			index += 1;
            		}
            	}
            	index = 0;
            	MAILBOX_DATA(0) = image[0];
            	MAILBOX_CMD_ADDR = 0x0;
            	break;

            case RECTANGLE:
            	x_start = MAILBOX_DATA(0);
            	y_start = MAILBOX_DATA(1);
            	x_size = MAILBOX_DATA(2);
				y_size = MAILBOX_DATA(3);
				color = MAILBOX_DATA(4);
            	fill = MAILBOX_DATA(5);
				if(fill == 1){ //fill shape
					fillRectangle(x_start, y_start, x_size, y_size, color);
				}
				else{
					drawRectangle(x_start, y_start, x_size, y_size, color);
				}
            	MAILBOX_CMD_ADDR = 0x0;
				break;

            case CIRCLE:
            	x_start = MAILBOX_DATA(0);
				y_start = MAILBOX_DATA(1);
				x_size = MAILBOX_DATA(2); //actually radius
				color = MAILBOX_DATA(3);
				fill = MAILBOX_DATA(4);
				if(fill){
					fillCircle(x_start, y_start, x_size, color);
				}
				else{
					drawCircle(x_start, y_start, x_size, color);
				}
				MAILBOX_CMD_ADDR = 0x0;
				break;

            case TRIANGLE:
            	x0 = MAILBOX_DATA(0);
            	y0 = MAILBOX_DATA(1);
            	x1 = MAILBOX_DATA(2);
            	y1 = MAILBOX_DATA(3);
            	x2 = MAILBOX_DATA(4);
            	y2 = MAILBOX_DATA(5);
            	color = MAILBOX_DATA(6);
            	fill = MAILBOX_DATA(7);
            	if(fill){
            		fillTriangle(x0, y0, x1, y1, x2, y2, color);
            	}
            	else{
            		drawTriangle(x0, y0, x1, y1, x2, y2, color);
            	}
				MAILBOX_CMD_ADDR = 0x0;
				break;

            case ELLIPSE:
            	x0 = MAILBOX_DATA(0);
            	y0 = MAILBOX_DATA(1);
            	x_size = MAILBOX_DATA(2);
            	y_size = MAILBOX_DATA(3);
            	color = MAILBOX_DATA(4);
            	fill = MAILBOX_DATA(5);
            	if(fill){
            		fillEllipse(x0, y0, x_size, y_size, color);
            	}
            	else{
            		drawEllipse(x0, y0, x_size, y_size, color);
            	}
				MAILBOX_CMD_ADDR = 0x0;
				break;

            case CURVE:
            	x0 = MAILBOX_DATA(0);
				y0 = MAILBOX_DATA(1);
				x_size = MAILBOX_DATA(2);
				y_size = MAILBOX_DATA(3);
				curve = MAILBOX_DATA(4);
				color = MAILBOX_DATA(5);
				fill = MAILBOX_DATA(6);
				if(fill){
					fillCurve(x0, y0, x_size, y_size, curve, color);
				}
				else{
					drawCurve(x0, y0, x_size, y_size, curve, color);
				}
				MAILBOX_CMD_ADDR = 0x0;
				break;

            case DRAWLINE:
            	x0 = MAILBOX_DATA(0);
				y0 = MAILBOX_DATA(1);
				x1 = MAILBOX_DATA(2);
				y1 = MAILBOX_DATA(3);
				color = MAILBOX_DATA(4);
				drawLine(x0, y0, x1, y1, color);
            	MAILBOX_CMD_ADDR = 0x0;
				break;

            case FONT:
            	textSetCursor(240,0);
            	textColor(0x0000, 0xffff);
            	textWrite(textBuffer, 19);
            	textEnlarge(3);
            	textTransparent(0xffff);
            	textSetCursor(0,0);
            	textWrite(textBuffer, 19);
            	MAILBOX_CMD_ADDR = 0x0;
				break;

            case LETTER_SETUP:
            	textSetCursor(240,240);
				textTransparent(0xffff);
				MAILBOX_CMD_ADDR = 0x0;
				break;

            case LETTER:
            	textWrite("a", 1);
            	MAILBOX_CMD_ADDR = 0x0;
            	break;

            case TEXTMODE:
            	textMode();
            	MAILBOX_CMD_ADDR = 0x0;
				break;

            case GRAPHICSMODE:
            	graphicsMode();
            	MAILBOX_CMD_ADDR = 0x0;
				break;

            case LAYER_MODE:
            	layer0 = MAILBOX_DATA(0);
            	useLayers(layer0);
            	MAILBOX_CMD_ADDR = 0x0;
            	break;

            case LAYER:
            	layer0 = MAILBOX_DATA(0);
            	writeTo(layer0);
            	MAILBOX_CMD_ADDR = 0x0;
				break;

            case LAYERTRANS:
            	layer0 = MAILBOX_DATA(0);
            	layer1 = MAILBOX_DATA(1);
            	layerTransparency(layer0, layer1);
            	MAILBOX_CMD_ADDR = 0x0;
            	break;

            case LAYER_EFFECT:
            	layer0 = MAILBOX_DATA(0);
            	layerEffect(layer0);
            	MAILBOX_CMD_ADDR = 0x0;
            	break;

            case CLEAR_WINDOW:
				layer0 = MAILBOX_DATA(0);
				clearActiveWindow(layer0);
            	MAILBOX_CMD_ADDR = 0x0;
				break;

            case CLEAR_MEM:
            	layer0 = MAILBOX_DATA(0);
            	clearMemory(layer0);
            	MAILBOX_CMD_ADDR = 0x0;
				break;

//            case CAMERA1:
//            	//size of image - 153600 bytes
//            	//capture image and save to DDR
//            	picAddr = MAILBOX_DATA(0) | 0x20000000;
//            	uart_write(uart_device, write_snap_cmd, 1);
//            	read_count = 0;
//				while(read_count < stream_size){
//					num_read = uart_read(uart_device, stream_in, 1);
//					if(num_read == 1){
//						picAddr[read_count]= stream_in[0];
//						++read_count;
//					}
//				}
//				MAILBOX_DATA(0) = read_count;
//            	MAILBOX_CMD_ADDR = 0x0;
//				break;

            case CAMERA1:
            	//size of image - 153600 bytes
            	//capture image and save to DDR
            	picAddr = MAILBOX_DATA(0) | 0x20000000;
            	drawAddr = MAILBOX_DATA(1) | 0x20000000;
            	uart_write(uart_device, write_snap_cmd, 1);
            	read_count = 0;
            	pixel_count = 0;
            	index = 0;
				while(read_count < stream_size){
					num_read = uart_read(uart_device, stream_in, 1);
					if(num_read == 1){
						picAddr[read_count]= stream_in[0];
						++read_count;
						if(read_count % 3 == 0){ //
							drawPixel8bit((pixel_count%320), (int)(pixel_count/320), drawAddr[pixel_count]);
							++pixel_count;
						}
						else if(read_count % 3 == 1){
							pixel8bit = (picAddr[index] & 0xE0) |  ((picAddr[index] & 0x07) << 2);
						}
						else{
							drawAddr[pixel_count] = (pixel8bit | ((picAddr[index+1]) & 0x18) >> 3);
							index += 2;
						}
					}
				}
				while((int)(pixel_count/320) < 240){
					pixel8bit = ((picAddr[pixel_count*2] & 0xE0) |  ((picAddr[pixel_count*2] & 0x07) << 2) | (((picAddr[(pixel_count*2)+1]) & 0x18) >> 3));
					drawPixel8bit((pixel_count%320), (int)(pixel_count/320), pixel8bit);
					++pixel_count;
				}
				MAILBOX_DATA(0) = read_count;
				MAILBOX_DATA(1) = pixel_count;
            	MAILBOX_CMD_ADDR = 0x0;
				break;


            case CAMERA2:
            	//send to display
				x_start = MAILBOX_DATA(0);
				y_start = MAILBOX_DATA(1);
				picAddr = MAILBOX_DATA(2) | 0x20000000;
				x_size = 320;
				y_size = 240;
				index = 0;
				for(y_pixel = y_start; y_pixel < y_size+y_start; y_pixel++){
					for(x_pixel = x_start; x_pixel < x_size+x_start; x_pixel++){
						//convert rgb565 to rgb332
						pixel8bit = ((picAddr[index] & 0xE0) |  ((picAddr[index] & 0x07) << 2) | ((picAddr[index+1] & 0x18) >> 3));
						//convert rgb888 to rgb332
//						pixel8bit = ((picAddr[index] & 0xE0) | ((picAddr[index+1] & 0xE0) >> 3) | (picAddr[index+2] >>6));
						drawPixel8bit(x_pixel, y_pixel, pixel8bit);
						index += 2;
					}
				}
				MAILBOX_DATA(0) = index;
				MAILBOX_CMD_ADDR = 0x0;
				break;

            case SIMPLE:
            	uart_write(uart_device, write_snap_cmd, 1);
				num_read = uart_read(uart_device, stream_in, 4);
				MAILBOX_DATA(0) =  stream_in;
				MAILBOX_DATA(1) = stream_in[0];
				MAILBOX_DATA(2) = stream_in[1];
				MAILBOX_DATA(3) = stream_in[2];
				MAILBOX_DATA(4) = stream_in[3];
				MAILBOX_CMD_ADDR = 0x0;
				break;

            case FONT_SIZE:
				scale = MAILBOX_DATA(0);
				textEnlarge(scale);
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case TEXT_TRANSPARENT:
				foreground_color = MAILBOX_DATA(0);
				textTransparent(foreground_color);
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case BACKGROUND_COLOR:
				foreground_color = MAILBOX_DATA(0);
				background_color = MAILBOX_DATA(1);
				textColor(foreground_color, background_color);
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case TEXT_CURSOR:
				x_pixel = MAILBOX_DATA(0);
				y_pixel = MAILBOX_DATA(1);
				textSetCursor(x_pixel, y_pixel);
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case TEXT_WRITE:
				text_val = MAILBOX_DATA(0);
				if(text_val == 0) {
					text = "ALERT!";
					text_length = 6;
				}
				else if(text_val == 1) {
					text = "CANCEL";
					text_length = 6;
				}
				else if(text_val == 2) {
					text = "MARK";
					text_length = 4;
				}
				else if(text_val == 3) {
					text = "VIEW";
					text_length = 4;
				}
				else if(text_val == 4) {
					text = "SELECT";
					text_length = 6;
				}
				else if(text_val == 5) {
					text = "E";
					text_length = 1;
				}
				else if(text_val == 6) {
					text = "W";
					text_length = 1;
				}
				else if(text_val == 7) {
					text = "N";
					text_length = 1;
				}
				else if(text_val == 8) {
					text = "S";
					text_length = 1;
				}
				else if(text_val == 9) {
					text = "HOME";
					text_length = 4;
				}
				else if(text_val == 10) {
					text = "GO BACK";
					text_length = 7;
				}
				else if(text_val == 11) {
					text = "Tag: ";
					text_length = 5;
				}
				else if(text_val == 12) {
					text = "Created By: ";
					text_length = 12;
				}
				else if(text_val == 13) {
					text = "Distance: ";
					text_length = 10;
				}
				else if(text_val == 14) {
					text = "OBJECT";
					text_length = 6;
				}
				else if(text_val == 15) {
					text = "CONFIRM";
					text_length = 7;
				}
				else if(text_val == 16) {
					text = "Direction: ";
					text_length = 11;
				}
				else if(text_val == 17) {
					text = "POINT TOWARDS";
					text_length = 13;
				}
				else if(text_val == 18) {
					text = "POINT OF INTEREST NEARBY";
					text_length = 24;
				}
				else if(text_val == 19) {
					text = "DANGER NEARBY";
					text_length = 13;
				}
				else if(text_val == 20) {
					text = "DISMISS";
					text_length = 7;
				}
				else if(text_val == 21){
					text = "10M";
					text_length = 3;
				}
				else if(text_val == 22){
					text = "20M";
					text_length = 3;
				}
				else if(text_val == 23){
					text = "30M";
					text_length = 3;
				}
				else {
					text = "DEFAULT";
					text_length = 7;
				}
				textWrite(text, text_length);
				MAILBOX_CMD_ADDR = 0x0;
				break;

            default:
                MAILBOX_CMD_ADDR = 0x0;
                break;
        }

    }
    return 0;
}


