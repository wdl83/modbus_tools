OBJ_DIR ?= ${PWD}/obj
export OBJ_DIR

DST_DIR ?= ${PWD}/dst
export DST_DIR

.PHONY: all build test clean purge

all: build test

build: \
	SerialPortTests.Makefile \
	bw_test.Makefile \
	chslv.Makefile \
	master_cli.Makefile \
	monitor.Makefile \
	probe.Makefile \
	tlog_dump.Makefile
	make -f SerialPortTests.Makefile
	make -f bw_test.Makefile
	make -f chslv.Makefile
	make -f master_cli.Makefile
	make -f monitor.Makefile
	make -f probe.Makefile
	make -f tlog_dump.Makefile

install: build
	make -f bw_test.Makefile install
	make -f chslv.Makefile install
	make -f master_cli.Makefile install
	make -f monitor.Makefile install
	make -f probe.Makefile install
	make -f tlog_dump.Makefile install

test: build
	make -f SerialPortTests.Makefile run

clean:
	-make -f SerialPortTests.Makefile clean
	-make -f bw_test.Makefile clean
	-make -f master_cli.Makefile clean
	-make -f monitor.Makefile clean
	-make -f probe.Makefile clean
	-make -f tlog_dump.Makefile clean

purge:
	-rm $(OBJ_DIR) -rf
