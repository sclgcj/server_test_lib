
TARGET = libpushclient.so

.PHONY:TARGET

CHPATH = -I. $(HPATH)
SRCPATH = ./
SRCS = $(wildcard $(SRCPATH)/*.c)

OBJS = $(SRCS:%.c=%.o)

CFLAGS += -D__LITTLE_ENDIAN__ -DSUPPORT_EPOLL -g
CC = gcc
RM = rm -rf

all: $(TARGET) 
%.o: %.c
	$(CC) $(CFLAGS) $(CHPATH) -c -o $@ $^

$(TARGET):$(OBJS)
	mv $^ $(OBJPATH)

clean:
	$(RM)  ./*.o
