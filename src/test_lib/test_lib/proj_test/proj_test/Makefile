
TARGET=$(CurDir)/$(CurPro)/udt_gateway_client
TARGET2=$(CurDir)/$(CurPro)/udt_mobile_client

.PHONY:TARGET

HPATH = -I$(CurLib)/lib_include
SRCPATH = ./
SRCS = $(wildcard $(SRCPATH)/*.c)

OBJS = $(SRCS:%.c=%.o)

CFLAGS += -DSUPPORT_EPOLL -D__LITTLE_ENDIAN__ -DSUPPORT_EPOLL -g
LDFLAGS += -L$(CurLib)/comm_lib/ -ltc -licudata -licuuc -ltoml  -lrt -lm  -lpthread

CC = gcc
RM = rm -rf

all: $(TARGET) 
%.o: %.c
	$(CC) $(CFLAGS) $(HPATH) -c -o $@ $^

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) $(HPATH) -o $@ $(OBJS) $(LDFLAGS)
	cp $(TARGET) $(TARGET2)
	rm -rf ./*.o

clean:
	rm -rf ./*.o
	$(RM) $(TARGET)
	$(RM) $(TARGET2)
