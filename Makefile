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

.PHONY: clean
clean:
	rm -f $(OBJECTS)
	rm -f $(DEPS)
	rm -f $(APP)
 
TYPES = src/types.vim
.PHONY: tags $(TYPES)
tags:
	ctags -R

# NOTE: For types to have any benefit, you would need the appropriate config
# in your .vimrc for the keyword 'DevType'
types: $(TYPES)
$(TYPES):
	ctags -R --languages=c++ --kinds-c=gstud -o- |\
		awk 'BEGIN{printf("syntax keyword DevType\t")}\
			{printf("%s ", $$1)}END{print ""}' > $@
