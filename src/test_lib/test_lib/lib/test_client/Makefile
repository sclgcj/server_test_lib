
CurPro = $(proj)
export CurPro

#here is the basic path that the lib needed to 
#copy to the target directory
CurProInc = 
CurProLib = 

all: 
	mkdir -p $(CurDir)/$(CurPro)
	make -C $(CurPro)
	#cp $(CurProInc) $(CommInc)
	#cp $(CurProLib) $(CommInc)

clean:
	make -C $(CurPro) clean
