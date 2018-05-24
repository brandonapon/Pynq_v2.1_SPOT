/******************************************************************************
 *  Copyright (c) 2016, Xilinx, Inc.
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1.  Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *
 *  2.  Redistributions in binary form must reproduce the above copyright 
 *      notice, this list of conditions and the following disclaimer in the 
 *      documentation and/or other materials provided with the distribution.
 *
 *  3.  Neither the name of the copyright holder nor the names of its 
 *      contributors may be used to endorse or promote products derived from 
 *      this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/******************************************************************************
 *
 *
 * @file arduino_grove_imu.c
 *
 * IOP code (MicroBlaze) for grove IMU 10DOF.
 * The grove IMU has to be connected to an arduino interface 
 * via a shield socket.
 * Grove IMU is read only, and has IIC interface.
 * Hardware version 1.1.
 * http://www.seeedstudio.com/wiki/Grove_-_IMU_10DOF
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   Who  Date     Changes
 * ----- --- ------- -----------------------------------------------
 * 1.00a yrq 04/25/16 release
 * 1.00d yrq 07/26/16 separate pmod and arduino
 *
 * </pre>
 *
 *****************************************************************************/

#include "Adafruit_BNO055.h"
#include "circular_buffer.h"
#include "timer.h"
#include "i2c.h"
#include <string.h>

#define NUM_SAMPLES              100

// Mailbox commands


static i2c device;

//Adafruit_BNO055(int32_t sensorID, uint8_t address)
//{
//  _sensorID = sensorID;
//  _address = address;
//}
// Byte operations
int iic_readBytes(uint8_t devAddr, uint8_t regAddr, 
                uint8_t length, uint8_t *data){
    i2c_write(device, devAddr, &regAddr, 1);
    i2c_read(device, devAddr, data, length);
    return length;
}

int iic_readByte(uint8_t devAddr, uint8_t regAddr, uint8_t *data){
    i2c_write(device, devAddr, &regAddr, 1);
    i2c_read(device, devAddr, data, 1);
    return 1;
}

void iic_writeBytes(uint8_t devAddr, uint8_t regAddr, 
                uint8_t length, uint8_t *data){
    int i;
    int len_total = (int)length+1;
    uint8_t temp[len_total];
    temp[0] = regAddr;
    for (i=1;i<len_total;i++){
        temp[i]=data[i-1];
    }
    i2c_write(device, devAddr, temp, len_total);
}

void iic_writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t *data){
    uint8_t temp[2];
    temp[0] = regAddr;
    temp[1] = *data;
    i2c_write(device, devAddr, temp, 2);
}

// Bit operations
int8_t iic_readBits(uint8_t devAddr, uint8_t regAddr, 
                    uint8_t bitStart, uint8_t width, uint8_t *data) {
    /*
     * 01101001 read byte
     * 76543210 bit numbers
     *    xxx   parameters: bitStart=4, width=3
     *    010   masked
     *   -> 010 shifted
     */
    uint8_t count, b;
    uint8_t mask;
    if ((count = iic_readBytes(devAddr, regAddr, 1, &b)) != 0) {
        mask = ((1 << width) - 1) << (bitStart - width + 1);
        b &= mask;
        b >>= (bitStart - width + 1);
        *data = b;
    }
    return count;
}
int8_t iic_readBit(uint8_t devAddr, uint8_t regAddr, 
                   uint8_t bitStart, uint8_t *data) {
    return iic_readBits(devAddr, regAddr, bitStart, (uint8_t) 1, data);
}
    
void iic_writeBits(uint8_t devAddr, uint8_t regAddr, 
                     uint8_t bitStart, uint8_t width, uint8_t *data) {
    /*
     * 010 value to write
     * 76543210 bit numbers
     *    xxx   parameters: bitStart=4, width=3
     * 00011100 mask byte
     * 10101111 original value (sample)
     * 10100011 original & ~mask
     * 10101011 masked | value
     */
    uint8_t b, temp;
    temp = *data;
    if (iic_readBytes(devAddr, regAddr, 1, &b) != 0) {
        uint8_t mask = ((1 << width) - 1) << (bitStart - width + 1);
        // shift data into correct position
        temp <<= (bitStart - width + 1);
        // zero all non-important bits in data
        temp &= mask;
        // zero all important bits in existing byte
        b &= ~(mask);
        // combine data with existing byte
        b |= temp;
        iic_writeByte(devAddr, regAddr, &b);
    }
}
void iic_writeBit(uint8_t devAddr, uint8_t regAddr, 
                    uint8_t bitStart, uint8_t *data) {
    iic_writeBits(devAddr, regAddr, bitStart, (uint8_t) 1, data);
}

