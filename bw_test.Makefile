include Makefile.defs

TARGET = bw_test

CXXSRCS = \
	Master.cpp \
	SerialPort.cpp \
	crc.cpp \
	json.cpp \
	bw_test.cpp

include Makefile.rules
