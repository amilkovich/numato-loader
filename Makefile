VERSION = 0.5
SRC = saturn-loader.c n25q128a.c
OBJ = $(SRC:.c=.o)
LIBS = -lftdi libmpsse/libmpsse.a
CFLAGS = -std=c11 -pedantic -Wall -Os -DVERSION=\"$(VERSION)\" -Ilibmpsse
LDFLAGS = -s $(LIBS)

all: saturn-loader

.o: .c
	$(CC) -c $(CFLAGS) $?

saturn-loader: $(OBJ)
	$(CC) -o saturn-loader $(OBJ) $(LDFLAGS)

clean:
	$(RM) saturn-loader $(OBJ)

dist: clean
	mkdir -p saturn-loader
	cp -r libmpsse saturn-loader
	cp saturn-loader.c n25q128a.h n25q128a.c LICENSE TODO Makefile README \
	saturn-loader
	tar -cvjf saturn-loader.tar.bz2 saturn-loader
	$(RM) -r saturn-loader

.PHONY: all clean dist
