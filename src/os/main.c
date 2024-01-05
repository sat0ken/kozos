#include "defines.h"
#include "serial.h"
#include "intr.h"
#include "interrupt.h"
#include "lib.h"

static void intr(softvec_type_t type, unsigned long sp)
{
    int c;
    static char buf[32];
    static int len;

    c = getc();

    if (c != '\n') {
        buf[len++] = c;
    } else {
        buf[len++] = '\0';
        if (!strncmp(buf, "echo", 4)) {
            puts(buf + 4);
            puts("\n");
        } else {
            puts("unkown.\n");
        }
        puts("> ");
        len = 0;
    }
}

int main(void)
{
    INTR_DISABLE;

    puts("kozos boot succeed!\n");

    // ソフトウェア割り込みベクタにシリアル割り込みハンドラを設定
    softvec_setintr(SOFTVEC_TYPE_SERINTR, intr);
    // シリアル受信割り込みを有効化
    serial_intr_recv_enable(SERIAL_DEFAULT_DEVICE);

    puts("> ");
    INTR_ENABLE;
    while (1) {
        asm volatile ("sleep");
    }
    return 0;
}


