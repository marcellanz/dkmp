CC	= gcc
ROOT	= $(HOME)/data/projects/dkm

#SMP
MAKE = make -j4

# compiler
WARNFLAGS = -pedantic -Wall -g
#DEMO = -DDEMO
#DEBUG = -DDEBUG
CXXFLAGS = $(WARNFLAGS) $(DEBUG)

CSDB_DEFINES = -DCSDB_LINEAR_SEARCH
#CSDB_DEFINES = -DCSDB_BINARY_SEARCH
# linker
LDFLAGS = -lpthread

# compile rules
#
# all
#
all:	csd
#
# csd
#
csda.o:	csda.c
	gcc $(DEBUG) $(CXXFLAGS) -c csda.c

csdb.o:	csdb.c
	gcc $(DEBUG) $(CXXFLAGS) $(CSDB_DEFINES) -c csdb.c

csc.o:	csc.c
	gcc $(DEBUG) $(CXXFLAGS) -c csc.c

csd.o:	csd.c
	gcc $(DEBUG) $(CXXFLAGS) -c csd.c

csp.o:	csp.c
	gcc $(DEBUG) $(CXXFLAGS) -c csp.c

csl.o:	csl.c
	gcc $(DEBUG) $(CXXFLAGS) -c csl.c

csd: 	csdb.o csc.o csp.o csda.o csl.o csd.o
	gcc  $(LDFLAGS) csda.o csdb.o csc.o csp.o csl.o csd.o -o csd

clean:
	rm *.o csd
