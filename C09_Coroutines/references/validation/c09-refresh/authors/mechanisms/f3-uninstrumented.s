	.text
	.file	"main.cpp"
	.globl	_Z12range_valuesi               # -- Begin function _Z12range_valuesi
	.p2align	4, 0x90
	.type	_Z12range_valuesi,@function
_Z12range_valuesi:                      # @_Z12range_valuesi
	.cfi_startproc
# %bb.0:
	pushq	%r14
	.cfi_def_cfa_offset 16
	pushq	%rbx
	.cfi_def_cfa_offset 24
	pushq	%rax
	.cfi_def_cfa_offset 32
	.cfi_offset %rbx, -24
	.cfi_offset %r14, -16
	movl	%esi, %ebx
	movq	%rdi, %r14
	movl	$48, %edi
	callq	_Znwm@PLT
	leaq	_Z12range_valuesi.resume(%rip), %rcx
	movq	%rcx, (%rax)
	leaq	_Z12range_valuesi.destroy(%rip), %rcx
	movq	%rcx, 8(%rax)
	movl	%ebx, 32(%rax)
	movl	%ebx, 16(%rax)
	movq	$0, 24(%rax)
	movq	%rax, (%r14)
	movb	$0, 40(%rax)
	movq	%r14, %rax
	addq	$8, %rsp
	.cfi_def_cfa_offset 24
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%r14
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end0:
	.size	_Z12range_valuesi, .Lfunc_end0-_Z12range_valuesi
	.cfi_endproc
                                        # -- End function
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0                          # -- Begin function _Z13local_consumei
.LCPI1_0:
	.long	1                               # 0x1
	.long	2                               # 0x2
	.zero	4
	.zero	4
.LCPI1_1:
	.long	2                               # 0x2
	.long	2                               # 0x2
	.zero	4
	.zero	4
.LCPI1_2:
	.long	4                               # 0x4
	.long	4                               # 0x4
	.zero	4
	.zero	4
	.text
	.globl	_Z13local_consumei
	.p2align	4, 0x90
	.type	_Z13local_consumei,@function
_Z13local_consumei:                     # @_Z13local_consumei
	.cfi_startproc
# %bb.0:
                                        # kill: def $edi killed $edi def $rdi
	cmpl	$2, %edi
	jge	.LBB1_3
# %bb.1:
	xorl	%eax, %eax
	retq
.LBB1_3:
	cmpl	$5, %edi
	jae	.LBB1_5
# %bb.4:
	movl	$1, %ecx
	xorl	%eax, %eax
	jmp	.LBB1_8
.LBB1_5:
	leal	-1(%rdi), %edx
	movl	%edx, %esi
	andl	$-4, %esi
	leal	1(%rsi), %ecx
	pxor	%xmm0, %xmm0
	movdqa	.LCPI1_0(%rip), %xmm1           # xmm1 = [1,2,u,u]
	movdqa	.LCPI1_1(%rip), %xmm3           # xmm3 = [2,2,u,u]
	movdqa	.LCPI1_2(%rip), %xmm4           # xmm4 = [4,4,u,u]
	movl	%esi, %eax
	pxor	%xmm5, %xmm5
	pxor	%xmm2, %xmm2
	.p2align	4, 0x90
.LBB1_6:                                # =>This Inner Loop Header: Depth=1
	movdqa	%xmm1, %xmm6
	paddd	%xmm3, %xmm6
	movdqa	%xmm1, %xmm7
	punpckldq	%xmm0, %xmm7            # xmm7 = xmm7[0],xmm0[0],xmm7[1],xmm0[1]
	paddq	%xmm7, %xmm5
	punpckldq	%xmm0, %xmm6            # xmm6 = xmm6[0],xmm0[0],xmm6[1],xmm0[1]
	paddq	%xmm6, %xmm2
	paddd	%xmm4, %xmm1
	addl	$-4, %eax
	jne	.LBB1_6
# %bb.7:
	paddq	%xmm5, %xmm2
	pshufd	$238, %xmm2, %xmm0              # xmm0 = xmm2[2,3,2,3]
	paddq	%xmm2, %xmm0
	movq	%xmm0, %rax
	cmpl	%esi, %edx
	je	.LBB1_2
.LBB1_8:
	movl	%ecx, %ecx
	movl	%edi, %edx
	.p2align	4, 0x90
.LBB1_9:                                # =>This Inner Loop Header: Depth=1
	addq	%rcx, %rax
	incq	%rcx
	cmpl	%ecx, %edx
	jne	.LBB1_9
.LBB1_2:
	retq
.Lfunc_end1:
	.size	_Z13local_consumei, .Lfunc_end1-_Z13local_consumei
	.cfi_endproc
                                        # -- End function
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0                          # -- Begin function _Z15escaped_consumei
.LCPI2_0:
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.byte	1                               # 0x1
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.byte	0                               # 0x0
	.byte	0                               # 0x0
.LCPI2_1:
	.quad	2                               # 0x2
	.quad	2                               # 0x2
