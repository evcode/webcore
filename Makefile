TARGET=miniweb
SRCS=
OBJS=
CFLAGS=-Wall -g



%.o:%.c
	gcc $(CFLAGS) -c $^ -o $@



clean:
	rm -rf $(TARGET)
	rm -rf *.o
