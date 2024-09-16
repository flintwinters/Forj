all:
	idris2 -q forj.idr --exec main

clean:
	rm -rf build