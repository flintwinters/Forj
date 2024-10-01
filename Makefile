all:
	rlwrap idris2 -q Main.idr -p contrib

exec:
	rlwrap idris2 -q Main.idr --exec main

rv:
	cd rv64;\
	riscv64-unknown-elf-gcc -static -nostdlib -g -ggdb -O0 main.c -o outmain;\
	qemu-riscv64 -g 2222 outmain &\
	gdb-multiarch -q -ex "source `pwd`/pyg.py";\
	pkill qemu-riscv64

asm:
	cd rv64;\
	riscv64-unknown-elf-as main.s -g -o outasm.o;\
	riscv64-unknown-elf-ld outasm.o -o outasm;\
	qemu-riscv64 -g 2222 outasm &\
	# riscv64-unknown-elf-gcc -static -nostdlib -g -ggdb -O0 main.c -S main.s;\
	gdb-multiarch -q -ex "source `pwd`/pyg.py";\
	pkill qemu-riscv64;\
	rm outasm.o

obj:
	cd rv64
	riscv64-unknown-elf-objdump outmain

push:
	git add -A
	git commit -m "$(msg)"
	git push -u origin

clean:
	rm -rf build
