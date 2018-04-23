# UART Control
#
# This example shows how to use the serial port on your OpenMV Cam. Attach pin
# P4 to the serial input of a serial LCD screen to see "Hello World!" printed
# on the serial LCD display.

import time, sensor, pyb
from pyb import UART

sensor.reset()                      # Reset and initialize the sensor.
sensor.set_pixformat(sensor.RGB565) # Set pixel format to RGB565
sensor.set_framesize(sensor.VGA)   # Set frame size to VGA 640x480

sensor.skip_frames() # By default, skips frames for 300 milliseconds
# Always pass UART 3 for the UART number for your OpenMV Cam.
# The second argument is the UART baud rate. For a more advanced UART control
# example see the BLE-Shield driver.
uart = UART(3, 9600)
# led = pyb.LED(3)

print("Started...")
clock = time.clock()
RED_LED_PIN = 1
BLUE_LED_PIN = 3

# send_out = "SPOT"
while(True):
  clock.tick()
  read_in = uart.read()
  print(read_in);
  if read_in == 0x1:
      pyb.LED(BLUE_LED_PIN).on()
      img = sensor.snapshot()
      pyb.LED(BLUE_LED_PIN).off()
  # elif read_in == 0x2:
      # do something
  # elif read_in == 0x3:
      # do something
  else:
      sensor.sleep()
  #uart.write("xA")
  print("FPS %f" % clock.fps())
  uart.write(img)
