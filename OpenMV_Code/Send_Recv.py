# Untitled - By: Brandon - Sun Mar 11 2018

#import sensor, image, time

#sensor.reset()
#sensor.set_pixformat(sensor.RGB565)
#sensor.set_framesize(sensor.QVGA)
#sensor.skip_frames(time = 2000)

#clock = time.clock()
#
#while(True):
#    clock.tick()
#    img = sensor.snapshot()
#    print(clock.fps())

# SPI with the Arduino as the master device and the OpenMV Cam as the slave.
#
# Please wire up your OpenMV Cam to your Arduino like this:
#
# OpenMV Cam Master Out Slave In (P0) - Arduino Uno MOSI (11)
# OpenMV Cam Master In Slave Out (P1) - Arduino Uno MISO (12)
# OpenMV Cam Serial Clock        (P2) - Arduino Uno SCK  (13)
# OpenMV Cam Slave Select        (P3) - Arduino Uno SS   (10)
# OpenMV Cam Ground                   - Arduino Ground

import pyb, ustruct

text = "Hello World!\n"
data = ustruct.pack("<bi%ds" % len(text), 85, len(text), text) # 85 is a sync char.
# Use "ustruct" to build data packets to send.
# "<" puts the data in the struct in little endian order.
# "b" puts a signed char in the data stream.
# "i" puts a signed integer in the data stream.
# "%ds" puts a string in the data stream. E.g. "13s" for "Hello World!\n" (13 chars).
# See https://docs.python.org/3/library/struct.html

# Zero pad data to a multiple of 4 bytes plus 4 bytes.
data += "\x00" * (4 + (len(data) % 4))

# READ ME!!!
#
# Please understand that when your OpenMV Cam is not the SPI master it may miss responding to
# sending data as a SPI slave no matter if you call "spi.send()" in an interrupt callback or in the
# main loop below. Because of this you must absolutely design your communications protocol such
# that if the slave device (the OpenMV Cam) doesn't call "spi.send()" in time that garbage data
# read from the SPI peripheral by the master (the Arduino) is tossed. To accomplish this we use a
# sync character of 85 (binary 01010101) which the Arduino will look for as the first byte read.
# If it doesn't see this then it aborts the SPI transaction and will try again. Second, in order to
# clear out the SPI peripheral state we always send a multiple of four bytes and an extra four zero
# bytes to make sure that the SPI peripheral doesn't hold any stale data which could be 85. Note
# that the OpenMV Cam will miss the "spi.send()" window randomly because it has to service
# interrupts. Interrupts will happen a lot more while connected to your PC.

# The hardware SPI bus for your OpenMV Cam is always SPI bus 2.
# polarity = 0 -> clock is idle low.
# phase = 0 -> sample data on rising clock edge, output data on falling clock edge.
spi = pyb.SPI(2, pyb.SPI.SLAVE, polarity=0, phase=0)
pin = pyb.Pin("P3", pyb.Pin.IN, pull=pyb.Pin.PULL_UP)
print("Waiting for Pynq...")

# Note that for sync up to work correctly the OpenMV Cam must be running this script before the
# Arduino starts to poll the OpenMV Cam for data. Otherwise the SPI byte framing gets messed up,
# and etc. So, keep the Arduino in reset until the OpenMV Cam is "Waiting for Arduino...".

#Original Code
#while(True):
    #while(pin.value()): pass
    #try:
        #spi.send(data, timeout=1000)
        ## If we failed to sync up the first time we'll sync up the next time.
        #print("Sent Data!") # Only reached on no error.
    #except OSError as err:
        #pass # Don't care about errors - so pass.
        ## Note that there are 3 possible errors. A timeout error, a general purpose error, or
        ## a busy error. The error codes are 116, 5, 16 respectively for "err.arg[0]".
    #while(not pin.value()): pass
send_data = [0,1,2,3,4,5,6,7]

while(True):
    while(pin.value()): pass
    try:
        received = spi.recv(8)

        # If we failed to sync up the first time we'll sync up the next time.
        #print("Sent Received! " + str(received[0]) + "\n") # Only reached on no error.
        #spi.send(send_data)
        #spi.send(69)
        print('received')
        for vals in received:
            print(vals)

    except OSError as err:
        pass # Don't care about errors - so pass.
        # Note that there are 3 possible errors. A time out error, a general purpose error, or
        # a busy error. The error codes are 116, 5, 16 respectively for "err.arg[0]".
    while(not pin.value()): pass

###################################################################################################
# Arduino Code
###################################################################################################
#
# #include <SPI.h>
# #define SS_PIN 10
# #define BAUD_RATE 19200
# #define CHAR_BUF 128
#
# void setup() {
#   pinMode(SS_PIN, OUTPUT);
#   Serial.begin(BAUD_RATE);
#   SPI.begin();
#   SPI.setBitOrder(MSBFIRST);
#   SPI.setClockDivider(SPI_CLOCK_DIV16);
#   SPI.setDataMode(SPI_MODE0);
#   delay(1000); // Give the OpenMV Cam time to bootup.
# }
#
# void loop() {
#   int32_t temp = 0;
#   char buff[CHAR_BUF] = {0};
#   digitalWrite(SS_PIN, LOW);
#   delay(1); // Give the OpenMV Cam some time to setup to send data.
#
#   if(SPI.transfer(1) == 85) { // saw sync char?
#     SPI.transfer(&temp, 4); // get length
#     int zero_legnth = 4 + ((temp + 1) % 4);
#     if (temp) {
#       SPI.transfer(&buff, min(temp, CHAR_BUF));
#       temp -= min(temp, CHAR_BUF);
#     }
#     while (temp--) SPI.transfer(0); // eat any remaining bytes
#     while (zero_legnth--) SPI.transfer(0); // eat zeros.
#   }
#
#   digitalWrite(SS_PIN, HIGH);
#   Serial.print(buff);
#   delay(1); // Don't loop to quickly.
# }