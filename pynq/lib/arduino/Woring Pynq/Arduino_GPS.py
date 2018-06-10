from . import Arduino
from . import ARDUINO_GROVE_I2C
from . import MAILBOX_OFFSET
from . import MAILBOX_PY2IOP_CMD_OFFSET

ARDUINO_GPS = "arduino_gps.bin"
READ_GPS_CMD = 0x01
BYTES_AVAILABLE_CMD = 0x02
TEST_CMD = 0x03
STREAM = 0x04

class Arduino_GPS(object):

	def __init__(self, mb_info):
		self.microblaze = Arduino(mb_info, ARDUINO_GPS)

	def readFromGPS(self):
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


