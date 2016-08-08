
CURPATH = $(shell pwd)

curl_path = $(CURPATH)/curl_lib
care_path = $(CURPATH)/cares/target/


SHELL=bash

#	pushd ./curl-7.45.0/;./configure --prefix=$(curl_path) --enable-ares=$(care_path);make > /dev/null;make install > /dev/null; popd;
#	pushd ./curl-7.50.0/;./configure --prefix=$(curl_path) --enable-ares=$(care_path);make > /dev/null;make install > /dev/null; popd;
all:
	echo $(CommInc)
	cp ./curl_lib/curl $(CommInc) -rf
	cp ./curl_lib/*.a $(CommLib)

#clean:

	#make -C ./curl-7.45.0/ clean > /dev/null
