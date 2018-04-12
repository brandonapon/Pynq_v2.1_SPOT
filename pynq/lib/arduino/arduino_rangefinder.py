from math import ceil
import asyncio
import os
from PIL import Image
from numpy import array
from ctypes import *
from . import Arduino
from . import MAILBOX_OFFSET
from . import MAILBOX_PY2IOP_CMD_OFFSET

ARDUINO_RANGEFINDER_PROGRAM = "arduino_rangefinder.bin"
TEST = 0x1
TEST_2 = 0x2

class Arduino_Rangefinder(object):
    '''
        Testing mailbox/circular buffer
    '''
    def __init__(self, mb_info):
        """Return a new instance of an Arduino_rangefinder object.
        
        Parameters
        ----------
        mb_info : dict
            A dictionary storing Microblaze information, such as the
            IP name and the reset name.

        """
        self.microblaze = Arduino(mb_info, ARDUINO_RANGEFINDER_PROGRAM)

    def test(self):
        self.microblaze.write_blocking_command(TEST)
        return self.microblaze.read_mailbox(0,1)

