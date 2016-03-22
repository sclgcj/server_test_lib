
TARGET=../udt_gateway_client

SRCPATH = .
SRCS=$(wildcard $(SRCPATH)/*.o)
HPATH = -I../../../lib/cJSON -I../../../lib/cur_lib/include -I../../../lib/libbase64 
CFLAGS = -DSUPPORT_EPOLL
LDFLAGS = -L../ -lst -lm -lcrypto -lpthread -lssl -lcrypto -lrt

#EOBJS = ../../../lib/objs/cJSON.o ../../../lib/objs/igd_base64.o

CC=gcc

all:$(TARGET)
	@echo $(CurDir)
	@echo $(SRCS)
$(TARGET):$(SRCS) 
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	rm -rf *.o
	rm -rf $(TARGET)

