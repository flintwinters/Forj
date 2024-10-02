all:
	pack --extra-args "-q" repl src/Main.idr

exec:
	rlwrap idris2 -q Main.idr --exec main

rv:
	cd rv64;\
	riscv64-unknown-elf-gcc -static -nostdlib -g -ggdb -O0 main.c -o outmain;\
	qemu-riscv64 -g 2222 outmain &\
	gdb-multiarch -q -ex "source `pwd`/pyg.py";\
	pkill qemu-riscv64

obj:
	cd rv64
	riscv64-unknown-elf-objdump outmain

push:
	git add -A && \
	git commit -m "$(msg)" && \
	git push -u origin

clean:
	rm -rf build
