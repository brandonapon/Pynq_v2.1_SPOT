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
#define DEVICE			0x06
#define STOP_LISTEN		0x07
#define START_LISTEN	0x08
#define START_TX		0x09

#define MAX_NODES 		6
#define PACKET_CNTER 	32
#define BASE_ADDRESS 	0xDEADBEEF00
#define TIMEOUT 		5
#define DELAY_TIME		450
#define MAX_LEN			32

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
u8 returnBuff[32];
u8 writeBuffer[33];
u8 pipe0_reading_address[5];
u8 addr_width = 5;
u8 payload_size = 32;
u8 packetToSend[32];
u8 getFromRX[32];

static const u8 child_pipe_enable[] = {
  ERX_P0, ERX_P1, ERX_P2, ERX_P3, ERX_P4, ERX_P5
};
static const u8 child_pipe[] = {
  RX_ADDR_P0, RX_ADDR_P1, RX_ADDR_P2, RX_ADDR_P3, RX_ADDR_P4, RX_ADDR_P5
};
static const u8 child_payload_size[] = {
  RX_PW_P0, RX_PW_P1, RX_PW_P2, RX_PW_P3, RX_PW_P4, RX_PW_P5
};

void chipSelectHigh(void) {
	gpio_write(gpio_device, 1);
}

void chipSelectLow(void) {
	gpio_write(gpio_device, 0);
}

u8 read_register(u8 reg) {
	writeBuffer[0] = (R_REGISTER | (REGISTER_MASK & reg));
	writeBuffer[1] = NOP;
	spi_transfer(spi_device, (const char*)writeBuffer, (char*)readBuffer, 2);
	return readBuffer[1];
}

void write_register_multi(u8 reg, const u8* buf, u8 len) {
	writeBuffer[0] = (W_REGISTER | (REGISTER_MASK & reg));
	for(int i = 0; i < len; ++i) {
		writeBuffer[i + 1] = buf[i];
	}
	spi_transfer(spi_device, (const char*)writeBuffer, NULL, len + 1);
}

void write_register(u8 reg, u8 buf) {
	writeBuffer[0] = (W_REGISTER | (REGISTER_MASK & reg));
	writeBuffer[1] = buf;
	spi_transfer(spi_device, (const char*)writeBuffer, NULL, 2);
}

void write_payload(const void* buf, u8 len, const u8 writeType) {
	const u8* current = (u8*) buf;
	u8 blank_left = MAX_LEN - len;

	writeBuffer[0] = writeType;
	for(int i = 0; i < len; ++i) {
		writeBuffer[i + 1] = current[i];
	}
	for(int i = 0; i < blank_left; ++i) {
		writeBuffer[i + len + 1] = 0;
	}

	spi_transfer(spi_device, (const char*)writeBuffer, NULL, MAX_LEN + 1);
}

void read_payload(void* buf, u8 len) {
	u8 status;
  	u8* current = (u8*)(buf);
  	u8 * prx = writeBuffer;
	u8 * ptx = readBuffer;
    u8 size = 33;

    *ptx++ =  R_RX_PAYLOAD;
	while(--size) 
		*ptx++ = NOP;

	spi_transfer(spi_device, (const char*) writeBuffer, (char*)readBuffer, 33);

	status = *prx++; // 1st byte is status	

	if (len > 0) {
      while ( --len ) // Decrement before to skip 1st status byte
          *current++ = *prx++;
		
      *current = *prx;
    }
}

void flushRX(void) {
	writeBuffer[0] = FLUSH_RX;
	spi_transfer(spi_device, (const char*)writeBuffer, NULL, 1);
}

void flushTX(void) {
	writeBuffer[0] = FLUSH_TX;
	spi_transfer(spi_device, (const char*)writeBuffer, NULL, 1);
}

u8 getStatus(void) {
	writeBuffer[0] = NOP;
	spi_transfer(spi_device, (const char*)writeBuffer, (char*)readBuffer, 1);
	return readBuffer[0];
}

void setChannel(u8 channel) {
	u8 newChannel;
	if(channel > 125) {
		newChannel = 124;
	}
	else {
		newChannel = channel;
	}
	write_register(RF_CH, newChannel);
}

u8 getChannel(void) {
	return read_register(RF_CH);
}

u8 BV(int val) {
	return 1 << val;
}