.LCPI2_2:
	.quad	4                               # 0x4
	.quad	4                               # 0x4
	.text
	.globl	_Z15escaped_consumei
	.p2align	4, 0x90
	.type	_Z15escaped_consumei,@function
_Z15escaped_consumei:                   # @_Z15escaped_consumei
	.cfi_startproc
# %bb.0:
	leaq	-8(%rsp), %rax
	movq	%rax, _ZL17escaped_generator(%rip)
	testl	%edi, %edi
	jle	.LBB2_1
# %bb.2:
	movl	%edi, %ecx
	cmpl	$4, %edi
	jae	.LBB2_4
# %bb.3:
	xorl	%edx, %edx
	xorl	%eax, %eax
	jmp	.LBB2_7
.LBB2_1:
	xorl	%eax, %eax
	retq
.LBB2_4:
	movl	%ecx, %edx
	andl	$2147483644, %edx               # imm = 0x7FFFFFFC
	pxor	%xmm0, %xmm0
	movdqa	.LCPI2_0(%rip), %xmm1           # xmm1 = [0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0]
	movdqa	.LCPI2_1(%rip), %xmm3           # xmm3 = [2,2]
	movdqa	.LCPI2_2(%rip), %xmm4           # xmm4 = [4,4]
	movq	%rdx, %rax
	pxor	%xmm2, %xmm2
	.p2align	4, 0x90
.LBB2_5:                                # =>This Inner Loop Header: Depth=1
	paddq	%xmm1, %xmm2
	paddq	%xmm3, %xmm2
	paddq	%xmm1, %xmm0
	paddq	%xmm4, %xmm1
	addq	$-4, %rax
	jne	.LBB2_5
# %bb.6:
	paddq	%xmm0, %xmm2
	pshufd	$238, %xmm2, %xmm0              # xmm0 = xmm2[2,3,2,3]
	paddq	%xmm2, %xmm0
	movq	%xmm0, %rax
	cmpq	%rcx, %rdx
	je	.LBB2_8
	.p2align	4, 0x90
.LBB2_7:                                # =>This Inner Loop Header: Depth=1
	addq	%rdx, %rax
	incq	%rdx
	cmpq	%rdx, %rcx
	jne	.LBB2_7
.LBB2_8:
	retq
.Lfunc_end2:
	.size	_Z15escaped_consumei, .Lfunc_end2-_Z15escaped_consumei
	.cfi_endproc
                                        # -- End function
	.globl	_Z8bench_nsPFliEii              # -- Begin function _Z8bench_nsPFliEii
	.p2align	4, 0x90
	.type	_Z8bench_nsPFliEii,@function
_Z8bench_nsPFliEii:                     # @_Z8bench_nsPFliEii
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	pushq	%r15
	.cfi_def_cfa_offset 24
	pushq	%r14
	.cfi_def_cfa_offset 32
	pushq	%r13
	.cfi_def_cfa_offset 40
	pushq	%r12
	.cfi_def_cfa_offset 48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	pushq	%rax
	.cfi_def_cfa_offset 64
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	.cfi_offset %rbp, -16
	movl	%edx, %ebx
	movl	%esi, %ebp
	movq	%rdi, %r15
	callq	_ZNSt6chrono3_V212steady_clock3nowEv@PLT
	movq	%rax, %r14
	xorl	%r12d, %r12d
	testl	%ebx, %ebx
	jle	.LBB3_3
# %bb.1:
	movl	%ebx, %r13d
	.p2align	4, 0x90
.LBB3_2:                                # =>This Inner Loop Header: Depth=1
	movl	%ebp, %edi
	callq	*%r15
	addq	%rax, %r12
	decl	%r13d
	jne	.LBB3_2
.LBB3_3:
	callq	_ZNSt6chrono3_V212steady_clock3nowEv@PLT
	addq	%r12, _ZL4sink(%rip)
	subq	%r14, %rax
	cvtsi2sd	%rax, %xmm0
	cvtsi2sd	%ebx, %xmm1
	divsd	%xmm1, %xmm0
	addq	$8, %rsp
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%r12
	.cfi_def_cfa_offset 40
	popq	%r13
	.cfi_def_cfa_offset 32
	popq	%r14
	.cfi_def_cfa_offset 24
	popq	%r15
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end3:
	.size	_Z8bench_nsPFliEii, .Lfunc_end3-_Z8bench_nsPFliEii
	.cfi_endproc
                                        # -- End function
	.globl	_Z14select_versionSt17basic_string_viewIcSt11char_traitsIcEE # -- Begin function _Z14select_versionSt17basic_string_viewIcSt11char_traitsIcEE
	.p2align	4, 0x90
	.type	_Z14select_versionSt17basic_string_viewIcSt11char_traitsIcEE,@function
_Z14select_versionSt17basic_string_viewIcSt11char_traitsIcEE: # @_Z14select_versionSt17basic_string_viewIcSt11char_traitsIcEE
	.cfi_startproc
# %bb.0:
	cmpq	$7, %rdi
	je	.LBB4_4
