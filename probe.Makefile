include Makefile.defs

TARGET = probe

CXXSRCS = \
	FdGuard.cpp \
	Master.cpp \
	SerialPort.cpp \
	crc.cpp \
	json.cpp \
	probe.cpp

include Makefile.rules
