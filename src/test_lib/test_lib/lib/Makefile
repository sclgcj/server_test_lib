SHELL=bash
PushServer = ./push_server/
NatServer = ./nat_server/
UdtServer = ./udt_nat_server/
Lib = ./lib/

JSON = ./cJSON
BASE64 = ./libbase64/
UDT = ./udt_lib/ UTIL = ./utils/
CURL = ./curl/
TOML = ./toml_lib/
ICU = ./icu_lib/
OPENSSL = ./openssl/

CurDirPrefix = $(shell pwd)
ProjConf = lib_conf

rep=
brep=

#ServerArray = libbase64 utils curl openssl cJSON icu_lib toml test_client 
ifeq ($(proj),)
	ServerArray = $(shell cat $(CurDirPrefix)/$(ProjConf) | tr '\n' ' ')
else
	upper_proj=$(shell echo $(proj) | tr '[a-z]' '[A-Z]')
	rep = $(shell grep -w $(proj) $(CurDirPrefix)/$(ProjConf) | tr '\n' ' ')
endif

comm = $(findstring $(comma),$(proj))
ifneq ($(comm),)
	proj_array=$(shell ../split.sh $(proj) | grep tr '\n' ' ')
	brep = $(rep)
else
	proj_array=$(rep)
endif

ifneq ($(proj_array),)
	ServerArray = $(proj_array)
endif

#$(JSON) $(BASE64) $(CURL) $(UTIL)  $(TOML) $(ICU) $(OPENSSL) $(UDT)
#ServerArray = ./toml_lib/

CommLib = $(CurLib)/comm_lib
CommInc = $(CurLib)/lib_include

export CommLib CommInc

all: 
	@echo "server Array $(ServerArray)"
	@for i in $(ServerArray); do \
		make -C $$i proj=$$i; \
	done

Lib:
	@for i in $(Lib); do \
		echo $$i; \
		make -C $$i; \
	done

clean: 
	@for i in $(ServerArray); do \
		make -C $$i clean proj=$$i; \
done
