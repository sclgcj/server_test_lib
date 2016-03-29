SRCPATH = manage 
OBJPATH = 
BINPATH = $(shell pwd)/bin
CONFPATH = $(shell pwd)/conf
LDPATH  = -L $(shell pwd)/lib/comm_lib
HPATH   = -I$(shell pwd)/lib/cJSON -I$(shell pwd)/lib/ml_lib
PROJ    = 

export OBJPATH BINPATH LDPATH HPATH PROJ

all:
	for i in $(SRCPATH);do \
		PROJ=$$i; \
		mkdir -p $(BINPATH)/$$PROJ; \
		mkdir -p $(CONFPATH)/$$PROJ; \
		make -C $$i; \
	done

manage:
	PROJ = manage
	mkdir -p $(BINPATH)/$(PROJ)
	mkdir -p $(CONFPATH)/$(PROJ)
	make -C manage

clib:
	make -C lib

clean:
	rm -rf gateway_client
	for i in $(SRCPATH);do \
		make -C $$i clean; \
	done

lib_clean:
	make -C lib clean

m_clean:
	PROJ = manage
	make -C manage clean


