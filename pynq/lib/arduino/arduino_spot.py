from math import ceil
from statistics import median
import asyncio
import os
import time
from PIL import Image
from numpy import array
from pynq import Xlnk
from ctypes import *
from . import Arduino
from . import ARDUINO_GROVE_I2C
from . import MAILBOX_OFFSET
from . import MAILBOX_PY2IOP_CMD_OFFSET


__author__ = "Neil O'Bryan & Brandon Pon"
__copyright__ = "Copyright 2018, SPOT"

ARDUINO_SPOT = "arduino_spot.bin"
"""multiple use command"""
CONFIG_IOP_SWITCH = 0x1
"""Haptic specific commands 0x1xxx"""
START_WAVEFORM = 0x1000
STOP_WAVEFORM = 0x1001
READ_IS_PLAYING = 0x1002
"""IMU specific commands 0x2xxx"""
GET_ACCL_DATA = 0x2000
GET_GYRO_DATA = 0x2001
GET_COMPASS_DATA = 0x2002
GET_EULER_VECTOR = 0x2003
GET_TEMPERATURE = 0x2004
RESET = 0x2005
"""GPS specific commands 0x3xxx"""
GPS_READ = 0x3000
"""Ultrasonic specific commands 0x4xxx"""
RANGE_ON = 0x4000
RANGE_OFF = 0x4001
RANGE_POLL = 0x4002
RANGE_DEVICE = 0x4003
"""Camera specific commands 0x5xxx"""
CAMERA = 0x5000
"""Display specific commands 0x6xxx"""
INIT = 0x6000
DRAW = 0x6001
DISPLAY = 0x6002
GPIO = 0x6003
PWM = 0x6004
FILL = 0x6005
FILL_COLOR = 0x6006
EIGHT = 0x6007
RECTANGLE = 0x6008
CIRCLE = 0x6009
TRIANGLE = 0x600a
ELLIPSE = 0x600b
CURVE = 0x600c
DRAWLINE = 0x600d
FONT = 0x600e
TEXTMODE = 0x600f
GRAPHICSMODE = 0x6010
LAYER = 0x6011
LAYERTRANS = 0x6012
FONT_SIZE = 0x6013
TEXT_TRANSPARENT = 0x6014
BACKGROUND_COLOR = 0x6015
TEXT_CURSOR = 0x6016
TEXT_WRITE = 0x6017
LAYER_MODE = 0x6018
LAYER_EFFECT = 0x6019
CLEAR_WINDOW = 0x601a
CLEAR_MEM = 0x601b
DRAW_IMAGE = 0x601c

"""Transceiver specific commands 0x7xxx"""
READ_FROM       =   0x7001
WRITE_TO        =   0x7002
HAS_MESSAGES    =   0x7003
READ_REG        =   0x7005
DEVICE          =   0x7006
START_TX        =   0x7009
WRITE_CAMERA    =   0x7010

"""MISC commands"""
IMAGE_ADDRESS 	= 0x8000

"""Display/Camera Values"""
delay = 0.05
longer = 0.1

"""TX stuff"""
MAX_LEN			=	29


def _reg2float(reg):
	"""Converts 32-bit register value to floats in Python.

	Parameters
	----------
	reg: int
		A 32-bit register value read from the mailbox.

	Returns
	-------
	float
		A float number translated from the register value.

	"""
	if reg == 0:
		return 0.0
	sign = (reg & 0x80000000) >> 31 & 0x01
	exp = ((reg & 0x7f800000) >> 23) - 127
	if exp == 0:
		man = (reg & 0x007fffff) / pow(2, 23)
	else:
		man = 1 + (reg & 0x007fffff) / pow(2, 23)
	result = pow(2, exp) * man * ((sign * -2) + 1)
	return float("{0:.2f}".format(result))


