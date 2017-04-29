
IO_SELECT?=1
DEBUG_ENVLIST?=0

##############################################
CC?=gcc
Q?=@

TARGET=miniweb

# all *.c here
SRCS=$(wildcard *.c)
# in list $(SRCS) replace *.c as *.o
OBJS=$(patsubst %.c,%.o,$(SRCS))

CFLAGS=-Wall -g

ifdef TEST_SEND_STRESS
CFLAGS += -DTEST_SEND_STRESS
endif

ifeq ($(HOSTOS),macos)
CFLAGS += -DMACOS
#CFLAGS += -DHOSTOS=$(HOSTOS)
endif

ifeq ($(IO_SELECT),1)
CFLAGS += -DTRANS_IO_SELECT
endif

ifeq ($(DEBUG_ENVLIST),1)
CFLAGS += -DDEBUG_ENVLIST
endif

$(TARGET):$(OBJS)
	@echo ""
	@echo "Building $@..."
	$(Q)$(CC) $(LDFLAGS) $^ -o $@ -lpthread
	@echo "Done"

cleano:
	find . -name '*.o'|xargs rm -f

clean:cleano
	rm -f $(TARGET)

%.o:%.c
	@echo ""
	@echo "Compiling $@..."
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

killall:
	ps -ef|grep $(TARGET)|grep -v grep|cut -c 9-11|xargs sudo kill -9

dump:
	@echo "SRCS=$(SRCS)"
	@echo "OBJS=$(OBJS)"
	@echo "CFLAGS=$(CFLAGS)"

#
# cgi
#

.PHONY:cgi
cgi:
	cd cgi/;gcc -o cgi cgi.c

cleancgi:cleano
	rm -f ./cgi/cgi

cleanall:clean cleancgi
	@echo "All cleaned"
