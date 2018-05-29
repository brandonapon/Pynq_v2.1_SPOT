/*
	C Driver for TX
*/

#include "xparameters.h"
#include "circular_buffer.h"
#include "gpio.h"
#include "spi_v2.h"
#include "timer.h"
#include "xio_switch.h"
#include "xil_cache.h"
#include "xsysmon.h"
#include "nRF24L01.h"
#include "math.h"
#include "arduino_tx.h"

#define MAX_NODES 		6
#define PACKET_CNTER 	32
#define BASE_ADDRESS 	0xDEADBEEF00
#define TIMEOUT 		5
#define DELAY_TIME		450
#define MESSAGE_LEN		153600
#define MAX_SEND_LEN	29


#define SYSMON_DEVICE_ID XPAR_SYSMON_0_DEVICE_ID

spi transceiver;
gpio gpio_device;
static XSysMon SysMonInst;
XSysMon_Config *SysMonConfigPtr;
XSysMon *SysMonInstPtr = &SysMonInst;

u8 myAddress;
u8 TXpacketCounters[MAX_NODES];
u8 RXpacketCounters[MAX_NODES];

u8 readBuffer[32];
u8 returnBuff[32];
u8 writeBuffer[32];
u8 pipe0_reading_address[5];
u8 addr_width = 5;
u8 payload_size = 32;
u8 packetToSend[32];
u8 getFromRX[32];

void setSPI(spi deviceId){
	transceiver = deviceId;
}

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
	spi_transfer(transceiver, (const char*)writeBuffer, (char*)readBuffer, 1);
	return readBuffer[0];
}

void setRegister(u8 registerVal, u8 value) {
	writeBuffer[0] = (W_REGISTER | (REGISTER_MASK & registerVal));
	writeBuffer[1] = value;
	spi_transfer(transceiver, (const char*)writeBuffer, (char*)NULL, 2);
}

void setRegisterMulti(u8 registerVal, u8* value, u8 length) { // WRITES MULTI BYTES
	writeBuffer[0] = W_REGISTER | (REGISTER_MASK & registerVal);
	for(int i = 0; i < length; ++i) {
		writeBuffer[i + 1] = value[i];
	}
	spi_transfer(transceiver, (const char*) writeBuffer, (char*)NULL, length + 1);
}

u8 getRegister(u8 registerVal) {
	writeBuffer[0] = R_REGISTER | (REGISTER_MASK & registerVal);
	writeBuffer[1] = NOP;
	spi_transfer(transceiver, (const char*)writeBuffer, (char*)readBuffer, 2);
	return readBuffer[1];
}

void flushRX(void) {
	writeBuffer[0] = FLUSH_RX;
	spi_transfer(transceiver, (const char*)writeBuffer, (char*)NULL, 1);
}

void flushTX(void) {
	writeBuffer[0] = FLUSH_TX;;
	spi_transfer(transceiver, (const char*)writeBuffer, (char*)NULL, 1);
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
	delay_us(200);

	if(getRegister(FEATURE) & BV(EN_ACK_PAY)) {
		delay_us(200);
		flushTX();
	}

	setRegister(NRF_CONFIG, getRegister(NRF_CONFIG) & ~BV(PRIM_RX));
	setRegister(EN_RXADDR, getRegister(EN_RXADDR) | BV(child_pipe_enable[0]));
}

void openWritingPipe(u64 val) {
	setRegisterMulti(RX_ADDR_P0, (u8*)(&val), addr_width);
	setRegisterMulti(TX_ADDR, (u8*)(&val), addr_width);

	setRegister(RX_PW_P0, payload_size);
}

