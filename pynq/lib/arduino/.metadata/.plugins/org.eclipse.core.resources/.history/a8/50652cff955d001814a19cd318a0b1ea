/*
	C Driver for TX
*/

#include <stdint.h>
#include "stdbool.h"
#include "xparameters.h"
#include "circular_buffer.h"
#include "gpio.h"
#include "spi_v2.h"
#include "timer.h"
#include "xio_switch.h"
#include "xil_cache.h"
#include "xsysmon.h"
#include "nRF24L01.h"

#define READ_FROM		0x01
#define WRITE_TO		0x02
#define HAS_MESSAGES	0x03
#define GET_SETUP		0x04
#define READ_REG		0x05

#define MAX_NODES 		6
#define PACKET_CNTER 	32
#define BASE_ADDRESS 	0xDEADBEEF00
#define TIMEOUT 		5
#define DELAY_TIME		450
#define MAX_LEN			30

#define SYSMON_DEVICE_ID XPAR_SYSMON_0_DEVICE_ID

spi spi_device;
gpio gpio_device;
static XSysMon SysMonInst;
XSysMon_Config *SysMonConfigPtr;
XSysMon *SysMonInstPtr = &SysMonInst;

u8 myAddress;
u8 TXpacketCounters[MAX_NODES];
u8 RXpacketCounters[MAX_NODES];

u8 readBuffer[32];
u8 writeBuffer[32];
u8 pipe0_reading_address[5];
u8 addr_width = 5;
u8 payload_size = 32;
u8 packetToSend[32];

static const u8 child_pipe_enable[] = {
  ERX_P0, ERX_P1, ERX_P2, ERX_P3, ERX_P4, ERX_P5
};
static const u8 child_pipe[] = {
  RX_ADDR_P0, RX_ADDR_P1, RX_ADDR_P2, RX_ADDR_P3, RX_ADDR_P4, RX_ADDR_P5
};
static const u8 child_payload_size[] = {
  RX_PW_P0, RX_PW_P1, RX_PW_P2, RX_PW_P3, RX_PW_P4, RX_PW_P5
};

u8 getStatus(void) {
	writeBuffer[0] = NOP;
	spi_transfer(spi_device, (const char*)writeBuffer, (char*)readBuffer, 1);
	return readBuffer[0];
}

void setRegister(u8 registerVal, u8 value) {
	writeBuffer[0] = (W_REGISTER | (REGISTER_MASK & registerVal));
	writeBuffer[1] = value;
	spi_transfer(spi_device, (const char*)writeBuffer, (char*)NULL, 2);
}

void setRegisterMulti(u8 registerVal, u8* value, u8 length) { // WRITES MULTI BYTES
	writeBuffer[0] = W_REGISTER | (REGISTER_MASK & registerVal);
	for(int i = 0; i < length; ++i) {
		writeBuffer[i + 1] = value[i];
	}
	spi_transfer(spi_device, (const char*) writeBuffer, (char*)NULL, length);
}

u8 getRegister(u8 registerVal) {
	writeBuffer[0] = R_REGISTER | (REGISTER_MASK & registerVal);
	writeBuffer[1] = NOP;
	writeBuffer[2] = NOP;
	spi_transfer(spi_device, (const char*)writeBuffer, (char*)readBuffer, 3);
	return readBuffer[1];
}

void flushRX(void) {
	writeBuffer[0] = FLUSH_RX;
	spi_transfer(spi_device, (const char*)writeBuffer, (char*)NULL, 1);
}

void flushTX(void) {
	writeBuffer[0] = FLUSH_TX;;
	spi_transfer(spi_device, (const char*)writeBuffer, (char*)NULL, 1);
}

int BV(int val) {
	return (1 << (val));
}

void closeReadingPipe(u8 pipe) {
	setRegister(EN_RXADDR, getRegister(EN_RXADDR) & ~BV(child_pipe_enable[pipe]));
}

bool hasMessages(void) {
	if(!(getRegister(FIFO_STATUS) & BV(RX_EMPTY))) {
		return 1;
	}
	return 0;
}

void startListening() {
	setRegister(NRF_CONFIG, getRegister(NRF_CONFIG) | BV(PRIM_RX));
	setRegister(NRF_STATUS, BV(RX_DR) | BV(TX_DS) | BV(MAX_RT));

	gpio_write(gpio_device, 1);

	if(pipe0_reading_address[0] > 0) {
		setRegisterMulti(RX_ADDR_P0, pipe0_reading_address, addr_width);
	}
	else {
		closeReadingPipe(0);
	}

	if(getRegister(FEATURE) & BV(EN_ACK_PAY)) {
		flushTX();
	}
}

