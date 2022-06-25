ifndef OBJ_DIR
OBJ_DIR = ${PWD}/obj
export OBJ_DIR
endif

ifndef DST_DIR
DST_DIR = ${PWD}/dst
export DST_DIR
endif

all: \
	bw_test.Makefile \
	chslv.Makefile \
	master_cli.Makefile \
	monitor.Makefile \
	probe.Makefile \
	tlog_dump.Makefile
	make -f bw_test.Makefile
	make -f chslv.Makefile
	make -f master_cli.Makefile
	make -f monitor.Makefile
	make -f probe.Makefile
	make -f tlog_dump.Makefile

install: \
	bw_test.Makefile \
	chslv.Makefile \
	master_cli.Makefile \
	monitor.Makefile \
	probe.Makefile \
	tlog_dump.Makefile
	make -f bw_test.Makefile install
	make -f chslv.Makefile install
	make -f master_cli.Makefile install
	make -f monitor.Makefile install
	make -f probe.Makefile install
	make -f tlog_dump.Makefile install

clean: \
	bw_test.Makefile \
	master_cli.Makefile \
	monitor.Makefile \
	probe.Makefile \
	tlog_dump.Makefile 
	make -f bw_test.Makefile clean
	make -f master_cli.Makefile clean
	make -f monitor.Makefile clean
	make -f probe.Makefile clean
	make -f tlog_dump.Makefile clean

purge:
	rm $(OBJ_DIR) -rf
