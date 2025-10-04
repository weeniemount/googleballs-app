EE_BIN = googleballs.elf
EE_OBJS = main.o

# Using raw draw library instead
EE_LIBS = -ldraw -lgraph -lpacket -lm
EE_INCS +=

all: $(EE_BIN)
	$(EE_STRIP) --strip-all $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

run: $(EE_BIN)
	ps2client execee host:$(EE_BIN)

reset:
	ps2client reset

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal