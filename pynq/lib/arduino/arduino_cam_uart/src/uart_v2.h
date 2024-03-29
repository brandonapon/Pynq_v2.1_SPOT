/*
 * uart_v2.h
 *
 *  Created on: Apr 9, 2018
 *      Author: Brandon
 */

#ifndef SRC_UART_V2_H_
#define SRC_UART_V2_H_

#include <xparameters.h>

#ifdef XPAR_XUARTLITE_NUM_INSTANCES

/*
 * UART API
 */
typedef int uart;

uart uart_open_device(unsigned int device);
uart uart_open(unsigned int tx, unsigned int rx);
unsigned int uart_read(uart dev_id, char* read_data, unsigned int length);
void uart_write(uart dev_id, char* write_data, unsigned int length);
void uart_close(uart dev_id);
unsigned int uart_get_num_devices(void);

#endif
#endif /* SRC_UART_V2_H_ */
