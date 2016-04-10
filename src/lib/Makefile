SRC = cJSON_lib ml_lib #toml_lib

CurDir = $(shell pwd)
TargetLib = $(CurDir)/comm_lib

export TargetLib

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
