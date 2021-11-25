CFLAGS ?= -Wall -g -O0

APP = macguffin
SOURCES = $(wildcard src/*.c)
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

all: $(APP)

$(APP): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

DEPS := $(OBJECTS:.o=.d)
-include $(DEPS)

$(OBJECTS): %.o : %.c
	$(CC) $(CFLAGS) -MMD -MF $(patsubst %.o,%.d,$@) -c $< -o $@

clean:
	rm -f $(OBJECTS)
	rm -f $(DEPS)
	rm -f $(APP)
