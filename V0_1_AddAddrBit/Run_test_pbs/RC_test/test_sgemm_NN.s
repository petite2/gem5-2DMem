# mark_description "Intel(R) C++ Intel(R) 64 Compiler for applications running on Intel(R) 64, Version 16.0.1.150 Build 20151021";
# mark_description "";
# mark_description "-static -O0 -S -fcode-asm -o test_sgemm_NN.s -L/usr/lib/x86_64-redhat-linux5E/lib64/";
	.file "test_sgemm_NN.cpp"
	.section .text._ZNSt11char_traitsIcE6lengthEPKc, "xaG",@progbits,_ZNSt11char_traitsIcE6lengthEPKc,comdat
..TXTST0:
# -- Begin  _ZNSt11char_traitsIcE6lengthEPKc
	.section .text._ZNSt11char_traitsIcE6lengthEPKc, "xaG",@progbits,_ZNSt11char_traitsIcE6lengthEPKc,comdat
# mark_begin;
       .align    2,0x90
	.weak _ZNSt11char_traitsIcE6lengthEPKc
# --- std::char_traits<char>::length(const std::char_traits<char>::char_type *)
_ZNSt11char_traitsIcE6lengthEPKc:
# parameter 1: %rdi
..B1.1:                         # Preds ..B1.0
	.cfi_startproc
	.cfi_personality 0x3,__gxx_personality_v0
..___tag_value__ZNSt11char_traitsIcE6lengthEPKc.1:
..L2:
                                                          #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.7
        pushq     %rbp                                          #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.7
	.cfi_def_cfa_offset 16
        movq      %rsp, %rbp                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.7
	.cfi_def_cfa 6, 16
	.cfi_offset 6, -16
        subq      $16, %rsp                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.7
        movq      %rdi, -16(%rbp)                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.7
        movq      -16(%rbp), %rax                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        movq      %rax, %rcx                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        xorl      %eax, %eax                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
..L7:                                                           #
        movzbl    (%rcx,%rax), %edx                             #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        testl     %edx, %edx                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        je        ..L6          # Prob 50%                      #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        movzbl    1(%rcx,%rax), %edx                            #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        addq      $2, %rax                                      #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        testl     %edx, %edx                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        jne       ..L7          # Prob 50%                      #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        decq      %rax                                          #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
..L6:                                                           #
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B1.4:                         # Preds ..B1.1
        movq      %rax, -8(%rbp)                                #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        movq      -8(%rbp), %rax                                #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        leave                                                   #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
	.cfi_restore 6
        ret                                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/char_traits.h:259.16
        .align    2,0x90
	.cfi_endproc
                                # LOE
# mark_end;
	.type	_ZNSt11char_traitsIcE6lengthEPKc,@function
	.size	_ZNSt11char_traitsIcE6lengthEPKc,.-_ZNSt11char_traitsIcE6lengthEPKc
	.data
# -- End  _ZNSt11char_traitsIcE6lengthEPKc
	.section .text._ZNKSt5ctypeIcE5widenEc, "xaG",@progbits,_ZNKSt5ctypeIcE5widenEc,comdat
..TXTST1:
# -- Begin  _ZNKSt5ctypeIcE5widenEc
	.section .text._ZNKSt5ctypeIcE5widenEc, "xaG",@progbits,_ZNKSt5ctypeIcE5widenEc,comdat
# mark_begin;
       .align    2,0x90
	.weak _ZNKSt5ctypeIcE5widenEc
# --- std::ctype<char>::widen(const std::ctype<char> *, char) const
_ZNKSt5ctypeIcE5widenEc:
# parameter 1: %rdi
# parameter 2: %esi
..B2.1:                         # Preds ..B2.0
	.cfi_startproc
	.cfi_personality 0x3,__gxx_personality_v0
..___tag_value__ZNKSt5ctypeIcE5widenEc.10:
..L11:
                                                         #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:866.7
        pushq     %rbp                                          #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:866.7
	.cfi_def_cfa_offset 16
        movq      %rsp, %rbp                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:866.7
	.cfi_def_cfa 6, 16
	.cfi_offset 6, -16
        subq      $32, %rsp                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:866.7
        movq      %rdi, -24(%rbp)                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:866.7
        movb      %sil, -16(%rbp)                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:866.7
        movq      -24(%rbp), %rax                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:867.6
        movsbl    56(%rax), %eax                                #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:867.6
        movsbq    %al, %rax                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:867.6
        testl     %eax, %eax                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:867.6
        je        ..B2.3        # Prob 50%                      #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:867.6
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B2.2:                         # Preds ..B2.1
        movsbl    -16(%rbp), %eax                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:868.47
        movsbq    %al, %rax                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:868.47
        movzbl    %al, %eax                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:868.11
        addq      -24(%rbp), %rax                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:868.11
        addq      $57, %rax                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:868.11
        movsbl    (%rax), %eax                                  #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:868.11
        leave                                                   #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:868.11
	.cfi_restore 6
        ret                                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:868.11
	.cfi_offset 6, -16
                                # LOE
..B2.3:                         # Preds ..B2.1
        movq      -24(%rbp), %rax                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:869.8
        movq      %rax, %rdi                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:869.8
..___tag_value__ZNKSt5ctypeIcE5widenEc.17:
        call      _ZNKSt5ctypeIcE13_M_widen_initEv              #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:869.8
..___tag_value__ZNKSt5ctypeIcE5widenEc.18:
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B2.4:                         # Preds ..B2.3
        movq      -24(%rbp), %rax                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:870.24
        movl      $6, %edx                                      #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:870.24
        movslq    %edx, %rdx                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:870.24
        imulq     $8, %rdx, %rdx                                #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:870.24
        addq      (%rax), %rdx                                  #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:870.24
        movq      (%rdx), %rax                                  #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:870.24
        movq      -24(%rbp), %rdx                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:870.24
        movsbl    -16(%rbp), %ecx                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:870.24
        movq      %rdx, %rdi                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:870.24
        movl      %ecx, %esi                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:870.24
..___tag_value__ZNKSt5ctypeIcE5widenEc.19:
        call      *%rax                                         #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:870.24
..___tag_value__ZNKSt5ctypeIcE5widenEc.20:
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip eax
..B2.8:                         # Preds ..B2.4
        movb      %al, -32(%rbp)                                #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:870.24
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B2.5:                         # Preds ..B2.8
        movsbl    -32(%rbp), %eax                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:870.24
        leave                                                   #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:870.24
	.cfi_restore 6
        ret                                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/locale_facets.h:870.24
        .align    2,0x90
	.cfi_endproc
                                # LOE
# mark_end;
	.type	_ZNKSt5ctypeIcE5widenEc,@function
	.size	_ZNKSt5ctypeIcE5widenEc,.-_ZNKSt5ctypeIcE5widenEc
	.data
# -- End  _ZNKSt5ctypeIcE5widenEc
	.section .text._ZStorSt12_Ios_IostateS_, "xaG",@progbits,_ZStorSt12_Ios_IostateS_,comdat
..TXTST2:
# -- Begin  _ZStorSt12_Ios_IostateS_
	.section .text._ZStorSt12_Ios_IostateS_, "xaG",@progbits,_ZStorSt12_Ios_IostateS_,comdat
# mark_begin;

	.weak _ZStorSt12_Ios_IostateS_
# --- std::operator|(std::_Ios_Iostate, std::_Ios_Iostate)
_ZStorSt12_Ios_IostateS_:
# parameter 1: %edi
# parameter 2: %esi
..B3.1:                         # Preds ..B3.0
	.cfi_startproc
	.cfi_personality 0x3,__gxx_personality_v0
..___tag_value__ZStorSt12_Ios_IostateS_.23:
..L24:
                                                         #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/ios_base.h:158.3
        pushq     %rbp                                          #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/ios_base.h:158.3
	.cfi_def_cfa_offset 16
        movq      %rsp, %rbp                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/ios_base.h:158.3
	.cfi_def_cfa 6, 16
	.cfi_offset 6, -16
        subq      $16, %rsp                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/ios_base.h:158.3
        movl      %edi, -16(%rbp)                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/ios_base.h:158.3
        movl      %esi, -8(%rbp)                                #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/ios_base.h:158.3
        movl      -8(%rbp), %eax                                #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/ios_base.h:158.66
        orl       -16(%rbp), %eax                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/ios_base.h:158.66
        leave                                                   #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/ios_base.h:158.66
	.cfi_restore 6
        ret                                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/ios_base.h:158.66
	.cfi_endproc
                                # LOE
# mark_end;
	.type	_ZStorSt12_Ios_IostateS_,@function
	.size	_ZStorSt12_Ios_IostateS_,.-_ZStorSt12_Ios_IostateS_
	.data
# -- End  _ZStorSt12_Ios_IostateS_
	.section .text._ZSt3maxIiERKT_S2_S2_, "xaG",@progbits,_ZSt3maxIiERKT_S2_S2_,comdat
..TXTST3:
# -- Begin  _ZSt3maxIiERKT_S2_S2_
	.section .text._ZSt3maxIiERKT_S2_S2_, "xaG",@progbits,_ZSt3maxIiERKT_S2_S2_,comdat
# mark_begin;

	.weak _ZSt3maxIiERKT_S2_S2_
# --- std::max<int>(const int &, const int &)
_ZSt3maxIiERKT_S2_S2_:
# parameter 1: %rdi
# parameter 2: %rsi
..B4.1:                         # Preds ..B4.0
	.cfi_startproc
	.cfi_personality 0x3,__gxx_personality_v0
..___tag_value__ZSt3maxIiERKT_S2_S2_.30:
..L31:
                                                         #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:217.5
        pushq     %rbp                                          #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:217.5
	.cfi_def_cfa_offset 16
        movq      %rsp, %rbp                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:217.5
	.cfi_def_cfa 6, 16
	.cfi_offset 6, -16
        subq      $16, %rsp                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:217.5
        movq      %rdi, -16(%rbp)                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:217.5
        movq      %rsi, -8(%rbp)                                #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:217.5
        movq      -16(%rbp), %rax                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:221.11
        movl      (%rax), %eax                                  #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:221.11
        movq      -8(%rbp), %rdx                                #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:221.17
        movl      (%rdx), %edx                                  #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:221.17
        cmpl      %edx, %eax                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:221.17
        jge       ..B4.3        # Prob 50%                      #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:221.17
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B4.2:                         # Preds ..B4.1
        movq      -8(%rbp), %rax                                #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:222.9
        leave                                                   #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:222.9
	.cfi_restore 6
        ret                                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:222.9
	.cfi_offset 6, -16
                                # LOE
