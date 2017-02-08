
CURPATH = $(shell pwd)

curl_path = $(CURPATH)/curl_lib
care_path = $(CURPATH)/cares/target/

ARLIBPATH = $(wildcard ./curl_lib/*.a)
ARLIB = $(notdir $(ARLIBPATH))
HEADER = curl


SHELL=bash

#	pushd ./curl-7.45.0/;./configure --prefix=$(curl_path) --enable-ares=$(care_path);make > /dev/null;make install > /dev/null; popd;
#	pushd ./curl-7.50.0/;./configure --prefix=$(curl_path) --enable-ares=$(care_path);make > /dev/null;make install > /dev/null; popd;
all:
	echo $(ARLIB)
	cp ./curl_lib/$(HEADER) $(CommInc) -rf
	@for i in $(ARLIBPATH); do \
		cp $$i $(CommLib); \
	done

clean:
	rm -rf $(CommInc)/$(HEADER)
	@for i in $(ARLIB); do \
		rm -rf $(CommLib)/$$i; \
	done
	#make -C ./curl-7.45.0/ clean > /dev/null
