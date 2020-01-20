CC=gcc
CPPFLAGS=-Wall -g  -DTEST_CHECK
#CPPFLAGS=-g

SOURCE=$(wildcard *.c)
SOURCE_DEP=$(subst .c,.o,$(SOURCE))
SOURCE_DEP_D=$(subst .c,.d,$(SOURCE))

LDFLAGS= -lrt -lpthread -llookup -L/home/jr/program/test-probe/lib -llookup
OBJ=test-probe

LDFLAGS+= -Wl,-rpath=/home/jr/program/test-probe/lib

%.d: %.c
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$;               \
	sed 's,\($*\)\.o[ :]*,\1.o $@: ,g' $@.$$$$ > $@; \
	rm -f $@.$$$$

all:
-include $(SOURCE_DEP_D)

.PHONY:all

all:$(OBJ) 

$(OBJ):$(SOURCE_DEP)
		$(CC) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean install uninstall 
clean:
		rm -f *.d
		rm -f *.o 
		rm -f $(OBJ)
