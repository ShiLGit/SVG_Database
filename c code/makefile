CC = gcc
CFLAGS = -Wall -g -std=c11 -pedantic
LDFLAGS= -L./bin/ ##linker look here for libraries

LIBPATH = /usr/include/libxml2
BINDIR = ./bin/
SRCDIR = ./src/
INCDIR = ./include/

##export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.:bin

all: $(BINDIR)libsvgparse.so maintest

$(BINDIR)libsvgparse.so: $(BINDIR)SVGParser.o $(BINDIR)LinkedListAPI.o
	$(CC) $(CFLAGS) -shared -o $(BINDIR)libsvgparse.so $(BINDIR)SVGParser.o  $(BINDIR)LinkedListAPI.o ;\
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.:bin

$(BINDIR)SVGParser.o: $(SRCDIR)SVGParser.c $(INCDIR)SVGParser.h
	$(CC) $(CFLAGS44) -c -fPIC $(SRCDIR)SVGParser.c -I$(INCDIR) -I$(LIBPATH) -o $(BINDIR)SVGParser.o

$(BINDIR)LinkedListAPI.o: $(SRCDIR)LinkedListAPI.c
	$(CC) $(CFLAGS) -c $(SRCDIR)LinkedListAPI.c -I$(INCDIR) -o$(BINDIR)LinkedListAPI.o
###########LIIIIIIIIIIIIIIIIIIIIIIB LIST

##MAKE MAIN
maintest: $(BINDIR)libsvgparse.so main.o
	$(CC) $(CFLAGS) $(LDFLAGS) main.o -o maintest -lsvgparse -I$(LIBPATH) -lxml2 ;\
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.:bin

main.o: $(SRCDIR)main.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c $(SRCDIR)main.c -Iinclude -L$(BINDIR) -I$(LIBPATH)

clean:
	rm -rf ./bin/*.o ./bin/*.so ./maintest *.o wtf.svg test.svg

#>>>make one big svgparse.so that uses liblist.so and libxml?
# -c flag == compile into .o; don't link4644
