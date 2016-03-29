
TARGET = $(TargetLib)/libcJSON.a

.PHONY:TARGET

HPATH = -I./
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
	@ar rcs $@ $^
	@rm -rf *.o

#	$(CC) $(CFLAGS) $(HPATH) -fPIC -c $(SRCS)

clean:
	$(RM)  ./*.o
	$(RM) $(TARGET)
