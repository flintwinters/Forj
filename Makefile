all:
	idris2 -q forj.idr

exec:
	idris2 -q forj.idr --exec main

push:
	git add -A
	git commit -m "$(msg)"
	git push -u origin

clean:
	rm -rf build