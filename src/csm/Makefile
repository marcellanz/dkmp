CC	= gcc
ROOT	= $(HOME)/data/projects/dkm

# compiler
#CXXFLAGS = -ansi -pedantic -Wall -g
# linker
#LDFLAGS

# compile rules

#
# all
#
all:	csm
#
# csd
#
csm.o: csm.c
	gcc $(CXXFLAGS) -c csm.c

csm:	csm.o csp.o
	gcc $(LDFLAGS) csm.o csp.o -o csm

clean:
	rm csm csm.o
