# mark_description "Intel(R) C++ Intel(R) 64 Compiler for applications running on Intel(R) 64, Version 16.0.1.150 Build 20151021";
# mark_description "";
# mark_description "-finline-limit=0 -S -fcode-asm -qopt-report=2 -qopt-report-phase=vec -vec-threshold0 -o test.s";
	.file "test.cpp"
	.text
..TXTST0:
# -- Begin  main
	.text
# mark_begin;
       .align    16,0x90
	.globl main
# --- main()
main:
..B1.1:                         # Preds ..B1.0
	.cfi_startproc
	.cfi_personality 0x3,__gxx_personality_v0
..___tag_value_main.1:
..L2:
                                                          #test.cpp:12.12
        pushq     %rbp                                          #test.cpp:12.12
	.cfi_def_cfa_offset 16
        movq      %rsp, %rbp                                    #test.cpp:12.12
	.cfi_def_cfa 6, 16
	.cfi_offset 6, -16
        andq      $-128, %rsp                                   #test.cpp:12.12
        subq      $384, %rsp                                    #test.cpp:12.12
        xorl      %esi, %esi                                    #test.cpp:12.12
        movl      $3, %edi                                      #test.cpp:12.12
        call      __intel_new_feature_proc_init                 #test.cpp:12.12
                                # LOE rbx r12 r13 r14 r15
..B1.12:                        # Preds ..B1.1
        stmxcsr   (%rsp)                                        #test.cpp:12.12
        movl      $4, %eax                                      #test.cpp:16.9
        orl       $32832, (%rsp)                                #test.cpp:12.12
        ldmxcsr   (%rsp)                                        #test.cpp:12.12
        movd      %eax, %xmm0                                   #test.cpp:16.9
        xorl      %eax, %eax                                    #test.cpp:15.5
        pshufd    $0, %xmm0, %xmm1                              #test.cpp:16.9
        movdqa    .L_2il0floatpacket.0(%rip), %xmm0             #test.cpp:16.9
                                # LOE rax rbx r12 r13 r14 r15 xmm0 xmm1
..B1.2:                         # Preds ..B1.2 ..B1.12
        movdqa    %xmm0, 16(%rsp,%rax,4)                        #test.cpp:16.9
        paddd     %xmm1, %xmm0                                  #test.cpp:16.9
        movdqa    %xmm0, 32(%rsp,%rax,4)                        #test.cpp:16.9
        paddd     %xmm1, %xmm0                                  #test.cpp:16.9
        movdqa    %xmm0, 48(%rsp,%rax,4)                        #test.cpp:16.9
        paddd     %xmm1, %xmm0                                  #test.cpp:16.9
        movdqa    %xmm0, 64(%rsp,%rax,4)                        #test.cpp:16.9
        paddd     %xmm1, %xmm0                                  #test.cpp:16.9
        movdqa    %xmm0, 80(%rsp,%rax,4)                        #test.cpp:16.9
        paddd     %xmm1, %xmm0                                  #test.cpp:16.9
        movdqa    %xmm0, 96(%rsp,%rax,4)                        #test.cpp:16.9
        paddd     %xmm1, %xmm0                                  #test.cpp:16.9
        movdqa    %xmm0, 112(%rsp,%rax,4)                       #test.cpp:16.9
        paddd     %xmm1, %xmm0                                  #test.cpp:16.9
        movdqa    %xmm0, 128(%rsp,%rax,4)                       #test.cpp:16.9
        addq      $32, %rax                                     #test.cpp:15.5
        paddd     %xmm1, %xmm0                                  #test.cpp:16.9
        cmpq      $64, %rax                                     #test.cpp:15.5
        jb        ..B1.2        # Prob 98%                      #test.cpp:15.5
                                # LOE rax rbx r12 r13 r14 r15 xmm0 xmm1
..B1.3:                         # Preds ..B1.2
        movl      $64, %edi                                     #test.cpp:18.5
        lea       16(%rsp), %rsi                                #test.cpp:18.5
