/*

	List of Functions for Arduino TX

*/

#ifndef __ARDUINO_TX_H__
#define __ARDUINO_TX_H__

#include <stdint.h>
#include <stdbool.h>

void setSPI(spi deviceId);
u8 getStatus(void);
void setRegister(u8 registerVal, u8 value);
void setRegisterMulti(u8 registerVal, u8* value, u8 length);
u8 getRegister(u8 registerVal);
void flushRX(void);
void flushTX(void);
int BV(int val);
void closeReadingPipe(u8 pipe);
bool hasMessages(void);
void startListening();
void stopListening();
void openWritingPipe(u64 val);
void openReadingPipe(u8 child, u64 address);
void powerUp(void);
void initTX(u8 address);
bool writePayload(u8* messageToSend);
bool writeMessage(u8 sendAddress, u8* message, u8 len);
void readMessage_payload(u8* buf);
bool readMessage(u8* buf);

#endif