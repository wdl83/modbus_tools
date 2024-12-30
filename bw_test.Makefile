include Makefile.defs

TARGET = bw_test

CXXSRCS = \
	FdGuard.cpp \
	Master.cpp \
	SerialPort.cpp \
	bw_test.cpp \
	crc.cpp \
	json.cpp

include Makefile.rules