..___tag_value_main.6:
#       foo0(int, int *)
        call      _Z4foo0iPi                                    #test.cpp:18.5
..___tag_value_main.7:
                                # LOE rbx r12 r13 r14 r15
..B1.4:                         # Preds ..B1.3
        xorl      %eax, %eax                                    #test.cpp:19.16
        movq      %r12, (%rsp)                                  #test.cpp:19.16
	.cfi_escape 0x10, 0x0c, 0x0e, 0x38, 0x1c, 0x0d, 0x80, 0xff, 0xff, 0xff, 0x1a, 0x0d, 0x80, 0xfe, 0xff, 0xff, 0x22
        movq      %rax, %r12                                    #test.cpp:19.16
                                # LOE rbx r12 r13 r14 r15
..B1.5:                         # Preds ..B1.7 ..B1.4
        movl      $_ZSt4cout, %edi                              #test.cpp:20.19
        movl      16(%rsp,%r12,4), %esi                         #test.cpp:20.19
..___tag_value_main.9:
#       std::basic_ostream<char, std::char_traits<char>>::operator<<(std::basic_ostream<char, std::char_traits<char>> *, int)
        call      _ZNSolsEi                                     #test.cpp:20.19
..___tag_value_main.10:
                                # LOE rax rbx r12 r13 r14 r15
..B1.6:                         # Preds ..B1.5
        movq      %rax, %rdi                                    #test.cpp:20.27
        movl      $.L_2__STRING.0, %esi                         #test.cpp:20.27
..___tag_value_main.11:
#       std::operator<<<std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>> &, const char *)
        call      _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc #test.cpp:20.27
..___tag_value_main.12:
                                # LOE rbx r12 r13 r14 r15
..B1.7:                         # Preds ..B1.6
        incq      %r12                                          #test.cpp:19.30
        cmpq      $64, %r12                                     #test.cpp:19.25
        jl        ..B1.5        # Prob 98%                      #test.cpp:19.25
                                # LOE rbx r12 r13 r14 r15
..B1.8:                         # Preds ..B1.7
        movl      $_ZSt4cout, %edi                              #test.cpp:22.15
        movl      $.L_2__STRING.1, %esi                         #test.cpp:22.15
        movq      (%rsp), %r12                                  #
	.cfi_restore 12
..___tag_value_main.14:
#       std::operator<<<std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>> &, const char *)
        call      _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc #test.cpp:22.15
..___tag_value_main.15:
	.cfi_escape 0x10, 0x0c, 0x0e, 0x38, 0x1c, 0x0d, 0x80, 0xff, 0xff, 0xff, 0x1a, 0x0d, 0x80, 0xfe, 0xff, 0xff, 0x22
                                # LOE rbx r12 r13 r14 r15 r12d r12b
..B1.9:                         # Preds ..B1.8
        xorl      %eax, %eax                                    #test.cpp:23.12
        movq      %rbp, %rsp                                    #test.cpp:23.12
        popq      %rbp                                          #test.cpp:23.12
	.cfi_def_cfa 7, 8
	.cfi_restore 6
        ret                                                     #test.cpp:23.12
        .align    16,0x90
	.cfi_endproc
                                # LOE
# mark_end;
	.type	main,@function
	.size	main,.-main
	.data
# -- End  main
	.section .text._ZStorSt12_Ios_IostateS_, "xaG",@progbits,_ZStorSt12_Ios_IostateS_,comdat
..TXTST1:
# -- Begin  _ZStorSt12_Ios_IostateS_
	.section .text._ZStorSt12_Ios_IostateS_, "xaG",@progbits,_ZStorSt12_Ios_IostateS_,comdat
# mark_begin;
       .align    16,0x90
	.weak _ZStorSt12_Ios_IostateS_
# --- std::operator|(std::_Ios_Iostate, std::_Ios_Iostate)
_ZStorSt12_Ios_IostateS_:
# parameter 1: %edi
# parameter 2: %esi
..B2.1:                         # Preds ..B2.0
	.cfi_startproc
	.cfi_personality 0x3,__gxx_personality_v0
