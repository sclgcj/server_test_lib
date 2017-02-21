SRC = cJSON_lib ml_lib icu_lib toml_lib

CurDir = $(shell pwd)
TargetLib = $(CurDir)/comm_lib
TInc = $(CurDir)/comm_include

export TargetLib TInc

SRC += $(PROJ)

all:
	echo $(SRC)
	for i in $(SRC);do \
		make -C $$i; \
	done

clean:
	for i in $(SRC);do \
		make -C $$i clean; \
	done
	rm -rf $(TargetLib)/*.so* $(TargetLib)/*.a
	rm -rf $(TInc)/*.h
