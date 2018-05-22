/*
	C Driver for GPS
*/

#include "circular_buffer.h"
#include "timer.h"
#include "i2c.h"
#include "gps_module.h"

#define NEO_M8P_DEVICE_ADDRESS	0x42




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

