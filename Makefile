CC=clang

CFLAGS=-c -Wall -Wextra -g -O0
#CFLAGS=-c -Wall -Wextra -O2

LDFLAGS=

SOURCES=main.c cfgreader.c network.c

OBJECTS=$(SOURCES:.c=.o)

EXECUTABLE=netex

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o netex
