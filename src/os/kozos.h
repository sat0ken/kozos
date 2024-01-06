#ifndef _KOZOS_H_INCLUDED_
#define _KOZOS_H_INCLUDED_

#include "defines.h"
#include "syscall.h"

// システムコール
// STEP9で優先度を追加
kz_thread_id_t kz_run(kz_func_t func, char *name, int priority, int skacksize, int argc, char *argv[]);

// スレッド終了のシステムコール
void kz_exit(void);

// 初期スレッドを起動し、OSの動作を開始する
// STEP9で優先度を追加
void kz_start(kz_func_t func, char *name, int priority, int stacksize, int argc, char *argv[]);

// 致命的エラー時に呼び出す
void kz_sysdown(void);

// システムコールを実行する
void kz_syscall(kz_syscall_type_t type, kz_syscall_param_t *param);

// STEP9でシステムコールを追加
int kz_wait(void);
int kz_sleep(void);
int kz_wakeup(kz_thread_id_t id);
kz_thread_id_t kz_getid(void);
int kz_chpri(int priority);
void *kz_kmalloc(int size);
int kz_kmfree(void *p);

// ユーザスレッド
//int test08_1_main(int argc, char *argv[]);
//int test09_1_main(int argc, char *argv[]);
//int test09_2_main(int argc, char *argv[]);
//int test09_3_main(int argc, char *argv[]);
int test10_1_main(int argc, char *argv[]);
extern kz_thread_id_t test09_1_id;
extern kz_thread_id_t test09_2_id;
extern kz_thread_id_t test09_3_id;

#endif