void openReadingPipe(u8 child, u64 address) {
	if(child == 0) {
		memcpy(pipe0_reading_address, &address, addr_width);
	}

	if(child <= 6) {
		if(child < 2) {
			setRegisterMulti(child_pipe[child], (u8*)(&address), addr_width);
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

	gpio_write(gpio_device, 0);

	delay_ms(5);

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
	setRegister(EN_AA, 0b111111);
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
	u8 messageToSendFinal[33];
	messageToSendFinal[0] = W_TX_PAYLOAD;
	for(int i = 0; i < 32; ++i) {
		messageToSendFinal[i + 1] = messageToSend[i];
	}
	spi_transfer(transceiver, (const char*)messageToSendFinal, (char*)NULL, 33);
	gpio_write(gpio_device, 1);
	
	while(!(getStatus() & (BV(TX_DS) | BV(MAX_RT))));

	gpio_write(gpio_device, 0);

	u8 status = getRegister(NRF_STATUS);
	setRegister(NRF_STATUS, BV(RX_DR) | BV(TX_DS) | BV(MAX_RT));
	if(status & BV(MAX_RT)) {
		flushTX();
		return false;
	}
	return true;
}

bool writeMessage(u8 sendAddress, u8* message, u8 len) {
	stopListening();
	bool success;
	if(myAddress == sendAddress) {
		return false;
	}
	packetToSend[0] = myAddress;
	packetToSend[1] = TXpacketCounters[sendAddress];
	for(int i = 0; i < len; i = i + 1) {
		packetToSend[i + 2] = message[i];
	}
	openWritingPipe((BASE_ADDRESS + ((myAddress + ((MAX_NODES + 1) * sendAddress)))));
	success = writePayload(packetToSend);
	if(success == false) {
		startListening();
		delay_ms(TIMEOUT * sendAddress);
		stopListening();
		success = writePayload(packetToSend);
	}
	TXpacketCounters[sendAddress] = ((TXpacketCounters[sendAddress] + 1) & (PACKET_CNTER - 1) );
	startListening();
	return success;
}

void readMessage_payload(u8* buf) {
	u8 readFromBuffer[32];
	readFromBuffer[0] = R_RX_PAYLOAD;
	for(int i = 1; i < 32; ++i) {
		readFromBuffer[i] = NOP;
	}
	spi_transfer(transceiver, (const char*)(readFromBuffer), (char*)(buf), 33);
	setRegister(NRF_STATUS, BV(RX_DR) | BV(TX_DS) | BV(MAX_RT));
}

bool readMessage(u8* buf) {
	stopListening();
	while(hasMessages()) {
		readMessage_payload(buf);
		if(buf[2] != RXpacketCounters[buf[1]]) { // NOT Duplicate
			RXpacketCounters[buf[1]] = ((RXpacketCounters[buf[1]] + 1) & (PACKET_CNTER - 1));
			startListening();
			return true;
		}
	}
	startListening();
	return false;
}

//int main(void) {
//
//	u32 cmd, xStatus;
//
//	// disable DCache
//	Xil_DCacheDisable();
//
//	/*
//	 * initialize SPIs with clk_polarity and clk_phase as 0
//	 * Configure D10-D13 as Shared SPI (MISO is not used)
//	 */
//	init_io_switch();
//	transceiver = spi_open(13, 12, 11, 10);
//	transceiver = spi_configure(transceiver, 0, 0);
//
//	gpio_device = gpio_open_device(0);
//	gpio_device = gpio_configure(gpio_device, 2, 2, 1);
//	gpio_set_direction(gpio_device, 0);
//
//	// SysMon Initialize
//	SysMonConfigPtr = XSysMon_LookupConfig(SYSMON_DEVICE_ID);
//	if(SysMonConfigPtr == NULL)
//		return -1;
//	xStatus = XSysMon_CfgInitialize(SysMonInstPtr,SysMonConfigPtr,
//									SysMonConfigPtr->BaseAddress);
//	if(XST_SUCCESS != xStatus)
//		return -1;
//
//	// Clear the old status
//	XSysMon_GetStatus(SysMonInstPtr);
//
//	Xil_Out32(XPAR_IOP_ARDUINO_INTR_BASEADDR+4,0x0);
//	Xil_Out32(XPAR_IOP_ARDUINO_INTR_BASEADDR,0x0);
//
//	u8 messageLength;
//	u8 sendToAddress;
//	u8 messageToSend[32];
//	u8 returnMessage[32];
//	bool readSuccess;
//	u8 registerValue;
//	u8 registerAll[33];
//	u8* whatWrite;
//	u8* cameraArray;
//	u16 numberOfMessages;
//	u8 numberToRead;
//	u8 counter;
//	u8 mailboxCounter;
//
//	while(1) {
//
//		while(MAILBOX_CMD_ADDR==0);
//		cmd = MAILBOX_CMD_ADDR;
//
//		switch(cmd) {
//
//			case BEGIN_TX:
//				initTX(MAILBOX_DATA(0));
//				MAILBOX_CMD_ADDR = 0x0;
//				break;
//
//			case GET_SETUP:
//				MAILBOX_DATA(0) = getRegister(RF_SETUP);
//				MAILBOX_CMD_ADDR = 0x0;
//				break;
//
//			case READ_REG:
//				registerValue = MAILBOX_DATA(0);
//				MAILBOX_DATA(0) = getRegister(registerValue);
//				MAILBOX_CMD_ADDR = 0x0;
//				break;
//
//			case HAS_MESSAGES:
//				MAILBOX_DATA(0) = hasMessages();
//				MAILBOX_CMD_ADDR = 0x0;
//				break;
//
//			case READ_FROM: // ONLY CALL AFTER HAS_MESSAGES
//				readMessage(registerAll); // ADD STRLEN
//				for(int i = 0; i < 33; ++i) {
//					MAILBOX_DATA(i) = registerAll[i];
//				}
//				MAILBOX_CMD_ADDR = 0x0;
//				break;
//
//			case WRITE_TO:
//				messageLength = MAILBOX_DATA(0);
//				sendToAddress = MAILBOX_DATA(1);
//				for(int i = 0; i < messageLength; ++i) {
//					messageToSend[i] = MAILBOX_DATA((i + 2));
//				}
//				readSuccess = writeMessage(sendToAddress, messageToSend, 30);
//				MAILBOX_DATA(0) = readSuccess;
//				for(int i = 0; i < messageLength; ++i) {
//					messageToSend[i] = 0;
//				}
//				MAILBOX_CMD_ADDR = 0x0;
//				break;
//
//			case DEVICE:
//				MAILBOX_DATA(0) = transceiver;
//				MAILBOX_CMD_ADDR = 0x0;
//				break;
//
//			case WRITE_CAMERA:
//				sendToAddress = MAILBOX_DATA(0);
//				cameraArray = CAMERA_ADDR | 0x20000000;
//				numberOfMessages = ceil(MESSAGE_LEN / MAX_SEND_LEN);
//				mailboxCounter = 0;
//				for(int i = 0; i < numberOfMessages; ++i) {
//					if(MESSAGE_LEN >= ((i * 1) * MAX_SEND_LEN)) {
//						numberToRead = MAX_SEND_LEN;
//					}
//					else {
//						numberToRead = MESSAGE_LEN % MAX_SEND_LEN;
//					}
//					counter = 0;
//					for(int j = (i * MAX_SEND_LEN); j < ((i * MAX_SEND_LEN) + numberToRead); ++j) {
//						messageToSend[counter] = cameraArray[j];
//						++counter;
//					}
//					readSuccess = writeMessage(sendToAddress, messageToSend, 30);
//					MAILBOX_DATA(mailboxCounter * 4) = readSuccess;
//					++mailboxCounter;
//				}
//				MAILBOX_CMD_ADDR = 0x0;
//				break;
//
//			default:
//				MAILBOX_CMD_ADDR = 0x0;
//				break;
//
//		}
//
//	}
//
//}