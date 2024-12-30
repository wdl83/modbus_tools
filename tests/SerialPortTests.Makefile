include ../Makefile.defs

TARGET = SerialPortTests

CXXFLAGS += -I .. -I ../utest

CXXSRCS = \
	FdGuard.cpp \
	SerialPort.cpp \
	SerialPortTests.cpp

include ../Makefile.rules