# %bb.1:
	cmpq	$5, %rdi
	jne	.LBB4_6
# %bb.2:
	movl	$1633906540, %eax               # imm = 0x61636F6C
	xorl	(%rsi), %eax
	movzbl	4(%rsi), %ecx
	xorl	$108, %ecx
	orl	%eax, %ecx
	jne	.LBB4_6
# %bb.3:
	leaq	_Z13local_consumei(%rip), %rax
	retq
.LBB4_4:
	movl	$1633907557, %eax               # imm = 0x61637365
	xorl	(%rsi), %eax
	movl	$1684369505, %ecx               # imm = 0x64657061
	xorl	3(%rsi), %ecx
	orl	%eax, %ecx
	je	.LBB4_5
.LBB4_6:
	xorl	%eax, %eax
	retq
.LBB4_5:
	leaq	_Z15escaped_consumei(%rip), %rax
	retq
.Lfunc_end4:
	.size	_Z14select_versionSt17basic_string_viewIcSt11char_traitsIcEE, .Lfunc_end4-_Z14select_versionSt17basic_string_viewIcSt11char_traitsIcEE
	.cfi_endproc
                                        # -- End function
	.globl	_Z7run_oneSt17basic_string_viewIcSt11char_traitsIcEEii # -- Begin function _Z7run_oneSt17basic_string_viewIcSt11char_traitsIcEEii
	.p2align	4, 0x90
	.type	_Z7run_oneSt17basic_string_viewIcSt11char_traitsIcEEii,@function
_Z7run_oneSt17basic_string_viewIcSt11char_traitsIcEEii: # @_Z7run_oneSt17basic_string_viewIcSt11char_traitsIcEEii
.Lfunc_begin0:
	.cfi_startproc
	.cfi_personality 155, DW.ref.__gxx_personality_v0
	.cfi_lsda 27, .Lexception0
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	pushq	%r15
	.cfi_def_cfa_offset 24
	pushq	%r14
	.cfi_def_cfa_offset 32
	pushq	%r13
	.cfi_def_cfa_offset 40
	pushq	%r12
	.cfi_def_cfa_offset 48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	subq	$72, %rsp
	.cfi_def_cfa_offset 128
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	.cfi_offset %rbp, -16
	movl	%ecx, %r15d
	movl	%edx, %r14d
	cmpq	$7, %rdi
	movq	%rsi, 32(%rsp)                  # 8-byte Spill
	movq	%rdi, 24(%rsp)                  # 8-byte Spill
	je	.LBB5_4
# %bb.1:
	cmpq	$5, %rdi
	jne	.LBB5_13
# %bb.2:
	movl	$1633906540, %eax               # imm = 0x61636F6C
	xorl	(%rsi), %eax
	movzbl	4(%rsi), %ecx
	xorl	$108, %ecx
	orl	%eax, %ecx
	jne	.LBB5_13
# %bb.3:
	leaq	_Z13local_consumei(%rip), %rbp
	jmp	.LBB5_6
.LBB5_4:
	movl	$1633907557, %eax               # imm = 0x61637365
	xorl	(%rsi), %eax
	movl	$1684369505, %ecx               # imm = 0x64657061
	xorl	3(%rsi), %ecx
	orl	%eax, %ecx
	jne	.LBB5_13
# %bb.5:
	leaq	_Z15escaped_consumei(%rip), %rbp
.LBB5_6:
	leal	-1(%r14), %eax
	cltq
	movslq	%r14d, %rdi
	imulq	%rdi, %rax
	movq	%rax, %rbx
	shrq	$63, %rbx
	addq	%rax, %rbx
	sarq	%rbx
                                        # kill: def $edi killed $edi killed $rdi
	callq	*%rbp
	movq	$42, 8(%rsp)
	leaq	.L.str.3(%rip), %rcx
	movq	%rcx, 16(%rsp)
	cmpq	%rbx, %rax
	jne	.LBB5_17
# %bb.7:
	callq	_ZNSt6chrono3_V212steady_clock3nowEv@PLT
	movl	$1000, %r13d                    # imm = 0x3E8
	xorl	%r12d, %r12d
	.p2align	4, 0x90
.LBB5_8:                                # =>This Inner Loop Header: Depth=1
	movl	%r14d, %edi
	callq	*%rbp
	addq	%rax, %r12
	decl	%r13d
	jne	.LBB5_8
# %bb.9:
	callq	_ZNSt6chrono3_V212steady_clock3nowEv@PLT
	addq	%r12, _ZL4sink(%rip)
	callq	_ZNSt6chrono3_V212steady_clock3nowEv@PLT
	movq	%rax, %r13
	xorl	%r12d, %r12d
	testl	%r15d, %r15d
	jle	.LBB5_12
# %bb.10:
	movl	%r15d, %ebx
	.p2align	4, 0x90
.LBB5_11:                               # =>This Inner Loop Header: Depth=1
	movl	%r14d, %edi
	callq	*%rbp
	addq	%rax, %r12
	decl	%ebx
	jne	.LBB5_11
