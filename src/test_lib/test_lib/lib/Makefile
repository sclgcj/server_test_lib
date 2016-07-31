PushServer = ./push_server/
NatServer = ./nat_server/
UdtServer = ./udt_nat_server/
Lib = ./lib/

JSON = ./cJSON
BASE64 = ./libbase64/
UDT = ./udt_lib/
UTIL = ./utils/
CURL = ./curl/
TOML = ./toml_lib/
ICU = ./icu_lib/
OPENSSL = ./openssl/

ServerArray = cJSON icu_lib toml test_client 

#$(JSON) $(BASE64) $(CURL) $(UTIL)  $(TOML) $(ICU) $(OPENSSL) $(UDT)
#ServerArray = ./toml_lib/

CommLib = $(CurLib)/comm_lib
CommInc = $(CurLib)/lib_include

export CommLib CommInc

all: 
	@for i in $(ServerArray); do \
		make -C $$i proj=$$i; \
	done
#	cp ./udt_lib/include/list* ./lib_include/
#	cp ./openssl/*.so* ./comm_lib/
#	cp udt_lib/libtrans_c.a comm_lib
#	cp udt_lib/server_udt/src/libudt.so comm_lib
#	cp udt_lib/server_udt/src/libudt.so /usr/lib

Lib:
	@for i in $(Lib); do \
		echo $$i; \
		make -C $$i; \
	done

clean: 
	rm -rf ./comm_lib/* ./cur_lib/*
	rm -rf ./lib_include/*
	@for i in $(ServerArray); do \
		make -C $$i clean proj=$$i; \
	done