..___tag_value__ZStorSt12_Ios_IostateS_.20:
..L21:
                                                         #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/ios_base.h:158.3
        orl       %esi, %edi                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/ios_base.h:158.66
        movl      %edi, %eax                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/ios_base.h:158.66
        ret                                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/ios_base.h:158.66
        .align    16,0x90
	.cfi_endproc
                                # LOE
# mark_end;
	.type	_ZStorSt12_Ios_IostateS_,@function
	.size	_ZStorSt12_Ios_IostateS_,.-_ZStorSt12_Ios_IostateS_
	.data
# -- End  _ZStorSt12_Ios_IostateS_
	.section .text._ZNSt11char_traitsIcE6lengthEPKc, "xaG",@progbits,_ZNSt11char_traitsIcE6lengthEPKc,comdat
..TXTST2:
# -- Begin  _ZNSt11char_traitsIcE6lengthEPKc
	.section .text._ZNSt11char_traitsIcE6lengthEPKc, "xaG",@progbits,_ZNSt11char_traitsIcE6lengthEPKc,comdat
# mark_begin;
       .align    16,0x90
	.weak _ZNSt11char_traitsIcE6lengthEPKc
# --- std::char_traits<char>::length(const std::char_traits<char>::char_type *)
_ZNSt11char_traitsIcE6lengthEPKc:
# parameter 1: %rdi
..B3.1:                         # Preds ..B3.0
	.cfi_startproc
	.cfi_personality 0x3,__gxx_personality_v0
..___tag_value__ZNSt11char_traitsIcE6lengthEPKc.23:
..L24:
                                                         #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.7
        pushq     %rsi                                          #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.7
	.cfi_def_cfa_offset 16
        movq      %rdi, %rdx                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        movq      %rdx, %rcx                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        andq      $-16, %rdx                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        pxor      %xmm0, %xmm0                                  #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        pcmpeqb   (%rdx), %xmm0                                 #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        pmovmskb  %xmm0, %eax                                   #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        andl      $15, %ecx                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        shrl      %cl, %eax                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        bsf       %eax, %eax                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        jne       ..L26         # Prob 60%                      #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        movq      %rdx, %rax                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        addq      %rcx, %rdx                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        call      __intel_sse2_strlen                           #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
..L26:                                                          #
                                # LOE rax rbx rbp r12 r13 r14 r15
..B3.4:                         # Preds ..B3.1
        popq      %rcx                                          #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
	.cfi_def_cfa_offset 8
        ret                                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        .align    16,0x90
	.cfi_endproc
                                # LOE
# mark_end;
	.type	_ZNSt11char_traitsIcE6lengthEPKc,@function
	.size	_ZNSt11char_traitsIcE6lengthEPKc,.-_ZNSt11char_traitsIcE6lengthEPKc
	.data
# -- End  _ZNSt11char_traitsIcE6lengthEPKc
	.text
# -- Begin  _Z4foo0iPi
	.text
# mark_begin;
       .align    16,0x90
	.globl _Z4foo0iPi
# --- foo0(int, int *)
_Z4foo0iPi:
# parameter 1: %edi
# parameter 2: %rsi
..B4.1:                         # Preds ..B4.0
	.cfi_startproc
	.cfi_personality 0x3,__gxx_personality_v0
..___tag_value__Z4foo0iPi.29:
..L30:
                                                         #test.cpp:4.1
        subq      $40, %rsp                                     #test.cpp:4.1
	.cfi_def_cfa_offset 48
        decl      %edi                                          #test.cpp:5.29
        testl     %edi, %edi                                    #test.cpp:5.29
        jle       ..B4.33       # Prob 50%                      #test.cpp:5.29
                                # LOE rbx rbp rsi r12 r13 r14 r15 edi
..B4.2:                         # Preds ..B4.1
        cmpl      $4, %edi                                      #test.cpp:5.5
        jl        ..B4.34       # Prob 10%                      #test.cpp:5.5
                                # LOE rbx rbp rsi r12 r13 r14 r15 edi
