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
import time
from PIL import Image
from numpy import array
from pynq import Xlnk
from ctypes import *
from . import Arduino
from . import MAILBOX_OFFSET
from . import MAILBOX_PY2IOP_CMD_OFFSET


ARDUINO_DISPLAYCAM_PROGRAM = "arduino_displaycam.bin"
#commands
INIT = 0x1
DRAW = 0x2
DISPLAY = 0x3
GPIO= 0x4
PWM = 0x5
FILL = 0x6
SLOW_FILL = 0x7
DRAW_IMAGE = 0x8
FILL_COLOR = 0x9
SIMPLE = 0xa
EIGHT = 0xb
RECTANGLE = 0xc
CIRCLE = 0xd
TRIANGLE = 0xe
ELLIPSE = 0xf
CURVE = 0x10
DRAWLINE = 0x11
FONT = 0x12
TEXTMODE = 0x13
GRAPHICSMODE = 0x14
LAYER = 0x15
LAYERTRANS = 0x16
CAMERA1 = 0x17
CAMERA2 = 0x18
LETTER = 0x19
LETTER_SETUP = 0x20
FONT_SIZE = 0x21
TEXT_TRANSPARENT = 0x22
BACKGROUND_COLOR = 0x23
TEXT_CURSOR = 0x24
TEXT_WRITE = 0x25
LAYER_MODE = 0x26
LAYER_EFFECT = 0x27
CLEAR_WINDOW = 0x28
CLEAR_MEM = 0x29

delay = 0.05
longer = 0.1

