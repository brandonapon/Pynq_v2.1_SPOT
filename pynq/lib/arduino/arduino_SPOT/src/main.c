#include "circular_buffer.h"
#include "timer.h"
#include "i2c.h"
#include <string.h>

#include "Adafruit_BNO055.h"
#include "gps_module.h"
#include "haptic_module.h"
#include "arduino_rangefinder.h"
#include "uart_v2.h"

#define BYTES_AVAILABLE_HIGH 	0xFD
#define BYTES_AVAILABLE_LOW		0xFE
#define DATA_REG				0xFF

#define CONFIG_IOP_SWITCH       0x1
#define START_WAVEFORM          0x2
#define STOP_WAVEFORM           0x3
#define READ_IS_PLAYING         0x4
#define GET_ACCL_DATA           0x5
#define GET_GYRO_DATA           0x6
#define GET_COMPASS_DATA        0x7
#define GET_EULER_VECTOR		0x8
#define GET_TEMPERATURE         0x9
#define RESET                   0xA
#define CMD_BYTES_AVAILABLE		0xB
#define TEST_CMD_ONE_BYTE		0xC
#define STREAM_TEST				0xD
#define RANGE_ON				0xE
#define RANGE_OFF				0xF
#define RANGE_POLL 				0x10
#define RANGE_NOTHING			0x11
#define RANGE_DEVICE			0x12

#define NUM_BYTES				5
#define NUM_SAMPLES             100

#define RX						0
#define TX						1

uart uart_device;
static i2c device;

int main(void)
{
    u32 cmd;

    //Haptic variables
    int i;
    u8 waveforms[8];

    //GPS variables
    int16_t counter = 0;
    char gpsOutput[1024];
    char numberofBytes = 0;
    char bytesLow = 0;
    char bytesHigh = 0;

    //IMU variables
    uint8_t id;
    uint8_t data = 0x20;
    uint8_t zeros = 0;
    uint8_t powerMode = POWER_MODE_NORMAL;

    char* range;

   init_io_switch();
   uart_device = uart_open(TX, RX);
   int status = 0;
//   uart_device = uart_open_device(0);
   if(uart_device == -1){
	   status = -1;
   }

    device = i2c_open_device(0);

      bno_init();

    id = bno_readByte(BNO055_CHIP_ID_ADDR);
        if(id != BNO055_ID)
        { 
            delay_ms(1000); // hold on for boot
            id = bno_readByte(BNO055_CHIP_ID_ADDR);
            if(id != BNO055_ID) {
            return false;  // still not? ok bail
        }
    }

    setMode(OPERATION_MODE_CONFIG);

    iic_writeByte(_address,BNO055_SYS_TRIGGER_ADDR, &data);
    while (id != BNO055_ID)
    {
        delay_ms(10);
        id = bno_readByte(BNO055_CHIP_ID_ADDR);
    }
    delay_ms(50);

    iic_writeByte(_address, BNO055_PWR_MODE_ADDR, &powerMode);
    delay_ms(10);

    iic_writeByte(_address, BNO055_PAGE_ID_ADDR, &zeros);

    iic_writeByte(_address, BNO055_SYS_TRIGGER_ADDR, 0x0);
    delay_ms(10);

    setMode(OPERATION_MODE_NDOF);
    delay_ms(20);

    delay_ms(1000);

    setExtCrystalUse(true);

    // Run application
    while(1){
        // wait and store valid command
        while((MAILBOX_CMD_ADDR)==0);

        cmd = MAILBOX_CMD_ADDR;

        switch(cmd){
            case CONFIG_IOP_SWITCH:
                // use dedicated I2C - no operation needed
            	bno_init();
                auto_calibrate_hapt();
                MAILBOX_CMD_ADDR = 0x0;
                break;
            case START_WAVEFORM:
                // read waveforms from mailbox
                for(i=0; i<8; i++)
                    waveforms[i] = MAILBOX_DATA(i) & 0xff;
                play_hapt(waveforms);
                MAILBOX_CMD_ADDR = 0x0;
                break;
            case STOP_WAVEFORM:
                stop_hapt();
                MAILBOX_CMD_ADDR = 0x0;
                break;
            case READ_IS_PLAYING:
                MAILBOX_DATA(0) = is_playing_hapt();
                MAILBOX_CMD_ADDR = 0x0;
                break;
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

         case GET_ACCL_DATA:
            getVector(VECTOR_ACCELEROMETER);
            MAILBOX_CMD_ADDR = 0x0; 
            break;
            
         case GET_GYRO_DATA:
          getVector(VECTOR_GYROSCOPE);
          MAILBOX_CMD_ADDR = 0x0; 
            break;
            
         case GET_COMPASS_DATA:
          getVector(VECTOR_MAGNETOMETER);
          MAILBOX_CMD_ADDR = 0x0;
            break;
            
         case GET_EULER_VECTOR:
          getVector(VECTOR_EULER);
          MAILBOX_CMD_ADDR = 0x0;
           break;

         case GET_TEMPERATURE:
            getTemp();
            MAILBOX_CMD_ADDR = 0x0;
            break;
            
         case RESET:
//            mpu_reset();
            MAILBOX_CMD_ADDR = 0x0; 
            break;

       	 case RANGE_DEVICE:
				MAILBOX_DATA(0) = status;
				MAILBOX_CMD_ADDR = 0x0;
				break;

		case RANGE_ON:
			uart_write(uart_device, on, 1);
			MAILBOX_CMD_ADDR = 0x0;
			break;

		case RANGE_OFF:
			uart_write(uart_device, off, 1);
			MAILBOX_CMD_ADDR = 0x0;
			break;

		case RANGE_POLL:
			range = readRF();
			for(int i = 0; i < 5; ++i){
				MAILBOX_DATA(i) = (char) range[i];
			}
			MAILBOX_CMD_ADDR = 0x0;
			break;

		case RANGE_NOTHING:
			MAILBOX_DATA(0) = 100;
			MAILBOX_CMD_ADDR = 0x0;
			break;

        default:
            MAILBOX_CMD_ADDR = 0x0; // reset command
            break;
        }
    }
    return 0;
}