all:
	rlwrap idris2 -q Main.idr -p contrib

exec:
	rlwrap idris2 -q Main.idr --exec main

push:
	git add -A
	git commit -m "$(msg)"
	git push -u origin

clean:
	rm -rf build