..B4.3:                         # Preds ..B4.1
        movq      -16(%rbp), %rax                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:223.14
        leave                                                   #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:223.14
	.cfi_restore 6
        ret                                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/stl_algobase.h:223.14
	.cfi_endproc
                                # LOE
# mark_end;
	.type	_ZSt3maxIiERKT_S2_S2_,@function
	.size	_ZSt3maxIiERKT_S2_S2_,.-_ZSt3maxIiERKT_S2_S2_
	.data
# -- End  _ZSt3maxIiERKT_S2_S2_
	.section .text._ZSt13__check_facetISt5ctypeIcEERKT_PS3_, "xaG",@progbits,_ZSt13__check_facetISt5ctypeIcEERKT_PS3_,comdat
..TXTST4:
# -- Begin  _ZSt13__check_facetISt5ctypeIcEERKT_PS3_
	.section .text._ZSt13__check_facetISt5ctypeIcEERKT_PS3_, "xaG",@progbits,_ZSt13__check_facetISt5ctypeIcEERKT_PS3_,comdat
# mark_begin;

	.weak _ZSt13__check_facetISt5ctypeIcEERKT_PS3_
# --- std::__check_facet<std::basic_ios<char, std::char_traits<char>>::__ctype_type>(const std::basic_ios<char, std::char_traits<char>>::__ctype_type *)
_ZSt13__check_facetISt5ctypeIcEERKT_PS3_:
# parameter 1: %rdi
..B5.1:                         # Preds ..B5.0
	.cfi_startproc
	.cfi_personality 0x3,__gxx_personality_v0
..___tag_value__ZSt13__check_facetISt5ctypeIcEERKT_PS3_.39:
..L40:
                                                         #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/basic_ios.h:47.5
        pushq     %rbp                                          #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/basic_ios.h:47.5
	.cfi_def_cfa_offset 16
        movq      %rsp, %rbp                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/basic_ios.h:47.5
	.cfi_def_cfa 6, 16
	.cfi_offset 6, -16
        subq      $16, %rsp                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/basic_ios.h:47.5
        movq      %rdi, -16(%rbp)                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/basic_ios.h:47.5
        movq      -16(%rbp), %rax                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/basic_ios.h:48.12
        testq     %rax, %rax                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/basic_ios.h:48.12
        jne       ..B5.3        # Prob 50%                      #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/basic_ios.h:48.12
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B5.2:                         # Preds ..B5.1
..___tag_value__ZSt13__check_facetISt5ctypeIcEERKT_PS3_.44:
        call      _ZSt16__throw_bad_castv                       #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/basic_ios.h:49.2
..___tag_value__ZSt13__check_facetISt5ctypeIcEERKT_PS3_.45:
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B5.3:                         # Preds ..B5.1
        movq      -16(%rbp), %rax                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/basic_ios.h:50.15
        leave                                                   #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/basic_ios.h:50.15
	.cfi_restore 6
        ret                                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/bits/basic_ios.h:50.15
	.cfi_endproc
                                # LOE
# mark_end;
	.type	_ZSt13__check_facetISt5ctypeIcEERKT_PS3_,@function
	.size	_ZSt13__check_facetISt5ctypeIcEERKT_PS3_,.-_ZSt13__check_facetISt5ctypeIcEERKT_PS3_
	.data
# -- End  _ZSt13__check_facetISt5ctypeIcEERKT_PS3_
	.text
..TXTST5:
# -- Begin  _Z10print_addrPdS_S_iii
	.section .rodata, "a"
	.align 4
	.align 4
.L_2__STRING.0:
	.long	1919181921
	.long	1601401701
	.word	23361
	.byte	0
	.type	.L_2__STRING.0,@object
	.size	.L_2__STRING.0,11
	.space 1, 0x00 	# pad
	.align 4
.L_2__STRING.1:
	.word	44
	.type	.L_2__STRING.1,@object
	.size	.L_2__STRING.1,2
	.space 2, 0x00 	# pad
	.align 4
.L_2__STRING.2:
	.long	2112093
	.type	.L_2__STRING.2,@object
	.size	.L_2__STRING.2,4
	.align 4
.L_2__STRING.3:
	.long	1919181921
	.long	1601401701
	.word	23362
	.byte	0
	.type	.L_2__STRING.3,@object
	.size	.L_2__STRING.3,11
	.space 1, 0x00 	# pad
	.align 4
.L_2__STRING.4:
	.long	1919181921
	.long	1601401701
	.word	23363
	.byte	0
	.type	.L_2__STRING.4,@object
	.size	.L_2__STRING.4,11
	.text
# mark_begin;

	.globl _Z10print_addrPdS_S_iii
# --- print_addr(double *, double *, double *, int, int, int)
_Z10print_addrPdS_S_iii:
# parameter 1: %rdi
# parameter 2: %rsi
# parameter 3: %rdx
# parameter 4: %ecx
# parameter 5: %r8d
# parameter 6: %r9d
..B6.1:                         # Preds ..B6.0
	.cfi_startproc
	.cfi_personality 0x3,__gxx_personality_v0
..___tag_value__Z10print_addrPdS_S_iii.48:
..L49:
                                                         #test_sgemm_NN.cpp:10.75
        pushq     %rbp                                          #test_sgemm_NN.cpp:10.75
	.cfi_def_cfa_offset 16
        movq      %rsp, %rbp                                    #test_sgemm_NN.cpp:10.75
	.cfi_def_cfa 6, 16
	.cfi_offset 6, -16
        subq      $224, %rsp                                    #test_sgemm_NN.cpp:10.75
        movq      %rdi, -224(%rbp)                              #test_sgemm_NN.cpp:10.75
        movq      %rsi, -216(%rbp)                              #test_sgemm_NN.cpp:10.75
        movq      %rdx, -208(%rbp)                              #test_sgemm_NN.cpp:10.75
        movl      %ecx, -200(%rbp)                              #test_sgemm_NN.cpp:10.75
        movl      %r8d, -192(%rbp)                              #test_sgemm_NN.cpp:10.75
        movl      %r9d, -184(%rbp)                              #test_sgemm_NN.cpp:10.75
        movl      $_ZSt4cout, %eax                              #test_sgemm_NN.cpp:11.15
        movl      $.L_2__STRING.0, %edx                         #test_sgemm_NN.cpp:11.15
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:11.15
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:11.15
..___tag_value__Z10print_addrPdS_S_iii.53:
#       std::operator<<<std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>> &, const char *)
        call      _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc #test_sgemm_NN.cpp:11.15
..___tag_value__Z10print_addrPdS_S_iii.54:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.25:                        # Preds ..B6.1
        movq      %rax, -176(%rbp)                              #test_sgemm_NN.cpp:11.15
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.2:                         # Preds ..B6.25
        movq      -176(%rbp), %rax                              #test_sgemm_NN.cpp:11.31
        movl      -200(%rbp), %edx                              #test_sgemm_NN.cpp:11.31
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:11.31
        movl      %edx, %esi                                    #test_sgemm_NN.cpp:11.31
..___tag_value__Z10print_addrPdS_S_iii.55:
        call      _ZNSolsEi                                     #test_sgemm_NN.cpp:11.31
..___tag_value__Z10print_addrPdS_S_iii.56:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.26:                        # Preds ..B6.2
        movq      %rax, -168(%rbp)                              #test_sgemm_NN.cpp:11.31
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.3:                         # Preds ..B6.26
        movq      -168(%rbp), %rax                              #test_sgemm_NN.cpp:11.36
        movl      $.L_2__STRING.1, %edx                         #test_sgemm_NN.cpp:11.36
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:11.36
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:11.36
..___tag_value__Z10print_addrPdS_S_iii.57:
#       std::operator<<<std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>> &, const char *)
        call      _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc #test_sgemm_NN.cpp:11.36
..___tag_value__Z10print_addrPdS_S_iii.58:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.27:                        # Preds ..B6.3
        movq      %rax, -160(%rbp)                              #test_sgemm_NN.cpp:11.36
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.4:                         # Preds ..B6.27
        movq      -160(%rbp), %rax                              #test_sgemm_NN.cpp:11.43
        movl      -192(%rbp), %edx                              #test_sgemm_NN.cpp:11.43
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:11.43
        movl      %edx, %esi                                    #test_sgemm_NN.cpp:11.43
..___tag_value__Z10print_addrPdS_S_iii.59:
        call      _ZNSolsEi                                     #test_sgemm_NN.cpp:11.43
..___tag_value__Z10print_addrPdS_S_iii.60:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.28:                        # Preds ..B6.4
        movq      %rax, -152(%rbp)                              #test_sgemm_NN.cpp:11.43
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.5:                         # Preds ..B6.28
        movq      -152(%rbp), %rax                              #test_sgemm_NN.cpp:11.48
        movl      $.L_2__STRING.2, %edx                         #test_sgemm_NN.cpp:11.48
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:11.48
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:11.48
..___tag_value__Z10print_addrPdS_S_iii.61:
#       std::operator<<<std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>> &, const char *)
        call      _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc #test_sgemm_NN.cpp:11.48
..___tag_value__Z10print_addrPdS_S_iii.62:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.29:                        # Preds ..B6.5
        movq      %rax, -144(%rbp)                              #test_sgemm_NN.cpp:11.48
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.6:                         # Preds ..B6.29
        movq      -144(%rbp), %rax                              #test_sgemm_NN.cpp:11.57
        movl      -184(%rbp), %edx                              #test_sgemm_NN.cpp:11.57
        imull     -200(%rbp), %edx                              #test_sgemm_NN.cpp:11.57
        addl      -192(%rbp), %edx                              #test_sgemm_NN.cpp:11.57
        movslq    %edx, %rdx                                    #test_sgemm_NN.cpp:11.57
        imulq     $8, %rdx, %rdx                                #test_sgemm_NN.cpp:11.57
        addq      -224(%rbp), %rdx                              #test_sgemm_NN.cpp:11.57
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:11.57
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:11.57
..___tag_value__Z10print_addrPdS_S_iii.63:
#       std::basic_ostream<char, std::char_traits<char>>::operator<<(std::basic_ostream<char, std::char_traits<char>> *, const void *)
        call      _ZNSolsEPKv                                   #test_sgemm_NN.cpp:11.57
