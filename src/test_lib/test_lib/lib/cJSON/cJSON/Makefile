
TARGET = libcJSON.a

HPATH = -I.
SRCPATH = ./
SRCS = $(wildcard $(SRCPATH)/*.c)

OBJS = $(SRCS:%.c=%.o)

CC = gcc
RM = rm -rf

all: $(TARGET)
	cp *.a $(CommLib)
	cp *.h $(CommInc)

%.o: %.c
	$(CC) $(CFLAGS) $(HPATH) -c -o $@ $^

$(TARGET):$(OBJS)
	ar rcs $@ $^
	rm -rf ./*.o

clean:
	$(RM) $(TARGET)
	$(RM) ./*.o
