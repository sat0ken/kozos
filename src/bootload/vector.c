#include "defines.h"

extern void start(void);            // スタートアップ
extern void intr_softerr(void);     // ソフトウェアエラー
extern void intr_syscall(void);     // システムコール
extern void intr_serintr(void);     // シリアル割り込み

void (*vectors[])(void) = {
  start, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  intr_syscall, intr_softerr, intr_serintr,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  intr_serintr, intr_serintr, intr_serintr, intr_serintr,   // SCI0の割り込みベクタ
  intr_serintr, intr_serintr, intr_serintr, intr_serintr,   // SCI1の割り込みベクタ
  intr_serintr, intr_serintr, intr_serintr, intr_serintr,   // SCI2の割り込みベクタ
};
