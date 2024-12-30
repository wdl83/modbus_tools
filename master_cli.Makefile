include Makefile.defs

TARGET = master_cli

CXXSRCS = \
	FdGuard.cpp \
	Master.cpp \
	SerialPort.cpp \
	crc.cpp \
	json.cpp \
	master_cli.cpp

include Makefile.rules
