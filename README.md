## numato-loader

Flashing utility for Numato Lab FPGA boards:
	Saturn

Boards not implemented yet (some have been tested and seem to work):
	Skoll
	Esso
	Mimas v2 (serial boards being implemented)

## FAQ

### What is numato-loader?

numato-loader is a flashing utility for Numato Lab's FPGA development boards. This was
initially written because I was not able to find a suitable Linux friendly
flashing utility for the Saturn development board.

### What license is numato-loader release under?

numato-loader is released under the MIT/X Consortium License, see LICENSE file
for details.

Currently, the library dependencies are:

libftdi (LGPLv2.1, dynamically linked)
libmpsse (modified BSD license, statically linked)

### What architectures does numato-loader support?

Currently numato-loader has been successfully built and tested on x86_64, i686,
and armv7l (Raspberry Pi 2).

### How do you use numato-loader?

First you need to build the project, install libftdi before proceeding:

	$ git clone https://github.com/amilkovich/numato-loader.git
	$ cd numato-loader
	$ make
	$ ./numato-loader -l

Some common examples of running numato-loader would be as follows:

	$ ./numato-loader design.bin # flash design.bin to numato
	$ ./numato-loader -e # erase the flash (bulk erase)

### Help! I get flash id mismatch every time I try to flash/erase!

If something weird happened, like the numato-loader process was killed in the
middle of a write, then the Xilinx Spartan's PROGRAM_B pin may be left asserted
(active low) and the Saturn may be in an undesirable state. In this case, you'll
need to manually tie the PROGRAM_B pin to GROUND. When that is wired up, flash
or erase as before. Once finished, disconnect the wire. See the Numato Lab
Saturn user guide for more details on the locations of PROGRAM_B (aka PROGB) and
available GROUND pins.