..___tag_value__Z10print_addrPdS_S_iii.64:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.30:                        # Preds ..B6.6
        movq      %rax, -136(%rbp)                              #test_sgemm_NN.cpp:11.57
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.7:                         # Preds ..B6.30
        movq      -136(%rbp), %rax                              #test_sgemm_NN.cpp:11.76
        movl      $_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_, %edx #test_sgemm_NN.cpp:11.76
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:11.76
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:11.76
..___tag_value__Z10print_addrPdS_S_iii.65:
#       std::basic_ostream<char, std::char_traits<char>>::operator<<(std::basic_ostream<char, std::char_traits<char>> *, std::basic_ostream<char, std::char_traits<char>>::__ostream_type &(*)(std::basic_ostream<char, std::char_traits<char>>::__ostream_type &))
        call      _ZNSolsEPFRSoS_E                              #test_sgemm_NN.cpp:11.76
..___tag_value__Z10print_addrPdS_S_iii.66:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.31:                        # Preds ..B6.7
        movq      %rax, -128(%rbp)                              #test_sgemm_NN.cpp:11.76
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.8:                         # Preds ..B6.31
        movl      $_ZSt4cout, %eax                              #test_sgemm_NN.cpp:12.15
        movl      $.L_2__STRING.3, %edx                         #test_sgemm_NN.cpp:12.15
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:12.15
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:12.15
..___tag_value__Z10print_addrPdS_S_iii.67:
#       std::operator<<<std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>> &, const char *)
        call      _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc #test_sgemm_NN.cpp:12.15
..___tag_value__Z10print_addrPdS_S_iii.68:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.32:                        # Preds ..B6.8
        movq      %rax, -120(%rbp)                              #test_sgemm_NN.cpp:12.15
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.9:                         # Preds ..B6.32
        movq      -120(%rbp), %rax                              #test_sgemm_NN.cpp:12.31
        movl      -200(%rbp), %edx                              #test_sgemm_NN.cpp:12.31
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:12.31
        movl      %edx, %esi                                    #test_sgemm_NN.cpp:12.31
..___tag_value__Z10print_addrPdS_S_iii.69:
        call      _ZNSolsEi                                     #test_sgemm_NN.cpp:12.31
..___tag_value__Z10print_addrPdS_S_iii.70:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.33:                        # Preds ..B6.9
        movq      %rax, -112(%rbp)                              #test_sgemm_NN.cpp:12.31
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.10:                        # Preds ..B6.33
        movq      -112(%rbp), %rax                              #test_sgemm_NN.cpp:12.36
        movl      $.L_2__STRING.1, %edx                         #test_sgemm_NN.cpp:12.36
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:12.36
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:12.36
..___tag_value__Z10print_addrPdS_S_iii.71:
#       std::operator<<<std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>> &, const char *)
        call      _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc #test_sgemm_NN.cpp:12.36
..___tag_value__Z10print_addrPdS_S_iii.72:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.34:                        # Preds ..B6.10
        movq      %rax, -104(%rbp)                              #test_sgemm_NN.cpp:12.36
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.11:                        # Preds ..B6.34
        movq      -104(%rbp), %rax                              #test_sgemm_NN.cpp:12.43
        movl      -192(%rbp), %edx                              #test_sgemm_NN.cpp:12.43
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:12.43
        movl      %edx, %esi                                    #test_sgemm_NN.cpp:12.43
..___tag_value__Z10print_addrPdS_S_iii.73:
        call      _ZNSolsEi                                     #test_sgemm_NN.cpp:12.43
..___tag_value__Z10print_addrPdS_S_iii.74:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.35:                        # Preds ..B6.11
        movq      %rax, -96(%rbp)                               #test_sgemm_NN.cpp:12.43
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.12:                        # Preds ..B6.35
        movq      -96(%rbp), %rax                               #test_sgemm_NN.cpp:12.48
        movl      $.L_2__STRING.2, %edx                         #test_sgemm_NN.cpp:12.48
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:12.48
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:12.48
..___tag_value__Z10print_addrPdS_S_iii.75:
#       std::operator<<<std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>> &, const char *)
        call      _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc #test_sgemm_NN.cpp:12.48
..___tag_value__Z10print_addrPdS_S_iii.76:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.36:                        # Preds ..B6.12
        movq      %rax, -88(%rbp)                               #test_sgemm_NN.cpp:12.48
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.13:                        # Preds ..B6.36
        movq      -88(%rbp), %rax                               #test_sgemm_NN.cpp:12.57
        movl      -184(%rbp), %edx                              #test_sgemm_NN.cpp:12.57
        imull     -200(%rbp), %edx                              #test_sgemm_NN.cpp:12.57
        addl      -192(%rbp), %edx                              #test_sgemm_NN.cpp:12.57
        movslq    %edx, %rdx                                    #test_sgemm_NN.cpp:12.57
        imulq     $8, %rdx, %rdx                                #test_sgemm_NN.cpp:12.57
        addq      -216(%rbp), %rdx                              #test_sgemm_NN.cpp:12.57
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:12.57
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:12.57
..___tag_value__Z10print_addrPdS_S_iii.77:
#       std::basic_ostream<char, std::char_traits<char>>::operator<<(std::basic_ostream<char, std::char_traits<char>> *, const void *)
        call      _ZNSolsEPKv                                   #test_sgemm_NN.cpp:12.57
..___tag_value__Z10print_addrPdS_S_iii.78:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.37:                        # Preds ..B6.13
        movq      %rax, -80(%rbp)                               #test_sgemm_NN.cpp:12.57
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.14:                        # Preds ..B6.37
        movq      -80(%rbp), %rax                               #test_sgemm_NN.cpp:12.76
        movl      $_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_, %edx #test_sgemm_NN.cpp:12.76
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:12.76
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:12.76
..___tag_value__Z10print_addrPdS_S_iii.79:
#       std::basic_ostream<char, std::char_traits<char>>::operator<<(std::basic_ostream<char, std::char_traits<char>> *, std::basic_ostream<char, std::char_traits<char>>::__ostream_type &(*)(std::basic_ostream<char, std::char_traits<char>>::__ostream_type &))
        call      _ZNSolsEPFRSoS_E                              #test_sgemm_NN.cpp:12.76
..___tag_value__Z10print_addrPdS_S_iii.80:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.38:                        # Preds ..B6.14
        movq      %rax, -72(%rbp)                               #test_sgemm_NN.cpp:12.76
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.15:                        # Preds ..B6.38
        movl      $_ZSt4cout, %eax                              #test_sgemm_NN.cpp:13.15
        movl      $.L_2__STRING.4, %edx                         #test_sgemm_NN.cpp:13.15
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:13.15
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:13.15
..___tag_value__Z10print_addrPdS_S_iii.81:
#       std::operator<<<std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>> &, const char *)
        call      _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc #test_sgemm_NN.cpp:13.15
..___tag_value__Z10print_addrPdS_S_iii.82:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.39:                        # Preds ..B6.15
        movq      %rax, -64(%rbp)                               #test_sgemm_NN.cpp:13.15
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.16:                        # Preds ..B6.39
        movq      -64(%rbp), %rax                               #test_sgemm_NN.cpp:13.31
        movl      -200(%rbp), %edx                              #test_sgemm_NN.cpp:13.31
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:13.31
        movl      %edx, %esi                                    #test_sgemm_NN.cpp:13.31
..___tag_value__Z10print_addrPdS_S_iii.83:
        call      _ZNSolsEi                                     #test_sgemm_NN.cpp:13.31
..___tag_value__Z10print_addrPdS_S_iii.84:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.40:                        # Preds ..B6.16
        movq      %rax, -56(%rbp)                               #test_sgemm_NN.cpp:13.31
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.17:                        # Preds ..B6.40
        movq      -56(%rbp), %rax                               #test_sgemm_NN.cpp:13.36
        movl      $.L_2__STRING.1, %edx                         #test_sgemm_NN.cpp:13.36
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:13.36
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:13.36
..___tag_value__Z10print_addrPdS_S_iii.85:
#       std::operator<<<std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>> &, const char *)
        call      _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc #test_sgemm_NN.cpp:13.36
..___tag_value__Z10print_addrPdS_S_iii.86:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.41:                        # Preds ..B6.17
        movq      %rax, -48(%rbp)                               #test_sgemm_NN.cpp:13.36
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.18:                        # Preds ..B6.41
        movq      -48(%rbp), %rax                               #test_sgemm_NN.cpp:13.43
        movl      -192(%rbp), %edx                              #test_sgemm_NN.cpp:13.43
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:13.43
        movl      %edx, %esi                                    #test_sgemm_NN.cpp:13.43
..___tag_value__Z10print_addrPdS_S_iii.87:
        call      _ZNSolsEi                                     #test_sgemm_NN.cpp:13.43
..___tag_value__Z10print_addrPdS_S_iii.88:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.42:                        # Preds ..B6.18
        movq      %rax, -40(%rbp)                               #test_sgemm_NN.cpp:13.43
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.19:                        # Preds ..B6.42
        movq      -40(%rbp), %rax                               #test_sgemm_NN.cpp:13.48
        movl      $.L_2__STRING.2, %edx                         #test_sgemm_NN.cpp:13.48
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:13.48
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:13.48
..___tag_value__Z10print_addrPdS_S_iii.89:
#       std::operator<<<std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>> &, const char *)
        call      _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc #test_sgemm_NN.cpp:13.48
..___tag_value__Z10print_addrPdS_S_iii.90:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.43:                        # Preds ..B6.19
        movq      %rax, -32(%rbp)                               #test_sgemm_NN.cpp:13.48
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.20:                        # Preds ..B6.43
        movq      -32(%rbp), %rax                               #test_sgemm_NN.cpp:13.57
        movl      -184(%rbp), %edx                              #test_sgemm_NN.cpp:13.57
        imull     -200(%rbp), %edx                              #test_sgemm_NN.cpp:13.57
        addl      -192(%rbp), %edx                              #test_sgemm_NN.cpp:13.57
        movslq    %edx, %rdx                                    #test_sgemm_NN.cpp:13.57
        imulq     $8, %rdx, %rdx                                #test_sgemm_NN.cpp:13.57
        addq      -208(%rbp), %rdx                              #test_sgemm_NN.cpp:13.57
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:13.57
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:13.57
..___tag_value__Z10print_addrPdS_S_iii.91:
#       std::basic_ostream<char, std::char_traits<char>>::operator<<(std::basic_ostream<char, std::char_traits<char>> *, const void *)
        call      _ZNSolsEPKv                                   #test_sgemm_NN.cpp:13.57
