#include "defines.h"
#include "kozos.h"
#include "intr.h"
#include "interrupt.h"
#include "syscall.h"
#include "lib.h"

#define THREAD_NUM 6
#define THREAD_NAME_SIZE 15

// スレッドのコンテキスト保存用の構造体
typedef struct _kz_context {
    // スタックポインタ
    uint32 sp;
} kz_context;

// タスクコントロールブロック
typedef struct _kz_thread {
    struct _kz_thread *next;            // レディーキューへの接続に利用するnextポインタ
    char name[THREAD_NAME_SIZE + 1];    // スレッド名
    char *stack;                        // スレッドのスタック

    // スレッドのスタートアップ(thread_init())に渡すパラメータ
    struct {
        kz_func_t func;     // スレッドのmain関数
        int argc;           // main関数の引数(argc)
        char **argv;        // main関数の引数(argv)
    } init;

    // システムコール用バッファ
    struct {
        kz_syscall_type_t type;
        kz_syscall_param_t *param;
    } syscall;

    kz_context context;     // スレッドのコンテキスト情報
} kz_thread;

// スレッドのレディーキュー
static struct {
    kz_thread *head;    // キューの先頭エントリ
    kz_thread *tail;    // キューの末尾エントリ
} readyque;

static kz_thread *current;                          // 現在実行中のスレッド
static kz_thread threads[THREAD_NUM];                // タスクコントロールブロック
static kz_handler_t handlers[SOFTVEC_TYPE_NUM];     // OSが管理する割り込みハンドラ

void dispatch(kz_context *context);                 // スレッドのディスパッチ用関数

// 実行中のスレッドをキューから抜き出す
static int getcurrent(void)
{
    if (current == NULL) {
        return -1;
    }
    // 先頭のエントリを抜き出す
    readyque.head = current->next;
    if (readyque.head == NULL) {
        readyque.tail = NULL;
    }
    // nextポインタをクリアする
    current->next = NULL;
    return 0;
}

// 実行中のスレッドをキューにつなげる
static int putcurrent(void)
{
    if (current == NULL) {
        return -1;
    }
    // 実行中のスレッドを末尾に接続する
    if (readyque.tail) {
        readyque.tail->next = current;
    } else {
        readyque.head = current;
    }
    readyque.tail = current;
    return 0;
}

// スレッドの終了
static void thread_end(void)
{
    kz_exit();
}

// スレッドのスタートアップ
static void thread_init(kz_thread *thp)
{
    // スレッドのmain関数を呼び出す
    thp->init.func(thp->init.argc, thp->init.argv);
    thread_end();
}

// システムコールの処理
static kz_thread_id_t thread_run(kz_func_t func, char *name, int stacksize, int argc, char *argv[])
{
    int i;
    kz_thread *thp;
    uint32 *sp;
    extern char userstack;
    static char *thread_stack = &userstack;

    for (i = 0; i < THREAD_NUM; i++) {
        thp = &threads[i];
        // 空きがあった
        if (!thp->init.func) {
            break;
        }
    }
    // 空きがなかった
    if (i == THREAD_NUM) {
        return -1;
    }
    // TCBをゼロクリア
    memset(thp, 0, sizeof(*thp));
    // TCBの設定
    strcpy(thp->name, name);
    thp->next = NULL;

    thp->init.func = func;
    thp->init.argc = argc;
    thp->init.argv = argv;
    // スタック領域を獲得しTCBに設定
    memset(thread_stack, 0, stacksize);
    thread_stack += stacksize;
    // スタックを設定
    thp->stack = thread_stack;
    // スタックの初期化
    sp = (uint32 *)thp->stack;
    *(--sp) = (uint32)thread_end;
    // プログラムカウンタを設定
    *(--sp) = (uint32)thread_init;
    *(--sp) = 0;    // ER6
    *(--sp) = 0;    // ER5
    *(--sp) = 0;    // ER4
    *(--sp) = 0;    // ER3
    *(--sp) = 0;    // ER2
    *(--sp) = 0;    // ER1
    // スレッドのスタートアップ関数に渡す引数
    *(--sp) = (uint32)thp;  // ER0
    // スレッドのコンテキストを設定
    thp->context.sp = (uint32)sp;
    // システムコールを呼び出したスレッドをキューに戻す
    putcurrent();
    // 新しいスレッドをキューに接続する
    current = thp;
    putcurrent();

    return (kz_thread_id_t)current;
}

