all:
	rlwrap idris2 -q src/Main.idr -p contrib

exec:
	rlwrap pack --extra-args "-q" exec src/Main.idr

# https://mth.st/blog/riscv-qemu/
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
		-s -S \
		-machine virt \
		-cpu rv64 \
		-m 128M \
		-nographic \
		-serial mon:stdio \
		-bios none \
		-kernel forjos & \
	gdb-multiarch -l 2 -q forjos -ex "source `pwd`/pyg.py" \
	)
	pkill -f qemu-system-riscv64

asm:
	cd rv64 && \
	riscv64-unknown-elf-gcc -S -static -nostdlib -g -ggdb -O0 -ffreestanding -Wall -Wextra -Werror -c main.c -o asmmain.s
	
# asm:
# 	cd rv64 &&\
# 	riscv64-unknown-elf-as main.s -g -o outasm.o &&\
# 	riscv64-unknown-elf-ld outasm.o -o outasm &&\
# 	(qemu-riscv64 -g 2222 outasm &\
# 	gdb-multiarch -q -ex "source `pwd`/pyg.py";\
# 	pkill qemu-riscv64;\
# 	rm outasm.o)

obj:
	cd rv64
	riscv64-unknown-elf-objdump outmain

push:
	git add -A
	git commit -m "$(msg)"
	git push -u origin

clean:
	rm -rf build
