include Makefile.defs

TARGET = chslv

CXXFLAGS += -I ensure

CXXSRCS = \
	chslv.cpp

include Makefile.rules
