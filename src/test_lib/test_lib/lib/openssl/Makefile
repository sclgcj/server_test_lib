
CurPro = $(proj)
export CurPro

#here is the basic path that the lib needed to 
#copy to the target directory
CurProInc = $(CurPro)/openssl/ 
CurProLib = $(CurPro)/*.a $(CurPro)/*.so*

all: 
	make -C $(CurPro)
	#cp -rf $(CurProInc) $(CommInc)
	#cp -rf $(CurProLib) $(CommLib)

clean:
	make -C $(CurPro) clean
