#ifndef _KOZOS_H_INCLUDED_
#define _KOZOS_H_INCLUDED_

#include "defines.h"
#include "syscall.h"

// システムコール
kz_thread_id_t kz_run(kz_func_t func, char *name, int skacksize, int argc, char *argv[]);

// スレッド終了のシステムコール
void kz_exit(void);

// 初期スレッドを起動し、OSの動作を開始する
void kz_start(kz_func_t func, char *name, int stacksize, int argc, char *argv[]);

// 致命的エラー時に呼び出す
void kz_sysdown(void);

// システムコールを実行する
void kz_syscall(kz_syscall_type_t type, kz_syscall_param_t *param);

// ユーザスレッド
int test08_1_main(int argc, char *argv[]);

#endif
