
#TRANS_IO?=IO_SELECT
TRANS_IO?=IO_EPOLL
TASKPOOL?=1

DEBUG_ENVLIST?=0

##############################################
CC?=gcc
Q?=@

TARGET=miniweb

# all *.c here
ALL_SRCS=$(wildcard *.c)
# excludes some files
SRCS=$(filter-out $(EXCLUDES), $(ALL_SRCS))
# in list $(SRCS) replace *.c as *.o
OBJS=$(patsubst %.c,%.o,$(SRCS))

CFLAGS=-Wall -g

ifdef TEST_SEND_STRESS
CFLAGS += -DTEST_SEND_STRESS
endif

ifeq ($(HOSTOS),macos)
CFLAGS += -DMACOS
#CFLAGS += -DHOSTOS=$(HOSTOS)

TRANS_IO=IO_SELECT
endif

ifeq ($(TRANS_IO),IO_SELECT)
CFLAGS += -DTRANS_IO=$(TRANS_IO)
# it's EXCLUDE!!!
EXCLUDES=trans_epoll.c
else ifeq ($(TRANS_IO),IO_EPOLL)
CFLAGS += -DTRANS_IO=$(TRANS_IO)
# it's EXCLUDE!!!
EXCLUDES=trans_io.c
else
# it's EXCLUDE!!!
EXCLUDES=trans_epoll.c trans_io.c trans_io_impl.c
endif

ifeq ($(TASKPOOL),1)
CFLAGS += -DENABLE_TASKPOOL
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
	@echo "ALL =$(ALL_SRCS)"
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
