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
#
# timer interrupt setup:
#
# 63 62-38 37  36 35-34 33-32 31-23 22  21 20  19  18 17   16-15 14-13 12-11 10-9  8   7    6    5   4    3   2    1    0
# SD WPRI MBE SBE SXL   UXL   WPRI TSR TW TVM MXR SUM MPRV  XS   FS    MPP   VS   SPP MPIE UBE SPIE WPRI MIE WPRI SIE WPRI
# 1   25   1   1   2    2      9    1  1   1   1   1   1    2    2     2     2     1   1   1    1    1    1    1   1    1
	la sp, mstack
	
	# enable global interrupts for m and s modes
	# set previous privilege level to s
    li      t0, 0x0880
    csrs    mstatus, t0

	# start mtimecmp at 40k
    li      t0, 50000
    li      t1, 0x2004000
    sd      t0, 0(t1)

# Allocating 1 kernel page
	# Set pmpcfg0 to allow read/write/exec of a physical memory region
	la t0, pagestart
	csrw pmpaddr0,t0
	li t0, 0x0f
	csrw pmpcfg0,t0

	# la t0, pagestart
	# slli t0, t0, 1
	# csrw pmpaddr1,t0
	# li t0, 0x0f
	# csrw pmpcfg1,t0

	sfence.vma x0, x0

	# set trap handler location
    la      t0, mtrap
    csrw    mtvec, t0

	la      t0, mtrap
    csrw    stvec, t0

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
	la 		t1, pagestart
	srai 	t1, t1, 12
	or 		t0, t0, t1
	csrw 	satp, t0

	# enable m interrupts
    li      t0, 0xa0 
    csrs    mie, t0
	# set supervisor kernel location
	la      t0, enterkernel
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
# 38-30  29-21   20-12  11-0
# VPN[2] VPN[1] VPN[0] page-offset
# 9      9      9      12
# 55-30   29-21  20-12  11-0
# PPN[2] PPN[1] PPN[0] page-offset
# 26     9      9      12
# 63 62-61 60-54   53-28  27-19 18-10   9-8  7  6  5  4  3  2  1  0
# N PBMT Reserved PPN[2] PPN[1] PPN[0] RSW  D  A  G  U  X  W  R  V
# 1  2 		7 		26 		9 		9   2   1  1  1  1  1  1  1  1

# a0 = virtual address
	la		t0, pagestart
	
	li 		t3, 30
	li 		t4, 3

v2ploop:
	srl		t1, a0, t3
	addi	t3, t3, -9

	andi	t1, t1, 0x1ff
	slli	t1, t1, 3

	add		t0, t0, t1
	lw		t0, 0(t0) # get the PTE

	srli	t0, t0, 10
	slli	t0, t0, 12

	bne		t3, t4, v2ploop
	li 		t1, 0xfff
	and 	a0, a0, t1
	add		t0, t0, a0
	mv		a0, t0
	ret

physicaltovirtual:
# a0 = virtual  address
# a1 = physical address
	la		t0, pagestart
	
	li		t2, 0xf
	li 		t3, 30
	li 		t4, 21

p2vloop:
	srl		t1, a0, t3
	addi	t3, t3, -9

	andi	t1, t1, 0x1ff
	slli	t1, t1, 3
	
	add		t0, t0, t1
	lw		t1, 0(t0) # get the PTE

	srli	a0, t1, 10
	slli	a0, a0, 12

	bne		t3, t4, p2vloop
	
pageerror:
	wfi

kernelpagesetup:
	la		t0, pagestart
	srli	t1, t0, 2
	addi	t1, t1, 0x401
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
	andi 	t3, t3, 0xf

	li 		t2, 3
	beq 	t2, t3, breakpoint
	li 		t2, 1
	beq 	t2, t3, instructionaccess
	li 		t2, 5
	beq 	t2, t3, loadaccess
	li 		t2, 7
	beq 	t2, t3, storeaccess
	li 		t2, 12
	beq 	t2, t3, instpage
	li 		t2, 13
	beq 	t2, t3, loadpage
	li 		t2, 15
	beq 	t2, t3, storepage

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
instpage:			# 12
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

enterkernel:
	j main

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
	j main


# align to 2^12 = 4096
# Sv39 page tables contain 2^9 Page Table Entries

# 38-30  29-21   20-12  11-0
# VPN[2] VPN[1] VPN[0] page-offset
# 9      9      9      12
# 0x80000000
# 55-30   29-21  20-12  11-0
# PPN[2] PPN[1] PPN[0] page-offset
# 26     9      9      12
# 63 62-61 60-54   53-28  27-19 18-10   9-8  7  6  5  4  3  2  1  0
# N PBMT Reserved PPN[2] PPN[1] PPN[0] RSW  D  A  G  U  X  W  R  V
# 1  2 		7 		26 		9 		9   2   1  1  1  1  1  1  1  1
.align 12
mstack:
.skip 4096,0
.align 12
pagestart:
.fill 2, 8, 0x0
.quad 0x20000801
.align 12

.quad 0x20000c01
.align 12

.quad 0x2000000f
.align 12

.size	_start, .-_start
.ident	"GCC: (gc891d8dc3e) 13.2.0"
