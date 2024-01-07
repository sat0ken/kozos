#include "defines.h"
#include "kozos.h"
#include "consdrv.h"
#include "lib.h"

// コンソールドライバの使用開始をドライバに依頼
static void send_use(int index)
{
    char *p;
    p = kz_kmalloc(3);
    p[0] = '0';
    p[1] = CONSDRV_CMD_USE; // 初期化コマンドをセット
    p[2] = '0' + index;
    // コンソールドライバのスレッドにメッセージを送信
    kz_send(MSGBOX_ID_CONSOUTPUT, 3, p);
}

// コンソールへの文字列出力をドライバに依頼
static void send_write(char *str)
{
    char *p;
    int len;
    len = strlen(str);
    p = kz_kmalloc(len + 2);
    p[0] = '0';
    p[1] = CONSDRV_CMD_WRITE;   // 文字列出力コマンドを設定
    memcpy(&p[2], str, len);
    // コンソールドライバのスレッドにメッセージを送信
    kz_send(MSGBOX_ID_CONSOUTPUT, len + 2, p);
}

// コマンドスレッドのmain関数
int command_main(int argc, char *argv[])
{
    char *p;
    int size;
    send_use(SERIAL_DEFAULT_DEVICE);

    while (1) {
        send_write("command> ");
        // コンソールドライバのスレッドから受信文字列を受け取る
        kz_recv(MSGBOX_ID_CONSINPUT, &size, &p);
        p[size] = '\0';

        // echoコマンドを処理する
        if (!strncmp(p, "echo", 4)) {
            send_write(p + 4);
            send_write("\n");
        } else {
            send_write("unknown.\n");
        }
        kz_kmfree(p);
    }
    return 0;
}
