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
#include "xil_cache.h"
#include "xsysmon.h"

#define WRITE	0x01

#define V_REF 3.33
#define SYSMON_DEVICE_ID XPAR_SYSMON_0_DEVICE_ID

u16 reset_dc_d7_to_d0;
static XSysMon SysMonInst;
XSysMon_Config *SysMonConfigPtr;
XSysMon *SysMonInstPtr = &SysMonInst;

int main(void)
{
    u32 cmd, xStatus;

    // disable DCache
    Xil_DCacheDisable();

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

    u32 num = 0;

    while(1) {
        while(MAILBOX_CMD_ADDR==0);
        cmd = MAILBOX_CMD_ADDR;

        switch(cmd) {
        	case WRITE:
        		num = MAILBOX_DATA(0);
        		MAILBOX_DATA(0) = num-1295;
        		MAILBOX_CMD_ADDR = 0x0;
        		break;
            default:
                MAILBOX_CMD_ADDR = 0x0;
                break;
        }
    }
    return 0;
}
