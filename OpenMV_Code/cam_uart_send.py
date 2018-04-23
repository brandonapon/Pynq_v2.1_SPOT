# UART Control
#
# This example shows how to use the serial port on your OpenMV Cam. Attach pin
# P4 to the serial input of a serial LCD screen to see "Hello World!" printed
# on the serial LCD display.

import time, sensor, pyb
from pyb import UART

sensor.reset()                      # Reset and initialize the sensor.
sensor.set_pixformat(sensor.GRAYSCALE) # Set pixel format to RGB565 (or GRAYSCALE)
sensor.set_framesize(sensor.QQVGA2)   # Set frame size to QVGA (320x240)
# Always pass UART 3 for the UART number for your OpenMV Cam.
# The second argument is the UART baud rate. For a more advanced UART control
# example see the BLE-Shield driver.
uart = UART(3, 9600)
led = pyb.LED(3)

print("Started...")

send_out = "SPOT"
while(True):
    read_in = uart.read()
    print(read_in);
    #uart.write("xA")
    uart.write(send_out)
