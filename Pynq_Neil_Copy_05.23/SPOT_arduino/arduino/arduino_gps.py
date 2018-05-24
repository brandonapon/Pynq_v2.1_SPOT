from . import Arduino
from . import ARDUINO_GROVE_I2C

ARDUINO_GPS = "arduino_gps.bin"
READ_GPS_CMD = 0x1

class Arduino_GPS(object):

	def __init__(self, mb_info, read_pin):
		self.microblaze = Arduino(mb_info, ARDUINO_GPS)

	def readFromGPS():
		dataBytes = []
		self.microblaze.write(READ_GPS_CMD)
		bytesToRead = (self.microblaze.read_mailbox(0) << 8) | self.microblaze.read_mailbox(1)
		for x in range(bytesToRead):
			dataBytes.append(self.microblaze.read_mailbox(x + 2))
		return dataBytes
