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
	# enable global interrupts for m and s modes
	# set previous privilege level to s
    li      t0, 0x1882
    csrw    mstatus, t0

	# start mtimecmp at 40k
    li      t0, 50000
    li      t1, 0x2004000
    sd      t0, 0(t1)

	# enable m interrupts
    li      t0, 0xa0 
    csrs    mie, t0

	# delegate m interrupts to s
    li      t0, 0x20
    csrs    mideleg, t0

	# set trap handler location
    la      t0, mtrap
    csrw    mtvec, t0

	# set supervisor kernel location
	la      t0, main
  	csrw    mepc, t0
	li      t0, 0x80
    csrc    mip, t0
    mret

mtrap:
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

	.size	_start, .-_start
	.ident	"GCC: (gc891d8dc23e) 13.2.0"