.LBB5_12:
	callq	_ZNSt6chrono3_V212steady_clock3nowEv@PLT
	addq	%r12, _ZL4sink(%rip)
	subq	%r13, %rax
	cvtsi2sd	%rax, %xmm0
	cvtsi2sd	%r15d, %xmm1
	divsd	%xmm1, %xmm0
	movq	_ZL4sink(%rip), %r9
	leaq	.L.str.4(%rip), %rdi
	movq	24(%rsp), %rsi                  # 8-byte Reload
                                        # kill: def $esi killed $esi killed $rsi
	movq	32(%rsp), %rdx                  # 8-byte Reload
	movl	%r14d, %ecx
	movl	%r15d, %r8d
	movb	$1, %al
	callq	printf@PLT
	xorl	%eax, %eax
	addq	$72, %rsp
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%r12
	.cfi_def_cfa_offset 40
	popq	%r13
	.cfi_def_cfa_offset 32
	popq	%r14
	.cfi_def_cfa_offset 24
	popq	%r15
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	retq
.LBB5_13:
	.cfi_def_cfa_offset 128
	movq	$32, 8(%rsp)
	leaq	.L.str.2(%rip), %rax
	movq	%rax, 16(%rsp)
	movl	$16, %edi
	callq	__cxa_allocate_exception@PLT
	movq	%rax, %rbx
.Ltmp8:
	leaq	40(%rsp), %rdi
	leaq	8(%rsp), %rsi
	leaq	7(%rsp), %rdx
	callq	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2ISt17basic_string_viewIcS2_EvEERKT_RKS3_
.Ltmp9:
# %bb.14:
	movb	$1, %bpl
.Ltmp11:
	leaq	40(%rsp), %rsi
	movq	%rbx, %rdi
	callq	_ZNSt13runtime_errorC1ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE@PLT
.Ltmp12:
# %bb.15:
	xorl	%ebp, %ebp
.Ltmp13:
	movq	_ZTISt13runtime_error@GOTPCREL(%rip), %rsi
	movq	_ZNSt13runtime_errorD1Ev@GOTPCREL(%rip), %rdx
	movq	%rbx, %rdi
	callq	__cxa_throw@PLT
.Ltmp14:
# %bb.16:
.LBB5_17:
	movl	$16, %edi
	callq	__cxa_allocate_exception@PLT
	movq	%rax, %rbx
.Ltmp0:
	leaq	40(%rsp), %rdi
	leaq	8(%rsp), %rsi
	leaq	7(%rsp), %rdx
	callq	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2ISt17basic_string_viewIcS2_EvEERKT_RKS3_
.Ltmp1:
# %bb.18:
	movb	$1, %bpl
.Ltmp3:
	leaq	40(%rsp), %rsi
	movq	%rbx, %rdi
	callq	_ZNSt13runtime_errorC1ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE@PLT
.Ltmp4:
# %bb.19:
	xorl	%ebp, %ebp
.Ltmp5:
	movq	_ZTISt13runtime_error@GOTPCREL(%rip), %rsi
	movq	_ZNSt13runtime_errorD1Ev@GOTPCREL(%rip), %rdx
	movq	%rbx, %rdi
	callq	__cxa_throw@PLT
.Ltmp6:
# %bb.20:
.LBB5_21:
.Ltmp7:
	jmp	.LBB5_24
.LBB5_22:
.Ltmp2:
	jmp	.LBB5_29
.LBB5_23:
.Ltmp15:
.LBB5_24:
	movq	%rax, %r14
	movq	40(%rsp), %rdi
	leaq	56(%rsp), %rax
	cmpq	%rax, %rdi
	jne	.LBB5_27
# %bb.25:
	testb	%bpl, %bpl
	jne	.LBB5_30
	jmp	.LBB5_26
.LBB5_27:
	callq	_ZdlPv@PLT
	testb	%bpl, %bpl
	jne	.LBB5_30
.LBB5_26:
	movq	%r14, %rdi
	callq	_Unwind_Resume@PLT
.LBB5_28:
.Ltmp10:
.LBB5_29:
	movq	%rax, %r14
.LBB5_30:
	movq	%rbx, %rdi
	callq	__cxa_free_exception@PLT
	movq	%r14, %rdi
	callq	_Unwind_Resume@PLT
.Lfunc_end5:
	.size	_Z7run_oneSt17basic_string_viewIcSt11char_traitsIcEEii, .Lfunc_end5-_Z7run_oneSt17basic_string_viewIcSt11char_traitsIcEEii
	.cfi_endproc
	.section	.gcc_except_table,"a",@progbits
	.p2align	2, 0x0
GCC_except_table5:
.Lexception0:
	.byte	255                             # @LPStart Encoding = omit
	.byte	255                             # @TType Encoding = omit
	.byte	1                               # Call site Encoding = uleb128
	.uleb128 .Lcst_end0-.Lcst_begin0