class Arduino_Displaycam(object):
    '''
        self.ing mailbox/circular buffer
    '''
    def __init__(self, mb_info):
        """Return a new instance of an ARDUINO_DISPLAYCAM object.
        
        Parameters
        ----------
        mb_info : dict
            A dictionary storing Microblaze information, such as the
            IP name and the reset name.

        """
        self.microblaze = Arduino(mb_info, ARDUINO_DISPLAYCAM_PROGRAM)
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

    def drawCompass(self):
        self.microblaze.write_mailbox(0, 56)
        self.microblaze.write_mailbox(4, 240)
        self.microblaze.write_mailbox(8, 1)
        self.microblaze.write_mailbox(12, 0x01c0)
        self.microblaze.write_mailbox(16, 1) # filled
        self.microblaze.write_blocking_command(CIRCLE)
        
    def drawRadar(self):
        self.microblaze.write_mailbox(0, 266)
        self.microblaze.write_mailbox(4, 240)
        self.microblaze.write_mailbox(8, 10)
        self.microblaze.write_mailbox(12, 0x01c0)
        self.microblaze.write_mailbox(16, 1) # filled
        self.microblaze.write_blocking_command(CIRCLE)

        time.sleep(delay)

        self.microblaze.write_mailbox(0, 266)
        self.microblaze.write_mailbox(4, 240)
        self.microblaze.write_mailbox(8, 70)
        self.microblaze.write_mailbox(12, 0x01c0)
        self.microblaze.write_mailbox(16, 0) #unfilled
        self.microblaze.write_blocking_command(CIRCLE)

        time.sleep(delay)

        self.microblaze.write_mailbox(0, 266)
        self.microblaze.write_mailbox(4, 240)
        self.microblaze.write_mailbox(8, 140)
        self.microblaze.write_mailbox(12, 0x01c0)
        self.microblaze.write_mailbox(16, 0) #unfilled
        self.microblaze.write_blocking_command(CIRCLE)

        time.sleep(longer)

        self.microblaze.write_mailbox(0, 266)
        self.microblaze.write_mailbox(4, 240)
        self.microblaze.write_mailbox(8, 210)
        self.microblaze.write_mailbox(12, 0x01c0)
        self.microblaze.write_mailbox(16, 0) #unfilled
        self.microblaze.write_blocking_command(CIRCLE)

        time.sleep(longer+delay)


    def drawUpperButton(self):
        self.microblaze.write_mailbox(0, 600)
        self.microblaze.write_mailbox(4, 50)
        self.microblaze.write_mailbox(8, 150)
        self.microblaze.write_mailbox(12, 100)
        self.microblaze.write_mailbox(16, 0x001f)
        self.microblaze.write_mailbox(20, 0) #unfilled
        self.microblaze.write_blocking_command(RECTANGLE)

        time.sleep(delay)

        self.microblaze.write_mailbox(0, 750)
        self.microblaze.write_mailbox(4, 50)
        self.microblaze.write_mailbox(8, 750)
        self.microblaze.write_mailbox(12, 150)
        self.microblaze.write_mailbox(16, 800)
        self.microblaze.write_mailbox(20, 100)
        self.microblaze.write_mailbox(24, 0x001f)
        self.microblaze.write_mailbox(28, 0) #unfilled
        self.microblaze.write_blocking_command(TRIANGLE)

        time.sleep(delay)

        self.microblaze.write_mailbox(0, 750)
        self.microblaze.write_mailbox(4, 51)
        self.microblaze.write_mailbox(8, 750)
        self.microblaze.write_mailbox(12, 149) 
        self.microblaze.write_mailbox(16, 0x0000)
        self.microblaze.write_blocking_command(DRAWLINE)

        time.sleep(delay)


    def drawLowerButton(self):
        self.microblaze.write_mailbox(0, 600) # x coordinate of top left corner
        self.microblaze.write_mailbox(4, 330) # y coordinate of top left corner
        self.microblaze.write_mailbox(8, 150) # width
        self.microblaze.write_mailbox(12, 100) # height
        self.microblaze.write_mailbox(16, 0x001f) # color
        self.microblaze.write_mailbox(20, 0) #unfilled (1 is filled)
        self.microblaze.write_blocking_command(RECTANGLE)

        time.sleep(delay)

        self.microblaze.write_mailbox(0, 750)
        self.microblaze.write_mailbox(4, 330)
        self.microblaze.write_mailbox(8, 750)
        self.microblaze.write_mailbox(12, 430)
        self.microblaze.write_mailbox(16, 800)
        self.microblaze.write_mailbox(20, 380)
        self.microblaze.write_mailbox(24, 0x001f)
        self.microblaze.write_mailbox(28, 0) #unfilled
        self.microblaze.write_blocking_command(TRIANGLE)

        time.sleep(delay)

        self.microblaze.write_mailbox(0, 750)
        self.microblaze.write_mailbox(4, 331)
        self.microblaze.write_mailbox(8, 750)
        self.microblaze.write_mailbox(12, 429) 
        self.microblaze.write_mailbox(16, 0x0000)
        self.microblaze.write_blocking_command(DRAWLINE)

        time.sleep(delay)


    def drawMarkArrow(self):
        self.microblaze.write_mailbox(0, 100) # x coordinate top left
        self.microblaze.write_mailbox(4, 200) # y coordinate top left
        self.microblaze.write_mailbox(8, 300) # width
        self.microblaze.write_mailbox(12, 80) # length
        self.microblaze.write_mailbox(16, 0x07FF)
        self.microblaze.write_mailbox(20, 1) # filled
        self.microblaze.write_blocking_command(RECTANGLE)

        time.sleep(delay)

        self.microblaze.write_mailbox(0, 400) # point 1 x
        self.microblaze.write_mailbox(4, 330) # point 1 y
        self.microblaze.write_mailbox(8, 400) # point 2 x
        self.microblaze.write_mailbox(12, 150) # point 2 y
        self.microblaze.write_mailbox(16, 550) # point 3 x
        self.microblaze.write_mailbox(20, 240) # point 3 y
        self.microblaze.write_mailbox(24, 0x7FF)
        self.microblaze.write_mailbox(28, 1)
        self.microblaze.write_blocking_command(TRIANGLE)

        time.sleep(delay)


    def drawLoadPicScreen(self):
        self.microblaze.write_mailbox(0, 0)
        self.microblaze.write_mailbox(4, 0)
        self.microblaze.write_mailbox(8, 320)
        self.microblaze.write_mailbox(12, 240)
        self.microblaze.write_mailbox(16, 0x001f)
        self.microblaze.write_mailbox(20, 0) # unfilled
        self.microblaze.write_blocking_command(RECTANGLE)

        time.sleep(delay)

    # Full UI's with graphics and text
    def drawMainScreen(self):
        self.drawRadar()
        self.drawUpperButton()
        self.drawLowerButton()
        self.write_COMPASS()
        self.write_VIEW(650, 86)
        self.write_MARK(655, 365)

    # Main->View
    def drawViewScreen(self):
        self.drawRadar()
        self.drawUpperButton()
        self.drawLowerButton()
        self.write_COMPASS()
        self.write_CANCEL(640, 86)
        self.write_SELECT(640, 365)
        
    # Main->View->Selected
    def drawSelectView(self):
        self.drawLoadPicScreen()
        self.drawUpperButton()
        self.drawLowerButton()
        self.write_TAG(100, 290)
        self.write_DISTANCE(100, 340)
        self.write_CB(100, 390)
        self.write_HOME(655, 86)
        self.write_GOBACK(635, 365)

    # Main->Mark
    def drawMarkScreen(self):
        self.drawMarkArrow()
        self.write_POINTTOWARDS(200, 85)
        self.write_OBJECT(250, 375)
        self.drawUpperButton()
        self.drawLowerButton()
        self.write_CANCEL(640, 86)
        self.write_MARK(655, 365)
        
    # Main->Mark->Mark
    def drawAfterMark(self):
        self.drawLoadPicScreen()
        self.write_TAG(100, 310)
        self.write_DISTANCE(100, 360)
        self.write_CB(100, 410)
        self.drawUpperButton()
        self.drawLowerButton()
        self.write_CANCEL(640, 86)
        self.write_CONFIRM(635, 365)

    # Alert (Interest)
    def drawAlertInterest(self):
        self.write_ALERT(240, 85)
        self.write_POINEAR(100, 120)
        self.write_DISTANCE(150, 250)
        self.write_DIRECTION(150, 300)
        self.write_TAG(150, 350)
        self.drawUpperButton()
        self.drawLowerButton()
        self.write_DISMISS(635, 86)
        self.write_VIEW(650, 365)

    # Alert (Danger)
    def drawAlertDanger(self):
        self.write_ALERT(240, 85)
        self.write_PODNEAR(175, 120)
        self.write_DISTANCE(150, 250)
        self.write_DIRECTION(150, 300)
        self.write_TAG(150, 350)
        self.drawUpperButton()
        self.drawLowerButton()
        self.write_DISMISS(635, 86)
        self.write_VIEW(650, 365)

    #text functions
    def textMode(self):
        self.microblaze.write_blocking_command(TEXTMODE)

    def graphicsMode(self):
        self.microblaze.write_blocking_command(GRAPHICSMODE)
        
    def textCursor(self,x,y):
        self.microblaze.write_mailbox(0, x)
        self.microblaze.write_mailbox(4, y)
        self.microblaze.write_blocking_command(TEXT_CURSOR)

    def fontSize(self, size):
        #note: sizes range from 0-3.
        self.microblaze.write_mailbox(0, size)
        self.microblaze.write_blocking_command(FONT_SIZE)
        
    def backgroundColor(self, foreground, background):
        self.microblaze.write_mailbox(0, foreground)
        self.microblaze.write_mailbox(4, background)
        self.microblaze.write_blocking_command(BACKGROUND_COLOR)

    def writeText(self, value):
        #values correspond to static text
        #list cases here
        self.microblaze.write_mailbox(0, value)
        self.microblaze.write_blocking_command(TEXT_WRITE)

    def write_ALERT(self, x, y):
        self.textMode()
        self.fontSize(2)
        self.transparentBackground(0xf700)
        self.textCursor(x, y)
        self.writeText(0)
        self.graphicsMode()

    def write_CANCEL(self, x, y):
        self.textMode()
        self.fontSize(2)
        self.transparentBackground(0xf700)
        self.textCursor(x, y)
        self.writeText(1)
        self.graphicsMode()

    def write_MARK(self, x, y):
        self.textMode()
        self.fontSize(2)
        self.transparentBackground(0xf700)
        self.textCursor(x, y)
        self.writeText(2)
        self.graphicsMode()

    def write_VIEW(self, x, y):
        self.textMode()
        self.fontSize(2)
        self.transparentBackground(0xf700)
        self.textCursor(x, y)
        self.writeText(3)
        self.graphicsMode()

    def write_SELECT(self, x, y):
        self.textMode()
        self.fontSize(2)
        self.transparentBackground(0xf700)
        self.textCursor(x, y)
        self.writeText(4)
        self.graphicsMode()

    def write_COMPASS(self):
        self.textMode()
        self.fontSize(2)
        self.transparentBackground(0xf700)
        self.textCursor(256, 0)
        self.writeText(7)
        self.textCursor(486, 225)
        self.writeText(5)
        self.textCursor(256, 445)
        self.writeText(8)
        self.textCursor(30, 225)
        self.writeText(6)
        self.graphicsMode()

    def write_HOME(self, x, y):
        self.textMode()
        self.fontSize(2)
        self.transparentBackground(0xf700)
        self.textCursor(x, y)
        self.writeText(9)
        self.graphicsMode()

    def write_GOBACK(self, x, y):
        self.textMode()
        self.fontSize(2)
        self.transparentBackground(0xf700)
        self.textCursor(x, y)
        self.writeText(10)
        self.graphicsMode()

    def write_TAG(self, x, y):
        self.textMode()
        self.fontSize(2)
        self.transparentBackground(0xf700)
        self.textCursor(x, y)
        self.writeText(11)
        self.graphicsMode()

    def write_CB(self, x, y):
        self.textMode()
        self.fontSize(2)
        self.transparentBackground(0xf700)
        self.textCursor(x, y)
        self.writeText(12)
        self.graphicsMode()

    def write_DISTANCE(self, x, y):
        self.textMode()
        self.fontSize(2)
        self.transparentBackground(0xf700)
        self.textCursor(x, y)
        self.writeText(13)
        self.graphicsMode()

    def write_OBJECT(self, x, y):
        self.textMode()
        self.fontSize(2)
        self.transparentBackground(0xf700)
        self.textCursor(x, y)
        self.writeText(14)
        self.graphicsMode()

    def write_CONFIRM(self, x, y):
        self.textMode()
        self.fontSize(2)
        self.transparentBackground(0xf700)
        self.textCursor(x, y)
        self.writeText(15)
        self.graphicsMode()

    def write_DIRECTION(self, x, y):
        self.textMode()
        self.fontSize(2)
        self.transparentBackground(0xf700)
        self.textCursor(x, y)
        self.writeText(16)
        self.graphicsMode()

    def write_POINTTOWARDS(self, x, y):
        self.textMode()
        self.fontSize(2)
        self.transparentBackground(0xf700)
        self.textCursor(x, y)
        self.writeText(17)
        self.graphicsMode()

    def write_POINEAR(self, x, y):
        self.textMode()
        self.fontSize(3)
        self.transparentBackground(0xf700)
        self.textCursor(x, y)
        self.writeText(18)
        self.graphicsMode()

    def write_PODNEAR(self, x, y):
        self.textMode()
        self.fontSize(3)
        self.transparentBackground(0xf700)
        self.textCursor(x, y)
        self.writeText(19)
        self.graphicsMode()

    def write_DISMISS(self, x, y):
        self.textMode()
        self.fontSize(2)
        self.transparentBackground(0xf700)
        self.textCursor(x, y)
        self.writeText(20)
        self.graphicsMode()

    def transparentBackground(self, font_color):
        self.microblaze.write_mailbox(0, font_color)
        self.microblaze.write_blocking_command(TEXT_TRANSPARENT)

    #graphics layers
    def layerMode(self, toggle):
        #on = 1
        self.microblaze.write_mailbox(0, toggle)
        self.microblaze.write_blocking_command(LAYER_MODE)

    def layer(self, num):
        #0 = foreground
        #1 = background
        self.microblaze.write_mailbox(0, num)
        self.microblaze.write_blocking_command(LAYER)
        
    def transparentLayer(self, foreground, background):
        #0-8 levels
        self.microblaze.write_mailbox(0, foreground)
        self.microblaze.write_mailbox(4, background)
        self.microblaze.write_blocking_command(LAYERTRANS)
        
    def layerEffect(self, val):
        #define LAYER1                  0 foreground
        #define LAYER2                  1 background
        #define TRANSPARENT             2
        #define LIGHTEN                 3
        #define OR                      4
        #define AND                     5
        #define FLOATING                6
        self.microblaze.write_mailbox(0,val)
        self.microblaze.write_blocking_command(LAYER_EFFECT)
        
    def clearMem(self, val):
        self.microblaze.write_mailbox(0, val)
        self.microblaze.write_blocking_command(CLEAR_MEM)
        
    def clearWindow(self, val):
        self.microblaze.write_mailbox(0, val)
        self.microblaze.write_blocking_command(CLEAR_WINDOW)

    def snapPic(self, x_start, y_start):
        stream_size = 153600
        buf0 = self.buf_manager.cma_alloc(stream_size, data_type="uint8_t")
        buf1 = self.buf_manager.cma_get_buffer(buf0, stream_size)
        phy_addr = self.buf_manager.cma_get_phy_addr(buf0)
        self.microblaze.write_mailbox(0, phy_addr)
        self.microblaze.write_blocking_command(CAMERA1) 
        self.microblaze.write_mailbox(0, x_start)
        self.microblaze.write_mailbox(4, y_start)
        self.microblaze.write_mailbox(8, phy_addr)
        self.microblaze.write_blocking_command(CAMERA2)

    def drawRect(self, x_start, y_start, x_size, y_size, color, fill):
        self.microblaze.write_mailbox(0, x_start)
        self.microblaze.write_mailbox(4, y_start)
        self.microblaze.write_mailbox(8, x_size)
        self.microblaze.write_mailbox(12, y_size)
        self.microblaze.write_mailbox(16, color)
        self.microblaze.write_mailbox(20, fill)
        self.microblaze.write_blocking_command(RECTANGLE)
        time.sleep(delay)

    def drawTriangle(self, x0, y0, x1, y1, x2, y2, color, fill):
        self.microblaze.write_mailbox(0, x0)
        self.microblaze.write_mailbox(4, y0)
        self.microblaze.write_mailbox(8, x1)
        self.microblaze.write_mailbox(12, y1)
        self.microblaze.write_mailbox(16, x2)
        self.microblaze.write_mailbox(20, y2)
        self.microblaze.write_mailbox(24, color)
        self.microblaze.write_mailbox(28, fill) #unfilled
        self.microblaze.write_blocking_command(TRIANGLE)
        time.sleep(delay)

    def drawLine(self, x0, y0, x1, y1, color):
        self.microblaze.write_mailbox(0, x0)
        self.microblaze.write_mailbox(4, y0)
        self.microblaze.write_mailbox(8, x1)
        self.microblaze.write_mailbox(12, y1) 
        self.microblaze.write_mailbox(16, color)
        self.microblaze.write_blocking_command(DRAWLINE)
        time.sleep(delay)

    def drawCircle(self, x, y, radius, color, fill):
        self.microblaze.write_mailbox(0, x)
        self.microblaze.write_mailbox(4, y)
        self.microblaze.write_mailbox(8, radius)
        self.microblaze.write_mailbox(12, color)
        self.microblaze.write_mailbox(16, fill) 
        self.microblaze.write_blocking_command(CIRCLE)
        time.sleep(delay)

    def draw1X(self, x, y, size, color):
        self.drawLine(x-size, y-size, x+size, y+size, color)

        self.drawLine(x+size, y-size, x-size, y+size, color)

    def draw2X(self, x, y, size, color):
        self.drawLine(x-size, y-size, x+size, y+size, color)
        self.drawLine(x-size+1, y-size, x+size+1, y+size, color)
        self.drawLine(x-size-1, y-size, x+size-1, y+size, color)

        self.drawLine(x+size, y-size, x-size, y+size, color)
        self.drawLine(x+size, y-size+1, x-size, y+size+1, color)
        self.drawLine(x+size, y-size-1, x-size, y+size-1, color)

    def drawTrianglePoint(self, x, y, size, color):
        self.drawTriangle(x-size, y+size, x+size, y+size, x, y-size, color, 1)
        # self.drawTriangle(x-size+1, y+size-1, x+size-1, y+size-1, x, y-size+1, color, 1)        

    def drawPoint(self, x, y, size, color):
        self.drawCircle(x, y, size, color, 1)