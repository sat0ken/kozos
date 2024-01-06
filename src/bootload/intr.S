#include "intr.h"
        .h8300h
        .section .text

        .global _intr_softerr
#       .type   _intr_softerr, @function
_intr_softerr:
        ; 汎用レジスタの値をスタックに保存する
        mov.l	er6,@-er7
        mov.l	er5,@-er7
        mov.l	er4,@-er7
        mov.l	er3,@-er7
        mov.l	er2,@-er7
        mov.l	er1,@-er7
        mov.l	er0,@-er7
        ; 第2引数にスタックポインタを設定
        mov.l	er7,er1
        ;
        mov.l	#_intrstack,sp
        mov.l	er1,@-er7
        ; 第1引数にソフトウェアエラーを設定
	    mov.w	#SOFTVEC_TYPE_SOFTERR,r0
        ; interrupt()の呼び出し
        jsr     @_interrupt
        ; スタックから汎用レジスタの値を復旧する
        mov.l	@er7+,er1
        mov.l	er1,er7
        mov.l	@er7+,er0
        mov.l	@er7+,er1
        mov.l	@er7+,er2
        mov.l	@er7+,er3
        mov.l	@er7+,er4
        mov.l	@er7+,er5
        mov.l	@er7+,er6
        ; 割り込み復帰命令の実行
        rte

        .global _intr_syscall
#       .type   _intr_syscall,@function
_intr_syscall:
        ; 汎用レジスタの値をスタックに保存する
        mov.l	er6,@-er7
        mov.l	er5,@-er7
        mov.l	er4,@-er7
        mov.l	er3,@-er7
        mov.l	er2,@-er7
        mov.l	er1,@-er7
        mov.l	er0,@-er7
        ; 第2引数にスタックポインタを設定
        mov.l   er7,er1
        mov.l	#_intrstack,sp
        mov.l	er1,@-er7
        ; 第1引数にシステムコールを設定
        mov.w   #SOFTVEC_TYPE_SYSCALL,r0
        ; interrupt()の呼び出し
        jsr     @_interrupt
        ; スタックから汎用レジスタの値を復旧する
        mov.l	@er7+,er1
        mov.l	er1,er7
        mov.l	@er7+,er0
        mov.l	@er7+,er1
        mov.l	@er7+,er2
        mov.l	@er7+,er3
        mov.l	@er7+,er4
        mov.l	@er7+,er5
        mov.l	@er7+,er6
        ; 割り込み復帰命令の実行
        rte

        .global	_intr_serintr
#       .type	_intr_serintr,@function
_intr_serintr:
        ; 汎用レジスタの値をスタックに保存する
        mov.l   er6,@-er7
        mov.l   er5,@-er7
        mov.l   er4,@-er7
        mov.l   er3,@-er7
        mov.l   er2,@-er7
        mov.l   er1,@-er7
        mov.l   er0,@-er7
        ; 第2引数にスタックポインタを設定
        mov.l   er7,er1
        mov.l	#_intrstack,sp
        mov.l	er1,@-er7
        ; 第1引数にシステムコールを設定
        mov.w   #SOFTVEC_TYPE_SERINTR,r0
        ; interrupt()の呼び出し
        jsr     @_interrupt
        ; スタックから汎用レジスタの値を復旧する
        mov.l	@er7+,er1
        mov.l	er1,er7
        mov.l	@er7+,er0
        mov.l	@er7+,er1
        mov.l	@er7+,er2
        mov.l	@er7+,er3
        mov.l	@er7+,er4
        mov.l	@er7+,er5
        mov.l	@er7+,er6
        ; 割り込み復帰命令の実行
        rte
