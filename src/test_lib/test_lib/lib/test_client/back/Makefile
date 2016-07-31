

TARGET = libtc.a


.PHONY:TARGET

HPATH = -I$(CurLib)/lib_include 
SRCPATH = ./
SRCS = $(wildcard $(SRCPATH)/*.c)

OBJS = $(SRCS:%.c=%.o)

CFLAGS += -DSUPPORT_EPOLL -D__LITTLE_ENDIAN__ -DSUPPORT_EPOLL -g

CC = gcc
RM = rm -rf

all: $(TARGET) 
	cp *.a $(CommLib)
	cp *.h $(CommInc)

%.o: %.c
	$(CC) $(CFLAGS) $(HPATH) -c -o $@ $^

$(TARGET):$(OBJS)
	ar rcs $@ $^
#	$(CC) $(CFLAGS) $(HPATH) -o $@ $(OBJS) $(LDFLAGS)
#	cp $(TARGET) $(TARGET2)
	rm -rf ./*.o

clean:
	$(RM) $(TARGET)
	$(RM) $(TARGET2)
