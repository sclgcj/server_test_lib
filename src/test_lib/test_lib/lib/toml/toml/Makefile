
libpath=./lib64/libtoml.so
lib = libtoml.so
headerpath=$(shell ls ./libtoml-master/*.h | grep -v private | tr '\n' ' ')
header = $(notdir $(headerpath))

all:
	#pushd libtoml-master;cmake -G "Unix Makefiles" .; popd;
	#make -C ./libtoml-master/	
	#mv ./libtoml-master/libtoml.* ../comm_lib

	cp ./lib64/libtoml.so $(CommLib)
	cp $(headerpath) $(CommInc)


clean:
	rm -rf $(CommLib)/$(lib)
	@for i in $(header); do \
		rm -rf $(CommInc)/$$i; \
	done

