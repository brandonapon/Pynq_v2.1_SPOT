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
 * @file arduino_cam_uart.c
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
// Mailbox commands
#define CONFIG_IOP_SWITCH       0x1
#define WRITE					0x2
#define READ					0x3
#define TRANSACTION				0x4

//Define Pins
#define RX						0
#define TX						1

uart uart_device;

// UART functions
//uart uart_open_device(unsigned int device);
//uart uart_open(unsigned int tx, unsigned int rx);
//void uart_read(uart dev_id, char* read_data, unsigned int length);
//void uart_write(uart dev_id, char* write_data, unsigned int length);
//void uart_close(uart dev_id);
//unsigned int uart_get_num_devices(void);

int main()
{
   int cmd;

   //UART Init
   init_io_switch();
   uart_device = uart_open(TX, RX);
   int status = 0;
//   uart_device = uart_open_device(0);
   if(uart_device == -1){
	   status = -1;
   }

   char write_snap_cmd[1] = {1};
   char write_data[4] = {0,1,2,3};
   char read_data[4] = {-1};
   int file_size;
   uint8_t ddr_address;

   // Run application
   while(1){
     // wait and store valid command
      while((MAILBOX_CMD_ADDR)==0);
      cmd = MAILBOX_CMD_ADDR;

		switch(cmd){
			case CONFIG_IOP_SWITCH:
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case WRITE:
				uart_write(uart_device, write_data, 4);
				MAILBOX_DATA(0) = 69;
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case READ:
				uart_read(uart_device, read_data, 4);
				for(int i = 0; i < 4; i++){
					MAILBOX_DATA(i) = read_data[i];
				}
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case TRANSACTION:
				ddr_address = MAILBOX_DATA(0) | 0x20000000;
				file_size = MAILBOX_DATA(1);
				uart_write(uart_device, write_snap_cmd, 1);
				uart_read(uart_device, &ddr_address, file_size);
				MAILBOX_DATA(0) = file_size;
				MAILBOX_CMD_ADDR = 0x0;
				break;

			default:
				MAILBOX_CMD_ADDR = 0x0;
				break;
		}
   }
   return 0;
}
