all:
	idris2 -q forj.idr --exec main

push:
	git add -A
	git commit -m "$(msg)"

clean:
	rm -rf build