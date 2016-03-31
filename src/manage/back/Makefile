
SRCPATH = src

OBJPATH = $(shell pwd)/objs

all:
	for i in $(SRCPATH);do \
		echo $$i;  \
		make -C $$i; \
	done

clean:
	rm -rf gateway_client
	for i in $(SRCPATH);do \
		make -C $$i clean; \
	done



