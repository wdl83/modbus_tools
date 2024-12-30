include Makefile.defs

TARGET = probe

CXXFLAGS += -I ensure

CXXSRCS = \
	FdGuard.cpp \
	Master.cpp \
	SerialPort.cpp \
	crc.cpp \
	json.cpp \
	probe.cpp

include Makefile.rules
