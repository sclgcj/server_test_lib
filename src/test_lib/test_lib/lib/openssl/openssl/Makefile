
arlibpath = $(wildcard ./*.a)
solibpath = $(wildcard ./*.so*)
arlib = $(notdir $(arlibpath))
solib = $(notdir $(solibpath))
header = openssl


all:
	cp $(arlibpath) $(CommLib)
	cp $(solibpath) $(CommLib)
	cp $(header) $(CommInc) -rf

clean:
	rm -rf $(CommInc)/$(header)
	@for i in $(arlib); do \
		rm -rf $(CommLib)/$$i; \
	done
	
	@for i in $(solib); do \
		rm -rf $(CommLib)/$$i; \
	done
