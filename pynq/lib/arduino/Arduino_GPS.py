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
					newVal = stringBuilder.split(",")
					if(newVal[0] == '$GNGLL' or newVal[0] == 'GNGLL'):
						print(self.getLatitude(newVal), self.getLongitude(newVal))
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