..B4.3:                         # Preds ..B4.2
        movl      $4, %edx                                      #test.cpp:5.34
        movl      %edi, %eax                                    #test.cpp:5.5
        andl      $-4, %eax                                     #test.cpp:5.5
        cltq                                                    #test.cpp:5.5
        pxor      %xmm9, %xmm9                                  #test.cpp:6.22
        movdqa    .L_2il0floatpacket.0(%rip), %xmm10            #test.cpp:5.34
        movd      %edx, %xmm0                                   #test.cpp:5.34
        xorl      %edx, %edx                                    #test.cpp:5.5
        pshufd    $0, %xmm0, %xmm11                             #test.cpp:5.34
        movdqa    .L_2il0floatpacket.1(%rip), %xmm8             #test.cpp:6.17
        movq      %r12, 16(%rsp)                                #test.cpp:6.17
	.cfi_offset 12, -32
        movq      %rsi, %r12                                    #test.cpp:6.17
        movq      %r13, 8(%rsp)                                 #test.cpp:6.17
	.cfi_offset 13, -40
        movq      %rdx, %r13                                    #test.cpp:6.17
        movq      %r14, (%rsp)                                  #test.cpp:6.17
	.cfi_offset 14, -48
        movl      %edi, %r14d                                   #test.cpp:6.17
        movq      %rbp, 24(%rsp)                                #test.cpp:6.17
	.cfi_offset 6, -24
        movq      %rax, %rbp                                    #test.cpp:6.17
                                # LOE rbx rbp r12 r13 r15 r14d xmm8 xmm9 xmm10 xmm11
..B4.4:                         # Preds ..B4.25 ..B4.3
        movdqa    %xmm10, %xmm0                                 #test.cpp:6.22
        movdqa    %xmm8, %xmm1                                  #test.cpp:6.22
        call      __svml_irem4                                  #test.cpp:6.22
        pcmpeqd   %xmm9, %xmm0                                  #test.cpp:6.22
        movmskps  %xmm0, %esi                                   #test.cpp:6.22
        testl     %esi, %esi                                    #test.cpp:6.22
        je        ..B4.25       # Prob 20%                      #test.cpp:6.22
                                # LOE rbx rbp r12 r13 r15 esi r14d xmm8 xmm9 xmm10 xmm11
..B4.5:                         # Preds ..B4.4
        movl      %esi, %r11d                                   #test.cpp:7.20
        andl      $1, %r11d                                     #test.cpp:7.20
        je        ..B4.7        # Prob 40%                      #test.cpp:7.20
                                # LOE rbx rbp r12 r13 r15 esi r11d r14d xmm8 xmm9 xmm10 xmm11
..B4.6:                         # Preds ..B4.5
        movl      4(%r12,%r13,4), %ecx                          #test.cpp:7.20
        jmp       ..B4.8        # Prob 100%                     #test.cpp:7.20
                                # LOE rbx rbp r12 r13 r15 ecx esi r11d r14d xmm8 xmm9 xmm10 xmm11
..B4.7:                         # Preds ..B4.5
        xorl      %ecx, %ecx                                    #test.cpp:7.20
                                # LOE rbx rbp r12 r13 r15 ecx esi r11d r14d xmm8 xmm9 xmm10 xmm11
..B4.8:                         # Preds ..B4.6 ..B4.7
        movl      %esi, %eax                                    #test.cpp:7.20
        andl      $2, %eax                                      #test.cpp:7.20
        je        ..B4.10       # Prob 40%                      #test.cpp:7.20
                                # LOE rbx rbp r12 r13 r15 eax ecx esi r11d r14d xmm8 xmm9 xmm10 xmm11
..B4.9:                         # Preds ..B4.8
        movl      8(%r12,%r13,4), %r8d                          #test.cpp:7.20
        jmp       ..B4.11       # Prob 100%                     #test.cpp:7.20
                                # LOE rbx rbp r12 r13 r15 eax ecx esi r8d r11d r14d xmm8 xmm9 xmm10 xmm11