.Lcst_begin0:
	.uleb128 .Lfunc_begin0-.Lfunc_begin0    # >> Call Site 1 <<
	.uleb128 .Ltmp8-.Lfunc_begin0           #   Call between .Lfunc_begin0 and .Ltmp8
	.byte	0                               #     has no landing pad
	.byte	0                               #   On action: cleanup
	.uleb128 .Ltmp8-.Lfunc_begin0           # >> Call Site 2 <<
	.uleb128 .Ltmp9-.Ltmp8                  #   Call between .Ltmp8 and .Ltmp9
	.uleb128 .Ltmp10-.Lfunc_begin0          #     jumps to .Ltmp10
	.byte	0                               #   On action: cleanup
	.uleb128 .Ltmp11-.Lfunc_begin0          # >> Call Site 3 <<
	.uleb128 .Ltmp14-.Ltmp11                #   Call between .Ltmp11 and .Ltmp14
	.uleb128 .Ltmp15-.Lfunc_begin0          #     jumps to .Ltmp15
	.byte	0                               #   On action: cleanup
	.uleb128 .Ltmp14-.Lfunc_begin0          # >> Call Site 4 <<
	.uleb128 .Ltmp0-.Ltmp14                 #   Call between .Ltmp14 and .Ltmp0
	.byte	0                               #     has no landing pad
	.byte	0                               #   On action: cleanup
	.uleb128 .Ltmp0-.Lfunc_begin0           # >> Call Site 5 <<
	.uleb128 .Ltmp1-.Ltmp0                  #   Call between .Ltmp0 and .Ltmp1
	.uleb128 .Ltmp2-.Lfunc_begin0           #     jumps to .Ltmp2
	.byte	0                               #   On action: cleanup
	.uleb128 .Ltmp3-.Lfunc_begin0           # >> Call Site 6 <<
	.uleb128 .Ltmp6-.Ltmp3                  #   Call between .Ltmp3 and .Ltmp6
	.uleb128 .Ltmp7-.Lfunc_begin0           #     jumps to .Ltmp7
	.byte	0                               #   On action: cleanup
	.uleb128 .Ltmp6-.Lfunc_begin0           # >> Call Site 7 <<
	.uleb128 .Lfunc_end5-.Ltmp6             #   Call between .Ltmp6 and .Lfunc_end5
	.byte	0                               #     has no landing pad
	.byte	0                               #   On action: cleanup
.Lcst_end0:
	.p2align	2, 0x0
                                        # -- End function
	.text
	.globl	main                            # -- Begin function main
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
.Lfunc_begin1:
	.cfi_startproc
	.cfi_personality 155, DW.ref.__gxx_personality_v0
	.cfi_lsda 27, .Lexception1
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	pushq	%r15
	.cfi_def_cfa_offset 24
	pushq	%r14
	.cfi_def_cfa_offset 32
	pushq	%r12
	.cfi_def_cfa_offset 40
	pushq	%rbx
	.cfi_def_cfa_offset 48
	subq	$64, %rsp
	.cfi_def_cfa_offset 112
	.cfi_offset %rbx, -48
	.cfi_offset %r12, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	.cfi_offset %rbp, -16
	cmpl	$5, %edi
	jne	.LBB6_15
# %bb.1:
	movq	%rsi, %rbx
	movq	8(%rsi), %r14
	movq	%r14, %rdi
	callq	strlen@PLT
	cmpq	$7, %rax
	jne	.LBB6_15
# %bb.2:
	movl	$1700932909, %eax               # imm = 0x65622D2D
	xorl	(%r14), %eax
	movl	$1751346789, %ecx               # imm = 0x68636E65
	xorl	3(%r14), %ecx
	orl	%eax, %ecx
	je	.LBB6_3
.LBB6_15:
	leaq	32(%rsp), %rax
	movq	%rax, _ZL17escaped_generator(%rip)
	leaq	.Lstr(%rip), %rdi
	callq	puts@PLT
	leaq	.Lstr.15(%rip), %rdi
	callq	puts@PLT
	leaq	.Lstr.16(%rip), %rdi
	callq	puts@PLT
	xorl	%eax, %eax
.LBB6_19:
	addq	$64, %rsp
	.cfi_def_cfa_offset 48
	popq	%rbx
	.cfi_def_cfa_offset 40
	popq	%r12
	.cfi_def_cfa_offset 32
	popq	%r14
	.cfi_def_cfa_offset 24
	popq	%r15
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	retq
.LBB6_3:
	.cfi_def_cfa_offset 112
	movq	16(%rbx), %r14
	movq	%r14, %rdi
	callq	strlen@PLT
	movq	%rax, %r15
	movq	24(%rbx), %rdi
	xorl	%esi, %esi
	movl	$10, %edx
	callq	__isoc23_strtol@PLT
	movq	%rax, %r12
	movq	32(%rbx), %rdi
	xorl	%esi, %esi
	movl	$10, %edx
	callq	__isoc23_strtol@PLT
	movq	$33, 16(%rsp)
	leaq	.L.str.6(%rip), %rcx
	movq	%rcx, 24(%rsp)
	testl	%r12d, %r12d
	jle	.LBB6_5
