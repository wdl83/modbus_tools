include Makefile.defs

TARGET = monitor

CXXFLAGS += -I ensure

CXXSRCS = \
	FdGuard.cpp \
	SerialPort.cpp \
	monitor.cpp

include Makefile.rules
