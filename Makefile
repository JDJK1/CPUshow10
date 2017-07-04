TARGET = CPUshow10
LIBS = -lm -lusb
CC = gcc
CFLAGS = -g -Wall -std=gnu99 -DLINUX

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(filter-out cpuusage_windows.c, $(wildcard *.c)))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