..___tag_value__Z10print_addrPdS_S_iii.92:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.44:                        # Preds ..B6.20
        movq      %rax, -24(%rbp)                               #test_sgemm_NN.cpp:13.57
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.21:                        # Preds ..B6.44
        movq      -24(%rbp), %rax                               #test_sgemm_NN.cpp:13.76
        movl      $_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_, %edx #test_sgemm_NN.cpp:13.76
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:13.76
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:13.76
..___tag_value__Z10print_addrPdS_S_iii.93:
#       std::basic_ostream<char, std::char_traits<char>>::operator<<(std::basic_ostream<char, std::char_traits<char>> *, std::basic_ostream<char, std::char_traits<char>>::__ostream_type &(*)(std::basic_ostream<char, std::char_traits<char>>::__ostream_type &))
        call      _ZNSolsEPFRSoS_E                              #test_sgemm_NN.cpp:13.76
..___tag_value__Z10print_addrPdS_S_iii.94:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B6.45:                        # Preds ..B6.21
        movq      %rax, -16(%rbp)                               #test_sgemm_NN.cpp:13.76
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B6.22:                        # Preds ..B6.45
        leave                                                   #test_sgemm_NN.cpp:14.1
	.cfi_restore 6
        ret                                                     #test_sgemm_NN.cpp:14.1
	.cfi_endproc
                                # LOE
# mark_end;
	.type	_Z10print_addrPdS_S_iii,@function
	.size	_Z10print_addrPdS_S_iii,.-_Z10print_addrPdS_S_iii
	.data
# -- End  _Z10print_addrPdS_S_iii
	.text
# -- Begin  main
	.section .rodata, "a"
	.space 5, 0x00 	# pad
	.align 8
.L_2il0floatpacket.1:
	.long	0x00000000,0x3ff00000
	.type	.L_2il0floatpacket.1,@object
	.size	.L_2il0floatpacket.1,8
	.align 4
.L_2__STRING.5:
	.long	1869771333
	.long	1159740018
	.long	1919906418
	.long	544108320
	.long	1869440365
	.long	1629518194
	.long	1668246636
	.long	1869182049
	.word	8558
	.byte	0
	.type	.L_2__STRING.5,@object
	.size	.L_2__STRING.5,35
	.space 1, 0x00 	# pad
	.align 4
.L_2__STRING.6:
	.long	1918989395
	.long	1735289204
	.long	1162302240
	.long	1314868557
	.word	78
	.type	.L_2__STRING.6,@object
	.size	.L_2__STRING.6,18
	.space 2, 0x00 	# pad
	.align 4
.L_2__STRING.7:
	.long	543452741
	.long	1394632303
	.long	1296909639
	.long	5131871
	.type	.L_2__STRING.7,@object
	.size	.L_2__STRING.7,16
	.text
# mark_begin;

	.globl main
# --- main(int, char **)
main:
# parameter 1: %edi
# parameter 2: %rsi
..B7.1:                         # Preds ..B7.0
	.cfi_startproc
	.cfi_personality 0x3,__gxx_personality_v0
..___tag_value_main.97:
..L98:
                                                         #test_sgemm_NN.cpp:17.1
        pushq     %rbp                                          #test_sgemm_NN.cpp:17.1
	.cfi_def_cfa_offset 16
        movq      %rsp, %rbp                                    #test_sgemm_NN.cpp:17.1
	.cfi_def_cfa 6, 16
	.cfi_offset 6, -16
        subq      $256, %rsp                                    #test_sgemm_NN.cpp:17.1
        movq      %rbx, -16(%rbp)                               #test_sgemm_NN.cpp:17.1
        movl      %edi, -168(%rbp)                              #test_sgemm_NN.cpp:17.1
        movq      %rsi, -160(%rbp)                              #test_sgemm_NN.cpp:17.1
        movl      $8, %eax                                      #test_sgemm_NN.cpp:21.15
        addq      -160(%rbp), %rax                              #test_sgemm_NN.cpp:21.15
        movq      (%rax), %rax                                  #test_sgemm_NN.cpp:21.15
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:21.15
..___tag_value_main.102:
#       atoi(const char *)
        call      atoi                                          #test_sgemm_NN.cpp:21.15
..___tag_value_main.103:
	.cfi_offset 3, -32
                                # LOE rbp rsp r12 r13 r14 r15 rip eax
..B7.30:                        # Preds ..B7.1
        movl      %eax, -256(%rbp)                              #test_sgemm_NN.cpp:21.15
        movl      -256(%rbp), %eax                              #test_sgemm_NN.cpp:21.15
        movl      %eax, -252(%rbp)                              #test_sgemm_NN.cpp:21.8
        movl      -252(%rbp), %eax                              #test_sgemm_NN.cpp:21.8
        movl      %eax, -248(%rbp)                              #test_sgemm_NN.cpp:21.8
        movl      -252(%rbp), %eax                              #test_sgemm_NN.cpp:21.8
        movl      %eax, -244(%rbp)                              #test_sgemm_NN.cpp:21.2
        movl      -248(%rbp), %eax                              #test_sgemm_NN.cpp:22.17
        imull     -244(%rbp), %eax                              #test_sgemm_NN.cpp:22.17
        movl      %eax, -240(%rbp)                              #test_sgemm_NN.cpp:22.10
        movl      $8, -236(%rbp)                                #test_sgemm_NN.cpp:23.35
        lea       -244(%rbp), %rax                              #test_sgemm_NN.cpp:23.19
        lea       -236(%rbp), %rdx                              #test_sgemm_NN.cpp:23.19
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:23.19
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:23.19
#       std::max<int>(const int &, const int &)
        call      _ZSt3maxIiERKT_S2_S2_                         #test_sgemm_NN.cpp:23.19
                                # LOE rax rbp rsp r12 r13 r14 r15 rip
..B7.29:                        # Preds ..B7.30
        movq      %rax, -152(%rbp)                              #test_sgemm_NN.cpp:23.19
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.2:                         # Preds ..B7.29
        movq      -152(%rbp), %rax                              #test_sgemm_NN.cpp:23.19
        movl      (%rax), %eax                                  #test_sgemm_NN.cpp:23.19
        movslq    %eax, %rax                                    #test_sgemm_NN.cpp:23.19
        imulq     $8, %rax, %rax                                #test_sgemm_NN.cpp:23.40
        movl      %eax, -232(%rbp)                              #test_sgemm_NN.cpp:23.17
        movl      -248(%rbp), %eax                              #test_sgemm_NN.cpp:25.29
        movl      %eax, -228(%rbp)                              #test_sgemm_NN.cpp:25.27
        movl      -244(%rbp), %eax                              #test_sgemm_NN.cpp:26.28
        movl      %eax, -224(%rbp)                              #test_sgemm_NN.cpp:26.26
        movl      -244(%rbp), %eax                              #test_sgemm_NN.cpp:27.35
        movl      %eax, -220(%rbp)                              #test_sgemm_NN.cpp:27.33
        movsd     .L_2il0floatpacket.1(%rip), %xmm0             #test_sgemm_NN.cpp:28.24
        movsd     %xmm0, -144(%rbp)                             #test_sgemm_NN.cpp:28.24
        movl      $1, -216(%rbp)                                #test_sgemm_NN.cpp:29.29
        lea       -216(%rbp), %rax                              #test_sgemm_NN.cpp:29.20
        lea       -228(%rbp), %rdx                              #test_sgemm_NN.cpp:29.20
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:29.20
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:29.20
#       std::max<int>(const int &, const int &)
        call      _ZSt3maxIiERKT_S2_S2_                         #test_sgemm_NN.cpp:29.20
                                # LOE rax rbp rsp r12 r13 r14 r15 rip
..B7.31:                        # Preds ..B7.2
        movq      %rax, -136(%rbp)                              #test_sgemm_NN.cpp:29.20
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.3:                         # Preds ..B7.31
        movq      -136(%rbp), %rax                              #test_sgemm_NN.cpp:29.20
        movl      (%rax), %eax                                  #test_sgemm_NN.cpp:29.20
        movl      %eax, -212(%rbp)                              #test_sgemm_NN.cpp:29.18
        movl      $1, -208(%rbp)                                #test_sgemm_NN.cpp:30.29
        lea       -208(%rbp), %rax                              #test_sgemm_NN.cpp:30.20
        lea       -220(%rbp), %rdx                              #test_sgemm_NN.cpp:30.20
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:30.20
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:30.20
#       std::max<int>(const int &, const int &)
        call      _ZSt3maxIiERKT_S2_S2_                         #test_sgemm_NN.cpp:30.20
                                # LOE rax rbp rsp r12 r13 r14 r15 rip
..B7.32:                        # Preds ..B7.3
        movq      %rax, -128(%rbp)                              #test_sgemm_NN.cpp:30.20
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.4:                         # Preds ..B7.32
        movq      -128(%rbp), %rax                              #test_sgemm_NN.cpp:30.20
        movl      (%rax), %eax                                  #test_sgemm_NN.cpp:30.20
        movl      %eax, -204(%rbp)                              #test_sgemm_NN.cpp:30.18
        pxor      %xmm0, %xmm0                                  #test_sgemm_NN.cpp:31.23
        movsd     %xmm0, -120(%rbp)                             #test_sgemm_NN.cpp:31.23
        movl      $1, -200(%rbp)                                #test_sgemm_NN.cpp:32.29
        lea       -200(%rbp), %rax                              #test_sgemm_NN.cpp:32.20
        lea       -228(%rbp), %rdx                              #test_sgemm_NN.cpp:32.20
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:32.20
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:32.20
#       std::max<int>(const int &, const int &)
        call      _ZSt3maxIiERKT_S2_S2_                         #test_sgemm_NN.cpp:32.20
                                # LOE rax rbp rsp r12 r13 r14 r15 rip
..B7.33:                        # Preds ..B7.4
        movq      %rax, -112(%rbp)                              #test_sgemm_NN.cpp:32.20
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.5:                         # Preds ..B7.33
        movq      -112(%rbp), %rax                              #test_sgemm_NN.cpp:32.20
        movl      (%rax), %eax                                  #test_sgemm_NN.cpp:32.20
        movl      %eax, -196(%rbp)                              #test_sgemm_NN.cpp:32.18
        lea       -104(%rbp), %rax                              #test_sgemm_NN.cpp:35.13
        movl      -232(%rbp), %edx                              #test_sgemm_NN.cpp:35.13
        movslq    %edx, %rdx                                    #test_sgemm_NN.cpp:35.13
        movl      -240(%rbp), %ecx                              #test_sgemm_NN.cpp:35.13
        movslq    %ecx, %rcx                                    #test_sgemm_NN.cpp:35.13
        imulq     $8, %rcx, %rcx                                #test_sgemm_NN.cpp:35.13
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:35.13
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:35.13
        movq      %rcx, %rdx                                    #test_sgemm_NN.cpp:35.13
