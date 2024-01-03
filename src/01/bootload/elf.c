#include "defines.h"
#include "elf.h"
#include "lib.h"

// ELFヘッダの構造体
struct elf_header {
    // 16byteの識別情報
    struct {
        unsigned char magic[4];
        unsigned char class;
        unsigned char format;
        unsigned char version;
        unsigned char abi;
        unsigned char abi_version;
        unsigned char reserve[7];
    } id;
    short type;
    short arch;
    long version;
    long entry_point;
    long program_header_offset;
    long section_header_offset;
    long flags;
    short header_size;
    short program_header_size;
    short program_header_num;
    short section_header_size;
    short section_header_num;
    short section_name_index;
};

// プログラムヘッダの定義
struct elf_program_header {
    long type;
    long offset;
    long virtual_addr;
    long physical_addr;
    long file_size;
    long memory_size;
    long flags;
    long align;
};

// ELFヘッダのチェック
static int elf_check(struct elf_header *header)
{
    // マジックナンバのチェック
    if (memcmp(header->id.magic, "\x7f" "ELF", 4)) {
        return -1;
    }
    // 各パラメータのチェック
    if (header->id.class != 1) {
        return -1;
    }
    if (header->id.format != 2) {
        return -1;
    }
    if (header->id.version != 1) {
        return -1;
    }
    if (header->type != 2) {
        return -1;
    }
    if (header->version != 1) {
        return -1;
    }
    // アーキテクチャがH8であるかチェック
    if ((header->arch != 46) && (header->arch != 47)) {
        return -1;
    }
    return 0;
}

// ELF形式のセグメント情報を解析する
static int elf_load_program(struct elf_header *header)
{
    int i;
    struct elf_program_header *pheader;

    // セグメント単位でループ
    for (i = 0; i < header->program_header_num; i++) {
        // プログラムヘッダを取得
        pheader = (struct elf_program_header *)((char *)header + header->program_header_offset + header->program_header_size * i);
        // ロード可能かチェック
        if (pheader->type != 1) {
            continue;
        }
        putxval(pheader->offset, 6);
        puts(" ");
        putxval(pheader->virtual_addr, 8);
        puts(" ");
        putxval(pheader->physical_addr, 8);
        puts(" ");
        putxval(pheader->file_size, 5);
        puts(" ");
        putxval(pheader->memory_size, 5);
        puts(" ");
        putxval(pheader->flags, 2);
        puts(" ");
        putxval(pheader->align, 2);
        puts("\n");
    }
    return 0;
}

int elf_load(char *buf)
{
    struct elf_header *header = (struct elf_header *)buf;
    if (elf_check(header) < 0) {
        return -1;
    }
    if (elf_load_program(header) < 0) {
        return -1;
    }
    return 0;
}

