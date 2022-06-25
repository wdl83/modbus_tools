Collection of Modbus Tools
==========================

Building
--------

```console
git clone --recurse-submodules https://github.com/wdl83/modbus_tools
cd modbus_tools
RELEASE=1 make
```
Build artifacts will be placed in 'obj' dir, if you have not defined OBJ_DIR.

Installing
----------

DST_DIR variable can be used to define prefix (must be absolute) path for:

1. $DST_DIR/bin

```console
RELEASE=1 DST_DIR=$HOME/opt make install
```

JSON Format
-----------
Format used depends on Modbus **function code** of given command.
Every utility which accepts json input expects array of requests:

```json
[
    {req1},
    ...,
    {reqN}
]
```

Every request must contain **slave** and **fcode**, the rest of parameters depend
on **function code**.

If request **function code** reads data from Modbus RTU device:

```json
[
  {
    "slave" : INTEGER,
    "fcode" : INTEGER,
    "addr" : INTEGER,
    "count" : INTEGER
  }
]
```

If request **function code** writes data to Modbus RTU device:

```json
[
  {
    "slave" : INTEGER,
    "fcode" : INTEGER,
    "addr" : INTEGER,
    "count" : INTEGER,
    "value" : [INTEGER, ..., INTEGER]
  }
]
```

1. **slave**: device address
1. **fcode**: function code
1. **addr**: device memory address
1. **count**: number of units of data to be read/written
1. **value**: array of data to be written

[Example json requests](https://github.com/wdl83/modbus_tools/tree/master/json)

Supported Modbus RTU commands
-----------------------------

1. RD_COILS  1
1. RD_HOLDING_REGISTERS 3
1. WR_COIL 5
1. WR_REGISTER 6
1. WR_REGISTERS 16
1. RD_BYTES 65
1. WR_BYTES 66

Custom Modbus RTU Commands
--------------------------
Currently 2 custom commands are implemented:

1. RD_BYTES 65 (read n-bytes starting at address)
1. WR_BYTES 66 (write n-bytes starting at address)

Those commands are designed for 8-bit microcontrollers with 16-bit
byte-addressable space.

master_cli
----------

```console
master_cli -d device -i input.json|- [-o output.json] [-r rate] [-p parity(O/E/N)]
```

Example (19200bps, Even parity), write reply to stdout:

```console
master_cli -d /dev/ttyUSB0 -i reboot.json -o - -r 19200 -p E
```

monitor
-------
Utility to monitor data on serial port. By default all data is dumped in HEX and
ASCII, if -t option is provided only ASCII will be emited. Currently baud rate,
parity, data bits and stop bits are fixed in source code.

