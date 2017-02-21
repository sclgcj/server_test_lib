
arrpath = $(wildcard ./*.so*)
arr = $(notdir $(arrpath))

all:
	echo $(arr)
	#pushd libtoml-master;cmake -G "Unix Makefiles" .; popd;
	#make -C ./libtoml-master/	
	#mv ./libtoml-master/libtoml.* ../comm_lib
	@for i in $(arrpath); do \
		cp $$i $(TargetLib); \
	done


clean:
	@for i in $(arr); do \
		rm -rf $(TargetLib)/$$i; \
	done
#	make -C ./libtoml-master/ clean
#	rm -rf ../comm_lib/libtoml.*