def _reg2int(reg):
	"""Converts 32-bit register value to signed integer in Python.

	Parameters
	----------
	reg: int
		A 32-bit register value read from the mailbox.

	Returns
	-------
	int
		A signed integer translated from the register value.

	"""
	result = -(reg >> 31 & 0x1) * (1 << 31)
	for i in range(31):
		result += (reg >> i & 0x1) * (1 << i)
	return result

class Arduino_SPOT(object):

	def __init__(self, mb_info, gr_pin):

		self.microblaze = Arduino(mb_info, ARDUINO_SPOT)
		self.reset()
		self.buf_manager = Xlnk()
		self.buf565 = []
		self.buf888 = []
		self.drawAddr = []

	def reset(self):
		"""Reset all the sensors on the adafruit IMU.

		Returns
		-------
		None

		"""
		self.microblaze.write_blocking_command(RESET)

	def get_accl(self):
		"""Get the data from the accelerometer.

		Returns
		-------
		list
			A list of the acceleration data along X-axis, Y-axis, and Z-axis.

		"""
		self.microblaze.write_blocking_command(GET_ACCL_DATA)
		data = self.microblaze.read_mailbox(0, 3)
		[ax, ay, az] = [_reg2int(i) for i in data]
		return [float("{0:.2f}".format(ax / 100)),
				float("{0:.2f}".format(ay / 100)),
				float("{0:.2f}".format(az / 100))]

	def get_gyro(self):
		"""Get the data from the gyroscope.

		Returns
		-------
		list
			A list of the gyro data along X-axis, Y-axis, and Z-axis.

		"""
		self.microblaze.write_blocking_command(GET_GYRO_DATA)
		data = self.microblaze.read_mailbox(0, 3)
		[gx, gy, gz] = [_reg2int(i) for i in data]
		return [float("{0:.2f}".format(gx / 16)),
				float("{0:.2f}".format(gy / 16)),
				float("{0:.2f}".format(gz / 16))]

	def get_compass(self):
		"""Get the data from the magnetometer.

		Returns
		-------
		list
			A list of the compass data along X-axis, Y-axis, and Z-axis.

		"""
		self.microblaze.write_blocking_command(GET_COMPASS_DATA)
		data = self.microblaze.read_mailbox(0, 3)
		[mx, my, mz] = [_reg2int(i) for i in data]
		return [float("{0:.2f}".format(mx / 16)),
				float("{0:.2f}".format(my / 16)),
				float("{0:.2f}".format(mz / 16))]

	def get_euler(self):
		self.microblaze.write_blocking_command(GET_EULER_VECTOR)
		data = self.microblaze.read_mailbox(0, 3)
		[ex, ey, ez] = [_reg2int(i) for i in data]
		return [float("{0:.2f}".format(ex / 16)),
				float("{0:.2f}".format(ey / 16)),
				float("{0:.2f}".format(ez / 16))]

	def get_heading(self):
		"""Get the value of the heading.

		Returns
		-------
		float
			The angle deviated from the X-axis, toward the positive Y-axis.

		"""
		[mx, my, _] = self.get_compass()
		heading = 180 * math.atan2(my, mx) / math.pi
		if heading < 0:
			heading += 360
		return float("{0:.2f}".format(heading))

	def get_tilt_heading(self):
		"""Get the value of the tilt heading.

		Returns
		-------
		float
			The tilt heading value.

		"""
		[ax, ay, _] = self.get_accl()
		[mx, my, mz] = self.get_compass()

		try:
			pitch = math.asin(-ax)
			roll = math.asin(ay / math.cos(pitch))
		except ZeroDivisionError:
			raise RuntimeError("Value out of range or device not connected.")

		xh = mx * math.cos(pitch) + mz * math.sin(pitch)
		yh = mx * math.sin(roll) * math.sin(pitch) + \
			my * math.cos(roll) - mz * math.sin(roll) * math.cos(pitch)
		_ = -mx * math.cos(roll) * math.sin(pitch) + \
			my * math.sin(roll) + mz * math.cos(roll) * math.cos(pitch)
		tilt_heading = 180 * math.atan2(yh, xh) / math.pi
		if yh < 0:
			tilt_heading += 360
		return float("{0:.2f}".format(tilt_heading))

	def get_temperature(self):
		"""Get the current temperature in degree C.

		Returns
		-------
		float
			The temperature value.

		"""
		self.microblaze.write_blocking_command(GET_TEMPERATURE)
		value = self.microblaze.read_mailbox(0)

		"""[temp, test] = [_reg2int(i) for i in value]

		return temp
		return test
		"""
		return value
		return _reg2int(value)
		return _reg2float(value)

	def readFromGPS(self):
		dataBytes = []
		stringBuilder = ""
		counter = 0
		self.microblaze.write_blocking_command(GPS_READ); # Returns 102 values

		x = 0;
		while(1):
			val = self.microblaze.read_mailbox(4 * x)
			if(val < 256 and val != ''):
				stringValue = chr(val)
				if(stringValue == '\n'):
					newVal = stringBuilder.split(",")
					if(newVal[0] == '$GNGLL' or newVal[0] == 'GNGLL'):
						stringBuilder = ""
						return (self.getLatitude(newVal), self.getLongitude(newVal))
					stringBuilder = ""
				else:
					stringBuilder += stringValue
			x = x + 1;
			if(x == 100):
				break

	def getLatitude(self, dataString):
		if(dataString[2] != ''):
			if(dataString[2] == 'N'):
				return self.convertToDD(str(dataString[1]))
			else:
				return self.convertToDD(str(dataString[1])) * -1
		else:
			return None

	def getLongitude(self, dataString):
		if(dataString[4] != ''):
			if(dataString[4] == 'E'):
				return self.convertToDD(str(dataString[3]))
			else:
				return self.convertToDD(str(dataString[3])) * -1
		else:
			return None

	def convertToDD(self, value):
		indexofPeriod = int(value.find('.'))
		if(indexofPeriod == -1):
			return None
		else:
			degree = float(value[0:indexofPeriod - 2])
			minutes = float(value[indexofPeriod - 2:])
			return degree + (minutes / 60)

	def play(self, effect):
		"""Play a vibration effect on the Grove Haptic Motor peripheral.

		Valid effect identifiers are in the range [1, 127].

		Parameters
		----------
		effect : int
			An integer that specifies the effect.

		Returns
		-------
		None

		"""
		if (effect < 1) or (effect > 127):
			raise ValueError("Valid effect identifiers are within 1 and 127.")
		self.microblaze.write_mailbox(0, [effect, 0])
		self.microblaze.write_blocking_command(START_WAVEFORM)

	def play_sequence(self, sequence):
		"""Play a sequence of effects possibly separated by pauses.

		At most 8 effects or pauses can be specified at a time.
		Pauses are defined using negative integer values in the
		range [-1, -127] that correspond to a pause length in the
		range [10, 1270] ms

		Valid effect identifiers are in the range [1, 127]

		As an example, in the following sequence example: [4,-20,5]
		effect 4 is played and after a pause of 200 ms effect 5 is played

		Parameters
		----------
		sequence : list
			At most 8 values specifying effects and pauses.

		Returns
		-------
		None

		"""
		length = len(sequence)
		if length < 1:
			raise ValueError("The sequence must contain at least one value.")
		if length > 8:
			raise ValueError("The sequence cannot contain more than 8 values.")
		for i in range(length):
			if sequence[i] < 0:
				if sequence[i] < -127:
					raise ValueError("Pause value must be smaller than -127")
				sequence[i] = -sequence[i] + 128
			else:
				if (sequence[i] < 1) or (sequence[i] > 127):
					raise ValueError("Valid effect identifiers are within " +
									 "1 and 127.")
		sequence += [0] * (8 - length)

		self.microblaze.write_mailbox(0, sequence)
		self.microblaze.write_blocking_command(START_WAVEFORM)

	def stop(self):
		"""Stop an effect or a sequence on the motor peripheral.

		Returns
		-------
		None

		"""
		self.microblaze.write_blocking_command(STOP_WAVEFORM)

	def is_playing(self):
		"""Check if a vibration effect is running on the motor.

		Returns
		-------
		bool
			True if a vibration effect is playing, false otherwise

		"""
		self.microblaze.write_blocking_command(READ_IS_PLAYING)
		flag = self.microblaze.read_mailbox(0)
		return flag == 1

	def range_poll(self):
		self.microblaze.write_blocking_command(RANGE_POLL)
		values = self.microblaze.read_mailbox(0,5)
		return values

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
		color = 0x01c0
		self.microblaze.write_mailbox(0, 600)
		self.microblaze.write_mailbox(4, 50)
		self.microblaze.write_mailbox(8, 150)
		self.microblaze.write_mailbox(12, 100)
		self.microblaze.write_mailbox(16, color)
		self.microblaze.write_mailbox(20, 0) #unfilled
		self.microblaze.write_blocking_command(RECTANGLE)

		time.sleep(delay)

		self.microblaze.write_mailbox(0, 750)
		self.microblaze.write_mailbox(4, 50)
		self.microblaze.write_mailbox(8, 750)
		self.microblaze.write_mailbox(12, 150)
		self.microblaze.write_mailbox(16, 800)
		self.microblaze.write_mailbox(20, 100)
		self.microblaze.write_mailbox(24, color)
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
		color = 0x01c0
		self.microblaze.write_mailbox(0, 600) # x coordinate of top left corner
		self.microblaze.write_mailbox(4, 330) # y coordinate of top left corner
		self.microblaze.write_mailbox(8, 150) # width
		self.microblaze.write_mailbox(12, 100) # height
		self.microblaze.write_mailbox(16, color) # color
		self.microblaze.write_mailbox(20, 0) #unfilled (1 is filled)
		self.microblaze.write_blocking_command(RECTANGLE)

		time.sleep(delay)

		self.microblaze.write_mailbox(0, 750)
		self.microblaze.write_mailbox(4, 330)
		self.microblaze.write_mailbox(8, 750)
		self.microblaze.write_mailbox(12, 430)
		self.microblaze.write_mailbox(16, 800)
		self.microblaze.write_mailbox(20, 380)
		self.microblaze.write_mailbox(24, color)
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
		color = 0x01c0
		self.microblaze.write_mailbox(0, 100) # x coordinate top left
		self.microblaze.write_mailbox(4, 200) # y coordinate top left
		self.microblaze.write_mailbox(8, 300) # width
		self.microblaze.write_mailbox(12, 80) # length
		self.microblaze.write_mailbox(16, color)
		self.microblaze.write_mailbox(20, 1) # filled
		self.microblaze.write_blocking_command(RECTANGLE)

		time.sleep(delay)

		self.microblaze.write_mailbox(0, 400) # point 1 x
		self.microblaze.write_mailbox(4, 330) # point 1 y
		self.microblaze.write_mailbox(8, 400) # point 2 x
		self.microblaze.write_mailbox(12, 150) # point 2 y
		self.microblaze.write_mailbox(16, 550) # point 3 x
		self.microblaze.write_mailbox(20, 240) # point 3 y
		self.microblaze.write_mailbox(24, color)
		self.microblaze.write_mailbox(28, 1)
		self.microblaze.write_blocking_command(TRIANGLE)

		time.sleep(delay)


	def drawLoadPicScreen(self):
		self.microblaze.write_mailbox(0, 100)
		self.microblaze.write_mailbox(4, 0)
		self.microblaze.write_mailbox(8, 320)
		self.microblaze.write_mailbox(12, 240)
		self.microblaze.write_mailbox(16, 0x001f)
		self.microblaze.write_mailbox(20, 0) # unfilled
		self.microblaze.write_blocking_command(RECTANGLE)
		time.sleep(delay)
		self.write_CUSTOM('LOADING...', 180, 100, 0xffe0)
		time.sleep(delay)

	# Full UI's with graphics and text
	def drawMainScreen(self):
		# self.layerMode(1)
		# self.layerEffect(2)
		self.layer(1)
		time.sleep(delay)
		self.drawRadar()
		self.drawUpperButton()
		self.drawLowerButton()
		self.write_COMPASS()
		self.write_VIEW(650, 86)
		self.write_MARK(655, 365)

	# Main->View
	def drawViewScreen(self):
		# self.layerMode(1)
		# self.layerEffect(2)
		self.layer(1)
		time.sleep(delay)
		self.drawRadar()
		self.drawUpperButton()
		self.drawLowerButton()
		self.write_COMPASS()
		self.write_CANCEL(640, 86)
		self.write_SELECT(640, 365)

	# Main->View->Select
	def drawInfoState(self):
		# self.layerMode(1)
		# self.layerEffect(2)
		self.layer(1)
		time.sleep(delay)
		self.drawLoadPicScreen()
		self.drawUpperButton()
		self.drawLowerButton()
		self.write_TAG(100, 315)
		self.write_DISTANCE(100, 365)
		self.write_CB(100, 415)
		self.write_TYPE()
		self.write_HOME(655, 86)
		self.write_GOBACK(635, 365)

	# Main->Mark
	def drawMarkScreen(self):
		# self.layerMode(1)
		# self.layerEffect(2)
		self.layer(1)
		time.sleep(delay)
		self.drawMarkArrow()
		self.write_POINTTOWARDS(200, 85)
		self.write_OBJECT(250, 375)
		self.drawUpperButton()
		self.drawLowerButton()
		self.write_CANCEL(640, 86)
		self.write_MARK(655, 365)

	# Main->Mark->Mark
	def drawAfterMark(self):
		# self.layerMode(1)
		# self.layerEffect(2)
		self.layer(1)
		time.sleep(delay)
		self.drawLoadPicScreen()
		self.write_TAG(100, 310)
		self.write_DISTANCE(100, 360)
		self.write_CB(100, 410)
		self.write_TYPE()
		self.drawUpperButton()
		self.drawLowerButton()
		self.write_CANCEL(640, 86)
		self.write_CONFIRM(635, 365)

	# Alert (Interest)
	def drawAlertInterest(self):
		# self.layerMode(1)
		# self.layerEffect(2)
		self.layer(1)
		time.sleep(delay)
		self.write_ALERT(240, 85)
		self.write_POINEAR(100, 120)
		self.write_DISTANCE(150, 250)
		self.write_DIRECTION(150, 300)
		self.write_TAG(150, 350)
		self.write_TYPE()
		self.drawUpperButton()
		self.drawLowerButton()
		self.write_DISMISS(635, 86)
		self.write_VIEW(650, 365)

	# Alert (Danger)
	def drawAlertDanger(self):
		self.layerMode(1)
		self.layerEffect(2)
		self.layer(0)
		time.sleep(delay)
		self.write_ALERT(240, 85)
		self.write_PODNEAR(175, 120)
		self.write_DISTANCE(150, 250)
		self.write_DIRECTION(150, 300)
		self.write_TAG(150, 350)
		self.write_TYPE()
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
		self.transparentBackground(0xffff)
		self.textCursor(x, y)
		self.writeText(1)
		self.graphicsMode()

	def write_MARK(self, x, y):
		self.textMode()
		self.fontSize(2)
		self.transparentBackground(0xffff)
		self.textCursor(x, y)
		self.writeText(2)
		self.graphicsMode()

	def write_VIEW(self, x, y):
		self.textMode()
		self.fontSize(2)
		self.transparentBackground(0xffff)
		self.textCursor(x, y)
		self.writeText(3)
		self.graphicsMode()

	def write_SELECT(self, x, y):
		self.textMode()
		self.fontSize(2)
		self.transparentBackground(0xffff)
		self.textCursor(x, y)
		self.writeText(4)
		self.graphicsMode()

	def write_COMPASS(self):
		self.textMode()
		self.transparentBackground(0x01c0)
		self.fontSize(1)
		self.textCursor(256, 0)
		self.writeText(7) #N
		self.textCursor(481, 225)
		self.writeText(5) #E
		self.textCursor(256, 445)
		self.writeText(8) #S
		self.textCursor(35, 225)
		self.writeText(6) #W
		self.transparentBackground(0xffff)
		self.fontSize(0)
		self.textCursor(296, 260)
		self.writeText(21) #10M
		self.textCursor(346, 310)
		self.writeText(22) #20M
		self.textCursor(396, 360)
		self.writeText(23) #30M
		self.graphicsMode()

	def write_HOME(self, x, y):
		self.textMode()
		self.fontSize(2)
		self.transparentBackground(0xffff)
		self.textCursor(x, y)
		self.writeText(9)
		self.graphicsMode()

	def write_GOBACK(self, x, y):
		self.textMode()
		self.fontSize(2)
		self.transparentBackground(0xffff)
		self.textCursor(x, y)
		self.writeText(10)
		self.graphicsMode()

	def write_TAG(self, x, y):
		self.textMode()
		self.fontSize(2)
		self.transparentBackground(0x041f)
		self.textCursor(x, y)
		self.writeText(11)
		self.graphicsMode()

	def write_TYPE(self):
		self.textMode()
		self.fontSize(2)
		self.transparentBackground(0x041f)
		self.textCursor(100, 265)
		word = "Type: "
		numBytes = len(word)
		print('numBytes = ',numBytes)
		self.microblaze.write_mailbox(4, numBytes)
		for i in range(0, numBytes):
			# self.microblaze.write_mailbox((i+1)*4, 0)
			self.microblaze.write_mailbox((i+2)*4, ord(word[i]))
		self.writeText(99)
		print('c-side numBytes = ', self.microblaze.read_mailbox(0))
		self.graphicsMode()

	def write_CUSTOM(self, word, x, y, color):
		self.textMode()
		self.fontSize(2)
		self.transparentBackground(color)
		self.textCursor(x, y)
		numBytes = len(word)
		print('numBytes =', numBytes)
		self.microblaze.write_mailbox(4, numBytes)
		for i in range(0, numBytes):
			# self.microblaze.write_mailbox((i+1)*4, 0)
			self.microblaze.write_mailbox((i+2)*4, ord(word[i]))
		self.writeText(99)
		print('c-side numBytes = ', self.microblaze.read_mailbox(0))
		self.graphicsMode()

	def write_CB(self, x, y):
		self.textMode()
		self.fontSize(2)
		self.transparentBackground(0x041f)
		self.textCursor(x, y)
		self.writeText(12)
		self.graphicsMode()

	def write_DISTANCE(self, x, y):
		self.textMode()
		self.fontSize(2)
		self.transparentBackground(0x041f)
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
		self.transparentBackground(0xffff)
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
		# stream_size = 153600
		# buf2 = self.buf_manager.cma_alloc(stream_size, data_type="uint8_t")
		# buf3 = self.buf_manager.cma_get_buffer(buf2, stream_size)
		# draw_addr = self.buf_manager.cma_get_phy_addr(self.buf565)
		# self.microblaze.write_mailbox(0, phy_addr)
		self.layer(0)
		time.sleep(delay)
		self.microblaze.write_mailbox(0, x_start)
		self.microblaze.write_mailbox(4, y_start)
		self.microblaze.write_blocking_command(CAMERA)
		self.layer(1)
		time.sleep(delay)
		self.drawRect(100, 0, 320, 240, 0x0000, 1)

	def drawImage(self, x_start, y_start):
		#uses drawAddr buffer
		self.microblaze.write_mailbox(0, x_start)
		self.microblaze.write_mailbox(4, y_start)
		self.microblaze.write_blocking_command(DRAW_IMAGE)

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

	def draw2X(self, x, y, size, color): # Alert
		self.drawLine(x-size, y-size, x+size, y+size, color)
		self.drawLine(x-size+1, y-size, x+size+1, y+size, color)
		self.drawLine(x-size-1, y-size, x+size-1, y+size, color)

		self.drawLine(x+size, y-size, x-size, y+size, color)
		self.drawLine(x+size, y-size+1, x-size, y+size+1, color)
		self.drawLine(x+size, y-size-1, x-size, y+size-1, color)
		time.sleep(0.01)

	def drawTrianglePoint(self, x, y, size, color): # Interest
		self.drawTriangle(x-size, y+size, x+size, y+size, x, y-size, color, 1)
		time.sleep(0.01)
		# self.drawTriangle(x-size+1, y+size-1, x+size-1, y+size-1, x, y-size+1, color, 1)

	def drawPoint(self, x, y, size, color): # Breadcrumbs
		self.drawCircle(x, y, size, color, 1)
		time.sleep(0.01)

	def drawDanger(self, x, y, color):
		self.layer(0)
		time.sleep(delay)
		self.draw2X(x, y, 15, color)

	def drawInterest(self, x, y, color):
		self.layer(0)
		time.sleep(delay)
		self.drawTrianglePoint(x, y, 15, color)

	def drawCrumb(self, x, y, color):
		self.layer(0)
		time.sleep(delay)
		self.drawPoint(x, y, 15, color)

	def hasMessages(self):
		self.microblaze.write_blocking_command(HAS_MESSAGES)
		return self.microblaze.read_mailbox(0);

	def beginCameraTransfer(self, address):
		self.microblaze.write_mailbox(0, address)
		self.microblaze.write_blocking_command(WRITE_CAMERA)
		return self.microblaze.read_mailbox(0)

	def writeToTX(self, address, message):
		messageLen = len(message)
		numPackets = ceil(messageLen / MAX_LEN)
		returnBytes = []
		if(numPackets == 1):
			self.writeHelper(address, message)
		else:
			for x in range(numPackets):
				if(messageLen >= ((x + 1) * MAX_LEN)): # HAS MORE PACKETS LEFT
					numToRead = MAX_LEN
				else: # FINAL PACKET
					numToRead = messageLen % MAX_LEN
				returnBytes.append(self.writeHelper(address, message[(x * MAX_LEN):(x * MAX_LEN) + numToRead]))
		return returnBytes

	def writeHelper(self, address, message):
		self.microblaze.write_mailbox(0, len(message))
		self.microblaze.write_mailbox(4, address)
		for x in range(len(message)):
			self.microblaze.write_mailbox(4 * (x + 2), ord(message[x]))
		self.microblaze.write_blocking_command(WRITE_TO)
		return self.microblaze.read_mailbox(0)

	def readFromTX(self):
		self.microblaze.write_blocking_command(READ_FROM)
		dataBytes = []
		for x in range(32):
			dataBytes.append(self.microblaze.read_mailbox(x * 4))
		return dataBytes

	def getDevice(self):
		self.microblaze.write_blocking_command(DEVICE)
		return self.microblaze.read_mailbox(0)

	def readRegister(self, register):
		self.microblaze.write_mailbox(0, register)
		self.microblaze.write_blocking_command(READ_REG)
		return self.microblaze.read_mailbox(0)

	def getSetup(self):
		self.microblaze.write_blocking_command(GET_SETUP)
		return self.microblaze.read_mailbox(0)

	def stopListening(self):
		self.microblaze.write_blocking_command(STOP_LISTEN)

	def startListening(self):
		self.microblaze.write_blocking_command(START_LISTEN)

	def beginTX(self, addrNo):
		self.microblaze.write_mailbox(0, addrNo);
		self.microblaze.write_blocking_command(START_TX)

	def setImageAddress(self):
		stream_size = 153600
		transceiver_size = 230400
		buf0 = self.buf_manager.cma_alloc(stream_size, data_type="uint8_t")
		buf1 = self.buf_manager.cma_get_buffer(buf0, stream_size)
		phy_addr = self.buf_manager.cma_get_phy_addr(buf0)

		buf2 = self.buf_manager.cma_alloc(transceiver_size, data_type="uint8_t")
		buf3 = self.buf_manager.cma_get_buffer(buf2, transceiver_size)
		tx_addr = self.buf_manager.cma_get_phy_addr(buf2)

		buf4 = self.buf_manager.cma_alloc(stream_size, data_type="uint8_t")
		buf5 = self.buf_manager.cma_get_buffer(buf0, stream_size)
		draw_addr = self.buf_manager.cma_get_phy_addr(buf4)

		self.microblaze.write_mailbox(0, phy_addr)
		self.microblaze.write_mailbox(4, draw_addr)
		self.microblaze.write_mailbox(8, tx_addr)
		self.microblaze.write_blocking_command(IMAGE_ADDRESS)
		self.buf565 = buf1
		self.buf888 = buf3
		self.drawAddr = buf5
		return buf1, buf3

	'''
	565 to 888
	'''
	def conversion(self):
		counter = 0
		for i in range(0,153600,2):
			temp = []
			temp.append(self.buf565[i].hex())
			temp[-1] = int(temp[-1], 16)
			temp.append(self.buf565[i+1].hex())
			temp[-1] = int(temp[-1], 16)

			red = (temp[0] & 0xF8) | ((temp[0] & 0xE0) >> 5)
			green = ((temp[0] & 0x07) << 5) | ((temp[1] & 0xE0) >> 3) | (temp[0] & 0x03)
			blue = ((temp[1] & 0x1F) << 3) | ((temp[1] & 0x1C) >> 2)
			self.buf888[counter] = bytes([red])
			self.buf888[counter+1] = bytes([green])
			self.buf888[counter+2] = bytes([blue])
			counter += 3

	def clearAll(self):
		"""
		Clears both layers (background & foreground).
		"""

		self.layerMode(1)
		self.layerEffect(2)
		self.layer(0)
		self.clearWindow(1)
		time.sleep(delay)
		self.layer(1)
		self.clearWindow(1)
		time.sleep(delay)

	def clearLayer(self, layer):
		"""
		:param layer: 0 (foreground), 1 (background)
		"""

		self.layerMode(1)
		self.layerEffect(2)
		self.layer(layer)
		self.clearWindow(1)
		time.sleep(delay)

	def prepareToSend(self):
		# self.beginTX(1)
		gpsVal = None
		# while(gpsVal == None):
		# 	gpsVal = self.parseGPS(self.readFromGPS())
		imuVal = self.parseIMU(self.get_euler())
		rangeVal = self.parseRange()
		tag = "t,Big Boi!"
		# self.writeToTX(4, gpsVal)
		self.writeToTX(4, imuVal)
		self.writeToTX(4, rangeVal)
		self.writeToTX(4, tag)
		print(gpsVal, imuVal, rangeVal, tag)

	def parseGPS(self, val):
		if(val[0] != None and val[1] != None):
			return "l," + str(round(val[0], 6)) + "," + str(round(val[1], 6))

	def parseIMU(self, val):
		returnString = ""
		for x in val:
			returnString += (str(x) + ",")
		return "v," + returnString[0:-1]

	def parseRange(self):
		return "r," + str(self.medianRange())

	def parseRangeChr(self, val):
		returnString = ""
		for x in val:
			returnString += (chr(x))
		return int(returnString[1:])

	def medianRange(self):
		count = 0
		medianList = []
		while(count < 20):
			medianList.append(self.parseRangeChr(self.range_poll()))
			count+=1
		return median(medianList)
