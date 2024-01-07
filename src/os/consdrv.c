#include "defines.h"
#include "kozos.h"
#include "intr.h"
#include "interrupt.h"
#include "serial.h"
#include "lib.h"
#include "consdrv.h"

#define CONS_BUFFER_SIZE 24

// コンソール管理用構造体
static struct consreg {
    kz_thread_id_t id;  // コンソールを利用するスレッド
    int index;          // 利用するシリアル番号

    char *send_buf;     // 送信バッファ
    char *recv_buf;     // 受信バッファ
    int send_len;       // 送信サイズ
    int recv_len;       // 受信サイズ

    long dummy[3];
} consreg[CONSDRV_DEVICE_NUM];

// 送信バッファの先頭1文字を送信する
static void send_char(struct consreg *cons)
{
    int i;
    serial_send_byte(cons->index, cons->send_buf[0]);
    cons->send_len--;
    for (i = 0; i < cons->send_len; i++) {
        cons->send_buf[i] = cons->send_buf[i + 1];
    }
}

// 文字列を送信バッファに書き込み送信開始する
static void send_string(struct consreg *cons, char *str, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (str[i] == '\n') {
            cons->send_buf[cons->send_len++] = '\r';
        }
        cons->send_buf[cons->send_len++] = str[i];
    }
    if (cons->send_len && !serial_intr_is_send_enable(cons->index)) {
        // 送信割り込み有効化
        serial_intr_send_enable(cons->index);
        // 送信開始
        send_char(cons);
    }
}

// 割り込みハンドラから呼ばれる割り込み処理
static int consdrv_intrproc(struct consreg *cons)
{
    unsigned char c;
    char *p;

    // 受信割り込みの処理
    if (serial_is_recv_enable(cons->index)) {
        c = serial_recv_byte(cons->index);
        if (c == '\r') {
            c = '\n';
        }
        send_string(cons, &c, 1);

        if (cons->id) {
            if (c != '\n') {
                // 改行でなければ受信バッファに読み込む
                cons->recv_buf[cons->recv_len++] = c;
            } else {
                // 改行が押されたら受信データをコマンドスレッドに送信する
                p = kx_kmalloc(CONS_BUFFER_SIZE);
                memcpy(p, cons->recv_buf, cons->recv_len);
                kx_send(MSGBOX_ID_CONSINPUT, cons->recv_len, p);
                cons->recv_len = 0;
            }
        }
    }

    // 送信割り込みの処理
    if (serial_is_send_enable(cons->index)) {
        if (!cons->id || !cons->send_len) {
            // 送信データが無ければ送信終了
            serial_intr_send_disable(cons->index);
        } else {
            // 送信データがあれば引き続き送信
            send_char(cons);
        }
    }

    return 0;
}

// 割り込みハンドラ
static void consdrv_intr(void)
{
    int i;
    struct consreg *cons;

    for (i = 0; i < CONSDRV_DEVICE_NUM; i++) {
        cons = &consreg[i];
        if (cons->id) {
            if (serial_is_send_enable(cons->index) || serial_is_recv_enable(cons->index)) {
                consdrv_intrproc(cons);
            }
        }
    }
}

// 初期化処理
static int consdrv_init(void)
{
    memset(consreg, 0, sizeof(consreg));
    return 0;
}

// 他スレッドからの要求を受けて処理を行う
static int consdrv_command(struct consreg *cons, kz_thread_id_t id, int index,
        int size, char *command)
{
    switch (command[0]) {
        case CONSDRV_CMD_USE:   // コンソールの初期化コマンド
            cons->id = id;
            cons->index = command[1] - '0';
            cons->send_buf = kz_kmalloc(CONS_BUFFER_SIZE);  // 送信バッファを取得
            cons->recv_buf = kz_kmalloc(CONS_BUFFER_SIZE);  // 受信バッファを取得
            cons->send_len = 0;
            cons->recv_len = 0;
            serial_init(cons->index);               // シリアルの初期化
            serial_intr_recv_enable(cons->index);   // シリアル受信割り込みを有効化
            break;
        case CONSDRV_CMD_WRITE: // コンソールへの文字列出力コマンド
            INTR_DISABLE;       // send_string()が再代入不可なので排他するため割り込み不可にする
            send_string(cons, command + 1, size - 1);   // 文字列の送信
            INTR_ENABLE;
            break;
        default:
            break;
    }
    return 0;
}

int consdrv_main(int argc, char *argv[])
{
    int size, index;
    kz_thread_id_t id;
    char *p;

    consdrv_init();
    // 割り込みハンドラを設定する
    kz_setintr(SOFTVEC_TYPE_SERINTR, consdrv_intr);

    while (1) {
        // 他スレッドからのコマンドの受付
        id = kz_recv(MSGBOX_ID_CONSOUTPUT, &size, &p);
        index = p[0] - '0';
        // コマンド処理を呼び出す
        consdrv_command(&consreg[index], id, index, size - 1, p + 1);
        kz_kmfree(p);
    }

    return 0;
}
