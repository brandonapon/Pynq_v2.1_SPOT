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
 * @file arduino_rangefinder.c
 *
 * IOP code (MicroBlaze) for grove IMU 10DOF.
 * The grove IMU has to be connected to an arduino interface 
 * via a shield socket.
 * Grove IMU is read only, and has IIC interface.
 * Hardware version 1.1.
 * http://www.seeedstudio.com/wiki/Grove_-_IMU_10DOF
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who  Date     Changes
 * ----- --- ------- -----------------------------------------------
 * 1.00a yrq 04/25/16 release
 * 1.00d yrq 07/26/16 separate pmod and arduino
 *
 * </pre>
 *
 *****************************************************************************/

#include "circular_buffer.h"
#include "timer.h"
#include "uart_v2.h"
#include "xio_switch.h"
#include "xparameters.h"
#include <stdint.h>
#include "xil_cache.h"
#include "xsysmon.h"

#define NUM_SAMPLES              100
//#define BIT9600DELAY			104
//#define HALFBIT9600DELAY		52
#define NUM_BYTES			5

#define RX						0
#define TX						1

uart uart_device;

char range[5] = {42};

// UART functions
//uart uart_open_device(unsigned int device);
//uart uart_open(unsigned int tx, unsigned int rx);
//void uart_read(uart dev_id, char* read_data, unsigned int length);
//void uart_write(uart dev_id, char* write_data, unsigned int length);
//void uart_close(uart dev_id);
//unsigned int uart_get_num_devices(void);

void readByte(char* byte){
	uart_read(uart_device, byte, 1);
}

char* readRF(void){
//	while(1){
//		readByte(&range[0]);
//		if(range[0] == 'R'){
//			int i = 0;
//			while(i < 5){
//				readByte(&range[++i]);
//			}
//			break;
//		}
//	}
	uart_read(uart_device, range, 5);
	return range;
}

//int main()
//{
//   int cmd;
//
//   //UART Init
//   init_io_switch();
//   uart_device = uart_open(TX, RX);
//   int status = 0;
////   uart_device = uart_open_device(0);
//   if(uart_device == -1){
//	   status = -1;
//   }
//
//   // Run application
//   while(1){
//     // wait and store valid command
//      while((MAILBOX_CMD_ADDR)==0);
//      cmd = MAILBOX_CMD_ADDR;
//
//		switch(cmd){
//			case CONFIG_IOP_SWITCH:
//				MAILBOX_CMD_ADDR = 0x0;
//				break;
//
//			case DEVICE:
//				MAILBOX_DATA(0) = status;
//				MAILBOX_CMD_ADDR = 0x0;
//				break;
//
//			case ON:
//				uart_write(uart_device, on, 1);
//				MAILBOX_CMD_ADDR = 0x0;
//				break;
//
//			case OFF:
//				uart_write(uart_device, off, 1);
//				MAILBOX_CMD_ADDR = 0x0;
//				break;
//
//			case POLL:
//				readRF();
//				for(int i = 0; i < 10; ++i){
//					MAILBOX_DATA(i) = (char) range[i];
//				}
//				MAILBOX_CMD_ADDR = 0x0;
//				break;
//
//			case NOTHING:
//				MAILBOX_DATA(0) = 100;
//				MAILBOX_CMD_ADDR = 0x0;
//				break;
//
//			default:
//				MAILBOX_CMD_ADDR = 0x0;
//				break;
//		}
//   }
//   return 0;
//}
