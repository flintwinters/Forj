# https://joe-degs.github.io/systems/2022/06/22/remote-debugging-gdb-qemu.html
# https://mth.st/blog/riscv-qemu/
# step by step walkthrough on loading riscv qemu
all: kerneldebugc
	@cd rv64 && \
	( \
	qemu-system-riscv64 \
		-s -S \
		-machine virt \
		-cpu rv64 \
		-nographic \
		-serial mon:stdio \
		-bios none \
		-kernel forjos \
		-drive format=raw,file=floppy.img,if=none,id=hd0 \
		-device virtio-blk-device,drive=hd0 &\
		gdb-multiarch \
			-l 2 -q forjos \
			-x `pwd`/pyg.py \
			-ex "py connect('$(CHALLENGE)')" \
	)
	pkill -f qemu-system-riscv64

kernel: fj
	@cd rv64 && \
	fj kernel.fj && \
	riscv64-unknown-elf-as setup.s -g -o setup.o &&\
	riscv64-unknown-elf-gcc \
		-mcmodel=medany \
		-T linker.ld \
		-O0 \
		-c alloc.c \
		-o alloc.s \
		-ffreestanding \
		-static -nostdlib -lgcc \
		-S -fverbose-asm && \
	riscv64-unknown-elf-as alloc.s -g -o alloc.o &&\
	riscv64-unknown-elf-gcc \
		-mcmodel=medany \
		-T linker.ld \
		setup.o \
		alloc.o \
		-o forjos \
		-ffreestanding \
		-O0 \
		-static \
		-nostdlib \
		-lgcc

kerneldebugc: fj 
	@cd rv64 && \
	fj kernel.fj && \
	riscv64-unknown-elf-as setup.s -g -o setup.o &&\
	riscv64-unknown-elf-gcc \
		-mcmodel=medany \
		-T linker.ld \
		-O0 -g \
		-c ../forj/fj.c \
		-o fj.o \
		-ffreestanding \
		-static -nostdlib -lgcc \
		-fverbose-asm && \
	riscv64-unknown-elf-gcc \
		-mcmodel=medany \
		-T linker.ld \
		setup.o \
		fj.o \
		-o forjos \
		-ffreestanding \
		-O0 \
		-static \
		-nostdlib \
		-lgcc

challenge: fjchall rvchall

rvchall:
	@cd rv64 && \
	python3 challenge.py

rv:
	@cd rv64 && \
	fj kernel.fj && \
	riscv64-unknown-elf-as setup.s -o setup.o &&\
	riscv64-unknown-elf-gcc -T \
		linker.ld \
		-o forjos \
		-ffreestanding \
		-O0 \
		-static \
		-nostdlib \
		setup.o \
		-lgcc && \
	( \
	qemu-system-riscv64 \
		-machine virt \
		-cpu rv64 \
		-nographic \
		-serial mon:stdio \
		-bios none \
		-kernel forjos \
		-drive format=raw,file=floppy.img,if=none,id=hd0 \
		-device virtio-blk-device,drive=hd0 \
	)

fjrun:
	cd forj && \
	g++ -g main.cpp -o fj && \
	gdb -q -ex "source `pwd`/pyg.py" --args ./fj test.qfj 


fjchall:
	@cd forj && \
	g++ -g main.cpp -o fj && \
	python3 challenge.py $(CHALLENGE) || \
	gdb -q -ex "source `pwd`/pyg.py" --args ./fj challenge

fj:
	@cd forj && \
	g++ -g main.cpp -o fj

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