void getTemp( void )
{
  uint8_t temp;
  iic_readByte(BNO055_ADDRESS_A,BNO055_TEMP_ADDR,&temp);
  MAILBOX_DATA(0) = temp;
}

void setMode(adafruit_bno055_opmode_t mode)
{
  _mode = mode;
  iic_writeByte(_address,BNO055_OPR_MODE_ADDR, &_mode);
  delay_ms(30);
}


uint16_t bno_readBytes(uint8_t address)
{
    // read 2 bytes from the address and return an int
    uint8_t msb, lsb;
    iic_readByte(_address, address, &msb);
    delay_ms(20);
    iic_readByte(_address, address+1, &lsb);
    delay_ms(20);
    return (int) msb<<8 | lsb;
}

uint8_t bno_readByte(unsigned char address)
{
    // read a single byte from the address
    uint8_t data;
    iic_readByte(_address, address, &data);
    delay_ms(20);
    return data;
}

void bno_init(){
    _address = BNO055_ADDRESS_A;
    _sensorID = -1;
    _mode = OPERATION_MODE_CONFIG;
  }

//  void getCalibration(uint8_t* sys, uint8_t* gyro, uint8_t* accel, uint8_t* mag) {
//  uint8_t calData = bno_readByte(BNO055_CALIB_STAT_ADDR);
//  if (sys != NULL) {
//    *sys = (calData >> 6) & 0x03;
//  }
//  if (gyro != NULL) {
//    *gyro = (calData >> 4) & 0x03;
//  }
//  if (accel != NULL) {
//    *accel = (calData >> 2) & 0x03;
//  }
//  if (mag != NULL) {
//    *mag = calData & 0x03;
//  }
//}

void setExtCrystalUse(bool usextal)
{
  adafruit_bno055_opmode_t modeback = _mode;

  /* Switch to config mode (just in case since this is the default) */
  setMode(OPERATION_MODE_CONFIG);
  delay_ms(25);
  iic_writeByte(_address, BNO055_PAGE_ID_ADDR, 0);
  if (usextal) {
    iic_writeByte(_address, BNO055_SYS_TRIGGER_ADDR, 0x80);
  } else {
    iic_writeByte(_address, BNO055_SYS_TRIGGER_ADDR, 0x00);
  }
  delay_ms(10);
  /* Set the requested operating mode (see section 3.3) */
  setMode(modeback);
  delay_ms(20);
}

void getVector(adafruit_vector_type_t vector_type)
{
  uint8_t buffer[6];
  memset (buffer, 0, 6);
  int16_t x, y, z;
  x = y = z = 0;

  /* Read vector data (6 bytes) */
  iic_readBytes(_address, (adafruit_bno055_reg_t)vector_type, 6, buffer);

  x = ((int16_t)buffer[0]) | (((int16_t)buffer[1]) << 8);
  y = ((int16_t)buffer[2]) | (((int16_t)buffer[3]) << 8);
  z = ((int16_t)buffer[4]) | (((int16_t)buffer[5]) << 8);

  /* Convert the value to an appropriate range (section 3.6.4) */
  /* and assign the value to the Vector type */
  switch(vector_type)
  {
    case VECTOR_MAGNETOMETER:
      /* 1uT = 16 LSB */
      MAILBOX_DATA(0) = x;
      MAILBOX_DATA(1) = y;
      MAILBOX_DATA(2) = z;
      break;
    case VECTOR_GYROSCOPE:
      /* 1dps = 16 LSB */
      MAILBOX_DATA(0) = x;
      MAILBOX_DATA(1) = y;
      MAILBOX_DATA(2) = z;
      break;
    case VECTOR_EULER:
      /* 1 degree = 16 LSB */
      MAILBOX_DATA(0) = x;
      MAILBOX_DATA(1) = y;
      MAILBOX_DATA(2) = z;
      break;
    case VECTOR_ACCELEROMETER:
    case VECTOR_LINEARACCEL:
    case VECTOR_GRAVITY:
      /* 1m/s^2 = 100 LSB */
      MAILBOX_DATA(0) = x;
      MAILBOX_DATA(1) = y;
      MAILBOX_DATA(2) = z;
      break;
  }
}




//float bmp_getTemperature()
//{
//    unsigned int ut;
//    long x1, x2;
//    float temp;
//    uint8_t data;
//
//    // get the uncompensated temperature
//    data = 0x2E;
//    iic_writeByte(bmpAddr, 0xF4, &data);
//    delay_ms(20);
//    ut = (long)bmp_readBytes(0xF6);
//
//    // calculate the compensated temperature
//    x1 = ((ut - ac6)*(long)ac5) >> 15;
//    x2 = ((long)mc << 11)/(x1 + md);
//    PressureCompensate = x1 + x2;
//    temp = (float)((PressureCompensate + 8)>>4);
//    temp = temp /10;
//
//    return temp;
//}


