
TARGET = libpushclient.so

.PHONY:TARGET

HPATH = -I. -I../comm
OBJPATH = ../objs
SRCPATH = ./
SRCS = $(wildcard $(SRCPATH)/*.c)

OBJS = $(SRCS:%.c=%.o)

CFLAGS += -D__LITTLE_ENDIAN__ -DSUPPORT_EPOLL -g
CC = gcc
RM = rm -rf

all: $(TARGET) 
%.o: %.c
	$(CC) $(CFLAGS) $(HPATH) -c -o $@ $^

$(TARGET):$(OBJS)
	mv $^ $(OBJPATH)

#	$(CC) $(CFLAGS) $(HPATH) -fPIC -c $(SRCS)

clean:
	$(RM)  ./*.o
