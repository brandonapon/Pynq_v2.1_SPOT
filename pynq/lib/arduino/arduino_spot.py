import math
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
GPS_READ = 0x3006
"""Ultrasonic specific commands 0x4xxx"""
RANGE_ON = 0x4000
RANGE_OFF = 0x4001
RANGE_POLL = 0x4002
RANGE_DEVICE = 0x4003
"""Camera specific commands 0x5xxx"""

"""Display specific commands 0x6xxx"""

"""Transceiver specific commands 0x7xxx"""

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
		"""Get current GPS data"""
		dataBytes = []
		stringBuilder = ""
		counter = 0
		self.microblaze.write_blocking_command(TEST_CMD); # Returns 102 values

		x = 0;
		while(1):
			val = self.microblaze.read_mailbox(4 * x)
			if(val < 256 and val != ''):
				stringValue = chr(val)
				if(stringValue == '\n'):
					print(stringBuilder.split(","))
					stringBuilder = ""
				else:
					stringBuilder += stringValue
			x = x + 1;
			if(x == 100):
				break

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

	def on(self):
		self.microblaze.write_blocking_command(RANGE_ON)


	def off(self):
		self.microblaze.write_blocking_command(RANGE_OFF)

	def nothing(self):
		self.microblaze.write_blocking_command(RANGE_NOTHING)
		nothing = self.microblaze.read_mailbox(0)
		return nothing

	def device(self):
		self.microblaze.write_blocking_command(RANGE_DEVICE)
		device = self.microblaze.read_mailbox(0)
		return device

	def poll(self):
		self.microblaze.write_blocking_command(RANGE_POLL)
		values = self.microblaze.read_mailbox(0,5)
		print (values)
		for val in values:
			print(chr(val))