void stopListening() {
	gpio_write(gpio_device, 0);

	delay_us(DELAY_TIME);

	if(getRegister(FEATURE) & BV(EN_ACK_PAY)) {
		delay_us(200);
		flushTX();
	}

	setRegister(NRF_CONFIG, getRegister(NRF_CONFIG) & ~BV(PRIM_RX));
	setRegister(EN_RXADDR, getRegister(EN_RXADDR) | BV(child_pipe_enable[0]));
}

void openWritingPipe(u64 val) {
	setRegisterMulti(RX_ADDR_P0, (u8*)(val), addr_width);
	setRegisterMulti(TX_ADDR, (u8*)(val), addr_width);

	setRegister(RX_PW_P0, payload_size);
}

void openReadingPipe(u8 child, u64 address) {
	if(child == 0) {
		memcpy(pipe0_reading_address, address, addr_width);
	}

	if(child <= 6) {
		if(child < 2) {
			setRegisterMulti(child_pipe[child], (u8*)(address), addr_width);
		}
		else {
			setRegister(child_pipe[child], (u8)(address));
		}
		setRegister(child_payload_size[child], payload_size);

		setRegister(EN_RXADDR, getRegister(EN_RXADDR) | BV(child_pipe_enable[child]));
	}
}

void powerUp(void) {
	u8 config = getRegister(NRF_CONFIG);

	if(!(config & BV(PWR_UP))) {
		setRegister(NRF_CONFIG, config | BV(PWR_UP));
		delay_ms(5);
	}
}

void initTX(u8 address) {
	int popIterator;
	for (popIterator = 0; popIterator < MAX_NODES; popIterator++) {
		TXpacketCounters[popIterator] = 0;
		RXpacketCounters[popIterator] = PACKET_CNTER+1;
	}
	myAddress = address;

	pipe0_reading_address[0] = 0;

	delay_us(5000);

	setRegister(NRF_CONFIG, 0b00001100);

	/* Set Data Rate */
	u8 setup = getRegister(RF_SETUP);
	setup &= ~(BV(RF_DR_LOW) | BV(RF_DR_HIGH));
	setRegister(RF_SETUP, setup);

	setRegister(FEATURE, 0);
	setRegister(DYNPD, 0);
	setRegister(NRF_STATUS, BV(RX_DR) | BV(TX_DS) | BV(MAX_RT));
	flushTX();
	flushRX();

	setRegister(RF_CH, 108); // Set Channel
	setRegister(EN_AA, 0b111111); // Set Auto Ack
	setRegister(SETUP_RETR, (((5 + myAddress) & 0xF) << 4) | (15 & 0xF) << 0); // Retries = 12

	int pipeNum = 1;
	for(popIterator = 0; popIterator < MAX_NODES; popIterator++) {
		if(popIterator != myAddress) {
			u64 rxAddress = (BASE_ADDRESS + ((popIterator) + ((MAX_NODES + 1)  *  ( myAddress))));
			openReadingPipe(pipeNum, rxAddress);
			pipeNum++;
		}
	}

	powerUp();

	setRegister(NRF_CONFIG, (getRegister(NRF_CONFIG) & ~BV(PRIM_RX)));

	setup = getRegister(RF_SETUP) & 0b11111000;
	u8 level = (3 << 1) + 1;
	setRegister(RF_SETUP, setup |= level); // Set Power Level
	startListening();
}

bool writePayload(u8* messageToSend) {
	gpio_write(gpio_device, 0);
	u8 messageToSendFinal[33];
	messageToSendFinal[0] = W_TX_PAYLOAD;
	for(int i = 0; i < 32; ++i) {
		messageToSendFinal[i + 1] = messageToSend[i];
	}
	spi_transfer(spi_device, (const char*)messageToSendFinal, (char*)NULL, 33);
	delay_us(130);

	gpio_write(gpio_device, 1); // HIGH

	setRegister(NRF_STATUS, BV(RX_DR) | BV(TX_DS) | BV(MAX_RT));
	if(getRegister(NRF_STATUS) & BV(MAX_RT)) {
		flushTX();
		return false;
	}
	return true;
}

