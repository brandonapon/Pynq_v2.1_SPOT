#ifndef __GPS_MODULE_H__
#define __GPS_MODULE_H__


char read_gps(char reg, i2c device);
void appendToCharArray(char* s, char c);
void clearArray(char* s);
char getBitPosition(char* s, int pos);

#endif