..___tag_value_main.105:
#       posix_memalign(void **, size_t, size_t)
        call      posix_memalign                                #test_sgemm_NN.cpp:35.13
..___tag_value_main.106:
                                # LOE rbp rsp r12 r13 r14 r15 rip eax
..B7.34:                        # Preds ..B7.5
        movl      %eax, -192(%rbp)                              #test_sgemm_NN.cpp:35.13
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.6:                         # Preds ..B7.34
        movl      -192(%rbp), %eax                              #test_sgemm_NN.cpp:35.13
        movl      %eax, -188(%rbp)                              #test_sgemm_NN.cpp:35.5
        lea       -96(%rbp), %rax                               #test_sgemm_NN.cpp:36.21
        movl      -232(%rbp), %edx                              #test_sgemm_NN.cpp:36.21
        movslq    %edx, %rdx                                    #test_sgemm_NN.cpp:36.21
        movl      -240(%rbp), %ecx                              #test_sgemm_NN.cpp:36.21
        movslq    %ecx, %rcx                                    #test_sgemm_NN.cpp:36.21
        imulq     $8, %rcx, %rcx                                #test_sgemm_NN.cpp:36.21
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:36.21
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:36.21
        movq      %rcx, %rdx                                    #test_sgemm_NN.cpp:36.21
..___tag_value_main.107:
#       posix_memalign(void **, size_t, size_t)
        call      posix_memalign                                #test_sgemm_NN.cpp:36.21
..___tag_value_main.108:
                                # LOE rbp rsp r12 r13 r14 r15 rip eax
..B7.35:                        # Preds ..B7.6
        movl      %eax, -184(%rbp)                              #test_sgemm_NN.cpp:36.21
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.7:                         # Preds ..B7.35
        movl      -184(%rbp), %eax                              #test_sgemm_NN.cpp:36.21
        addl      -188(%rbp), %eax                              #test_sgemm_NN.cpp:36.21
        movl      %eax, -188(%rbp)                              #test_sgemm_NN.cpp:36.5
        lea       -88(%rbp), %rax                               #test_sgemm_NN.cpp:37.21
        movl      -232(%rbp), %edx                              #test_sgemm_NN.cpp:37.21
        movslq    %edx, %rdx                                    #test_sgemm_NN.cpp:37.21
        movl      -240(%rbp), %ecx                              #test_sgemm_NN.cpp:37.21
        movslq    %ecx, %rcx                                    #test_sgemm_NN.cpp:37.21
        imulq     $8, %rcx, %rcx                                #test_sgemm_NN.cpp:37.21
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:37.21
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:37.21
        movq      %rcx, %rdx                                    #test_sgemm_NN.cpp:37.21
..___tag_value_main.109:
#       posix_memalign(void **, size_t, size_t)
        call      posix_memalign                                #test_sgemm_NN.cpp:37.21
..___tag_value_main.110:
                                # LOE rbp rsp r12 r13 r14 r15 rip eax
..B7.36:                        # Preds ..B7.7
        movl      %eax, -180(%rbp)                              #test_sgemm_NN.cpp:37.21
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.8:                         # Preds ..B7.36
        movl      -180(%rbp), %eax                              #test_sgemm_NN.cpp:37.21
        addl      -188(%rbp), %eax                              #test_sgemm_NN.cpp:37.21
        movl      %eax, -188(%rbp)                              #test_sgemm_NN.cpp:37.5
        movl      -188(%rbp), %eax                              #test_sgemm_NN.cpp:44.9
        testl     %eax, %eax                                    #test_sgemm_NN.cpp:44.9
        je        ..B7.12       # Prob 50%                      #test_sgemm_NN.cpp:44.9
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.9:                         # Preds ..B7.8
        movl      $_ZSt4cout, %eax                              #test_sgemm_NN.cpp:45.19
        movl      $.L_2__STRING.5, %edx                         #test_sgemm_NN.cpp:45.19
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:45.19
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:45.19
..___tag_value_main.111:
#       std::operator<<<std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>> &, const char *)
        call      _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc #test_sgemm_NN.cpp:45.19
..___tag_value_main.112:
                                # LOE rax rbp rsp r12 r13 r14 r15 rip
..B7.37:                        # Preds ..B7.9
        movq      %rax, -80(%rbp)                               #test_sgemm_NN.cpp:45.19
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.10:                        # Preds ..B7.37
        movq      -80(%rbp), %rax                               #test_sgemm_NN.cpp:45.59
        movl      $_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_, %edx #test_sgemm_NN.cpp:45.59
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:45.59
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:45.59
..___tag_value_main.113:
#       std::basic_ostream<char, std::char_traits<char>>::operator<<(std::basic_ostream<char, std::char_traits<char>> *, std::basic_ostream<char, std::char_traits<char>>::__ostream_type &(*)(std::basic_ostream<char, std::char_traits<char>>::__ostream_type &))
        call      _ZNSolsEPFRSoS_E                              #test_sgemm_NN.cpp:45.59
..___tag_value_main.114:
                                # LOE rax rbp rsp r12 r13 r14 r15 rip
..B7.38:                        # Preds ..B7.10
        movq      %rax, -72(%rbp)                               #test_sgemm_NN.cpp:45.59
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.11:                        # Preds ..B7.38
        movl      $0, %eax                                      #test_sgemm_NN.cpp:46.16
        movq      -16(%rbp), %rbx                               #test_sgemm_NN.cpp:46.16
	.cfi_restore 3
        leave                                                   #test_sgemm_NN.cpp:46.16
	.cfi_restore 6
        ret                                                     #test_sgemm_NN.cpp:46.16
	.cfi_offset 3, -32
	.cfi_offset 6, -16
                                # LOE
..B7.12:                        # Preds ..B7.8
        movl      $0, -176(%rbp)                                #test_sgemm_NN.cpp:55.16
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.13:                        # Preds ..B7.14 ..B7.12
        movl      -176(%rbp), %eax                              #test_sgemm_NN.cpp:55.21
        movl      -244(%rbp), %edx                              #test_sgemm_NN.cpp:55.25
        cmpl      %edx, %eax                                    #test_sgemm_NN.cpp:55.25
        jl        ..B7.15       # Prob 50%                      #test_sgemm_NN.cpp:55.25
        jmp       ..B7.18       # Prob 100%                     #test_sgemm_NN.cpp:55.25
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.14:                        # Preds ..B7.16
        movl      $1, %eax                                      #test_sgemm_NN.cpp:55.34
        addl      -176(%rbp), %eax                              #test_sgemm_NN.cpp:55.34
        movl      %eax, -176(%rbp)                              #test_sgemm_NN.cpp:55.34
        jmp       ..B7.13       # Prob 100%                     #test_sgemm_NN.cpp:55.34
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.15:                        # Preds ..B7.13
        movl      $0, -172(%rbp)                                #test_sgemm_NN.cpp:56.20
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.16:                        # Preds ..B7.17 ..B7.15
        movl      -172(%rbp), %eax                              #test_sgemm_NN.cpp:56.25
        movl      -244(%rbp), %edx                              #test_sgemm_NN.cpp:56.29
        cmpl      %edx, %eax                                    #test_sgemm_NN.cpp:56.29
        jge       ..B7.14       # Prob 50%                      #test_sgemm_NN.cpp:56.29
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.17:                        # Preds ..B7.16
        movsd     .L_2il0floatpacket.1(%rip), %xmm0             #test_sgemm_NN.cpp:57.13
        movl      -244(%rbp), %eax                              #test_sgemm_NN.cpp:57.26
        imull     -176(%rbp), %eax                              #test_sgemm_NN.cpp:57.26
        addl      -172(%rbp), %eax                              #test_sgemm_NN.cpp:57.34
        movslq    %eax, %rax                                    #test_sgemm_NN.cpp:57.13
        imulq     $8, %rax, %rax                                #test_sgemm_NN.cpp:57.13
        addq      -104(%rbp), %rax                              #test_sgemm_NN.cpp:57.13
        movsd     %xmm0, (%rax)                                 #test_sgemm_NN.cpp:57.13
        movsd     .L_2il0floatpacket.1(%rip), %xmm0             #test_sgemm_NN.cpp:58.13
        movl      -244(%rbp), %eax                              #test_sgemm_NN.cpp:58.26
        imull     -176(%rbp), %eax                              #test_sgemm_NN.cpp:58.26
        addl      -172(%rbp), %eax                              #test_sgemm_NN.cpp:58.34
        movslq    %eax, %rax                                    #test_sgemm_NN.cpp:58.13
        imulq     $8, %rax, %rax                                #test_sgemm_NN.cpp:58.13
        addq      -96(%rbp), %rax                               #test_sgemm_NN.cpp:58.13
        movsd     %xmm0, (%rax)                                 #test_sgemm_NN.cpp:58.13
        pxor      %xmm0, %xmm0                                  #test_sgemm_NN.cpp:59.13
        movl      -244(%rbp), %eax                              #test_sgemm_NN.cpp:59.26
        imull     -176(%rbp), %eax                              #test_sgemm_NN.cpp:59.26
        addl      -172(%rbp), %eax                              #test_sgemm_NN.cpp:59.34
        movslq    %eax, %rax                                    #test_sgemm_NN.cpp:59.13
        imulq     $8, %rax, %rax                                #test_sgemm_NN.cpp:59.13
        addq      -88(%rbp), %rax                               #test_sgemm_NN.cpp:59.13
        movsd     %xmm0, (%rax)                                 #test_sgemm_NN.cpp:59.13
        movl      $1, %eax                                      #test_sgemm_NN.cpp:56.38
        addl      -172(%rbp), %eax                              #test_sgemm_NN.cpp:56.38
        movl      %eax, -172(%rbp)                              #test_sgemm_NN.cpp:56.38
        jmp       ..B7.16       # Prob 100%                     #test_sgemm_NN.cpp:56.38
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.18:                        # Preds ..B7.13
        movl      $_ZSt4cout, %eax                              #test_sgemm_NN.cpp:63.15
        movl      $.L_2__STRING.6, %edx                         #test_sgemm_NN.cpp:63.15
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:63.15
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:63.15
..___tag_value_main.119:
#       std::operator<<<std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>> &, const char *)
        call      _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc #test_sgemm_NN.cpp:63.15
