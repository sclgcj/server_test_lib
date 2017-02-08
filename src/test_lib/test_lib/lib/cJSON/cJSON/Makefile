
TARGET = libcJSON.a

HPATH = -I.
SRCPATH = ./
SRCS = $(wildcard $(SRCPATH)/*.c)
HEADERPATH = $(wildcard ./*.h)
HEADER = $(notdir $(HEADERPATH))

OBJS = $(SRCS:%.c=%.o)

CC = gcc
RM = rm -rf

all: $(TARGET)
	echo $(HEADER)
	cp *.a $(CommLib)
	@for i in $(HEADERPATH); do \
		cp $$i $(CommInc); \
	done

%.o: %.c
	$(CC) $(CFLAGS) $(HPATH) -c -o $@ $^

$(TARGET):$(OBJS)
	ar rcs $@ $^
	rm -rf ./*.o

clean:
	$(RM) $(TARGET)
	$(RM) ./*.o
	for i in $(HEADER); do \
		rm -rf $(CommInc)/$$i; \
	done
	$(RM) $(CommLib)/$(TARGET)

