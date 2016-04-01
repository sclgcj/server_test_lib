
TARGET= $(BINPATH)/$(PROJ)/gateway
TARGET2 = $(BINPATH)/$(PROJ)/mobile

.PHONY:TARGET

CHPATH = -I. $(HPATH)
SRCPATH = ./
SRCS = $(wildcard $(SRCPATH)/*.c)
LDFLAGS = $(LDPATH) -lml -lcJSON -lm -lcrypto -lpthread -lssl -lcrypto -lrt
CFLAGS  = -DSUPPORT_EPOLL $(CHPATH)

OBJS = $(SRCS:%.c=%.o)

CFLAGS += -D__LITTLE_ENDIAN__ -DSUPPORT_EPOLL -g
CC = gcc
RM = rm -rf

all: $(TARGET) $(TARGET2)
%.o: %.c
	$(CC) $(CFLAGS)  -c -o $@ $^

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
$(TARGET2):$(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM)  ./*.o
