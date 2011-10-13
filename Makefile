#
# Makefile for network
# @(#)Makefile	4.21 (Berkeley) 02/04/99
#
HDRS=	netprot.h network.h netwait.h
DOBJS=	terminal.o rmove.o network.o netprot.o rwrite.o rread.o \
	sendit.o continue.o rinit.o control.o error.o rmap.o edit.o \
	rexec.o master.o handle.o pipes.o packet.o socket.o ether.o \
	netmap.o startup.o solve.o data.o signal.o rcopy.o \
	system.o ctlmod.o
OBJS=	$(DOBJS) find.o inter.o
CFILES=	terminal.c rmove.c network.c netprot.c rwrite.c rread.c \
	sendit.c continue.c rinit.c control.c error.c rmap.c edit.c \
	rexec.c master.c handle.c pipes.c packet.c socket.c ether.c \
	netmap.c inter.c startup.c solve.c data.c signal.c rcopy.c \
	system.c ctlmod.c find.c
MISC_C=	ropen.c netwait.c netmisc.c
MISC=	Makefile $(MISC_C)

DEFS=		-DMASTER -DDUMP -DALLSCORES -DUSE_OLD_TTY
CFLAGS=		-g $(DEFS)
#CFLAGS=		-Bcpp/ -tp -O $(DEFS)
PROFLAGS=	-pg -O
#LDFLAGS=	-i	# For PDP-11's
LDFLAGS=	-g	# For VAXen
CRLIB=		-lncurses
#CRLIB=		-lcurses
#CRLIB=		libcurses.a

#SCOREFILE=	/usr/public/n_rogue_roll
SCOREFILE=	/usr/games/rogue.scores
#SCOREFILE=	./net_hist
SF=		-DSCOREFILE='"$(SCOREFILE)"'
NAMELIST=	/vmunix
NL=		-DNAMELIST='"$(NAMELIST)"'
#MACHDEP=	-DMAXLOAD=40 -DLOADAV -DCHECKTIME=4

LD=	ld
VGRIND=	/usr/ucb/vgrind
# for sites without sccs front end, GET= get
GET=	sccs get

# Use ansi flag to gcc
#CC = gcc -ansi
CC = cc

.DEFAULT:
	$(GET) $@

a.out: $(HDRS) $(OBJS)
	-rm -f a.out
	@rm -f x.c
	-$(CC) $(LDFLAGS) $(OBJS) $(CRLIB)
#	-$(CC) $(LDFLAGS) $(OBJS) $(CRLIB) -ltermlib
#	-$(CC) $(LDFLAGS) $(OBJS) $(CRLIB) -lcrypt
	size a.out
#	@echo -n 'You still have to remove '		# 11/70's
#	@size a.out | sed 's/+.*/ 1024 63 * - p/' | dc	# 11/70's

terminal.o:
	$(CC) -c $(CFLAGS) terminal.c

find.o: find.c
	$(CC) -c $(CFLAGS) $(SF) $(NL) $(MACHDEP) find.c

inter.o: inter.c
	$(CC) -c $(CFLAGS) $(SF) $(NL) $(MACHDEP) inter.c

network: newvers a.out
	cp a.out network
	strip network

ropen: ropen.c
	$(CC) -s -o ropen ropen.c

netwait: netwait.o netmisc.o terminal.o
	$(CC) -s -o netwait terminal.o netwait.o netmisc.o -lcurses

netmisc.o netwait.o:
	$(CC) -O -c $(SF) $*.c

newvers:
	$(GET) -e terminal.c
	sccs delta -y terminal.c

flist: $(HDRS) $(CFILES) $(MISC_C)
	-mv flist tags
	ctags -u $?
	ed - tags < :rofix
	sort tags -o flist
	rm -f tags

lint:
	/bin/csh -c "lint -hxbc $(DEFS) $(MACHDEP) $(SF) $(NL) $(CFILES) -lcurses > linterrs"

clean:
	rm -f $(OBJS) core a.out p.out network strings ? network.tar vgrind.* x.c x.o xs.c linterrs ropen

xtar: $(HDRS) $(CFILES) $(MISC)
	rm -f network.tar
	tar cf network.tar $? :rofix
	touch xtar

vgrind:
	@csh $(VGRIND) -t -h "Rogue Version 3.7" $(HDRS) *.c > vgrind.out
	@ctags -v $(HDRS) *.c > index
	@csh $(VGRIND) -t -x index > vgrind.out.tbl

wc:
	@echo "  bytes  words  lines  pages file"
	@wc -cwlp $(HDRS) $(CFILES)

cfiles: $(CFILES)