# %bb.4:
	testl	%eax, %eax
	jle	.LBB6_5
# %bb.14:
.Ltmp24:
	movq	%r15, %rdi
	movq	%r14, %rsi
	movl	%r12d, %edx
	movl	%eax, %ecx
	callq	_Z7run_oneSt17basic_string_viewIcSt11char_traitsIcEEii
	xorl	%eax, %eax
.Ltmp25:
	jmp	.LBB6_19
.LBB6_5:
	movl	$16, %edi
	callq	__cxa_allocate_exception@PLT
	movq	%rax, %rbx
.Ltmp16:
	leaq	32(%rsp), %rdi
	leaq	16(%rsp), %rsi
	leaq	15(%rsp), %rdx
	callq	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2ISt17basic_string_viewIcS2_EvEERKT_RKS3_
.Ltmp17:
# %bb.6:
	movb	$1, %bpl
.Ltmp19:
	leaq	32(%rsp), %rsi
	movq	%rbx, %rdi
	callq	_ZNSt13runtime_errorC1ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE@PLT
.Ltmp20:
# %bb.7:
	xorl	%ebp, %ebp
.Ltmp21:
	movq	_ZTISt13runtime_error@GOTPCREL(%rip), %rsi
	movq	_ZNSt13runtime_errorD1Ev@GOTPCREL(%rip), %rdx
	movq	%rbx, %rdi
	callq	__cxa_throw@PLT
.Ltmp22:
# %bb.13:
.LBB6_9:
.Ltmp23:
	movq	%rdx, %r15
	movq	%rax, %r14
	movq	32(%rsp), %rdi
	leaq	48(%rsp), %rax
	cmpq	%rax, %rdi
	je	.LBB6_11
# %bb.10:
	callq	_ZdlPv@PLT
.LBB6_11:
	testb	%bpl, %bpl
	jne	.LBB6_12
	jmp	.LBB6_17
.LBB6_8:
.Ltmp18:
	movq	%rdx, %r15
	movq	%rax, %r14
.LBB6_12:
	movq	%rbx, %rdi
	callq	__cxa_free_exception@PLT
	jmp	.LBB6_17
.LBB6_16:
.Ltmp26:
	movq	%rdx, %r15
	movq	%rax, %r14
.LBB6_17:
	movq	%r14, %rdi
	cmpl	$1, %r15d
	jne	.LBB6_20
# %bb.18:
	callq	__cxa_begin_catch@PLT
	movq	stderr@GOTPCREL(%rip), %rcx
	movq	(%rcx), %rbx
	movq	(%rax), %rcx
	movq	%rax, %rdi
	callq	*16(%rcx)
	leaq	.L.str.12(%rip), %rsi
	movq	%rbx, %rdi
	movq	%rax, %rdx
	xorl	%eax, %eax
	callq	fprintf@PLT
	callq	__cxa_end_catch@PLT
	movl	$1, %eax
	jmp	.LBB6_19
.LBB6_20:
	callq	_Unwind_Resume@PLT
.Lfunc_end6:
	.size	main, .Lfunc_end6-main
	.cfi_endproc
	.section	.gcc_except_table,"a",@progbits
	.p2align	2, 0x0
GCC_except_table6:
.Lexception1:
	.byte	255                             # @LPStart Encoding = omit
	.byte	155                             # @TType Encoding = indirect pcrel sdata4
	.uleb128 .Lttbase0-.Lttbaseref0
.Lttbaseref0:
	.byte	1                               # Call site Encoding = uleb128
	.uleb128 .Lcst_end1-.Lcst_begin1
.Lcst_begin1:
	.uleb128 .Ltmp24-.Lfunc_begin1          # >> Call Site 1 <<
	.uleb128 .Ltmp25-.Ltmp24                #   Call between .Ltmp24 and .Ltmp25
	.uleb128 .Ltmp26-.Lfunc_begin1          #     jumps to .Ltmp26
	.byte	3                               #   On action: 2
	.uleb128 .Ltmp25-.Lfunc_begin1          # >> Call Site 2 <<
	.uleb128 .Ltmp16-.Ltmp25                #   Call between .Ltmp25 and .Ltmp16
	.byte	0                               #     has no landing pad
	.byte	0                               #   On action: cleanup
	.uleb128 .Ltmp16-.Lfunc_begin1          # >> Call Site 3 <<
	.uleb128 .Ltmp17-.Ltmp16                #   Call between .Ltmp16 and .Ltmp17
	.uleb128 .Ltmp18-.Lfunc_begin1          #     jumps to .Ltmp18
	.byte	3                               #   On action: 2
	.uleb128 .Ltmp19-.Lfunc_begin1          # >> Call Site 4 <<
	.uleb128 .Ltmp22-.Ltmp19                #   Call between .Ltmp19 and .Ltmp22
	.uleb128 .Ltmp23-.Lfunc_begin1          #     jumps to .Ltmp23
	.byte	3                               #   On action: 2
	.uleb128 .Ltmp22-.Lfunc_begin1          # >> Call Site 5 <<
	.uleb128 .Lfunc_end6-.Ltmp22            #   Call between .Ltmp22 and .Lfunc_end6
	.byte	0                               #     has no landing pad
	.byte	0                               #   On action: cleanup
