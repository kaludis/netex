CC=gcc

CFLAGS=-c -Wall -Wextra -ansi

LDFLAGS=

SOURCES=main.c cfgreader.c

OBJECTS=$(SOURCES:.c=.o)

EXECUTABLE=netex

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o netex
