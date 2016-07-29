SRCPATH = manage 
OBJPATH = 
CURPATH = $(shell pwd)
BINPATH = $(CURPATH)/bin
CONFPATH = $(CURPATH)/conf
LDPATH  = -L $(CURPATH)/lib/comm_lib
HPATH  = -I$(CURPATH)/lib/comm_include
PROJ   = 

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
	rm -rf $(BINPATH)/$(PROJ)
	make -C manage clean

mlib_clean:
	PROJ=manage
	make -C lib clean