.Lcst_end1:
	.byte	0                               # >> Action Record 1 <<
                                        #   Cleanup
	.byte	0                               #   No further actions
	.byte	1                               # >> Action Record 2 <<
                                        #   Catch TypeInfo 1
	.byte	125                             #   Continue to action 1
	.p2align	2, 0x0
                                        # >> Catch TypeInfos <<
.Ltmp27:                                # TypeInfo 1
	.long	.L_ZTISt9exception.DW.stub-.Ltmp27
.Lttbase0:
	.p2align	2, 0x0
                                        # -- End function
	.section	.text._ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2ISt17basic_string_viewIcS2_EvEERKT_RKS3_,"axG",@progbits,_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2ISt17basic_string_viewIcS2_EvEERKT_RKS3_,comdat
	.weak	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2ISt17basic_string_viewIcS2_EvEERKT_RKS3_ # -- Begin function _ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2ISt17basic_string_viewIcS2_EvEERKT_RKS3_
	.p2align	4, 0x90
	.type	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2ISt17basic_string_viewIcS2_EvEERKT_RKS3_,@function
_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2ISt17basic_string_viewIcS2_EvEERKT_RKS3_: # @_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2ISt17basic_string_viewIcS2_EvEERKT_RKS3_
	.cfi_startproc
# %bb.0:
	pushq	%r15
	.cfi_def_cfa_offset 16
	pushq	%r14
	.cfi_def_cfa_offset 24
	pushq	%r12
	.cfi_def_cfa_offset 32
	pushq	%rbx
	.cfi_def_cfa_offset 40
	pushq	%rax
	.cfi_def_cfa_offset 48
	.cfi_offset %rbx, -40
	.cfi_offset %r12, -32
	.cfi_offset %r14, -24
	.cfi_offset %r15, -16
	movq	%rdi, %r14
	movq	(%rsi), %rbx
	movq	8(%rsi), %r15
	leaq	16(%rdi), %r12
	movq	%r12, (%rdi)
	testq	%rbx, %rbx
	je	.LBB7_2
# %bb.1:
	testq	%r15, %r15
	je	.LBB7_11
.LBB7_2:
	cmpq	$16, %rbx
	jb	.LBB7_6
# %bb.3:
	testq	%rbx, %rbx
	js	.LBB7_12
# %bb.4:
	movq	%rbx, %rdi
	incq	%rdi
	js	.LBB7_13
# %bb.5:
	callq	_Znwm@PLT
	movq	%rax, %r12
	movq	%rax, (%r14)
	movq	%rbx, 16(%r14)
.LBB7_6:
	testq	%rbx, %rbx
	je	.LBB7_10
# %bb.7:
	cmpq	$1, %rbx
	jne	.LBB7_9
# %bb.8:
	movzbl	(%r15), %eax
	movb	%al, (%r12)
	jmp	.LBB7_10
.LBB7_9:
	movq	%r12, %rdi
	movq	%r15, %rsi
	movq	%rbx, %rdx
	callq	memcpy@PLT
.LBB7_10:
	movq	%rbx, 8(%r14)
	movb	$0, (%r12,%rbx)
	addq	$8, %rsp
	.cfi_def_cfa_offset 40
	popq	%rbx
	.cfi_def_cfa_offset 32
	popq	%r12
	.cfi_def_cfa_offset 24
	popq	%r14
	.cfi_def_cfa_offset 16
	popq	%r15
	.cfi_def_cfa_offset 8
	retq
.LBB7_13:
	.cfi_def_cfa_offset 48
	callq	_ZSt17__throw_bad_allocv@PLT
.LBB7_11:
	leaq	.L.str.13(%rip), %rdi
	callq	_ZSt19__throw_logic_errorPKc@PLT
.LBB7_12:
	leaq	.L.str.14(%rip), %rdi
	callq	_ZSt20__throw_length_errorPKc@PLT
.Lfunc_end7:
	.size	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2ISt17basic_string_viewIcS2_EvEERKT_RKS3_, .Lfunc_end7-_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2ISt17basic_string_viewIcS2_EvEERKT_RKS3_
	.cfi_endproc
                                        # -- End function
	.text
	.p2align	4, 0x90                         # -- Begin function _Z12range_valuesi.resume
	.type	_Z12range_valuesi.resume,@function
_Z12range_valuesi.resume:               # @_Z12range_valuesi.resume
	.cfi_startproc
# %bb.0:
	cmpb	$0, 40(%rdi)
	je	.LBB8_1
# %bb.3:
	movl	36(%rdi), %eax
	incl	%eax
	cmpl	32(%rdi), %eax
	jne	.LBB8_2
.LBB8_4:
	movq	$0, (%rdi)
	retq
