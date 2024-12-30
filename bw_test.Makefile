include Makefile.defs

TARGET = bw_test

CXXFLAGS += -I ensure

CXXSRCS = \
	FdGuard.cpp \
	Master.cpp \
	SerialPort.cpp \
	bw_test.cpp \
	crc.cpp \
	json.cpp

include Makefile.rules
