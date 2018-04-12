
#include <stdint.h>
#include "xparameters.h"
#include "circular_buffer.h"
#include "gpio.h"
#include "spi.h"
#include "timer.h"
#include "xio_switch.h"
#include "xil_cache.h"
#include "xsysmon.h"
//#include "bmp.h"

#define INIT				    0x1
#define READ 					0x2
#define ROTARY					0x3

/* Pin Assignments */
//gpio - 0

#define V_REF 3.33
#define SYSMON_DEVICE_ID XPAR_SYSMON_0_DEVICE_ID

gpio gpio_device_1;
gpio gpio_device_2;
u16 reset_dc_d7_to_d0;
static XSysMon SysMonInst;
XSysMon_Config *SysMonConfigPtr;
XSysMon *SysMonInstPtr = &SysMonInst;

unsigned int mask = 0x1; //mask off rest of bits
int button_press(gpio device){
	int data_int = gpio_read(device);
	return data_int & mask;
}

int output[10];

int* rotary_enc_old(gpio device){
	int counter = 0;
	int old = -1;
	while(1){
		output[counter] = (gpio_read(device) >> 1); //shift out button press int
		if(output[counter] != old){
			old = output[counter];
			counter++;
		}
		if(counter >= 10){
			break;
		}
	}
	return output;
}

int input;
int rotary_enc(gpio device){
	//add timeout?
	int direction = 0; // 2 = left; 1 = right
	int count = 1;
	int right[4] = {3, 1, 0, 2};
	int left[4] = {3, 2, 0, 1};
	while(1){
		input = (gpio_read(device) >> 1);
		if(direction == 1){
			if(input == right[count]){
				++count;
			}
		}
		else if(direction == 2){
			if(input == left[count]){
				++count;
			}
		}
		else{
			if(input == right[1]){
				direction = 1;
				++count;
			}
			else if(input == left[1]){
				direction = 2;
				++count;
			}
			else{
				count = 1;
			}
		}
		if(count == 4){
			return direction;
		}
	}
}

int main(void)
{
    u32 cmd, xStatus;

    // disable DCache
    Xil_DCacheDisable();

    init_io_switch();

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
//    gpio_device_1 = gpio_open_device(0);
    gpio_device_2 = gpio_open_device(0);
//    gpio_device_1 = gpio_configure(gpio_device_1, 0, 0, 1);
//    gpio_set_direction(gpio_device_1, 1);

    gpio_device_2 = gpio_configure(gpio_device_2, 0, 2, 1);
    gpio_set_direction(gpio_device_2, 1);

    Xil_Out32(XPAR_IOP_ARDUINO_INTR_BASEADDR+4,0x0);
    Xil_Out32(XPAR_IOP_ARDUINO_INTR_BASEADDR,0x0);

  	int* data;


    while(1) {
        while(MAILBOX_CMD_ADDR==0);
        cmd = MAILBOX_CMD_ADDR;

        switch(cmd) {
            case INIT:
            	// Assign default pin configurations - no operations needed
                MAILBOX_CMD_ADDR = 0x0;
                break;

            case READ:
            	MAILBOX_DATA(0) = button_press(gpio_device_2);
            	MAILBOX_CMD_ADDR = 0x0;
            	break;

            case ROTARY:
            	MAILBOX_DATA(0) = rotary_enc(gpio_device_2);
            	MAILBOX_CMD_ADDR = 0x0;
            	break;

            default:
                MAILBOX_CMD_ADDR = 0x0;
                break;
        }

    }
    return 0;
}


