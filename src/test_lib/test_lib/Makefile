
SHELL=bash
CFLAGS += -DDEBUG_LOG
ProjConf = proj_conf
CurDirPrefix = $(shell pwd)
CurDir = $(CurDirPrefix)/bin
CurLib = $(CurDirPrefix)/lib
CurPro = hh
ExistName = $(shell grep $(proj) $(CurDirPrefix)/$(ProjConf))
ExistData = 0
CurTime = $(shell date +%Y_%m_%d_%H_%M_%S)
proj_array=
brep=
rep=
comma = ,
#LDFLAGS += -L$(CurLib)/comm_lib/ -licudata -licuuc -ltoml

ServerArray = $(Server360)
#ServerArray  = $(GetUrl)
#ServerArray = $(GetUrl) $(Server360) $(SmartHome) 
#ServerArray = $(SmartHome)
#ServerArray = $(UpushServer)
#ServerArray = $(PushServer) $(NatServer) $(UdtServer) $(SmartHome)
CommArray = $(CurLib)

export CurDir CurLib CFLAGS CurPro
		
ifeq ($(proj),)
	ServerArray = $(shell cat $(CurDirPrefix)/$(ProjConf) | tr '\n' ' ')
else
	rep = $(shell grep -w $(proj) $(CurDirPrefix)/$(ProjConf) | tr '\n' ' ')
endif

comm = $(findstring $(comma),$(proj))
ifneq ($(comm),)
	proj_array=$(shell ./split.sh $(proj) | tr '\n' ' ')
	brep = $(rep)
else
	proj_array=$(rep)
endif

ifneq ($(proj_array),)
	ServerArray = $(proj_array)	
endif


ifeq ($(rep),)
	ExistData = 0
else
	ExistData = 1
endif

define set_proj_info 
	echo ---$(1); \
	rm -rf ./bin/$(1); \
	mkdir -p ./bin/$(1); \
	mkdir -p ./log/$(1); \
	pushd $(1);make proj=$(1);popd; \
	mkdir -p ./log/$(1); \
	mkdir -p ./packet/log/$(1); \
	cp -rf ./bin/$(1)/ ./packet/bin/; \
	cp -rf ./config/$(1)/ ./packet/config; \
	echo ./run.sh -t udt_mobile -p $(1) > ./mobile_$(1).sh; \
	chmod 777 ./mobile_$(1).sh; \
	echo ./run.sh -t udt_gateway -p $(1) > ./gateway_$(1).sh; \
	chmod 777 ./gateway_$(1).sh; \
	mv ./mobile_$(1).sh ./packet; \
	mv ./gateway_$(1).sh ./packet; \
	pushd ./packet;find -name .svn | xargs rm -rf;popd
endef

define enter_dir
	@echo $(1)
	@for i in $(1);do \
		$(call set_proj_info,$$i); \
	done
endef

define enter_dir_lib
	@echo $(1)
	@for i in $(1);do \
		$(call set_proj_info,$$i); \
	done
endef