u8* writeMessage(u8 sendAddress, u8* message) {
	stopListening();
	bool success;
	if(myAddress == sendAddress) {
		return false;
	}
	packetToSend[0] = myAddress;
	packetToSend[1] = TXpacketCounters[sendAddress];
	int messageLength = (strlen((const char*)message) < 30) ? strlen((const char*)message) : 30;
	for(int i = 0; i < messageLength; i = i + 1) {
		packetToSend[i + 2] = message[i];
	}
	openWritingPipe((BASE_ADDRESS + ((myAddress + ((MAX_NODES + 1) * sendAddress)))));
	success = writePayload(packetToSend);
	if(success == false) {
		startListening();
		delay_us(TIMEOUT * sendAddress);
		stopListening();
		success = writePayload(packetToSend);
	}
	TXpacketCounters[sendAddress] = ((TXpacketCounters[sendAddress] + 1) & (PACKET_CNTER - 1) );
	startListening();
	return packetToSend;
	//return success;
}

void readMessage_payload(void) {
	writeBuffer[0] = R_RX_PAYLOAD;
	for(int i = 1; i < 32; ++i) {
		writeBuffer[i] = NOP;
	}
	spi_transfer(spi_device, (const char*)writeBuffer, (char*)readBuffer, 32); // readBuffer now has all data
	setRegister(NRF_STATUS, BV(RX_DR) | BV(TX_DS) | BV(MAX_RT));
}

bool readMessage(u8* returnPacket) {
	stopListening();
	while(hasMessages()) {
		readMessage_payload();
		if(readBuffer[2] != RXpacketCounters[readBuffer[1]]) { // NOT Duplicate
			*returnPacket = readBuffer;
			RXpacketCounters[readBuffer[1]] = ((RXpacketCounters[readBuffer[1]] + 1) & (PACKET_CNTER - 1));
			startListening();
			return true;
		}
	}
	startListening();
	return false;
}

int main(void) {

	u32 cmd, xStatus;

	// disable DCache
	Xil_DCacheDisable();

	/*
	 * initialize SPIs with clk_polarity and clk_phase as 0
	 * Configure D10-D13 as Shared SPI (MISO is not used)
	 */
	init_io_switch();
	spi_device = spi_open(13, 12, 11, 10);
	spi_device = spi_configure(spi_device, 0, 0);

	gpio_device = gpio_open_device(0);
	gpio_device = gpio_configure(gpio_device, 2, 2, 1);
	gpio_set_direction(gpio_device, 0);

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

	Xil_Out32(XPAR_IOP_ARDUINO_INTR_BASEADDR+4,0x0);
	Xil_Out32(XPAR_IOP_ARDUINO_INTR_BASEADDR,0x0);

	u8 messageLength;
	u8 sendToAddress;
	u8 messageToSend[32];
	u8 returnMessage[32];
	bool readSuccess;
	u8 registerValue;
	u8* registerAll;
	u8* whatWrite;

	initTX(1);

	while(1) {

		while(MAILBOX_CMD_ADDR==0);
		cmd = MAILBOX_CMD_ADDR;

		switch(cmd) {

			case GET_SETUP:
				MAILBOX_DATA(0) = getRegister(RF_SETUP);
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case READ_REG:
				registerValue = MAILBOX_DATA(0);
				MAILBOX_DATA(0) = getRegister(registerValue);
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case HAS_MESSAGES:
				MAILBOX_DATA(0) = hasMessages();
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case READ_FROM: // ONLY CALL AFTER HAS_MESSAGES
				readSuccess = readMessage(returnMessage); // ADD STRLEN
				MAILBOX_DATA(0) = strlen((const char*)returnMessage);
				for(int i = 0; i < strlen((const char*)returnMessage); ++i) {
					MAILBOX_DATA(i + 1) = returnMessage[i];
				}
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case WRITE_TO:
				messageLength = MAILBOX_DATA(0);
				sendToAddress = MAILBOX_DATA(1);
				MAILBOX_DATA(0) = sendToAddress;
				MAILBOX_DATA(1) = myAddress;
				for(int i = 0; i < messageLength; ++i) {
					messageToSend[i] = MAILBOX_DATA((i + 2));
				}
				whatWrite = writeMessage(sendToAddress, messageToSend);
				for(int i = 0; i < 32; ++i) {
					MAILBOX_DATA(i) = whatWrite[i];
				}
				MAILBOX_CMD_ADDR = 0x0;
				break;

			default:
				MAILBOX_CMD_ADDR = 0x0;
				break;

		}

	}

}
