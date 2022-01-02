CC=gcc
CFLAGS=-I.
DEPS = modbus.h
OBJ = sdm120.c 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sdm120: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
install: 
	cp sdm120 /usr/local/bin/sdm120
	cp sdm120.py /usr/local/bin/sdm120.py
