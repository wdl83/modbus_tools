include Makefile.defs

TARGET = SerialPortTests

CXXFLAGS += -I. -I ensure -I utest

CXXSRCS = \
	FdGuard.cpp \
	PseudoSerial.cpp \
	SerialPort.cpp \
	tests/SerialPortTests.cpp

include Makefile.rules
