# riscv isa reference:
# https://msyksphinz-self.github.io/riscv-isadoc/html/rva.html

# timer interrupt info.
# https://danielmangum.com/posts/risc-v-bytes-timer-interrupts/
# https://riscv.org/wp-content/uploads/2016/07/Tue0900_RISCV-20160712-Interrupts.pdf

.option norvc
.section .data
.section .text.init

.global _start
_start:
# priv-isa-asciidoc_20240411.pdf
# 10.4.1: Addressing and Memory Protection Sv39
#
# paging interrupt setup
#
	# Setting our satp mode to Sv39 (8) (39-bit virtual addressing)
	li		t0, 8
	slli 	t0, t0, 60
 	# Gets the Physical Page Number of the Page Table
	la 		t1, _page_table_start
	srai 	t1, t1, 12
	or 		t0, t0, t1

	csrw 	satp, t0

#
# timer interrupt setup:
#
# 63 62-38 37  36 35-34 33-32 31-23 22  21 20  19  18 17   16-15 14-13 12-11 10-9  8   7    6    5   4    3   2    1    0
# SD WPRI MBE SBE SXL   UXL   WPRI TSR TW TVM MXR SUM MPRV  XS   FS    MPP   VS   SPP MPIE UBE SPIE WPRI MIE WPRI SIE WPRI
# 1   25   1   1   2    2      9    1  1   1   1   1   1    2    2     2     2     1   1   1    1    1    1    1   1    1
	# enable global interrupts for m and s modes
	# set previous privilege level to s
    li      t0, 0x1880
    csrw    mstatus, t0

	# start mtimecmp at 40k
    li      t0, 50000
    li      t1, 0x2004000
    sd      t0, 0(t1)


	# # delegate s interrupts to m
    li      t0, 0x20
    csrs    mideleg, t0

	# set trap handler location
    la      t0, mtrap
    csrw    mtvec, t0

	# uart init
	li  t0, 0x10000000
    li  t1, 0x3
    sb  t1, 0x3(t0)

	# enable m interrupts
    li      t0, 0xa0 
    csrs    mie, t0

	# set supervisor kernel location
	la      t0, enterkernel
  	csrw    mepc, t0
	li      t0, 0x80
    csrc    mip, t0
    mret

enterkernel:

	j main

mtrap:
#
# general trap handler:
#
# all registers will need to be saved including tX
	# parse mcause
	# 3.1.15. Machine Cause Register (mcause)
	csrr	t3, mcause
	li 		t2, 0
	blt 	t3, t2, interrupts

	# exception codes:
	# 0 Instruction address misaligned
	# 1 Instruction access fault
	# 2 Illegal instruction
	# 3 Breakpoint
	# 4 Load address misaligned
	# 5 Load access fault
	# 6 Store/AMO address misaligned
	# 7 Store/AMO access fault
	# 8 Environment call from U-mode
	# 9 Environment call from S-mode
	# 10 Reserved
	# 11 Environment call from M-mode
	# > 12 Instruction page fault
	# > 13 Load page fault
	# 14 Reserved
	# > 15 Store page fault
	# probably should use a switch statement for this
	mret

# debugging:
breakpoint:			# 3

# faults:
# Page needed to be loaded for an instruction.
instructionaccess:	# 1
# Tried to access unallowed memory.
loadaccess:			# 5
storeaccess: 		# 7
# Need to load/store pages for demand paging.
loadpage: 			# 13
storepage:			# 15
	#
	mret

interrupts:
	andi 	t3, t3, 0xf

	li		t2, 0x07
	beq 	t2, t3, timerhandler
	mret

timerhandler:
	# add 30k to mtimecmp
	li      t3, 0x2004000
	ld      t2, 0(t3)
	li		t4, 30000
	add 	t2, t2, t4
	sd		t2, 0(t3)

    mret

# uart
# https://github.com/safinsingh/ns16550a/blob/master/src/ns16550a.s
serial:

# kernel
main:
	la t1, _page_table_start
	li t0, 0x8000
	add t1, t1, t0
	lb t0, 0(t1)
	# Uart getchar:
# 	li  t0, 0x10000000

# uartloop:
# 	lb  	t1, 5(t0)
# 	andi 	t1, t1, 0x1
# 	beqz 	t1, nouart
	
# 	lb 		t1, 0(t0)
# 	sb 		t1, 0(t0)

# 	j uartloop

# nouart:
	j main


.align 12
# Sv39 page tables contain 2^9 Page Table Entries
_page_table_start:
	.skip 4096

	.size	_start, .-_start
	.ident	"GCC: (gc891d8dc3e) 13.2.0"