void setPALevel(u8 level) {
	u8 setup = read_register(RF_SETUP) & 0b11111000;

	if(level > 3) {
		level = (3 << 1) + 1;
	}
	else {
		level = (level << 1) + 1;
	}

	write_register(RF_SETUP, setup |= level);
}

void setDataRate(void) {
	u8 setup = read_register(RF_SETUP);

	setup &= ~(BV(RF_DR_LOW) | BV(RF_DR_HIGH));

	write_register(RF_SETUP, setup);
}

void setRetries(u8 delay, u8 count) {
	write_register(SETUP_RETR, (delay & 0xf) << ARD | (count & 0xf) << ARC);
}

void powerUp(void) {
	u8 config = read_register(NRF_CONFIG);

	if(!(config & BV(PWR_UP))) {
		write_register(NRF_CONFIG, read_register(NRF_CONFIG) | BV(PWR_UP));
		delay_ms(5);
	}
}

void toggle_features(void) {
	writeBuffer[0] = ACTIVATE;
	writeBuffer[1] = 0x73;
	spi_transfer(spi_device, (const char*)writeBuffer, NULL, 2);
}

void setAutoAck(u8 enable) {
	if(enable) {
		write_register(EN_AA, 0b111111);
	}
	else {
		write_register(EN_AA, 0);
	}
}

void openWritingPipe(const u64 address) {
	write_register_multi(RX_ADDR_P0, (u8*)(&address), addr_width);
	write_register_multi(TX_ADDR, (u8*)(&address), addr_width);

	write_register(RX_PW_P0, payload_size);
}

void openReadingPipe(u8 child, u64 address) {
	if(child == 0) {
		memcpy(pipe0_reading_address, &address, addr_width);
	}
	if(child <= 6) {
		if(child < 2) {
			write_register_multi(child_pipe[child], (u8*)(&address), addr_width);
		}
		else {
			write_register(child_pipe[child], (u8)address);
		}
		write_register(child_payload_size[child], payload_size);

		write_register(EN_RXADDR, read_register(EN_RXADDR) | BV(child_pipe_enable[child]));
	}
}

bool begin(u8 myAddress) {
	u8 setup = 0;
	chipSelectLow();

	delay_ms(5);

	write_register(NRF_CONFIG, 0b00001100);

	setRetries(5 + myAddress, 15);

	setup = read_register(RF_SETUP);

	setDataRate();

	toggle_features();
	write_register(FEATURE, 0);
	write_register(DYNPD, 0);

	write_register(NRF_STATUS, BV(RX_DR) | BV(TX_DS) | BV(MAX_RT));

	setChannel(108);

	flushTX();
	flushRX();

	powerUp();

	write_register(NRF_CONFIG, (read_register(NRF_CONFIG) & ~BV(PRIM_RX)));

	return (setup != 0 && setup != NOP);
}

void closeReadingPipe(u8 pipe) {
	write_register(EN_RXADDR, read_register(EN_RXADDR) & ~BV(child_pipe_enable[pipe]));
}

void startListening(void) {
	write_register(NRF_CONFIG, read_register(NRF_CONFIG) | BV(PRIM_RX));
	write_register(NRF_STATUS, BV(RX_DR) | BV(TX_DS) | BV(MAX_RT));

	chipSelectHigh();

	if(pipe0_reading_address[0] > 0) {
		write_register_multi(RX_ADDR_P0, pipe0_reading_address, addr_width);
	}
	else {
		closeReadingPipe(0);
	}

	if(read_register(FEATURE) & BV(EN_ACK_PAY)){
		flushTX();
  	}

  	delay_us(450);
}

void stopListening(void) {
	chipSelectLow();

	delay_us(450);

	if(read_register(FEATURE) & BV(EN_ACK_PAY)) {
		delay_us(200);
		flushTX();
	}

	write_register(NRF_CONFIG, read_register(NRF_CONFIG) & ~BV(PRIM_RX));
	write_register(EN_RXADDR, read_register(EN_RXADDR) | BV(child_pipe_enable[0]));
}

void powerDown(void) {
	chipSelectLow();
	write_register(NRF_CONFIG, read_register(NRF_CONFIG) & ~BV(PWR_UP));
}

void startFastWrite(void* buf, u8 len, const bool multicast) {
	write_payload(buf, len, multicast ? W_TX_PAYLOAD_NO_ACK : W_TX_PAYLOAD);
	chipSelectHigh();
}