..B4.10:                        # Preds ..B4.8
        xorl      %r8d, %r8d                                    #test.cpp:7.20
                                # LOE rbx rbp r12 r13 r15 eax ecx esi r8d r11d r14d xmm8 xmm9 xmm10 xmm11
..B4.11:                        # Preds ..B4.9 ..B4.10
        movl      %esi, %edx                                    #test.cpp:7.20
        andl      $4, %edx                                      #test.cpp:7.20
        je        ..B4.13       # Prob 40%                      #test.cpp:7.20
                                # LOE rbx rbp r12 r13 r15 eax edx ecx esi r8d r11d r14d xmm8 xmm9 xmm10 xmm11
..B4.12:                        # Preds ..B4.11
        movl      12(%r12,%r13,4), %r9d                         #test.cpp:7.20
        jmp       ..B4.14       # Prob 100%                     #test.cpp:7.20
                                # LOE rbx rbp r12 r13 r15 eax edx ecx esi r8d r9d r11d r14d xmm8 xmm9 xmm10 xmm11
..B4.13:                        # Preds ..B4.11
        xorl      %r9d, %r9d                                    #test.cpp:7.20
                                # LOE rbx rbp r12 r13 r15 eax edx ecx esi r8d r9d r11d r14d xmm8 xmm9 xmm10 xmm11
..B4.14:                        # Preds ..B4.12 ..B4.13
        andl      $8, %esi                                      #test.cpp:7.20
        je        ..B4.16       # Prob 40%                      #test.cpp:7.20
                                # LOE rbx rbp r12 r13 r15 eax edx ecx esi r8d r9d r11d r14d xmm8 xmm9 xmm10 xmm11
..B4.15:                        # Preds ..B4.14
        movl      16(%r12,%r13,4), %r10d                        #test.cpp:7.20
        jmp       ..B4.17       # Prob 100%                     #test.cpp:7.20
                                # LOE rbx rbp r12 r13 r15 eax edx ecx esi r8d r9d r10d r11d r14d xmm8 xmm9 xmm10 xmm11
..B4.16:                        # Preds ..B4.14
        xorl      %r10d, %r10d                                  #test.cpp:7.20
                                # LOE rbx rbp r12 r13 r15 eax edx ecx esi r8d r9d r10d r11d r14d xmm8 xmm9 xmm10 xmm11
..B4.17:                        # Preds ..B4.15 ..B4.16
        movd      %ecx, %xmm5                                   #test.cpp:7.20
        movd      %r8d, %xmm2                                   #test.cpp:7.20
        movd      %r9d, %xmm4                                   #test.cpp:7.20
        movd      %r10d, %xmm3                                  #test.cpp:7.20
        punpcklqdq %xmm2, %xmm5                                 #test.cpp:7.20
        testl     %r11d, %r11d                                  #test.cpp:7.13
        punpcklqdq %xmm3, %xmm4                                 #test.cpp:7.20
        shufps    $136, %xmm4, %xmm5                            #test.cpp:7.20
        je        ..B4.19       # Prob 40%                      #test.cpp:7.13
                                # LOE rbx rbp r12 r13 r15 eax edx esi r14d xmm5 xmm8 xmm9 xmm10 xmm11
..B4.18:                        # Preds ..B4.17
        movd      %xmm5, (%r12,%r13,4)                          #test.cpp:7.13
                                # LOE rbx rbp r12 r13 r15 eax edx esi r14d xmm5 xmm8 xmm9 xmm10 xmm11
..B4.19:                        # Preds ..B4.17 ..B4.18
        psrldq    $4, %xmm5                                     #test.cpp:7.13
        testl     %eax, %eax                                    #test.cpp:7.13
        je        ..B4.21       # Prob 40%                      #test.cpp:7.13
                                # LOE rbx rbp r12 r13 r15 edx esi r14d xmm5 xmm8 xmm9 xmm10 xmm11
..B4.20:                        # Preds ..B4.19
        movd      %xmm5, 4(%r12,%r13,4)                         #test.cpp:7.13
                                # LOE rbx rbp r12 r13 r15 edx esi r14d xmm5 xmm8 xmm9 xmm10 xmm11
