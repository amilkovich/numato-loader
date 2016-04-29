VERSION = 0.5
SRC = saturn-loader.c n25q128a.c
OBJ = $(SRC:.c=.o)
LIBS = -lftdi1 $(LIBMPSSE_ARCHIVE)
CFLAGS = -std=c11 -pedantic -Wall -Os -DVERSION=\"$(VERSION)\" -DLIBFTDI1=1 -Ilibmpsse-master/src
LDFLAGS = -s $(LIBS)
LIBMPSSE_ARCHIVE = libmpsse-master/src/libmpsse.a
LIBMPSSE_URL = https://github.com/devttys0/libmpsse/archive/master.zip

all: saturn-loader

libmpsse-master.zip:
	@wget -O libmpsse-master.zip $(LIBMPSSE_URL)

libmpsse-master: libmpsse-master.zip
	@unzip -o libmpsse-master.zip
	@cd libmpsse-master/src; CFLAGS=-DLIBFTDI1=$$\(LIBFTDI1\) ./configure --disable-python
	@make -C libmpsse-master/src

$(LIBMPSSE_ARCHIVE): libmpsse-master

.o: .c
	$(CC) -c $(CFLAGS) $?

saturn-loader: $(LIBMPSSE_ARCHIVE) $(OBJ)
	$(CC) -o saturn-loader $(OBJ) $(LDFLAGS)

clean:
	$(RM) saturn-loader $(OBJ)

dist: clean
	mkdir -p saturn-loader
	cp saturn-loader.c n25q128a.h n25q128a.c LICENSE TODO Makefile README \
	saturn-loader
	tar -cvjf saturn-loader.tar.bz2 saturn-loader
	$(RM) -r saturn-loader

.PHONY: all clean dist
