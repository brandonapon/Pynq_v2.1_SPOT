from math import ceil
import asyncio
import os
from PIL import Image
from numpy import array
from ctypes import *
from . import Arduino
from . import MAILBOX_OFFSET
from . import MAILBOX_PY2IOP_CMD_OFFSET

ARDUINO_CAM_UART_PROGRAM = "arduino_cam_uart.bin"
WRITE = 0x2
READ = 0x

class Arduino_CAM_UART(object):
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
        self.microblaze = Arduino(mb_info, ARDUINO_CAM_UART_PROGRAM)

    def test(self):
        self.microblaze.write_blocking_command(WRITE)
        return self.microblaze.read_mailbox(0,1)

    def test_2(self):
        self.microblaze.write_blocking_command(READ)
        return self.microblaze.read_mailbox(0,8)
