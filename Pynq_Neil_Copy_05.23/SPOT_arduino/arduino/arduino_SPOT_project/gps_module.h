#ifndef __HAPTIC_MODULE_H__
#define __HAPTIC_MODULE_H__

char read_gps(char reg);
void appendToCharArray(char* s, char c);
void clearArray(char* s);
char getBitPosition(char* s, int pos);

#endif
