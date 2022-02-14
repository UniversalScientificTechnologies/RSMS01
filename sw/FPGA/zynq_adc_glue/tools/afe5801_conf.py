#!/usr/bin/python

import sys
from pymlab import config

cfg = config.Config(
	i2c = {"port": 0},
        bus = [
            {
                "type": "i2chub",
                "address": 0x73,
                "children": [
                    {"name": "i2cspi", "type": "i2cspi" , "channel": 0, "address": 44 },   
                ],
            },
        ],
)
cfg.initialize()
i2cspi = cfg.get_device("i2cspi")

def reg_write(addr, data):
	addr, data = int(addr), int(data)
	bytes = [addr, (data >> 8) & 0xff, data & 0xff]
	i2cspi.SPI_write(0x0f, bytes)

def reg_read(addr):
	reg_write(0x0, 0x2)
	reg_write(addr, 0x0)
	ret = i2cspi.SPI_read(3)[1:]
	reg_write(0x0, 0x0)
	return ret[0] << 8 | ret[1]

def readout():
	for a in range(1, 7):
		print "\treg[%d] = 0x%x" % (a, reg_read(a))

def main():
	print "resetting"
	reg_write(0x0, 0x1)

	if len(sys.argv) < 2:
		readout()
		return

	ptr_mode = int(sys.argv[1], 2)
	print "setting PATTERN_MODE to 0x%x" % ptr_mode
	reg_write(0x2, ptr_mode << 13)

	if len(sys.argv) >= 3:
		custom = int(sys.argv[2], 2)
		print "setting CUSTOM_PATTERN to 0x%x" % custom
		reg_write(0x5, custom)

	readout()

if __name__ == "__main__":
	main()

