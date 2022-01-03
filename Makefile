CFLAGS ?= -Wall -g -O0

APP = macguffin
SOURCES = $(wildcard src/*.c)
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

all: $(APP) compdb tags types

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
 
TYPES = types.vim
tags: $(OBJECTS)
	ctags -R

# NOTE: For types to have any benefit, you would need the appropriate config
# in your .vimrc for the keyword 'DevType'
types: $(TYPES)
$(TYPES): $(OBJECTS)
	@echo "Re-building custom types"
	@ctags -R --languages=c++ --kinds-c=gstud -o- |\
		awk 'BEGIN{printf("syntax keyword DevType\t")}\
			{printf("%s ", $$1)}END{print ""}' > $@

# REF: https://pypi.org/project/compiledb/
.PHONY: compdb
compdb: compile_commands.json

compile_commands.json: Makefile
	make -Bnwk | compiledb