..B4.21:                        # Preds ..B4.19 ..B4.20
        psrldq    $4, %xmm5                                     #test.cpp:7.13
        testl     %edx, %edx                                    #test.cpp:7.13
        je        ..B4.23       # Prob 40%                      #test.cpp:7.13
                                # LOE rbx rbp r12 r13 r15 esi r14d xmm5 xmm8 xmm9 xmm10 xmm11
..B4.22:                        # Preds ..B4.21
        movd      %xmm5, 8(%r12,%r13,4)                         #test.cpp:7.13
                                # LOE rbx rbp r12 r13 r15 esi r14d xmm5 xmm8 xmm9 xmm10 xmm11
..B4.23:                        # Preds ..B4.21 ..B4.22
        psrldq    $4, %xmm5                                     #test.cpp:7.13
        testl     %esi, %esi                                    #test.cpp:7.13
        je        ..B4.25       # Prob 40%                      #test.cpp:7.13
                                # LOE rbx rbp r12 r13 r15 r14d xmm5 xmm8 xmm9 xmm10 xmm11
..B4.24:                        # Preds ..B4.23
        movd      %xmm5, 12(%r12,%r13,4)                        #test.cpp:7.13
                                # LOE rbx rbp r12 r13 r15 r14d xmm8 xmm9 xmm10 xmm11
..B4.25:                        # Preds ..B4.4 ..B4.23 ..B4.24
        addq      $4, %r13                                      #test.cpp:5.5
        paddd     %xmm11, %xmm10                                #test.cpp:5.34
        cmpq      %rbp, %r13                                    #test.cpp:5.5
        jb        ..B4.4        # Prob 82%                      #test.cpp:5.5
                                # LOE rbx rbp r12 r13 r15 r14d xmm8 xmm9 xmm10 xmm11
..B4.26:                        # Preds ..B4.25
        movq      8(%rsp), %r13                                 #
	.cfi_restore 13
        movq      %rbp, %rax                                    #
        movq      24(%rsp), %rbp                                #
	.cfi_restore 6
        movq      %r12, %rsi                                    #
        movq      16(%rsp), %r12                                #
	.cfi_restore 12
        movl      %r14d, %edi                                   #
        movq      (%rsp), %r14                                  #
	.cfi_restore 14
                                # LOE rax rbx rbp rsi r12 r13 r14 r15 edi
..B4.27:                        # Preds ..B4.26 ..B4.34
        cmpl      %edi, %eax                                    #test.cpp:5.5
        jae       ..B4.33       # Prob 9%                       #test.cpp:5.5
                                # LOE rbx rbp rsi r12 r13 r14 r15 eax edi
..B4.29:                        # Preds ..B4.27 ..B4.31
        movl      %eax, %edx                                    #test.cpp:6.17
        andl      $-2147483647, %edx                            #test.cpp:6.17
        jge       ..B4.37       # Prob 50%                      #test.cpp:6.17
                                # LOE rbx rbp rsi r12 r13 r14 r15 eax edx edi
..B4.38:                        # Preds ..B4.29
        subl      $1, %edx                                      #test.cpp:6.17
        orl       $-2, %edx                                     #test.cpp:6.17
        incl      %edx                                          #test.cpp:6.17
                                # LOE rbx rbp rsi r12 r13 r14 r15 eax edx edi
..B4.37:                        # Preds ..B4.29 ..B4.38
        testl     %edx, %edx                                    #test.cpp:6.22
        jne       ..B4.31       # Prob 50%                      #test.cpp:6.22
                                # LOE rbx rbp rsi r12 r13 r14 r15 eax edi
..B4.30:                        # Preds ..B4.37
        movslq    %eax, %rax                                    #test.cpp:7.20
        movl      4(%rsi,%rax,4), %edx                          #test.cpp:7.20
        movl      %edx, (%rsi,%rax,4)                           #test.cpp:7.13
                                # LOE rbx rbp rsi r12 r13 r14 r15 eax edi
