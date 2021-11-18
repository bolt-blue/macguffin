CFLAGS ?= -Wall -g -O0

APP = macguffin
SOURCES = $(wildcard src/*.c)
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

all: $(APP)

$(APP): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

$(OBJECTS): %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c

clean:
	rm -f $(OBJECTS)
	rm -f $(APP)
