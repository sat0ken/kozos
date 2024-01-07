#include "defines.h"
#include "kozos.h"
#include "lib.h"
#include "memory.h"

// メモリブロック構造体
// メモリ領域の先頭に付加されるヘッダ
typedef struct _kzmem_block {
    struct _kzmem_block *next;
    int size;
} kzmem_block;

// メモリプール構造体
typedef struct _kzmem_pool {
    int size;
    int num;
    kzmem_block *free;
} kzmem_pool;

// メモリプールの定義
// 3つの固定サイズによる提供
static kzmem_pool pool[] = {
        {16, 8, NULL},
        {32, 8, NULL},
        {64, 4, NULL},
};

#define MEMORY_AREA_NUM (sizeof(pool) / sizeof(*pool))

// メモリプールの初期化
static int kzmem_init_pool(kzmem_pool *p)
{
    int i;
    kzmem_block *mp;
    kzmem_block **mpp;
    extern char freearea;   // リンカスクリプトで定義した領域
    static char *area = &freearea;

    mp = (kzmem_block *)area;

    // ここの領域をすべて解放済みリンクリストに繋げる
    mpp = &p->free;
    for (i = 0; i < p->num; i++) {
        *mpp = mp;
        memset(mp, 0, sizeof(*mp));
        mp->size = p->size;
        mpp = &(mp->next);
        mp = (kzmem_block *)((char *)mp + p->size);
        area += p->size;
    }

    return 0;
}

// 動的メモリの初期化
int kzmem_init(void)
{
    int i;
    for (i = 0; i < MEMORY_AREA_NUM; i++) {
        kzmem_init_pool(&pool[i]);
    }
    return 0;
}

// 動的メモリの確保
void *kzmem_alloc(int size)
{
    kzmem_block *mp;
    kzmem_pool *p;
    int i;

    // 要求サイズを格納できるメモリプールを探す
    for (i = 0; i < MEMORY_AREA_NUM; i++) {
        p = &pool[i];
        if (size <= p->size - sizeof(kzmem_block)) {
            if (p->free == NULL) {
                // 空きがないのでダウン
                kz_sysdown();
                return NULL;
            }
            // 空いている領域を取得
            mp = p->free;
            p->free = p->free->next;
            mp->next = NULL;
            // 先頭には管理用ヘッダのメモリブロック構造体があるので+1をして返す
            return mp + 1;
        }
    }
    kz_sysdown();
    return NULL;
}

// メモリを解放
void kzmem_free(void *mem)
{
    kzmem_block *mp;
    kzmem_pool *p;
    int i;

    // 領域の前にあるヘッダを取得
    mp = ((kzmem_block *)mem -1);

    for (i = 0; i < MEMORY_AREA_NUM; i++) {
        p = &pool[i];
        // 同じサイズのメモリプールを検索
        if (mp->size == p->size) {
            // 解放済みリストに戻す
            mp->next = p->free;
            p->free = mp;
            return;
        }
    }
    kz_sysdown();
}
