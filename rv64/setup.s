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
#
# timer interrupt setup:
#
	# enable global interrupts for m and s modes
	# set previous privilege level to s
    li      t0, 0x1882
    csrw    mstatus, t0

	# start mtimecmp at 40k
    li      t0, 50000
    li      t1, 0x2004000
    sd      t0, 0(t1)

# Allocating 1 kernel page
	call kernelpagesetup
	li		a0, 0
	call virtualtophysical

	# delegate m interrupts to s
    li      t0, 0x20
    csrs    mideleg, t0

	# set trap handler location
    la      t0, mtrap
    csrw    mtvec, t0

	# uart init
	li  t0, 0x10000000
    li  t1, 0x3
    sb  t1, 0x3(t0)

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

	# enable m interrupts
    li      t0, 0xa0 
    csrs    mie, t0
	# set supervisor kernel location
	la      t0, main
  	csrw    mepc, t0

    mret

virtualtophysical:
# 10.3.2. Virtual Address Translation Process
# A virtual address va is translated into a physical address pa as follows:
# 1. Let a be satp.ppn×PAGESIZE, and let i=2. The
# satp register must be active, i.e., the effective privilege mode must be S-mode or U-mode.
# 2. Let pte be the value of the PTE at address a+va.vpn[i]×8.
# If accessing pte violates a PMA or PMP check, raise an access-fault exception corresponding to the
# original access type.
# 3. If pte.v=0, or if (pte.r=0 and pte.w=1), or if any bits or encodings that are reserved for future standard
# use are set within pte, stop and raise a page-fault exception corresponding to the original access
# type.
# 63 62-61 60-54   53-28  27-19 18-10   9-8  7  6  5  4  3  2  1  0
# N PBMT Reserved PPN[2] PPN[1] PPN[0] RSW  D  A  G  U  X  W  R  V
# 1  2 		7 		26 		9 		9   2   1  1  1  1  1  1  1  1

# a0 = virtual address
	la		t0, _page_table_start
	
	li		t2, 0xf
	li 		t3, 30
	# li		t4, 2

pageloop:
	srl		t1, a0, t3
	addi	t3, t3, -9
	# addi	t4, t4, -1
	andi	t1, t1, 0x1ff
	slli	t1, t1, 3
	
	add		t0, t0, t1
	lw		t0, 0(t0) # get the PTE

	# and  	t1, t0, t2
	# bne		t1, t2, pageerror

	srli	a0, t0, 10
	slli	a0, a0, 12
	# j 		pageloop
	
pageerror:
	ret

kernelpagesetup:
	la		t0, _page_table_start
	srli	t1, t0, 2
	addi	t1, t1, 0x40f
	sw		t1, 0(t0)
	ret

# 4. Otherwise, the PTE is valid. If pte.r=1 or pte.x=1, go to step 5. Otherwise, this PTE is a pointer to the
# next level of the page table. Let i=i-1. If i<0, stop and raise a page-fault exception corresponding to
# the original access type. Otherwise, let a=pte.ppn×8 and go to step 2.
# 5. A leaf PTE has been found. Determine if the requested memory access is allowed by the pte.r, pte.w,
# pte.x, and pte.u bits, given the current privilege mode and the value of the SUM and MXR fields of
# the mstatus register. If not, stop and raise a page-fault exception corresponding to the original
# access type.
# 6. If i>0 and pte.ppn[i-1:0] ≠ 0, this is a misaligned superpage; stop and raise a page-fault exception
# corresponding to the original access type.
# 7. If pte.a=0, or if the original memory access is a store and pte.d=0:
# ◦ If the Svade extension is implemented, stop and raise a page-fault exception corresponding to
# the original access type.
# ◦ If a store to pte would violate a PMA or PMP check, raise an access-fault exception
# corresponding to the original access type.
# ◦ Perform the following steps atomically:
# ▪ Compare pte to the value of the PTE at address a+va.vpn[i]×PTESIZE.
# ▪ If the values match, set pte.a to 1 and, if the original memory access is a store, also set pte.d
# to 1.
# ▪ If the comparison fails, return to step 2.
# 8. The translation is successful. The translated physical address is given as follows:
# ◦ pa.pgoff = va.pgoff.
# ◦ If i>0, then this is a superpage translation and pa.ppn[i-1:0] = va.vpn[i-1:0].
# ◦ pa.ppn[LEVELS-1:i] = pte.ppn[LEVELS-1:i].


mtrap:
#
# general trap handler:
#
# all registers will need to be saved including tX
	# unset interrupt pending
	# li      t0, 0xa0
    # csrc    mip, t0

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
	andi 	t1, t1, 0xf

	li 		t0, 3
	beq 	t0, t1, breakpoint
	li 		t0, 1
	beq 	t0, t1, instructionaccess
	li 		t0, 5
	beq 	t0, t1, loadaccess
	li 		t0, 7
	beq 	t0, t1, storeaccess
	li 		t0, 13
	beq 	t0, t1, loadpage
	li 		t0, 15
	beq 	t0, t1, storepage

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
	# add 30k to mtimecmp
	li      t1, 0x2004000
	ld      t0, 0(t1)
	li		t2, 30000
	add 	t0, t0, t2
	sd		t0, 0(t1)

    mret

# uart
# https://github.com/safinsingh/ns16550a/blob/master/src/ns16550a.s
serial:
	# interrupts = <0x0a>;
	# interrupt-parent = <0x03>;
	# clock-frequency = "\08@";
	# reg = <0x00 0x10000000 0x00 0x100>;
	# compatible = "ns16550a";

# kernel
main:
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

	# li		t0, 0x100000000
	# lb		t0, 0(t0)
	j main


# align to 2^12 = 4096
# Sv39 page tables contain 2^9 Page Table Entries
.align 12
_page_table_start:
	.skip 4096

.size	_start, .-_start
.ident	"GCC: (gc891d8dc3e) 13.2.0"
