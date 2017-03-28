CC?=gcc
Q?=@

TARGET=miniweb

# all *.c here
SRCS=$(wildcard *.c)
# in list $(SRCS) replace *.c as *.o
OBJS=$(patsubst %.c,%.o,$(SRCS))

CFLAGS=-Wall -g

$(TARGET):$(OBJS)
	@echo ""
	@echo "Building $@..."
	$(Q)$(CC) $(LDFLAGS) $^ -o $@
	@echo "Done"

clean:
	rm -rf $(TARGET)
	rm -rf *.o

%.o:%.c
	@echo ""
	@echo "Compiling $@..."
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

dump:
	@echo "SRCS=$(SRCS)"
	@echo "OBJS=$(OBJS)"
	@echo "CFLAGS=$(CFLAGS)"