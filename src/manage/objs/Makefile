
TARGET= $(BINPATH)/$(PROJ)/gateway

SRCPATH = .
SRCS=$(wildcard $(SRCPATH)/*.o)
CFLAGS = -DSUPPORT_EPOLL $(HPATH)
LDFLAGS = $(LDPATH) -lml -lm -lcrypto -lpthread -lssl -lcrypto -lrt

CC=gcc

all:$(TARGET)
	@echo $(CurDir)
	@echo $(SRCS)
$(TARGET):$(SRCS) 
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	rm -rf *.o
	rm -rf $(TARGET)

