# riscv isa reference:
# https://msyksphinz-self.github.io/riscv-isadoc/html/rva.html

# timer interrupt info.
# https://danielmangum.com/posts/risc-v-bytes-timer-interrupts/
# https://riscv.org/wp-content/uploads/2016/07/Tue0900_RISCV-20160712-Interrupts.pdf

.section .text.init

.global _start
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
	# enable global interrupts for m and s modes
	# set previous privilege level to s
    li      t0, 0x1882
    csrw    mstatus, t0

	# start mtimecmp at 40k
    li      t0, 30000
    li      t1, 0x2004000
    sd      t0, 0(t1)


	# delegate m interrupts to s
    li      t0, 0x20
    csrs    mideleg, t0

	# set trap handler location
    la      t0, mtrap
    csrw    mtvec, t0

	# enable m interrupts
    li      t0, 0xa0 
    csrs    mie, t0

	# set supervisor kernel location
	la      t0, main
  	csrw    mepc, t0
	li      t0, 0x80
    csrc    mip, t0
    mret

mtrap:
#
# general trap handler:
#
# all register need to be saved including tX
	# parse mcause
	# 3.1.15. Machine Cause Register (mcause)
	csrr	t1, mcause
	srli 	t0, t1, 0x3f
	bnez 	t0, interrupts

	# non-interrupt codes:
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
	# 12 Instruction page fault
	# 13 Load page fault
	# 14 Reserved
	# 15 Store page fault
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
	andi 	t1, t1, 0xf

	li		t0, 0x07
	beq 	t0, t1, timerhandler
	mret
	
timerhandler:
	# unset timer flag
	li      t0, 0x80
    csrc    mip, t0
	
	# add 30k to mtimecmp
	li      t1, 0x2004000
	ld      t0, 0(t1)
	li		t2, 30000
	add 	t0, t0, t2
	sd		t0, 0(t1)

    mret

# kernel
main:
	j main

.align 12
# Sv39 page tables contain 2^9 Page Table Entries
_page_table_start:
	.skip 4096

	.size	_start, .-_start
	.ident	"GCC: (gc891d8dc23e) 13.2.0"
