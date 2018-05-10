/*
 * uart_v2.c
 *
 *  Created on: Apr 9, 2018
 *      Author: Brandon
 */
#include <xparameters.h>
#include "uart_v2.h"

#ifdef XPAR_XUARTLITE_NUM_INSTANCES
#include "xuartlite.h"
#include "xuartlite_i.h"
static XUartLite xuart[XPAR_XUARTLITE_NUM_INSTANCES];
XUartLite* xuart_ptr = &xuart[0];
extern XUartLite_Config XUartLite_ConfigTable[XPAR_XUARTLITE_NUM_INSTANCES];

/************************** Function Definitions ***************************/
uart uart_open_device(unsigned int device){
    int status;
    u16 dev_id;
    if (device < XPAR_XUARTLITE_NUM_INSTANCES) {
        dev_id = (u16)device;
    } else {
        int found = 0;
        for (u16 i = 0; i < XPAR_XUARTLITE_NUM_INSTANCES; ++i) {
            if (XUartLite_ConfigTable[i].RegBaseAddr == device) {
                found = 1;
                dev_id = i;
                break;
            }
        }
        if (!found) return -1;
    }
    status = XUartLite_Initialize(&xuart[dev_id], dev_id);
    if (status != XST_SUCCESS) {
        return -1;
    }
    return (uart)dev_id;
}


#ifdef XPAR_IO_SWITCH_NUM_INSTANCES
#ifdef XPAR_IO_SWITCH_0_UART0_BASEADDR
#include "xio_switch.h"
static int last_tx = -1;
static int last_rx = -1;

uart uart_open(unsigned int tx, unsigned int rx){
    if (last_tx != -1) set_pin(last_tx, GPIO);
    if (last_rx != -1) set_pin(last_rx, GPIO);
    last_tx = tx;
    last_rx = rx;
    set_pin(tx, UART0_TX);
    set_pin(rx, UART0_RX);
    return uart_open_device(XPAR_IO_SWITCH_0_UART0_BASEADDR);
}
#endif
#endif


unsigned int uart_read(uart dev_id, char* read_data, unsigned int length){
    return XUartLite_Recv(&xuart[dev_id], read_data, length);
}


void uart_write(uart dev_id, char* write_data, unsigned int length){
    XUartLite_Send(&xuart[dev_id], write_data, length);
}


void uart_close(uart dev_id){
    XUartLite_ClearStats(&xuart[dev_id]);
}


unsigned int uart_get_num_devices(void){
    return XPAR_XUARTLITE_NUM_INSTANCES;
}

#endif
