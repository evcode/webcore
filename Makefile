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

$(TARGET):$(OBJS)
	@echo ""
	@echo "Building $@..."
	$(Q)$(CC) $(LDFLAGS) $^ -o $@ -lpthread
	@echo "Done"

clean:
	rm -rf $(TARGET)
	rm -rf *.o

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
