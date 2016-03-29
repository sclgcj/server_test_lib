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
	PROJ=manage
	mkdir -p $(BINPATH)/$(PROJ)
	mkdir -p $(CONFPATH)/$(PROJ)
	make -C manage

clib:
	make -C lib

m_lib:
	PROJ=manage
	make -C lib

clean:
	for i in $(SRCPATH);do \
		rm -rf $(BINPATH)/$$i; \
		make -C $$i clean; \
	done

lib_clean:
	make -C lib clean

m_clean:
	PROJ=manage
	rm -rf $(BINPATH)/$$PROJ
	make -C manage clean

mlib_clean:
	PROJ=manage
	make -C lib clean


