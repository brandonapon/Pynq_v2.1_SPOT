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
 * @file arduino_cam.c
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
#include "timer.h"
#include "xio_switch.h"
#include "xil_cache.h"
#include "xsysmon.h"


/* Pin Assignments */
//cs - 0
//reset - 7
#define cs						0x0
#define reset					0x7F
#define	start_transfer			0xFE
#define end_transfer			0xFF

#define V_REF 3.33
#define SYSMON_DEVICE_ID XPAR_SYSMON_0_DEVICE_ID

/*PYNQ Commands */
#define TEST 					0x01
#define TEST_2					0x02
#define COMBO					0x03

/* SPI Commands */
#define SNAPSHOT				0x01
#define SEND					0x02
#define SR						0x03
#define CONFIRM					0x04

spi spi_device;
u16 reset_dc_d7_to_d0;
static XSysMon SysMonInst;
XSysMon_Config *SysMonConfigPtr;
XSysMon *SysMonInstPtr = &SysMonInst;


uint8_t writeBuffer[8], readBuffer[8];

/*Functions*/
void transfer(uint8_t cmd, uint8_t data){
//	writeBuffer[0] = RA8875_CMDWRITE;
//	spi_transfer(spi_device, (const char*) writeBuffer, (char*) readBuffer, 1);
//	writeBuffer[0] = cmd;
//	spi_transfer(spi_device, (const char*) writeBuffer, (char*) readBuffer, 1);
	writeBuffer[0] = cmd;
	writeBuffer[1] = data;
	spi_transfer(spi_device, (const char*) writeBuffer, (char*) readBuffer, 2);
}

/*
 * Cmd = command
 * data = array of data values
 * length = cmd+data # of bytes
 */
void transfer_mult(uint8_t cmd, uint8_t* data_in, uint8_t* data_out, uint8_t length){
//	writeBuffer[0] = RA8875_CMDWRITE;
//	spi_transfer(spi_device, (const char*) writeBuffer, (char*) readBuffer, 1);
//	writeBuffer[0] = cmd;
//	spi_transfer(spi_device, (const char*) writeBuffer, (char*) readBuffer, 1);
	writeBuffer[0] = cmd;
	for(int i = 0; i < length-1; i++){
		writeBuffer[i] = data_in[i];
	}
	spi_transfer(spi_device, (const char*) writeBuffer, (char*) readBuffer, length);
	for(int i = 0; i < length; i++){
		data_out[i] = readBuffer[i];
	}
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
    //spi_device = spi_open(13, 12, 11, 10);
    spi_device = spi_open_device(XPAR_SPI_0_DEVICE_ID); //not sure if device is correct
    spi_device = spi_configure(spi_device, 0, 0);

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

    Xil_Out32(XPAR_IOP_ARDUINO_INTR_BASEADDR+4,0x0);
    Xil_Out32(XPAR_IOP_ARDUINO_INTR_BASEADDR,0x0);

    uint8_t data = 69;
    uint8_t data_stream[8] = {0,1,2,3,4,5,6,7};
    uint8_t recv_stream[15] = {-1};
    while(1) {
        while(MAILBOX_CMD_ADDR==0);
        cmd = MAILBOX_CMD_ADDR;
        switch(cmd) {
            case TEST:
            	transfer(SEND, data);
            	MAILBOX_DATA(0) = data;
            	MAILBOX_CMD_ADDR = 0x0;
                break;

            case TEST_2:
            	transfer_mult(SR, data_stream, recv_stream, 8);
            	for(int i = 0; i < 8; i++){
            		MAILBOX_DATA(i) = recv_stream[i];
            	}
            	MAILBOX_CMD_ADDR = 0x0;
            	break;

            case COMBO:
            	spi_transfer_mod(spi_device, data_stream, recv_stream, 8, 15);
            	for(int i = 0; i < 8; i++){
            		MAILBOX_DATA(i) = recv_stream[i];
            	}
            	MAILBOX_CMD_ADDR = 0x0;
            	break;

            default: //add error message here
                MAILBOX_CMD_ADDR = 0x0;
                break;
        }

    }
    return 0;
}


