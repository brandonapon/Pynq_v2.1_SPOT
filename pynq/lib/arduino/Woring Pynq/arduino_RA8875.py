#   Copyright (c) 2016, Xilinx, Inc.
#   All rights reserved.
# 
#   Redistribution and use in source and binary forms, with or without 
#   modification, are permitted provided that the following conditions are met:
#
#   1.  Redistributions of source code must retain the above copyright notice, 
#       this list of conditions and the following disclaimer.
#
#   2.  Redistributions in binary form must reproduce the above copyright 
#       notice, this list of conditions and the following disclaimer in the 
#       documentation and/or other materials provided with the distribution.
#
#   3.  Neither the name of the copyright holder nor the names of its 
#       contributors may be used to endorse or promote products derived from 
#       this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
#   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
#   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
#   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
#   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
#   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#   OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
#   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
#   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
#   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


from math import ceil
import asyncio
import os
from PIL import Image
from numpy import array
from pynq import Xlnk
from ctypes import *
from . import Arduino
from . import MAILBOX_OFFSET
from . import MAILBOX_PY2IOP_CMD_OFFSET

ARDUINO_RA8875_PROGRAM = "arduino_RA8875.bin"
INIT = 0x1
DRAW = 0x2
DISPLAY = 0x3
GPIO = 0x4
PWM = 0x5
FILL = 0x6
SLOW_FILL = 0x7
DRAW_IMAGE = 0x8
FILL_COLOR = 0x9

class Arduino_RA8875(object):
    '''
        Testing mailbox/circular buffer
    '''
    def __init__(self, mb_info):
        """Return a new instance of an Arduino_mailboxTest object.
        
        Parameters
        ----------
        mb_info : dict
            A dictionary storing Microblaze information, such as the
            IP name and the reset name.

        """
        self.microblaze = Arduino(mb_info, ARDUINO_RA8875_PROGRAM)
        self.buf_manager = Xlnk()

    def init(self):
        self.microblaze.write_blocking_command(INIT)
        return self.microblaze.read_mailbox(0,1)

    def drawPixel(self, x, y, color):
        self.microblaze.write_mailbox(0, x)
        self.microblaze.write_mailbox(4, y)
        self.microblaze.write_mailbox(8, color)
        self.microblaze.write_blocking_command(DRAW)
        return self.microblaze.read_mailbox(0,1)

    def slowFill(self, color):
        self.microblaze.write_mailbox(0, color)
        self.microblaze.write_blocking_command(SLOW_FILL)

    def display(self):
        self.microblaze.write_blocking_command(DISPLAY)
        return self.microblaze.read_mailbox(0,1)

    def gpio(self):
        self.microblaze.write_blocking_command(GPIO)
        return self.microblaze.read_mailbox(0,1)

    def pwm(self):
        self.microblaze.write_blocking_command(PWM)
        return self.microblaze.read_mailbox(0,1)

    def fill(self):
        self.microblaze.write_blocking_command(FILL)
        return self.microblaze.read_mailbox(0,1)

    def fill_color(self, color):
        self.microblaze.write_mailbox(0, color)
        self.microblaze.write_blocking_command(FILL_COLOR)
        return self.microblaze.read_mailbox(0,1)

    def draw_yoga(self, x_start, y_start):
        x_start = 0
        y_start = 0
        file_size = 240*240*2
        buf0 = self.buf_manager.cma_alloc(file_size, data_type="uint8_t")
        buf1 = self.buf_manager.cma_get_buffer(buf0, file_size)
        phy_addr = self.buf_manager.cma_get_phy_addr(buf0)
        index = 0
        for pixel in self.yoga:
            buf1[index] = bytes([(pixel&0xFF00)>>8])
            buf1[index+1] = bytes([pixel&0x00FF])
            index += 2
        self.microblaze.write_mailbox(0, x_start)
        self.microblaze.write_mailbox(4, y_start)
        self.microblaze.write_mailbox(8, phy_addr)
        self.microblaze.write_mailbox(12, 240)
        self.microblaze.write_mailbox(16, 240)
        self.microblaze.write_blocking_command(DRAW_IMAGE)
        return self.microblaze.read_mailbox(0,5)