// スレッドの終了
static int thread_exit(void)
{
    puts(current->name);
    puts(" EXIT.\n");
    memset(current, 0, sizeof(*current));
    return 0;
}

// 割り込みハンドラの登録
static int setintr(softvec_type_t type, kz_handler_t handler)
{
    static void thread_intr(softvec_type_t type, unsigned long sp);

    softvec_setintr(type, thread_intr);
    handlers[type] = handler;

    return 0;
}

// システムコールの処理関数の呼び出し
static void call_functions(kz_syscall_type_t type, kz_syscall_param_t *param)
{
    switch (type) {
        case KZ_SYSCALL_TYPE_RUN:
            // kz_run()の処理関数を呼び出す
            param->un.run.ret = thread_run(param->un.run.func, param->un.run.name,
                                           param->un.run.stacksize, param->un.run.argc, param->un.run.argv);
            break;
        case KZ_SYSCALL_TYPE_EXIT:
            // kz_exit()の処理関数を呼び出す
            thread_exit();
            break;
        default:
            break;
    }
}

// システムコールの処理
static void syscall_proc(kz_syscall_type_t type, kz_syscall_param_t *param)
{
    // カレントスレッドをキューから外す
    getcurrent();
    // システムコールの処理関数を呼ぶ
    call_functions(type, param);
}

// スレッドのスケジューリング
static void schedule(void)
{
    if (!readyque.head) {
        kz_sysdown();
    }
    // 先頭のスレッドをスケジュールする
    current = readyque.head;
}

// システムコールの呼び出し
static void syscall_intr(void)
{
    syscall_proc(current->syscall.type, current->syscall.param);
}

// ソフトウェアエラーの発生
static void softerr_intr(void)
{
    puts(current->name);
    puts("DOWN.\n");
    getcurrent();
    // エラー時はスレッドを強制終了する
    thread_exit();
}

// 割り込み処理の入口関数
static void thread_intr(softvec_type_t type, unsigned long sp)
{
    // カレントスレッドのコンテキストを保存
    current->context.sp = sp;

    // 割り込みごとの処理を実行する
    if (handlers[type]) {
        handlers[type]();
    }
    // 次に動作するスレッドをスケジューリング
    schedule();
    // スケジューリングされたスレッドをディスパッチ
    dispatch(&current->context);
}

// 初期スレッドを起動しOSの動作を開始
void kz_start(kz_func_t func, char *name, int stacksize, int argc, char *argv[])
{
    current = NULL;
    // 各種データの初期化
    readyque.head = readyque.tail = NULL;
    memset(threads, 0, sizeof(threads));
    memset(handlers, 0, sizeof(handlers));
    // 割り込みハンドラの登録
    setintr(SOFTVEC_TYPE_SYSCALL, syscall_intr);
    setintr(SOFTVEC_TYPE_SOFTERR, softerr_intr);
    // 初期スレッドを生成
    current = (kz_thread *)thread_run(func, name, stacksize, argc, argv);
    // スレッドを起動
    dispatch(&current->context);
}

// OS内部で致命的エラーが発生した時に呼ばれる関数
void kz_sysdown(void)
{
    puts("system error!\n");
    while (1)
        ;
}

// システムコール呼び出し用ライブラリ関数
void kz_syscall(kz_syscall_type_t type, kz_syscall_param_t *param)
{
    current->syscall.type = type;
    current->syscall.param = param;
    // トラップ命令で割り込みを発生させる
    asm volatile ("trapa #0");
}