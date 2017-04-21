OBJS =	terminal.o rmove.o network.o netprot.o rwrite.o rread.o \
	sendit.o continue.o rinit.o control.o error.o rmap.o edit.o \
	rexec.o master.o handle.o pipes.o packet.o socket.o ether.o \
	netmap.o startup.o solve.o data.o signal.o rcopy.o \
	system.o ctlmod.o \
	find.o inter.o

LDFLAGS = $(shell pkg-config --libs ncurses)

default: rogue

rogue: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJS) rogue