.LBB8_1:
	xorl	%eax, %eax
	cmpl	$0, 32(%rdi)
	jle	.LBB8_4
.LBB8_2:
	movl	%eax, 36(%rdi)
	movl	%eax, 16(%rdi)
	movb	$1, 40(%rdi)
	retq
.Lfunc_end8:
	.size	_Z12range_valuesi.resume, .Lfunc_end8-_Z12range_valuesi.resume
	.cfi_endproc
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function _Z12range_valuesi.destroy
	.type	_Z12range_valuesi.destroy,@function
_Z12range_valuesi.destroy:              # @_Z12range_valuesi.destroy
	.cfi_startproc
# %bb.0:
	cmpq	$0, 24(%rdi)
	je	_ZdlPv@PLT                      # TAILCALL
# %bb.1:
	pushq	%rbx
	.cfi_def_cfa_offset 16
	.cfi_offset %rbx, -16
	leaq	24(%rdi), %rax
	movq	%rdi, %rbx
	movq	%rax, %rdi
	callq	_ZNSt15__exception_ptr13exception_ptr10_M_releaseEv@PLT
	movq	%rbx, %rdi
	popq	%rbx
	.cfi_def_cfa_offset 8
	.cfi_restore %rbx
	jmp	_ZdlPv@PLT                      # TAILCALL
.Lfunc_end9:
	.size	_Z12range_valuesi.destroy, .Lfunc_end9-_Z12range_valuesi.destroy
	.cfi_endproc
                                        # -- End function
	.type	_ZL17escaped_generator,@object  # @_ZL17escaped_generator
	.local	_ZL17escaped_generator
	.comm	_ZL17escaped_generator,8,8
	.type	_ZL4sink,@object                # @_ZL4sink
	.local	_ZL4sink
	.comm	_ZL4sink,8,8
	.type	.L.str,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"local"
	.size	.L.str, 6

	.type	.L.str.1,@object                # @.str.1
.L.str.1:
	.asciz	"escaped"
	.size	.L.str.1, 8

	.type	.L.str.2,@object                # @.str.2
.L.str.2:
	.asciz	"version must be local or escaped"
	.size	.L.str.2, 33

	.type	.L.str.3,@object                # @.str.3
.L.str.3:
	.asciz	"selected version must compute the same sum"
	.size	.L.str.3, 43

	.type	.L.str.4,@object                # @.str.4
.L.str.4:
	.asciz	"version=%.*s,n=%d,iterations=%d,ns_per_iter=%.3f,sink=%lld\n"
	.size	.L.str.4, 60

	.type	.L.str.5,@object                # @.str.5
.L.str.5:
	.asciz	"--bench"
	.size	.L.str.5, 8

	.type	.L.str.6,@object                # @.str.6
.L.str.6:
	.asciz	"n and iterations must be positive"
	.size	.L.str.6, 34

	.type	.L.str.12,@object               # @.str.12
.L.str.12:
	.asciz	"starter check failed: %s\n"
	.size	.L.str.12, 26

	.type	.L.str.13,@object               # @.str.13
.L.str.13:
	.asciz	"basic_string: construction from null is not valid"
	.size	.L.str.13, 50

	.type	.L.str.14,@object               # @.str.14
.L.str.14:
	.asciz	"basic_string::_M_create"
	.size	.L.str.14, 24

	.type	.Lstr,@object                   # @str
.Lstr:
	.asciz	"F3 starter correctness OK"
	.size	.Lstr, 26

	.type	.Lstr.15,@object                # @str.15
.Lstr.15:
	.asciz	"Run sample_f3.py with this executable for 1 warmup + 5 independent processes."
	.size	.Lstr.15, 78

	.type	.Lstr.16,@object                # @str.16
.Lstr.16:
	.asciz	"Use compiler remark/IR/assembly for HALO; timing alone is not proof."
	.size	.Lstr.16, 69

	.section	".linker-options","e",@llvm_linker_options
	.data
	.p2align	3, 0x0
.L_ZTISt9exception.DW.stub:
	.quad	_ZTISt9exception
	.hidden	DW.ref.__gxx_personality_v0
	.weak	DW.ref.__gxx_personality_v0
	.section	.data.DW.ref.__gxx_personality_v0,"awG",@progbits,DW.ref.__gxx_personality_v0,comdat
	.p2align	3, 0x0
	.type	DW.ref.__gxx_personality_v0,@object
	.size	DW.ref.__gxx_personality_v0, 8
DW.ref.__gxx_personality_v0:
	.quad	__gxx_personality_v0
	.ident	"Ubuntu clang version 18.1.3 (1ubuntu1)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym _Z13local_consumei
	.addrsig_sym __gxx_personality_v0
	.addrsig_sym _Z15escaped_consumei
	.addrsig_sym _Z12range_valuesi.resume
	.addrsig_sym _Z12range_valuesi.destroy
	.addrsig_sym _Unwind_Resume
	.addrsig_sym _ZL4sink
	.addrsig_sym _ZTISt9exception
	.addrsig_sym _ZTISt13runtime_error
