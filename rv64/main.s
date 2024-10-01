	.file	"main.c"
	.option nopic
	.attribute arch, "rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
	.globl	m
	.bss
	.align	3
	.type	m, @object
	.size	m, 40
m:
	.zero	40
	.text
	.align	1
	.globl	_start
	.type	_start, @function
_start:
	addi	sp,sp,-32
	sd	s0,24(sp)
	addi	s0,sp,32
.L4:
	sw	zero,-20(s0)
	j	.L2
.L3:
	lui	a5,%hi(m)
	addi	a4,a5,%lo(m)
	lw	a5,-20(s0)
	slli	a5,a5,2
	add	a5,a4,a5
	lw	a5,0(a5)
	lw	a4,-20(s0)
	addw	a5,a4,a5
	sext.w	a4,a5
	lui	a5,%hi(m)
	addi	a3,a5,%lo(m)
	lw	a5,-20(s0)
	slli	a5,a5,2
	add	a5,a3,a5
	sw	a4,0(a5)
	lw	a5,-20(s0)
	addiw	a5,a5,1
	sw	a5,-20(s0)
.L2:
	lw	a5,-20(s0)
	sext.w	a4,a5
	li	a5,9
	ble	a4,a5,.L3
	j	.L4
	.size	_start, .-_start
	.ident	"GCC: (gc891d8dc23e) 13.2.0"