define enter_dir_lib_old
	@for i in $(1);do \
		echo $$i; \
		make -C $$i; \
		mkdir -p ./packet/log/$$i; \
		cp -rf ./bin/$$i/ ./packet/bin/; \
		cp -rf ./redis/redis_qt ./packet/bin; \
		cp -rf ./config/$$i/ ./packet/config; \
		cp -rf ./config/comm/*.txt ./packet/config/comm; \
		echo ./run.sh -t udt_mobile -p $$i > ./mobile_$$i.sh; \
		chmod 777 ./mobile_$$i.sh; \
		echo ./run.sh -t udt_gateway -p $$i > ./gateway_$$i.sh; \
		chmod 777 ./gateway_$$i.sh; \
		cp ./mobile_$$i.sh ./packet; \
		cp ./gateway_$$i.sh ./packet; \
		cp -rf ./packet_lib/lib/comm_lib/*.so* ./packet/lib/comm_lib; \
		pushd ./packet;find -name .svn | xargs rm -rf;popd; \
	done
endef

define clean_dir
	@for i in $(1);do \
		pushd $$i;make proj=$$i clean;popd; \
		rm -rf packet_$$i.tar.xz; \
		rm -rf packet_lib_$$i.tar.xz; \
	done
endef

define create_new_packet
	rm -rf packet
	mkdir -p packet
	cp run.sh packet
	mkdir -p ./packet/lib/comm_lib
	mkdir -p ./packet/config/comm
	mkdir -p ./packet/bin/
	mkdir -p ./packet/log/
	cp -rf ./config/comm/*.txt ./packet/config/comm
endef

define tar_packet
	rm -rf packet.tar.xz
	tar -Jcf packet.tar.xz packet
endef

define tar_packet_lib
	cp ./lib/comm_lib/*.so* ./packet/lib/comm_lib/
	rm -rf packet_lib.tar.xz
	tar -cJf packet_lib.tar.xz packet
endef

define create_new_project_old
	@proj=$(1); \
	mkdir -p $(proj); \
	cp ./project_model/proj_root_makefile $(proj)/Makefile; \
	pushd $(proj);mkdir doc; mkdir $(proj);cp ../project_model/proj_src_makefile $(proj)/Makefile; popd; \
	echo $(proj) >> $(CurDirPrefix)/$(ProjConf)
endef

define create_new_project_ok
	data=""
	proj=$(1)
	echo $(ExistName)
	for i in $(ExistName);do \
		if [ "$$i" -eq "$(proj)" ];then \
			data=$(proj) \
		fi \
	done
	mkdir -p $(proj)
	echo $(data)
	if [ $(data) -eq "" ];then
		cp ./project_model/proj_root_makefile $(proj)/Makefile
		pushd $(proj);mkdir doc; mkdir $(proj);cp ../project_model/proj_src_makefile $(proj)/Makefile; popd
		echo $(proj) >> $(CurDirPrefix)/$(ProjConf)
	fi
endef


define create_new_project
	proj=$(1)
	@if [ $(ExistData) -eq 0 ];then \
		mkdir -p $(proj); \
		mkdir -p ./config/$(proj); \
		mkdir -p ./bin/$(proj); \
		mkdir -p ./log/$(proj); \
		cp ./project_model/proj_root_makefile $(proj)/Makefile; \
		pushd $(proj);mkdir doc; mkdir $(proj);cp ../project_model/proj_src_makefile $(proj)/Makefile; popd; \
		echo $(proj) >> $(CurDirPrefix)/$(ProjConf); \
	fi
endef

define create_script
	for i in $(1); do \
		echo "make proj=$$i pro_lib" > lib_$$i.sh; \
		chmod 777 lib_$$i.sh; \
		echo "make proj=$$i" > norm_$$i.sh; \
		chmod 777 norm_$$i.sh; \
		echo "make proj=$$i test" > test_$$i.sh; \
		chmod 777 test_$$i.sh;\
		echo "make proj=$$i clean" > clean_$$i.sh; \
		chmod 777 clean_$$i.sh; \
	done
endef

define create_new_lib
	proj = $(1); \	
	if [ -d ./lib/$(proj) ];then  \
		echo ./lib/$(proj) has existed; \
		exit; \
	else \
		mkdir -p ./lib/$(proj)/$(proj); \
	fi
	cp ./project_model/proj_root_makefile ./lib/$(proj)
endef	

all:
	echo $(ServerArray)
	$(call create_new_packet,)
	$(call enter_dir, $(ServerArray))
	$(call tar_packet,)

pro_lib:
	echo ---$(ServerArray)
	$(call create_new_packet,)
	$(call enter_dir, $(ServerArray))
	$(call tar_packet_lib,)
test:
	$(call enter_dir, $(ServerArray))
	cp run.sh ./packet/
script:
	$(call create_script, $(ServerArray))

push_server:
	make -C $(PushServer)
	
nat_server:
	make -C $(NatServer)

udt_server:
	make -C $(UdtServer)

sync:
	make -C $(SyncServer)

360:
	make -C $(Server360)
smart:
	make -C $(SmartHome)
upush:
	make -C $(UpushServer)

ldlib:
	@for i in $(CurLib); do \
		echo $$i; \
		make -C $$i; \
	done

new:
	$(call create_new_project,$(proj))
del:
	tar -cJf $(proj).$(CurTime).tar.xz $(proj)
	rm -rf $(proj)
	rm -rf ./config/$(proj)
	rm -rf ./log/$(proj)
	rm -rf ./bin/$(proj)
	@sed -i '/$(proj)/d' $(CurDirPrefix)/$(ProjConf)
new_lib: 
	$(call create_new_lib, $(proj))

clean:
	echo $(ServerArray)
	echo "proj = $(proj)"
	echo "rep = $(rep)"
	echo "proj_array = $(proj_array)"
	echo "brep = $(brep)"
	echo "comm = $(comm)"
	$(call clean_dir, $(ServerArray))
	@rm -rf ./packet/*

push_clean:
	make -C $(PushServer) clean

nat_clean:
	make -C $(NatServer) clean

udt_clean:
	make -C $(UdtServer) clean
sync_clean:
	make -C $(SyncServer) clean
360_clean:
	make -C $(Server360) clean

lib_clean:
	make -C lib clean

