# makefile for Lua compiler

LUA= ../..

include $(LUA)/config

INCS= -I$(INC) -I.. $(EXTRA_INCS)
OBJS= luac.o print.o lopcodes.o
SRCS= luac.c print.c
POBJS= pluac.o pdb.o print.o lopcodes.o
PSRCS= pluac.c pdb.c print.c

T= $(BIN)/luac
PT= $(BIN)/plua2c

all:	$T $(PT)

$T:	$(OBJS) $(LIB)/liblua.a $(LIB)/liblualib.a
	$(CC) -o $@ $(MYLDFLAGS) $(OBJS) -L$(LIB) -llua -llualib $(EXTRA_LIBS) $(DLLIB)

$(PT):	$(POBJS) $(LIB)/liblua.a $(LIB)/liblualib.a
	$(CC) -o $@ $(MYLDFLAGS) $(POBJS) -L$(LIB) -llua -llualib $(EXTRA_LIBS)
$(DLLIB)

# print.c needs opcode names from lopcodes.c
lopcodes.o:	../lopcodes.c ../lopcodes.h
	$(CC) -o $@ -c $(CFLAGS) -DLUA_OPNAMES ../lopcodes.c

$(LIB)/liblua.a:
	cd ..; $(MAKE)

$(LIB)/liblualib.a:
	cd ../lib; $(MAKE)

clean:
	rm -f $(OBJS) $T $(POBJS) $(PT)

co:
	co -q -f -M $(SRCS)

klean:	clean
	rm -f $(SRCS)