..___tag_value_main.120:
                                # LOE rax rbp rsp r12 r13 r14 r15 rip
..B7.39:                        # Preds ..B7.18
        movq      %rax, -64(%rbp)                               #test_sgemm_NN.cpp:63.15
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.19:                        # Preds ..B7.39
        movq      -64(%rbp), %rax                               #test_sgemm_NN.cpp:63.38
        movl      $_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_, %edx #test_sgemm_NN.cpp:63.38
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:63.38
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:63.38
..___tag_value_main.121:
#       std::basic_ostream<char, std::char_traits<char>>::operator<<(std::basic_ostream<char, std::char_traits<char>> *, std::basic_ostream<char, std::char_traits<char>>::__ostream_type &(*)(std::basic_ostream<char, std::char_traits<char>>::__ostream_type &))
        call      _ZNSolsEPFRSoS_E                              #test_sgemm_NN.cpp:63.38
..___tag_value_main.122:
                                # LOE rax rbp rsp r12 r13 r14 r15 rip
..B7.40:                        # Preds ..B7.19
        movq      %rax, -56(%rbp)                               #test_sgemm_NN.cpp:63.38
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.20:                        # Preds ..B7.40
        addq      $-32, %rsp                                    #test_sgemm_NN.cpp:65.5
        movl      -228(%rbp), %eax                              #test_sgemm_NN.cpp:65.5
        movl      -224(%rbp), %edx                              #test_sgemm_NN.cpp:65.5
        movl      -220(%rbp), %ecx                              #test_sgemm_NN.cpp:65.5
        movsd     -144(%rbp), %xmm0                             #test_sgemm_NN.cpp:65.5
        movq      -104(%rbp), %rbx                              #test_sgemm_NN.cpp:65.5
        movl      -212(%rbp), %esi                              #test_sgemm_NN.cpp:65.5
        movq      -96(%rbp), %rdi                               #test_sgemm_NN.cpp:65.5
        movl      -204(%rbp), %r8d                              #test_sgemm_NN.cpp:65.5
        movl      %r8d, (%rsp)                                  #test_sgemm_NN.cpp:65.5
        movsd     -120(%rbp), %xmm1                             #test_sgemm_NN.cpp:65.5
        movq      -88(%rbp), %r8                                #test_sgemm_NN.cpp:65.5
        movq      %r8, 8(%rsp)                                  #test_sgemm_NN.cpp:65.5
        movl      -196(%rbp), %r8d                              #test_sgemm_NN.cpp:65.5
        movl      %r8d, 16(%rsp)                                #test_sgemm_NN.cpp:65.5
        movq      %rdi, -32(%rbp)                               #test_sgemm_NN.cpp:65.5
        movl      %eax, %edi                                    #test_sgemm_NN.cpp:65.5
        movl      %esi, -24(%rbp)                               #test_sgemm_NN.cpp:65.5
        movl      %edx, %esi                                    #test_sgemm_NN.cpp:65.5
        movl      %ecx, %edx                                    #test_sgemm_NN.cpp:65.5
        movq      %rbx, %rcx                                    #test_sgemm_NN.cpp:65.5
        movl      -24(%rbp), %eax                               #test_sgemm_NN.cpp:65.5
        movl      %eax, %r8d                                    #test_sgemm_NN.cpp:65.5
        movq      -32(%rbp), %rax                               #test_sgemm_NN.cpp:65.5
        movq      %rax, %r9                                     #test_sgemm_NN.cpp:65.5
..___tag_value_main.123:
        call      _Z8SGEMM_NNiiidPdiS_idS_i                     #test_sgemm_NN.cpp:65.5
..___tag_value_main.124:
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.41:                        # Preds ..B7.20
        addq      $32, %rsp                                     #test_sgemm_NN.cpp:65.5
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.21:                        # Preds ..B7.41
        movl      $_ZSt4cout, %eax                              #test_sgemm_NN.cpp:67.15
        movl      $.L_2__STRING.7, %edx                         #test_sgemm_NN.cpp:67.15
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:67.15
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:67.15
..___tag_value_main.125:
#       std::operator<<<std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>> &, const char *)
        call      _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc #test_sgemm_NN.cpp:67.15
..___tag_value_main.126:
                                # LOE rax rbp rsp r12 r13 r14 r15 rip
..B7.42:                        # Preds ..B7.21
        movq      %rax, -48(%rbp)                               #test_sgemm_NN.cpp:67.15
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.22:                        # Preds ..B7.42
        movq      -48(%rbp), %rax                               #test_sgemm_NN.cpp:67.36
        movl      $_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_, %edx #test_sgemm_NN.cpp:67.36
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:67.36
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:67.36
..___tag_value_main.127:
#       std::basic_ostream<char, std::char_traits<char>>::operator<<(std::basic_ostream<char, std::char_traits<char>> *, std::basic_ostream<char, std::char_traits<char>>::__ostream_type &(*)(std::basic_ostream<char, std::char_traits<char>>::__ostream_type &))
        call      _ZNSolsEPFRSoS_E                              #test_sgemm_NN.cpp:67.36
..___tag_value_main.128:
                                # LOE rax rbp rsp r12 r13 r14 r15 rip
..B7.43:                        # Preds ..B7.22
        movq      %rax, -40(%rbp)                               #test_sgemm_NN.cpp:67.36
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.23:                        # Preds ..B7.43
        movq      -104(%rbp), %rax                              #test_sgemm_NN.cpp:74.5
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:74.5
..___tag_value_main.129:
#       free(void *)
        call      free                                          #test_sgemm_NN.cpp:74.5
..___tag_value_main.130:
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.24:                        # Preds ..B7.23
        movq      -96(%rbp), %rax                               #test_sgemm_NN.cpp:75.5
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:75.5
..___tag_value_main.131:
#       free(void *)
        call      free                                          #test_sgemm_NN.cpp:75.5
..___tag_value_main.132:
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.25:                        # Preds ..B7.24
        movq      -88(%rbp), %rax                               #test_sgemm_NN.cpp:76.5
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:76.5
..___tag_value_main.133:
#       free(void *)
        call      free                                          #test_sgemm_NN.cpp:76.5
..___tag_value_main.134:
                                # LOE rbp rsp r12 r13 r14 r15 rip
..B7.26:                        # Preds ..B7.25
        movl      $0, %eax                                      #test_sgemm_NN.cpp:78.12
        movq      -16(%rbp), %rbx                               #test_sgemm_NN.cpp:78.12
	.cfi_restore 3
        leave                                                   #test_sgemm_NN.cpp:78.12
	.cfi_restore 6
        ret                                                     #test_sgemm_NN.cpp:78.12
	.cfi_endproc
                                # LOE
# mark_end;
	.type	main,@function
	.size	main,.-main
	.data
# -- End  main
	.text
# -- Begin  _Z8SGEMM_NNiiidPdiS_idS_i
	.section .rodata, "a"
	.align 4
.L_2__STRING.8:
	.long	1869771333
	.long	1461729906
	.long	1735290738
	.long	1886284064
	.long	2192501
	.type	.L_2__STRING.8,@object
	.size	.L_2__STRING.8,20
	.text
# mark_begin;

	.globl _Z8SGEMM_NNiiidPdiS_idS_i
# --- SGEMM_NN(int, int, int, double, double *, int, double *, int, double, double *, int)
_Z8SGEMM_NNiiidPdiS_idS_i:
# parameter 1: %edi
# parameter 2: %esi
# parameter 3: %edx
# parameter 4: %xmm0
# parameter 5: %rcx
# parameter 6: %r8d
# parameter 7: %r9
# parameter 8: 16 + %rbp
# parameter 9: %xmm1
# parameter 10: 24 + %rbp
# parameter 11: 32 + %rbp
..B8.1:                         # Preds ..B8.0
	.cfi_startproc
	.cfi_personality 0x3,__gxx_personality_v0
..___tag_value__Z8SGEMM_NNiiidPdiS_idS_i.138:
..L139:
                                                        #test_sgemm_NN.cpp:81.123
        pushq     %rbp                                          #test_sgemm_NN.cpp:81.123
	.cfi_def_cfa_offset 16
        movq      %rsp, %rbp                                    #test_sgemm_NN.cpp:81.123
	.cfi_def_cfa 6, 16
	.cfi_offset 6, -16
        subq      $128, %rsp                                    #test_sgemm_NN.cpp:81.123
        movl      %edi, -104(%rbp)                              #test_sgemm_NN.cpp:81.123
        movl      %esi, -96(%rbp)                               #test_sgemm_NN.cpp:81.123
        movl      %edx, -88(%rbp)                               #test_sgemm_NN.cpp:81.123
        movsd     %xmm0, -80(%rbp)                              #test_sgemm_NN.cpp:81.123
        movq      %rcx, -72(%rbp)                               #test_sgemm_NN.cpp:81.123
        movl      %r8d, -64(%rbp)                               #test_sgemm_NN.cpp:81.123
        movq      %r9, -56(%rbp)                                #test_sgemm_NN.cpp:81.123
        movsd     %xmm1, -48(%rbp)                              #test_sgemm_NN.cpp:81.123
        movl      -104(%rbp), %eax                              #test_sgemm_NN.cpp:85.13
        movl      %eax, -128(%rbp)                              #test_sgemm_NN.cpp:85.5
        movl      -88(%rbp), %eax                               #test_sgemm_NN.cpp:87.13
        movl      %eax, -124(%rbp)                              #test_sgemm_NN.cpp:87.5
        movl      -104(%rbp), %eax                              #test_sgemm_NN.cpp:89.10
        testl     %eax, %eax                                    #test_sgemm_NN.cpp:89.14
        jl        ..B8.7        # Prob 50%                      #test_sgemm_NN.cpp:89.14
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.2:                         # Preds ..B8.1
        movl      -96(%rbp), %eax                               #test_sgemm_NN.cpp:90.10
        testl     %eax, %eax                                    #test_sgemm_NN.cpp:90.14
        jl        ..B8.7        # Prob 50%                      #test_sgemm_NN.cpp:90.14
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.3:                         # Preds ..B8.2
        movl      -88(%rbp), %eax                               #test_sgemm_NN.cpp:91.10
        testl     %eax, %eax                                    #test_sgemm_NN.cpp:91.14
        jl        ..B8.7        # Prob 50%                      #test_sgemm_NN.cpp:91.14
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.4:                         # Preds ..B8.3
        movl      -64(%rbp), %eax                               #test_sgemm_NN.cpp:92.10
        movl      -128(%rbp), %edx                              #test_sgemm_NN.cpp:92.16
        cmpl      %edx, %eax                                    #test_sgemm_NN.cpp:92.16
        jl        ..B8.7        # Prob 50%                      #test_sgemm_NN.cpp:92.16
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.5:                         # Preds ..B8.4
        movl      16(%rbp), %eax                                #test_sgemm_NN.cpp:93.10
        movl      -124(%rbp), %edx                              #test_sgemm_NN.cpp:93.16
        cmpl      %edx, %eax                                    #test_sgemm_NN.cpp:93.16
        jl        ..B8.7        # Prob 50%                      #test_sgemm_NN.cpp:93.16
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.6:                         # Preds ..B8.5
        movl      32(%rbp), %eax                                #test_sgemm_NN.cpp:94.10
        movl      -104(%rbp), %edx                              #test_sgemm_NN.cpp:94.16
        cmpl      %edx, %eax                                    #test_sgemm_NN.cpp:94.16
        jge       ..B8.10       # Prob 50%                      #test_sgemm_NN.cpp:94.16
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.7:                         # Preds ..B8.1 ..B8.2 ..B8.3 ..B8.4 ..B8.5
                                #       ..B8.6
        movl      $_ZSt4cout, %eax                              #test_sgemm_NN.cpp:95.19
        movl      $.L_2__STRING.8, %edx                         #test_sgemm_NN.cpp:95.19
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:95.19
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:95.19
..___tag_value__Z8SGEMM_NNiiidPdiS_idS_i.143:
#       std::operator<<<std::char_traits<char>>(std::basic_ostream<char, std::char_traits<char>> &, const char *)
        call      _ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc #test_sgemm_NN.cpp:95.19
