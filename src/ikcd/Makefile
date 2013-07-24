CC	= gcc
ROOT	= $(HOME)/data/projects/dkm

#SMP
MAKE = make -j4

# compiler
#CXXFLAGS = -pedantic -Wall -g
DEBUG = -DDEBUG
# linker
LDFLAGS = -lpthread

#
# all
#
all:	ikcd
#
# csd
#
ikcd.o:	ikcd.c
	gcc $(DEBUG) $(CXXFLAGS) -c ikcd.c

ikcp.o:	ikcp.c
	gcc $(DEBUG) $(CXXFLAGS) -c ikcp.c

ikcl.o:	ikcl.c
	gcc $(DEBUG) $(CXXFLAGS) -c ikcl.c

ikcr.o: ikcr.c
	gcc $(DEBUG) $(CXXFLAGS) -c ikcr.c

ikcq.o: ikcq.c
	gcc $(DEBUG) $(CXXFLAGS) -c ikcq.c

ikcbl.o: ikcbl.c
	gcc $(DEBUG) $(CXXFLAGS) -c ikcbl.c

dkmctl_sys.o:	dkmctl_sys.c
	gcc $(DEBUG) $(CXXFLAGS) -c dkmctl_sys.c

ikcd: 	dkmctl_sys.o ikcd.o ikcp.o ikcl.o ikcr.o ikcq.o ikcbl.o
	gcc  $(LDFLAGS) dkmctl_sys.o ikcp.o ikcl.o ikcr.o ikcq.o ikcbl.o ikcd.o -o ikcd

clean:
	rm *.o ikcd
