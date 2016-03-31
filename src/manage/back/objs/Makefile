

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
	rm -rf $(TARGET2)
	ln -s $@ $(TARGET2)

clean:
	rm -rf *.o
	rm -rf $(TARGET)

