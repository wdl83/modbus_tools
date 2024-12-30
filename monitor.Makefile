include Makefile.defs

TARGET = monitor

CXXSRCS = \
	FdGuard.cpp \
	SerialPort.cpp \
	monitor.cpp

include Makefile.rules
