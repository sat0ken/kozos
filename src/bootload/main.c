#include "defines.h"
#include "serial.h"
#include "lib.h"
#include "xmodem.h"
#include "elf.h"
#include "interrupt.h"

int global_data = 0x10;
int global_bss;
static int static_data = 0x20;
static int static_bss;

static int init(void)
{
    // リンカスクリプトで定義したシンボルを参照する
    extern int erodata, data_start, edata, bss_start, ebss;
    // ROMからRAMへデータコピー
    memcpy(&data_start, &erodata, (long)&edata - (long)&data_start);
    // RAMの.bssセクションを初期化することで配置された初期値を持たない変数を初期化
    memset(&bss_start, 0, (long)&ebss - (long)&bss_start);

    // ソフトウェア割り込みベクタを初期化する
    softvec_init();

    // シリアルの初期化
    serial_init(SERIAL_DEFAULT_DEVICE);
    return 0;
}

static int dump(char *buf, long size)
{
    long i;
    if (size < 0) {
        puts("no data.\n");
        return -1;
    }
    for (i = 0; i < size; i++) {
        putxval(buf[i], 2);
        if ((i & 0xf) == 15) {
            puts("\n");
        } else {
            if ((i & 0xf) == 7) {
                puts(" ");
                puts(" ");
            }
        }
    }
    puts("\n");
    return 0;
}

static void wait()
{
    volatile long i;
    for (i = 0; i < 300000; i++)
        ;
}

int main(void)
{
    static char buf[16];
    static long size = -1;
    static unsigned char *loadbuf = NULL;
    extern int buffer_start;
    char *entry_point;
    void (*f)(void);

    // 最初の初期化処理は割り込み無効の状態で行う
    INTR_DISABLE;

    init();
    puts("kzload (kozos boot loader) started.\n");

    while (1) {
        puts("kzload> ");
        gets(buf);

        if (!strcmp(buf, "load")) {
            loadbuf = (char *)(&buffer_start);
            size = xmodem_recv(loadbuf);
            wait();
            if (size < 0) {
                puts("\nXMODEM recieve error!\n");
            } else {
                puts("\nXMODEM recieve succeeded.\n");
            }
        } else if (!strcmp(buf, "dump")) {
            puts("size: ");
            putxval(size, 0);
            puts("\n");
            dump(loadbuf, size);
        } else if (!strcmp(buf, "run")) {
            // runコマンドでエントリポイントに処理を渡すようにする
            puts("start elf_load()\n");
            entry_point = elf_load(loadbuf);
            if (!entry_point) {
                puts("run error!\n");
            } else {
                puts("starting from entrypoint: ");
                putxval((unsigned long)entry_point, 0);
                puts("\n");
                puts("set entry point\n");
                f = (void (*)(void))entry_point;
                // ロードしたプログラムに処理を渡す
                f();
            }
        } else {
            puts("unknown.\n");
        }
    }

    return 0;
}
