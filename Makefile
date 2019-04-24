all: unit-test
	
Q=@
CC=gcc

CFLAGS = -O2 
LDFLAGS =

SOURCE=\
	btree_tests.c \
	\
	compat/dm-block-manager.c \
	\
	framework.c \
	main.c \
	\
	dm-transaction-manager.c \
	dm-space-map-common.c \
	dm-space-map-disk.c \
	dm-space-map-metadata.c \
	dm-btree-remove.c \
	dm-btree-spine.c \
	dm-btree.c

OBJECTS=$(subst .c,.o,$(SOURCE))
DEPENDS=$(subst .c,.d,$(SOURCE))
	

INCLUDES=
		
.SUFFIXES: .c .d .o

%.o: %.c
	@echo "    [CC] $<"
	$(Q) $(CC) -c $(INCLUDES) -Wall $< -o $@

%.d: %.c
	@echo "    [DEP] $<"
	$(Q) $(MKDIR_P) $(dir $@); \
	set -e; \
	FILE=`echo $@ | sed 's/\\//\\\\\\//g;s/\\.d//g'`; \
	DEPS=`echo $(DEPS) | sed -e 's/\\//\\\\\\//g'`; \
	$(CC) -MM $(INCLUDES) -o $@ $<; \
	sed -i "s/\(.*\)\.o[ :]*/$$FILE.o $$FILE.d $$FILE.pot: $$DEPS /g" $@; \
	DEPLIST=`sed 's/ \\\\//;s/.*://;' < $@`; \
	echo $$DEPLIST | fmt -1 | sed 's/ //g;s/\(.*\)/\1:/' >> $@; \
	[ -s $@ ] || $(RM) $@

unit-test: $(OBJECTS)
	@echo "    [LD] $@"
	$(Q) $(CC) -o $@ $+

.PHONEY: clean

clean:
	rm -f $(OBJECTS) $(DEPENDS) unit-test

-include $(DEPENDS)
