
CurPro = $(proj)
export CurPro

#here is the basic path that the lib needed to 
#copy to the target directory
CurProInc = $(CurPro)/libtoml-master/*.h 
CurProLib = $(CurPro)/lib64/libtoml.so

all: 
	make -C $(CurPro)
#	cp $(CurProInc) $(CommInc)
#	cp $(CurProLib) $(CommLib)

clean:
	make -C $(CurPro) clean
