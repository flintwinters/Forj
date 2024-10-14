all:
	rlwrap pack --extra-args "-q" repl src/Main.idr

exec:
	rlwrap pack --extra-args "-q" exec src/Main.idr

# https://joe-degs.github.io/systems/2022/06/22/remote-debugging-gdb-qemu.html
# https://mth.st/blog/riscv-qemu/
# step by step walkthrough on loading riscv qemu
rv:
	cd rv64 && \
	riscv64-unknown-elf-gcc -static -nostdlib -g -ggdb -O0 -ffreestanding -Wall -Wextra -Werror -c main.c -o outmain.o && \
	riscv64-unknown-elf-as setup.s -g -o setup.o &&\
	riscv64-unknown-elf-gcc -T \
		linker.ld \
		-o forjos \
		-ffreestanding \
		-O0 \
		-static \
		-nostdlib \
		setup.o \
		outmain.o \
		-lgcc && \
	( \
	qemu-system-riscv64 \
		-s \
		-machine virt \
		-cpu rv64 \
		-nographic \
		-serial mon:stdio \
		-bios none \
		-kernel forjos \
	# gdb-multiarch -l 2 -q forjos -ex "source `pwd`/pyg.py" \
	)
	# pkill -f qemu-system-riscv64

asm:
	cd rv64 && \
	riscv64-unknown-elf-gcc -S -static -nostdlib -g -ggdb -O0 -ffreestanding -Wall -Wextra -Werror -c main.c -o asmmain.s
	
obj:
	cd rv64
	riscv64-unknown-elf-objdump outmain

push:
	git add -u
	git commit -m "$(msg)"
	git push

clean:
	rm -rf build