..___tag_value__Z8SGEMM_NNiiidPdiS_idS_i.144:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B8.35:                        # Preds ..B8.7
        movq      %rax, -40(%rbp)                               #test_sgemm_NN.cpp:95.19
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.8:                         # Preds ..B8.35
        movq      -40(%rbp), %rax                               #test_sgemm_NN.cpp:95.44
        movl      $_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_, %edx #test_sgemm_NN.cpp:95.44
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:95.44
        movq      %rdx, %rsi                                    #test_sgemm_NN.cpp:95.44
..___tag_value__Z8SGEMM_NNiiidPdiS_idS_i.145:
#       std::basic_ostream<char, std::char_traits<char>>::operator<<(std::basic_ostream<char, std::char_traits<char>> *, std::basic_ostream<char, std::char_traits<char>>::__ostream_type &(*)(std::basic_ostream<char, std::char_traits<char>>::__ostream_type &))
        call      _ZNSolsEPFRSoS_E                              #test_sgemm_NN.cpp:95.44
..___tag_value__Z8SGEMM_NNiiidPdiS_idS_i.146:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B8.36:                        # Preds ..B8.8
        movq      %rax, -32(%rbp)                               #test_sgemm_NN.cpp:95.44
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.9:                         # Preds ..B8.36
        leave                                                   #test_sgemm_NN.cpp:96.9
	.cfi_restore 6
        ret                                                     #test_sgemm_NN.cpp:96.9
	.cfi_offset 6, -16
                                # LOE
..B8.10:                        # Preds ..B8.6
        movl      -88(%rbp), %eax                               #test_sgemm_NN.cpp:99.22
        movslq    %eax, %rax                                    #test_sgemm_NN.cpp:99.22
        imulq     $8, %rax, %rax                                #test_sgemm_NN.cpp:99.22
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:99.22
..___tag_value__Z8SGEMM_NNiiidPdiS_idS_i.149:
#       malloc(size_t)
        call      malloc                                        #test_sgemm_NN.cpp:99.22
..___tag_value__Z8SGEMM_NNiiidPdiS_idS_i.150:
                                # LOE rax rbx rbp rsp r12 r13 r14 r15 rip
