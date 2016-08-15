
TARGET=min_heap

.PHONY:TARGET

HPATH = -I../../lib/lib_include
SRCPATH = ./
SRCS = $(wildcard $(SRCPATH)/*.c)

OBJS = $(SRCS:%.c=%.o)

CFLAGS += -DSUPPORT_EPOLL -D__LITTLE_ENDIAN__ -DSUPPORT_EPOLL -g
LDFLAGS += -L../../lib/comm_lib/ -lcJSON -lrt -lm  -lpthread

CC = gcc
RM = rm -rf

all: $(TARGET) 
%.o: %.c
	$(CC) $(CFLAGS) $(HPATH) -c -o $@ $^

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) $(HPATH) -o $@ $(OBJS) $(LDFLAGS)
#	cp $(TARGET) $(TARGET2)
	rm -rf ./*.o

clean:
	rm -rf ./*.o
	$(RM) $(TARGET)
	$(RM) $(TARGET2)
