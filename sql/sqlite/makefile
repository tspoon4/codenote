#CFLAGS = -msse3 -ffast-math -g
CFLAGS = -DNDEBUG -O3 -msse3 -ffast-math -g

all: analyzer.elf shell.elf

analyzer.elf: main.o sqlite3.o
	g++ main.o sqlite3.o -g -ltbb -o analyzer.elf

main.o: main.cpp
	g++ main.cpp -c $(CFLAGS)

shell.elf: shell.o sqlite3.o
	gcc shell.o sqlite3.o -g -o shell.elf

shell.o: shell.c sqlite3.h sqlite3ext.h
	gcc shell.c -c $(CFLAGS)

sqlite3.o: sqlite3.c sqlite3.h sqlite3ext.h
	gcc sqlite3.c -c $(CFLAGS)

clean:
	rm -f *.o *.elf
