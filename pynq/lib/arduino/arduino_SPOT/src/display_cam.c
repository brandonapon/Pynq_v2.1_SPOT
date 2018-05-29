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
#include "display_cam.h"

/* shared variables */
int textScale = 1;
int currentLayer = 0;

spi spi_device;
uart camera;

void setSPIDisplay(spi device){
	spi_device = device;
}

/* LOW LEVEL FUNCTIONS*/
void writeCommand(uint8_t cmd){
	writeBuffer[0] = RA8875_CMDWRITE;
	writeBuffer[1] = cmd;
	spi_transfer_slow(spi_device, (const uint8_t*) writeBuffer, (uint8_t*) readBuffer, 2);
}

void writeData(uint8_t d){
	writeBuffer[0] = RA8875_DATAWRITE;
	writeBuffer[1] = d;
	spi_transfer_slow(spi_device, (const uint8_t*) writeBuffer, (uint8_t*) readBuffer, 2);
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

//void PLLinit(void){ //Fixed to 800x480
//	//Initializes PLL on Chip
//	slowWriteReg(RA8875_PLLC1, RA8875_PLLC1_PLLDIV1 + 10);
//	slowWriteReg(RA8875_PLLC2, RA8875_PLLC2_DIV4);
//}
//
//void initialize(void){ //Fixed to 800x480
//	//Initializes the driver IC (clock setup, etc.)
//	PLLinit();
//	//Change RA8875_SYSR_16BPP to RA8875_SYSR_8BPP to change from 64k to 256 colors. Allows for layers to be used
//	//RA8875_SYSR_MCU8 - 8 bit mode, RA8875_SYSR_MCU16 - 16 bit mode
////	slowWriteReg(RA8875_SYSR, RA8875_SYSR_16BPP | RA8875_SYSR_MCU8);
//	slowWriteReg(RA8875_SYSR, RA8875_SYSR_MCU8 | RA8875_SYSR_MCU8);
//
//	/* Timing values */
//	uint8_t pixclk;
//	uint8_t hsync_start;
//	uint8_t hsync_pw;
//	uint8_t hsync_finetune;
//	uint8_t hsync_nondisp;
//	uint8_t vsync_pw;
//	uint16_t vsync_nondisp;
//	uint16_t vsync_start;
//
//	pixclk          = RA8875_PCSR_PDATL | RA8875_PCSR_2CLK;
//	hsync_nondisp   = 26;
//	hsync_start     = 32;
//	hsync_pw        = 96;
//	hsync_finetune  = 0;
//	vsync_nondisp   = 32;
//	vsync_start     = 23;
//	vsync_pw        = 2;
//
//	slowWriteReg(RA8875_PCSR, pixclk);
//
//	/* Horizontal settings registers */
//	slowWriteReg(RA8875_HDWR, (width / 8) - 1);                          // H width: (HDWR + 1) * 8 = 480
//	slowWriteReg(RA8875_HNDFTR, RA8875_HNDFTR_DE_HIGH + hsync_finetune);
//	slowWriteReg(RA8875_HNDR, (hsync_nondisp - hsync_finetune - 2)/8);    // H non-display: HNDR * 8 + HNDFTR + 2 = 10
//	slowWriteReg(RA8875_HSTR, hsync_start/8 - 1);                         // Hsync start: (HSTR + 1)*8
//	slowWriteReg(RA8875_HPWR, RA8875_HPWR_LOW + (hsync_pw/8 - 1));        // HSync pulse width = (HPWR+1) * 8
//
//	/* Vertical settings registers */
//	slowWriteReg(RA8875_VDHR0, (uint16_t)(height - 1) & 0xFF);
//	slowWriteReg(RA8875_VDHR1, (uint16_t)(height - 1) >> 8);
//	slowWriteReg(RA8875_VNDR0, vsync_nondisp-1);                          // V non-display period = VNDR + 1
//	slowWriteReg(RA8875_VNDR1, vsync_nondisp >> 8);
//	slowWriteReg(RA8875_VSTR0, vsync_start-1);                            // Vsync start position = VSTR + 1
//	slowWriteReg(RA8875_VSTR1, vsync_start >> 8);
//	slowWriteReg(RA8875_VPWR, RA8875_VPWR_LOW + vsync_pw - 1);            // Vsync pulse width = VPWR + 1
//
//	/* Set active window X */
//	slowWriteReg(RA8875_HSAW0, 0);                                        // horizontal start point
//	slowWriteReg(RA8875_HSAW1, 0);
//	slowWriteReg(RA8875_HEAW0, (uint16_t)(width - 1) & 0xFF);            // horizontal end point
//	slowWriteReg(RA8875_HEAW1, (uint16_t)(width - 1) >> 8);
//
//	/* Set active window Y */
//	slowWriteReg(RA8875_VSAW0, 0);                                        // vertical start point
//	slowWriteReg(RA8875_VSAW1, 0);
//	slowWriteReg(RA8875_VEAW0, (uint16_t)(height - 1) & 0xFF);           // horizontal end point
//	slowWriteReg(RA8875_VEAW1, (uint16_t)(height - 1) >> 8);
//
//	 /* Clear the entire window */
//	slowWriteReg(RA8875_MCLR, RA8875_MCLR_START | RA8875_MCLR_FULL);
//}

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
	spi_transfer(spi_device, (const uint8_t*) w_buffer, (uint8_t*) r_buffer, 3);
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