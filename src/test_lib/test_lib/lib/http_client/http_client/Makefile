
TARGET = libhttp_client.so

.PHONY:TARGET

HPATH = -I$(CurLib)/lib_include
SRCPATH = ./
SRCS = $(wildcard $(SRCPATH)/*.c)
HEADER=$(shell ls *.h | grep -v private | tr '\n' ' ')

OBJS = $(SRCS:%.c=%.o)

CFLAGS += -DSUPPORT_EPOLL -D__LITTLE_ENDIAN__ -DSUPPORT_EPOLL -g -fPIC
SFLAG = -shared

CC = gcc
RM = rm -rf

all: $(TARGET) 
	strip $(TARGET)
	mv *.so $(CommLib)
	cp $(HEADER) $(CommInc)
%.o: %.c
	$(CC) $(CFLAGS) $(HPATH) -c -o $@ $^

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) $(SFLAG) $(HPATH) -o $@ $(OBJS)
	rm -rf ./*.o

clean:
	rm -rf ./*.o
	$(RM) $(TARGET)
	@for i in $(HEADER); do \
		rm -rf $(CommInc)/$$i; \
	done
	$(RM) $(CommLib)/$(TARGET)