bool write(void* buf, u8 len) {
	startFastWrite(buf, len, 0);

	while(!(getStatus() & (BV(TX_DS) | BV(MAX_RT))));

	chipSelectLow();

	write_register(NRF_STATUS, BV(RX_DR) | BV(TX_DS) | BV(MAX_RT));
	u8 status = read_register(NRF_STATUS);

	if(status & BV(MAX_RT)) {
		flushTX();
		return 0;
	}
	return 1;
}

bool isFIFOFull(void) {
	return read_register(FIFO_STATUS) & BV(RX_FULL);
}

bool available(void) {
	if(!(read_register(FIFO_STATUS) & BV(RX_EMPTY))) {
		return 1;
	}
	return 0;
}

void read(void* buf, u8 len) {
	read_payload(buf, len);

	write_register(NRF_STATUS, BV(RX_DR) | BV(TX_DS) | BV(MAX_RT));
}

bool readMessage(u8* buf) {
	u8 packet[33];
	stopListening();
	if(available()) {
		read(&packet, 32);
		if(buf[1] != RXpacketCounters[buf[0]]) {
			*buf = packet;
			RXpacketCounters[buf[0]] = ((RXpacketCounters[buf[0]] + 1) & (PACKET_CNTER - 1));
			startListening();
			return true;
		}
	}
	startListening();
	return false;
}

bool writeMessage(u8 sendToAddress, u8* buf, u8 len) {
	stopListening();
	bool success;
	if(sendToAddress == myAddress) {
		return false;
	}
	openWritingPipe((BASE_ADDRESS + ((myAddress) + ((MAX_NODES + 1)  *  sendToAddress))));
	success = write(&buf, len);
	if(success == false) {
		startListening();
		delay_ms(TIMEOUT * myAddress);
		stopListening();
		success = write(&buf, len);
	}
	TXpacketCounters[sendToAddress] = ((TXpacketCounters[sendToAddress] + 1) & (PACKET_CNTER - 1));
	startListening();
	return success;
}

void beginTX(u8 address) {
	int popIterator;
	for (popIterator = 0; popIterator < (MAX_NODES); popIterator++) {
		TXpacketCounters[popIterator] = 0;
		RXpacketCounters[popIterator] = PACKET_CNTER+1;
	}
	myAddress = address;
	u8 setup;

	setup = begin(myAddress);
	setAutoAck(1);
	int pipenum = 1;
	for (popIterator = 0; popIterator < (MAX_NODES); popIterator++) {
		if ((popIterator) != myAddress) {
			u64 rxAddress = (BASE_ADDRESS + ((popIterator) + ((MAX_NODES + 1)  *  ( myAddress))));
			openReadingPipe(pipenum, rxAddress);
			pipenum++;
		}
	}

	setPALevel(3);
	startListening();
	return (setup != 0 && setup != NOP);
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
	u8 registerAll[33];
	u8* whatWrite;
	u8 txHasBegun;

	while(1) {

		while(MAILBOX_CMD_ADDR==0);
		cmd = MAILBOX_CMD_ADDR;

		switch(cmd) {

			case START_TX:
				beginTX(MAILBOX_DATA(0));
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case GET_SETUP:
				MAILBOX_DATA(0) = read_register(RF_SETUP);
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case READ_REG:
				registerValue = MAILBOX_DATA(0);
				MAILBOX_DATA(0) = read_register(registerValue);
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case HAS_MESSAGES:
				MAILBOX_DATA(0) = available();
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case READ_FROM: // ONLY CALL AFTER HAS_MESSAGES
				readMessage(registerAll); // ADD STRLEN
				for(int i = 0; i < 32; ++i) {
					MAILBOX_DATA(i) = registerAll[i];
				}
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case WRITE_TO:
				messageLength = MAILBOX_DATA(0);
				sendToAddress = MAILBOX_DATA(1);
				for(int i = 0; i < messageLength; ++i) {
					messageToSend[i] = MAILBOX_DATA((i + 2));
				}
				readSuccess = writeMessage(sendToAddress, messageToSend, messageLength);
				MAILBOX_DATA(0) = readSuccess;
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case DEVICE:
				MAILBOX_DATA(0) = spi_device;
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case STOP_LISTEN:
				stopListening();
				MAILBOX_CMD_ADDR = 0x0;
				break;

			case START_LISTEN:
				startListening();
				MAILBOX_CMD_ADDR = 0x0;
				break;

			default:
				MAILBOX_CMD_ADDR = 0x0;
				break;

		}

	}

}
