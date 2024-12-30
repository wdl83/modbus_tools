include Makefile.defs

TARGET = tlog_dump

CXXFLAGS += -I ensure

CXXSRCS = \
	tlog_dump.cpp

include Makefile.rules
