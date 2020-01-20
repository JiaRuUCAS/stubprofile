CC=gcc
CPPFLAGS=-Wall -g  -DTEST_CHECK -O2 -fPIC
#CPPFLAGS=-g -fPIC

SOURCE=$(wildcard *.c)
SOURCE_DEP=$(subst .c,.o,$(SOURCE))
SOURCE_DEP_D=$(subst .c,.d,$(SOURCE))

LDFLAGS= -lrt
OBJ=liblookup.so

%.d: %.c
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$;               \
	sed 's,\($*\)\.o[ :]*,\1.o $@: ,g' $@.$$$$ > $@; \
	rm -f $@.$$$$

all:
#-include $(SOURCE_DEP_D)
-include ts_s

.PHONY:all

all:$(OBJ) 


$(OBJ):$(SOURCE_DEP)
		$(CC) $(CPPFLAGS) -shared $^ -o $@ $(LDFLAGS)

.PHONY: clean install uninstall 
clean:
		rm -f *.d
		rm -f *.o 
		rm -f $(OBJ)