..B4.31:                        # Preds ..B4.37 ..B4.30
        incl      %eax                                          #test.cpp:5.5
        cmpl      %edi, %eax                                    #test.cpp:5.5
        jb        ..B4.29       # Prob 82%                      #test.cpp:5.5
                                # LOE rbx rbp rsi r12 r13 r14 r15 eax edi
..B4.33:                        # Preds ..B4.31 ..B4.27 ..B4.1
        addq      $40, %rsp                                     #test.cpp:10.1
	.cfi_def_cfa_offset 8
        ret                                                     #test.cpp:10.1
	.cfi_def_cfa_offset 48
                                # LOE
..B4.34:                        # Preds ..B4.2                  # Infreq
        xorl      %eax, %eax                                    #test.cpp:5.5
        jmp       ..B4.27       # Prob 100%                     #test.cpp:5.5
        .align    16,0x90
	.cfi_endproc
                                # LOE rax rbx rbp rsi r12 r13 r14 r15 edi
# mark_end;
	.type	_Z4foo0iPi,@function
	.size	_Z4foo0iPi,.-_Z4foo0iPi
	.data
# -- End  _Z4foo0iPi
	.text
# -- Begin  __sti__$E
	.text
# mark_begin;
       .align    16,0x90
# --- __sti__$E()
__sti__$E:
..B5.1:                         # Preds ..B5.0
	.cfi_startproc
	.cfi_personality 0x3,__gxx_personality_v0
..___tag_value___sti__$E.43:
..L44:
                                                         #
        pushq     %rsi                                          #
	.cfi_def_cfa_offset 16
        movl      $_ZSt8__ioinit, %edi                          #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
..___tag_value___sti__$E.46:
#       std::ios_base::Init::Init(std::ios_base::Init *)
        call      _ZNSt8ios_base4InitC1Ev                       #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
..___tag_value___sti__$E.47:
                                # LOE rbx rbp r12 r13 r14 r15
..B5.2:                         # Preds ..B5.1
        movl      $_ZNSt8ios_base4InitD1Ev, %edi                #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
        movl      $_ZSt8__ioinit, %esi                          #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
        movl      $__dso_handle, %edx                           #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
        addq      $8, %rsp                                      #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
	.cfi_def_cfa_offset 8
#       __cxa_atexit()
        jmp       __cxa_atexit                                  #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
        .align    16,0x90
	.cfi_endproc
                                # LOE
# mark_end;
	.type	__sti__$E,@function
	.size	__sti__$E,.-__sti__$E
	.data
# -- End  __sti__$E
	.bss
	.align 4
	.align 1
_ZSt8__ioinit:
	.type	_ZSt8__ioinit,@object
	.size	_ZSt8__ioinit,1
	.space 1	# pad
	.section .rodata, "a"
	.align 16
	.align 16
.L_2il0floatpacket.0:
	.long	0x00000000,0x00000001,0x00000002,0x00000003
	.type	.L_2il0floatpacket.0,@object
	.size	.L_2il0floatpacket.0,16
	.align 16
.L_2il0floatpacket.1:
	.long	0x00000002,0x00000002,0x00000002,0x00000002
	.type	.L_2il0floatpacket.1,@object
	.size	.L_2il0floatpacket.1,16
	.section .rodata.str1.4, "aMS",@progbits,1
	.align 4
	.align 4
.L_2__STRING.0:
	.word	8236
	.byte	0
	.type	.L_2__STRING.0,@object
	.size	.L_2__STRING.0,3
	.space 1, 0x00 	# pad
	.align 4
.L_2__STRING.1:
	.word	10
	.type	.L_2__STRING.1,@object
	.size	.L_2__STRING.1,2
	.section .ctors, "wa"
	.align 8
__init_0:
	.type	__init_0,@object
	.size	__init_0,8
	.quad	__sti__$E
	.data
	.hidden __dso_handle
# mark_proc_addr_taken __sti__$E;
	.section .note.GNU-stack, ""
// -- Begin DWARF2 SEGMENT .eh_frame
	.section .eh_frame,"a",@progbits
.eh_frame_seg:
	.align 8
# End
