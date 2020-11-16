CC=gcc
CFLAGS=-I.
DEPS = modbus.h
OBJ = sdm120.c 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sdm120: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
