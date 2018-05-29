/*
	C Driver for GPS
*/

#include "circular_buffer.h"
#include "timer.h"
#include "i2c.h"

#define NEO_M8P_DEVICE_ADDRESS	0x42

#define BYTES_AVAILABLE_HIGH 	0xFD
#define BYTES_AVAILABLE_LOW		0xFE
#define DATA_REG				0xFF
#define CMD_READ_DATA			0x01
#define CMD_BYTES_AVAILABLE		0x02
#define TEST_CMD_ONE_BYTE		0x03
#define STREAM_TEST				0x04

static i2c device;

char read_gps(char reg) {
    char data;

    data = reg; // Set the address pointer register
    i2c_write(device, NEO_M8P_DEVICE_ADDRESS, &data, 1);
    i2c_read(device, NEO_M8P_DEVICE_ADDRESS, &data, 1);
    return data;
}

void appendToCharArray(char* s, char c) {
	int len = strlen(s);
    s[len] = c;
    s[len+1] = '\0';
}

void clearArray(char* s) {
	memset(&s[0], 0, sizeof(s));
}

char getBitPosition(char* s, int pos) {
	return s[pos];
}

int main(void) {

	// Select the GPS module
	device = i2c_open_device(0);

	int16_t counter = 0;
	char gpsOutput[1024];
	u32 cmd;
	char numberofBytes = 0;
	char bytesLow = 0;
	char bytesHigh = 0;

	// Run the read loop
	while(1) {

		while((MAILBOX_CMD_ADDR)==0);

        cmd = MAILBOX_CMD_ADDR;

        switch(cmd) {

        	case TEST_CMD_ONE_BYTE:

        		bytesLow = read_gps(BYTES_AVAILABLE_LOW);
				bytesHigh = read_gps(BYTES_AVAILABLE_HIGH);

				MAILBOX_DATA(0) = bytesHigh;
				MAILBOX_DATA(1) = bytesLow;
				// Write data to mailbox
				while(1) {
					char readBit = read_gps(DATA_REG);
					int intBit = readBit - '0';
					appendToCharArray(gpsOutput, readBit);
					if(intBit == '255') {
						break;
					}
					else if(readBit == '\n') {
						if(getBitPosition(gpsOutput, 5) == 'L') {
							for(int i = 0; i < strlen(gpsOutput); i++) {
								MAILBOX_DATA(i) = getBitPosition(gpsOutput, i);
							}
							break;
						}
						else {
							clearArray(gpsOutput);
						}
					}
				}

				MAILBOX_CMD_ADDR = 0x0;
                break;

        	case CMD_BYTES_AVAILABLE:

        		bytesLow = read_gps(BYTES_AVAILABLE_LOW);
				bytesHigh = read_gps(BYTES_AVAILABLE_HIGH);
				MAILBOX_DATA(0) = bytesHigh;
				MAILBOX_DATA(4) = bytesLow;

				MAILBOX_CMD_ADDR = 0x0;
                break;

            case STREAM_TEST:

            	while(1) {
            		u8 gpsData = read_gps(DATA_REG);
            		if(gpsData == 255) { // 0xFF
            			break;
            		}
            		MAILBOX_DATA(counter) = gpsData;
            		counter = counter + 1;
            	}
            	counter = 0;

            	MAILBOX_CMD_ADDR = 0x0;
                break;

            default:
            	MAILBOX_CMD_ADDR = 0x0; // RESET
                break;

        }
	}

	return 0;

}