..B8.37:                        # Preds ..B8.10
        movq      %rax, -24(%rbp)                               #test_sgemm_NN.cpp:99.22
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.11:                        # Preds ..B8.37
        movq      -24(%rbp), %rax                               #test_sgemm_NN.cpp:99.22
        movq      %rax, -16(%rbp)                               #test_sgemm_NN.cpp:99.5
        movl      $0, -120(%rbp)                                #test_sgemm_NN.cpp:102.10
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.12:                        # Preds ..B8.13 ..B8.11
        movl      -120(%rbp), %eax                              #test_sgemm_NN.cpp:102.17
        movl      -104(%rbp), %edx                              #test_sgemm_NN.cpp:102.21
        cmpl      %edx, %eax                                    #test_sgemm_NN.cpp:102.21
        jl        ..B8.14       # Prob 50%                      #test_sgemm_NN.cpp:102.21
        jmp       ..B8.31       # Prob 100%                     #test_sgemm_NN.cpp:102.21
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.13:                        # Preds ..B8.23
        movl      $1, %eax                                      #test_sgemm_NN.cpp:102.26
        addl      -120(%rbp), %eax                              #test_sgemm_NN.cpp:102.26
        movl      %eax, -120(%rbp)                              #test_sgemm_NN.cpp:102.26
        jmp       ..B8.12       # Prob 100%                     #test_sgemm_NN.cpp:102.26
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.14:                        # Preds ..B8.12
        movsd     -48(%rbp), %xmm0                              #test_sgemm_NN.cpp:103.13
        pxor      %xmm1, %xmm1                                  #test_sgemm_NN.cpp:103.9
        ucomisd   %xmm1, %xmm0                                  #test_sgemm_NN.cpp:103.21
        jne       ..B8.18       # Prob 50%                      #test_sgemm_NN.cpp:103.21
        jp        ..B8.18       # Prob 0%                       #test_sgemm_NN.cpp:103.21
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.15:                        # Preds ..B8.14
        movl      $0, -116(%rbp)                                #test_sgemm_NN.cpp:104.18
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.16:                        # Preds ..B8.17 ..B8.15
        movl      -116(%rbp), %eax                              #test_sgemm_NN.cpp:104.25
        movl      -96(%rbp), %edx                               #test_sgemm_NN.cpp:104.29
        cmpl      %edx, %eax                                    #test_sgemm_NN.cpp:104.29
        jge       ..B8.22       # Prob 50%                      #test_sgemm_NN.cpp:104.29
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.17:                        # Preds ..B8.16
        pxor      %xmm0, %xmm0                                  #test_sgemm_NN.cpp:105.17
        movl      -96(%rbp), %eax                               #test_sgemm_NN.cpp:105.23
        imull     -120(%rbp), %eax                              #test_sgemm_NN.cpp:105.23
        addl      -116(%rbp), %eax                              #test_sgemm_NN.cpp:105.27
        movslq    %eax, %rax                                    #test_sgemm_NN.cpp:105.17
        imulq     $8, %rax, %rax                                #test_sgemm_NN.cpp:105.17
        addq      24(%rbp), %rax                                #test_sgemm_NN.cpp:105.17
        movsd     %xmm0, (%rax)                                 #test_sgemm_NN.cpp:105.17
        movl      $1, %eax                                      #test_sgemm_NN.cpp:104.34
        addl      -116(%rbp), %eax                              #test_sgemm_NN.cpp:104.34
        movl      %eax, -116(%rbp)                              #test_sgemm_NN.cpp:104.34
        jmp       ..B8.16       # Prob 100%                     #test_sgemm_NN.cpp:104.34
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.18:                        # Preds ..B8.14
        movsd     -48(%rbp), %xmm0                              #test_sgemm_NN.cpp:107.20
        movsd     .L_2il0floatpacket.1(%rip), %xmm1             #test_sgemm_NN.cpp:107.16
        ucomisd   %xmm1, %xmm0                                  #test_sgemm_NN.cpp:107.28
        jp        ..B8.19       # Prob 0%                       #test_sgemm_NN.cpp:107.28
        je        ..B8.22       # Prob 50%                      #test_sgemm_NN.cpp:107.28
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.19:                        # Preds ..B8.18
        movl      $0, -116(%rbp)                                #test_sgemm_NN.cpp:108.18
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.20:                        # Preds ..B8.21 ..B8.19
        movl      -116(%rbp), %eax                              #test_sgemm_NN.cpp:108.25
        movl      -96(%rbp), %edx                               #test_sgemm_NN.cpp:108.29
        cmpl      %edx, %eax                                    #test_sgemm_NN.cpp:108.29
        jge       ..B8.22       # Prob 50%                      #test_sgemm_NN.cpp:108.29
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.21:                        # Preds ..B8.20
        movsd     -48(%rbp), %xmm0                              #test_sgemm_NN.cpp:109.32
        movl      -96(%rbp), %eax                               #test_sgemm_NN.cpp:109.43
        imull     -120(%rbp), %eax                              #test_sgemm_NN.cpp:109.43
        addl      -116(%rbp), %eax                              #test_sgemm_NN.cpp:109.47
        movslq    %eax, %rax                                    #test_sgemm_NN.cpp:109.37
        imulq     $8, %rax, %rax                                #test_sgemm_NN.cpp:109.37
        addq      24(%rbp), %rax                                #test_sgemm_NN.cpp:109.37
        movsd     (%rax), %xmm1                                 #test_sgemm_NN.cpp:109.37
        mulsd     %xmm1, %xmm0                                  #test_sgemm_NN.cpp:109.37
        movl      -96(%rbp), %eax                               #test_sgemm_NN.cpp:109.23
        imull     -120(%rbp), %eax                              #test_sgemm_NN.cpp:109.23
        addl      -116(%rbp), %eax                              #test_sgemm_NN.cpp:109.27
        movslq    %eax, %rax                                    #test_sgemm_NN.cpp:109.17
        imulq     $8, %rax, %rax                                #test_sgemm_NN.cpp:109.17
        addq      24(%rbp), %rax                                #test_sgemm_NN.cpp:109.17
        movsd     %xmm0, (%rax)                                 #test_sgemm_NN.cpp:109.17
        movl      $1, %eax                                      #test_sgemm_NN.cpp:108.34
        addl      -116(%rbp), %eax                              #test_sgemm_NN.cpp:108.34
        movl      %eax, -116(%rbp)                              #test_sgemm_NN.cpp:108.34
        jmp       ..B8.20       # Prob 100%                     #test_sgemm_NN.cpp:108.34
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.22:                        # Preds ..B8.16 ..B8.20 ..B8.18
        movl      $0, -116(%rbp)                                #test_sgemm_NN.cpp:112.14
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.23:                        # Preds ..B8.24 ..B8.22
        movl      -116(%rbp), %eax                              #test_sgemm_NN.cpp:112.21
        movl      -96(%rbp), %edx                               #test_sgemm_NN.cpp:112.25
        cmpl      %edx, %eax                                    #test_sgemm_NN.cpp:112.25
        jl        ..B8.25       # Prob 50%                      #test_sgemm_NN.cpp:112.25
        jmp       ..B8.13       # Prob 100%                     #test_sgemm_NN.cpp:112.25
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.24:                        # Preds ..B8.29
        movl      $1, %eax                                      #test_sgemm_NN.cpp:112.30
        addl      -116(%rbp), %eax                              #test_sgemm_NN.cpp:112.30
        movl      %eax, -116(%rbp)                              #test_sgemm_NN.cpp:112.30
        jmp       ..B8.23       # Prob 100%                     #test_sgemm_NN.cpp:112.30
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.25:                        # Preds ..B8.23
        movl      $0, -112(%rbp)                                #test_sgemm_NN.cpp:113.18
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.26:                        # Preds ..B8.27 ..B8.25
        movl      -112(%rbp), %eax                              #test_sgemm_NN.cpp:113.25
        movl      -88(%rbp), %edx                               #test_sgemm_NN.cpp:113.29
        cmpl      %edx, %eax                                    #test_sgemm_NN.cpp:113.29
        jge       ..B8.28       # Prob 50%                      #test_sgemm_NN.cpp:113.29
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.27:                        # Preds ..B8.26
        movl      -96(%rbp), %eax                               #test_sgemm_NN.cpp:114.33
        imull     -120(%rbp), %eax                              #test_sgemm_NN.cpp:114.33
        addl      -112(%rbp), %eax                              #test_sgemm_NN.cpp:114.37
        movslq    %eax, %rax                                    #test_sgemm_NN.cpp:114.27
        imulq     $8, %rax, %rax                                #test_sgemm_NN.cpp:114.27
        addq      -72(%rbp), %rax                               #test_sgemm_NN.cpp:114.27
        movsd     (%rax), %xmm0                                 #test_sgemm_NN.cpp:114.27
        movl      -88(%rbp), %eax                               #test_sgemm_NN.cpp:114.48
        imull     -112(%rbp), %eax                              #test_sgemm_NN.cpp:114.48
        addl      -116(%rbp), %eax                              #test_sgemm_NN.cpp:114.52
        movslq    %eax, %rax                                    #test_sgemm_NN.cpp:114.42
        imulq     $8, %rax, %rax                                #test_sgemm_NN.cpp:114.42
        addq      -56(%rbp), %rax                               #test_sgemm_NN.cpp:114.42
        movsd     (%rax), %xmm1                                 #test_sgemm_NN.cpp:114.42
        mulsd     %xmm1, %xmm0                                  #test_sgemm_NN.cpp:114.42
        movl      -112(%rbp), %eax                              #test_sgemm_NN.cpp:114.22
        movslq    %eax, %rax                                    #test_sgemm_NN.cpp:114.17
        imulq     $8, %rax, %rax                                #test_sgemm_NN.cpp:114.17
        addq      -16(%rbp), %rax                               #test_sgemm_NN.cpp:114.17
        movsd     %xmm0, (%rax)                                 #test_sgemm_NN.cpp:114.17
        movl      $1, %eax                                      #test_sgemm_NN.cpp:113.34
        addl      -112(%rbp), %eax                              #test_sgemm_NN.cpp:113.34
        movl      %eax, -112(%rbp)                              #test_sgemm_NN.cpp:113.34
        jmp       ..B8.26       # Prob 100%                     #test_sgemm_NN.cpp:113.34
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.28:                        # Preds ..B8.26
        movl      $0, -112(%rbp)                                #test_sgemm_NN.cpp:116.18
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.29:                        # Preds ..B8.30 ..B8.28
        movl      -112(%rbp), %eax                              #test_sgemm_NN.cpp:116.25
        movl      -88(%rbp), %edx                               #test_sgemm_NN.cpp:116.29
        cmpl      %edx, %eax                                    #test_sgemm_NN.cpp:116.29
        jge       ..B8.24       # Prob 50%                      #test_sgemm_NN.cpp:116.29
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.30:                        # Preds ..B8.29
        movl      -96(%rbp), %eax                               #test_sgemm_NN.cpp:117.38
        imull     -120(%rbp), %eax                              #test_sgemm_NN.cpp:117.38
        addl      -116(%rbp), %eax                              #test_sgemm_NN.cpp:117.42
        movslq    %eax, %rax                                    #test_sgemm_NN.cpp:117.32
        imulq     $8, %rax, %rax                                #test_sgemm_NN.cpp:117.32
        addq      24(%rbp), %rax                                #test_sgemm_NN.cpp:117.32
        movsd     -80(%rbp), %xmm0                              #test_sgemm_NN.cpp:117.47
        movl      -112(%rbp), %edx                              #test_sgemm_NN.cpp:117.60
        movslq    %edx, %rdx                                    #test_sgemm_NN.cpp:117.55
        imulq     $8, %rdx, %rdx                                #test_sgemm_NN.cpp:117.55
        addq      -16(%rbp), %rdx                               #test_sgemm_NN.cpp:117.55
        movsd     (%rdx), %xmm1                                 #test_sgemm_NN.cpp:117.55
        mulsd     %xmm1, %xmm0                                  #test_sgemm_NN.cpp:117.55
        movsd     (%rax), %xmm1                                 #test_sgemm_NN.cpp:117.32
        addsd     %xmm0, %xmm1                                  #test_sgemm_NN.cpp:117.55
        movl      -96(%rbp), %eax                               #test_sgemm_NN.cpp:117.23
        imull     -120(%rbp), %eax                              #test_sgemm_NN.cpp:117.23
        addl      -116(%rbp), %eax                              #test_sgemm_NN.cpp:117.27
        movslq    %eax, %rax                                    #test_sgemm_NN.cpp:117.17
        imulq     $8, %rax, %rax                                #test_sgemm_NN.cpp:117.17
        addq      24(%rbp), %rax                                #test_sgemm_NN.cpp:117.17
        movsd     %xmm1, (%rax)                                 #test_sgemm_NN.cpp:117.17
        movl      $1, %eax                                      #test_sgemm_NN.cpp:116.34
        addl      -112(%rbp), %eax                              #test_sgemm_NN.cpp:116.34
        movl      %eax, -112(%rbp)                              #test_sgemm_NN.cpp:116.34
        jmp       ..B8.29       # Prob 100%                     #test_sgemm_NN.cpp:116.34
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.31:                        # Preds ..B8.12
        movq      -16(%rbp), %rax                               #test_sgemm_NN.cpp:129.5
        movq      %rax, %rdi                                    #test_sgemm_NN.cpp:129.5
..___tag_value__Z8SGEMM_NNiiidPdiS_idS_i.151:
#       free(void *)
        call      free                                          #test_sgemm_NN.cpp:129.5
..___tag_value__Z8SGEMM_NNiiidPdiS_idS_i.152:
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B8.32:                        # Preds ..B8.31
        leave                                                   #test_sgemm_NN.cpp:131.5
	.cfi_restore 6
        ret                                                     #test_sgemm_NN.cpp:131.5
	.cfi_endproc
                                # LOE
# mark_end;
	.type	_Z8SGEMM_NNiiidPdiS_idS_i,@function
	.size	_Z8SGEMM_NNiiidPdiS_idS_i,.-_Z8SGEMM_NNiiidPdiS_idS_i
	.data
# -- End  _Z8SGEMM_NNiiidPdiS_idS_i
	.text
# -- Begin  __sti__$E
	.bss
	.align 4
	.align 1
_ZSt8__ioinit:
	.type	_ZSt8__ioinit,@object
	.size	_ZSt8__ioinit,1
	.space 1	# pad
	.section .ctors, "wa"
	.align 8
__init_0:
	.type	__init_0,@object
	.size	__init_0,8
	.quad	__sti__$E
	.text
# mark_begin;

# --- __sti__$E()
__sti__$E:
..B9.1:                         # Preds ..B9.0
	.cfi_startproc
	.cfi_personality 0x3,__gxx_personality_v0
..___tag_value___sti__$E.155:
..L156:
                                                        #
        pushq     %rbp                                          #
	.cfi_def_cfa_offset 16
        movq      %rsp, %rbp                                    #
	.cfi_def_cfa 6, 16
	.cfi_offset 6, -16
        subq      $16, %rsp                                     #
        movl      $_ZSt8__ioinit, %eax                          #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
        movq      %rax, %rdi                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
..___tag_value___sti__$E.160:
#       std::ios_base::Init::Init(std::ios_base::Init *)
        call      _ZNSt8ios_base4InitC1Ev                       #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
..___tag_value___sti__$E.161:
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B9.2:                         # Preds ..B9.1
        movl      $_ZNSt8ios_base4InitD1Ev, %eax                #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
        movl      $_ZSt8__ioinit, %edx                          #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
        movl      $__dso_handle, %ecx                           #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
        movq      %rax, %rdi                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
        movq      %rdx, %rsi                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
        movq      %rcx, %rdx                                    #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
..___tag_value___sti__$E.162:
        call      __cxa_atexit                                  #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
..___tag_value___sti__$E.163:
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip eax
..B9.6:                         # Preds ..B9.2
        movl      %eax, -16(%rbp)                               #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
                                # LOE rbx rbp rsp r12 r13 r14 r15 rip
..B9.3:                         # Preds ..B9.6
        leave                                                   #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
	.cfi_restore 6
        ret                                                     #/home/software/gcc/gcc-4.8.3/include/c++/4.8.3/iostream:74.25
	.cfi_endproc
                                # LOE
# mark_end;
	.type	__sti__$E,@function
	.size	__sti__$E,.-__sti__$E
	.data
# -- End  __sti__$E
	.data
	.hidden __dso_handle
# mark_proc_addr_taken __sti__$E;
	.section .note.GNU-stack, ""
// -- Begin DWARF2 SEGMENT .eh_frame
	.section .eh_frame,"a",@progbits
.eh_frame_seg:
	.align 8
# End
