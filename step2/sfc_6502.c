#include "sfc_6502.h"
#include <assert.h>
#include <string.h>



/// <summary>
/// 命令名称
/// </summary>
struct sfc_opname {
    // 3字名称
    char        name[3];
    // 寻址模式
    uint8_t     mode;
};

/// <summary>
/// 反汇编用数据
/// </summary>
static const struct sfc_opname s_opname_data[256] = {
    { 'B', 'R', 'K', SFC_AM_IMP },
    { 'O', 'R', 'A', SFC_AM_INX },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'S', 'L', 'O', SFC_AM_INX },
    { 'N', 'O', 'P', SFC_AM_ZPG },
    { 'O', 'R', 'A', SFC_AM_ZPG },
    { 'A', 'S', 'L', SFC_AM_ZPG },
    { 'S', 'L', 'O', SFC_AM_ZPG },
    { 'P', 'H', 'P', SFC_AM_IMP },
    { 'O', 'R', 'A', SFC_AM_IMM },
    { 'A', 'S', 'L', SFC_AM_ACC },
    { 'A', 'N', 'C', SFC_AM_IMM },
    { 'N', 'O', 'P', SFC_AM_ABS },
    { 'O', 'R', 'A', SFC_AM_ABS },
    { 'A', 'S', 'L', SFC_AM_ABS },
    { 'S', 'L', 'O', SFC_AM_ABS },

    { 'B', 'P', 'L', SFC_AM_REL },
    { 'O', 'R', 'A', SFC_AM_INY },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'S', 'L', 'O', SFC_AM_INY },
    { 'N', 'O', 'P', SFC_AM_ZPX },
    { 'O', 'R', 'A', SFC_AM_ZPX },
    { 'A', 'S', 'L', SFC_AM_ZPX },
    { 'S', 'L', 'O', SFC_AM_ZPX },
    { 'C', 'L', 'C', SFC_AM_IMP },
    { 'O', 'R', 'A', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_IMP },
    { 'S', 'L', 'O', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_ABX },
    { 'O', 'R', 'A', SFC_AM_ABX },
    { 'A', 'S', 'L', SFC_AM_ABX },
    { 'S', 'L', 'O', SFC_AM_ABX },

    { 'J', 'S', 'R', SFC_AM_ABS },
    { 'A', 'N', 'D', SFC_AM_INX },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'R', 'L', 'A', SFC_AM_INX },
    { 'B', 'I', 'T', SFC_AM_ZPG },
    { 'A', 'N', 'D', SFC_AM_ZPG },
    { 'R', 'O', 'L', SFC_AM_ZPG },
    { 'R', 'L', 'A', SFC_AM_ZPG },
    { 'P', 'L', 'P', SFC_AM_IMP },
    { 'A', 'N', 'D', SFC_AM_IMM },
    { 'R', 'O', 'L', SFC_AM_ACC },
    { 'A', 'N', 'C', SFC_AM_IMM },
    { 'B', 'I', 'T', SFC_AM_ABS },
    { 'A', 'N', 'D', SFC_AM_ABS },
    { 'R', 'O', 'L', SFC_AM_ABS },
    { 'R', 'L', 'A', SFC_AM_ABS },

    { 'B', 'M', 'I', SFC_AM_REL },
    { 'A', 'N', 'D', SFC_AM_INY },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'R', 'L', 'A', SFC_AM_INY },
    { 'N', 'O', 'P', SFC_AM_ZPX },
    { 'A', 'N', 'D', SFC_AM_ZPX },
    { 'R', 'O', 'L', SFC_AM_ZPX },
    { 'R', 'L', 'A', SFC_AM_ZPX },
    { 'S', 'E', 'C', SFC_AM_IMP },
    { 'A', 'N', 'D', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_IMP },
    { 'R', 'L', 'A', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_ABX },
    { 'A', 'N', 'D', SFC_AM_ABX },
    { 'R', 'O', 'L', SFC_AM_ABX },
    { 'R', 'L', 'A', SFC_AM_ABX },

    { 'R', 'T', 'I', SFC_AM_IMP },
    { 'E', 'O', 'R', SFC_AM_INX },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'S', 'R', 'E', SFC_AM_INX },
    { 'N', 'O', 'P', SFC_AM_ZPG },
    { 'E', 'O', 'R', SFC_AM_ZPG },
    { 'L', 'S', 'R', SFC_AM_ZPG },
    { 'S', 'R', 'E', SFC_AM_ZPG },
    { 'P', 'H', 'A', SFC_AM_IMP },
    { 'E', 'O', 'R', SFC_AM_IMM },
    { 'L', 'S', 'R', SFC_AM_ACC },
    { 'A', 'S', 'R', SFC_AM_IMM },
    { 'J', 'M', 'P', SFC_AM_ABS },
    { 'E', 'O', 'R', SFC_AM_ABS },
    { 'L', 'S', 'R', SFC_AM_ABS },
    { 'S', 'R', 'E', SFC_AM_ABS },

    { 'B', 'V', 'C', SFC_AM_REL },
    { 'E', 'O', 'R', SFC_AM_INY },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'S', 'R', 'E', SFC_AM_INY },
    { 'N', 'O', 'P', SFC_AM_ZPX },
    { 'E', 'O', 'R', SFC_AM_ZPX },
    { 'L', 'S', 'R', SFC_AM_ZPX },
    { 'S', 'R', 'E', SFC_AM_ZPX },
    { 'C', 'L', 'I', SFC_AM_IMP },
    { 'E', 'O', 'R', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_IMP },
    { 'S', 'R', 'E', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_ABX },
    { 'E', 'O', 'R', SFC_AM_ABX },
    { 'L', 'S', 'R', SFC_AM_ABX },
    { 'S', 'R', 'E', SFC_AM_ABX },

    { 'R', 'T', 'S', SFC_AM_IMP },
    { 'A', 'D', 'C', SFC_AM_INX },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'R', 'R', 'A', SFC_AM_INX },
    { 'N', 'O', 'P', SFC_AM_ZPG },
    { 'A', 'D', 'C', SFC_AM_ZPG },
    { 'R', 'O', 'R', SFC_AM_ZPG },
    { 'R', 'R', 'A', SFC_AM_ZPG },
    { 'P', 'L', 'A', SFC_AM_IMP },
    { 'A', 'D', 'C', SFC_AM_IMM },
    { 'R', 'O', 'R', SFC_AM_ACC },
    { 'A', 'R', 'R', SFC_AM_IMM },
    { 'J', 'M', 'P', SFC_AM_IND },
    { 'A', 'D', 'C', SFC_AM_ABS },
    { 'R', 'O', 'R', SFC_AM_ABS },
    { 'R', 'R', 'A', SFC_AM_ABS },

    { 'B', 'V', 'S', SFC_AM_REL },
    { 'A', 'D', 'C', SFC_AM_INY },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'R', 'R', 'A', SFC_AM_INY },
    { 'N', 'O', 'P', SFC_AM_ZPX },
    { 'A', 'D', 'C', SFC_AM_ZPX },
    { 'R', 'O', 'R', SFC_AM_ZPX },
    { 'R', 'R', 'A', SFC_AM_ZPX },
    { 'S', 'E', 'I', SFC_AM_IMP },
    { 'A', 'D', 'C', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_IMP },
    { 'R', 'R', 'A', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_ABX },
    { 'A', 'D', 'C', SFC_AM_ABX },
    { 'R', 'O', 'R', SFC_AM_ABX },
    { 'R', 'R', 'A', SFC_AM_ABX },

    { 'N', 'O', 'P', SFC_AM_IMM },
    { 'S', 'T', 'A', SFC_AM_INX },
    { 'N', 'O', 'P', SFC_AM_IMM },
    { 'S', 'A', 'X', SFC_AM_INX },
    { 'S', 'T', 'Y', SFC_AM_ZPG },
    { 'S', 'T', 'A', SFC_AM_ZPG },
    { 'S', 'T', 'X', SFC_AM_ZPG },
    { 'S', 'A', 'X', SFC_AM_ZPG },
    { 'D', 'E', 'Y', SFC_AM_IMP },
    { 'N', 'O', 'P', SFC_AM_IMM },
    { 'T', 'A', 'X', SFC_AM_IMP },
    { 'X', 'X', 'A', SFC_AM_IMM },
    { 'S', 'T', 'Y', SFC_AM_ABS },
    { 'S', 'T', 'A', SFC_AM_ABS },
    { 'S', 'T', 'X', SFC_AM_ABS },
    { 'S', 'A', 'X', SFC_AM_ABS },

    { 'B', 'C', 'C', SFC_AM_REL },
    { 'S', 'T', 'A', SFC_AM_INY },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'A', 'H', 'X', SFC_AM_INY },
    { 'S', 'T', 'Y', SFC_AM_ZPX },
    { 'S', 'T', 'A', SFC_AM_ZPX },
    { 'S', 'T', 'X', SFC_AM_ZPY },
    { 'S', 'A', 'X', SFC_AM_ZPY },
    { 'T', 'Y', 'A', SFC_AM_IMP },
    { 'S', 'T', 'A', SFC_AM_ABY },
    { 'T', 'X', 'S', SFC_AM_IMP },
    { 'T', 'A', 'S', SFC_AM_ABY },
    { 'S', 'H', 'Y', SFC_AM_ABX },
    { 'S', 'T', 'A', SFC_AM_ABX },
    { 'S', 'H', 'X', SFC_AM_ABY },
    { 'A', 'H', 'X', SFC_AM_ABY },

    { 'L', 'D', 'Y', SFC_AM_IMM },
    { 'L', 'D', 'A', SFC_AM_INX },
    { 'L', 'D', 'X', SFC_AM_IMM },
    { 'L', 'A', 'X', SFC_AM_INX },
    { 'L', 'D', 'Y', SFC_AM_ZPG },
    { 'L', 'D', 'A', SFC_AM_ZPG },
    { 'L', 'D', 'X', SFC_AM_ZPG },
    { 'L', 'A', 'X', SFC_AM_ZPG },
    { 'T', 'A', 'Y', SFC_AM_IMP },
    { 'L', 'D', 'A', SFC_AM_IMM },
    { 'T', 'A', 'X', SFC_AM_IMP },
    { 'L', 'A', 'X', SFC_AM_IMM },
    { 'L', 'D', 'Y', SFC_AM_ABS },
    { 'L', 'D', 'A', SFC_AM_ABS },
    { 'L', 'D', 'X', SFC_AM_ABS },
    { 'L', 'A', 'X', SFC_AM_ABS },

    { 'B', 'C', 'S', SFC_AM_REL },
    { 'L', 'D', 'A', SFC_AM_INY },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'L', 'A', 'X', SFC_AM_INY },
    { 'L', 'D', 'Y', SFC_AM_ZPX },
    { 'L', 'D', 'A', SFC_AM_ZPX },
    { 'L', 'D', 'X', SFC_AM_ZPY },
    { 'L', 'A', 'X', SFC_AM_ZPY },
    { 'C', 'L', 'V', SFC_AM_IMP },
    { 'L', 'D', 'A', SFC_AM_ABY },
    { 'T', 'S', 'X', SFC_AM_IMP },
    { 'L', 'A', 'S', SFC_AM_ABY },
    { 'L', 'D', 'Y', SFC_AM_ABX },
    { 'L', 'D', 'A', SFC_AM_ABX },
    { 'L', 'D', 'X', SFC_AM_ABY },
    { 'L', 'A', 'X', SFC_AM_ABY },

    { 'C', 'P', 'Y', SFC_AM_IMM },
    { 'C', 'M', 'P', SFC_AM_INX },
    { 'N', 'O', 'P', SFC_AM_IMM },
    { 'D', 'C', 'P', SFC_AM_INX },
    { 'C', 'P', 'Y', SFC_AM_ZPG },
    { 'C', 'M', 'P', SFC_AM_ZPG },
    { 'D', 'E', 'C', SFC_AM_ZPG },
    { 'D', 'C', 'P', SFC_AM_ZPG },
    { 'I', 'N', 'Y', SFC_AM_IMP },
    { 'C', 'M', 'P', SFC_AM_IMM },
    { 'D', 'E', 'X', SFC_AM_IMP },
    { 'A', 'X', 'S', SFC_AM_IMM },
    { 'C', 'P', 'Y', SFC_AM_ABS },
    { 'C', 'M', 'P', SFC_AM_ABS },
    { 'D', 'E', 'C', SFC_AM_ABS },
    { 'D', 'C', 'P', SFC_AM_ABS },

    { 'B', 'N', 'E', SFC_AM_REL },
    { 'C', 'M', 'P', SFC_AM_INY },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'D', 'C', 'P', SFC_AM_INY },
    { 'N', 'O', 'P', SFC_AM_ZPX },
    { 'C', 'M', 'P', SFC_AM_ZPX },
    { 'D', 'E', 'C', SFC_AM_ZPX },
    { 'D', 'C', 'P', SFC_AM_ZPX },
    { 'C', 'L', 'D', SFC_AM_IMP },
    { 'C', 'M', 'P', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_IMP },
    { 'D', 'C', 'P', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_ABX },
    { 'C', 'M', 'P', SFC_AM_ABX },
    { 'D', 'E', 'C', SFC_AM_ABX },
    { 'D', 'C', 'P', SFC_AM_ABX },

    { 'C', 'P', 'X', SFC_AM_IMM },
    { 'S', 'B', 'C', SFC_AM_INX },
    { 'N', 'O', 'P', SFC_AM_IMM },
    { 'I', 'S', 'B', SFC_AM_INX },
    { 'C', 'P', 'X', SFC_AM_ZPG },
    { 'S', 'B', 'C', SFC_AM_ZPG },
    { 'I', 'N', 'C', SFC_AM_ZPG },
    { 'I', 'S', 'B', SFC_AM_ZPG },
    { 'I', 'N', 'X', SFC_AM_IMP },
    { 'S', 'B', 'C', SFC_AM_IMM },
    { 'N', 'O', 'P', SFC_AM_IMP },
    { 'S', 'B', 'C', SFC_AM_IMM },
    { 'C', 'P', 'X', SFC_AM_ABS },
    { 'S', 'B', 'C', SFC_AM_ABS },
    { 'I', 'N', 'C', SFC_AM_ABS },
    { 'I', 'S', 'B', SFC_AM_ABS },

    { 'B', 'E', 'Q', SFC_AM_REL },
    { 'S', 'B', 'C', SFC_AM_INY },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'I', 'S', 'B', SFC_AM_INY },
    { 'N', 'O', 'P', SFC_AM_ZPX },
    { 'S', 'B', 'C', SFC_AM_ZPX },
    { 'I', 'N', 'C', SFC_AM_ZPX },
    { 'I', 'S', 'B', SFC_AM_ZPX },
    { 'S', 'E', 'D', SFC_AM_IMP },
    { 'S', 'B', 'C', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_IMP },
    { 'I', 'S', 'B', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_ABX },
    { 'S', 'B', 'C', SFC_AM_ABX },
    { 'I', 'N', 'C', SFC_AM_ABX },
    { 'I', 'S', 'B', SFC_AM_ABX },
};

/// <summary>
/// 十六进制字符数据
/// </summary>
static const char SFC_HEXDATA[] = "0123456789ABCDEF";

/// <summary>
/// 转换为16进制
/// </summary>
/// <param name="o">The o.</param>
/// <param name="b">The b.</param>
extern inline void sfc_btoh(char o[], uint8_t b) {
    // 高半字节
    o[0] = SFC_HEXDATA[b >> 4];
    // 低半字节
    o[1] = SFC_HEXDATA[b & (uint8_t)0x0F];
}

/// <summary>
/// 转换为有符号10进制
/// </summary>
/// <param name="o">The o.</param>
/// <param name="b">The b.</param>
static inline void sfc_btod(char o[], uint8_t b) {
    const int8_t sb = (int8_t)b;
    if (sb < 0) {
        o[0] = '-';
        b = -b;
    }
    else o[0] = '+';
    o[1] = SFC_HEXDATA[(uint8_t)b / 100];
    o[2] = SFC_HEXDATA[(uint8_t)b / 10 % 10];
    o[3] = SFC_HEXDATA[(uint8_t)b % 10];
}

#define sfc_fallthrough

/// <summary>
/// StepFC: 反汇编
/// </summary>
/// <param name="code">The code.</param>
/// <param name="buf">The buf.</param>
void sfc_6502_disassembly(sfc_6502_code_t code, char buf[SFC_DISASSEMBLY_BUF_LEN]) {
    enum {
        NAME_FIRSH = 0,
        ADDR_FIRSH = NAME_FIRSH + 4,
        LEN = ADDR_FIRSH + 9
    };
    memset(buf, ' ', LEN); buf[LEN] = ';'; buf[LEN + 1] = 0;
    static_assert(LEN + 1 < SFC_DISASSEMBLY_BUF_LEN, "");
    const struct sfc_opname opname = s_opname_data[code.op];
    // 设置操作码
    buf[NAME_FIRSH + 0] = opname.name[0];
    buf[NAME_FIRSH + 1] = opname.name[1];
    buf[NAME_FIRSH + 2] = opname.name[2];
    // 查看寻址模式
    switch (opname.mode)
    {
    case SFC_AM_UNK:
        sfc_fallthrough;
    case SFC_AM_IMP:
        // XXX     ;
        break;
    case SFC_AM_ACC:
        // XXX A   ;
        buf[ADDR_FIRSH + 0] = 'A';
        break;
    case SFC_AM_IMM:
        // XXX #$AB
        buf[ADDR_FIRSH + 0] = '#';
        buf[ADDR_FIRSH + 1] = '$';
        sfc_btoh(buf + ADDR_FIRSH + 2, code.a1);
        break;
    case SFC_AM_ABS:
        // XXX $ABCD
        sfc_fallthrough;
    case SFC_AM_ABX:
        // XXX $ABCD, X
        sfc_fallthrough;
    case SFC_AM_ABY:
        // XXX $ABCD, Y
        // REAL
        buf[ADDR_FIRSH] = '$';
        sfc_btoh(buf + ADDR_FIRSH + 1, code.a2);
        sfc_btoh(buf + ADDR_FIRSH + 3, code.a1);
        if (opname.mode == SFC_AM_ABS) break;
        buf[ADDR_FIRSH + 5] = ',';
        buf[ADDR_FIRSH + 7] = opname.mode == SFC_AM_ABX ? 'X' : 'Y';
        break;
    case SFC_AM_ZPG:
        // XXX $AB
        sfc_fallthrough;
    case SFC_AM_ZPX:
        // XXX $AB, X
        sfc_fallthrough;
    case SFC_AM_ZPY:
        // XXX $AB, Y
        // REAL
        buf[ADDR_FIRSH] = '$';
        sfc_btoh(buf + ADDR_FIRSH + 1, code.a1);
        if (opname.mode == SFC_AM_ZPG) break;
        buf[ADDR_FIRSH + 3] = ',';
        buf[ADDR_FIRSH + 5] = opname.mode == SFC_AM_ABX ? 'X' : 'Y';
        break;
    case SFC_AM_INX:
        // XXX ($AB, X)
        buf[ADDR_FIRSH + 0] = '(';
        buf[ADDR_FIRSH + 1] = '$';
        sfc_btoh(buf + ADDR_FIRSH + 2, code.a1);
        buf[ADDR_FIRSH + 4] = ',';
        buf[ADDR_FIRSH + 6] = 'X';
        buf[ADDR_FIRSH + 7] = ')';
        break;
    case SFC_AM_INY:
        // XXX ($AB), Y
        buf[ADDR_FIRSH + 0] = '(';
        buf[ADDR_FIRSH + 1] = '$';
        sfc_btoh(buf + ADDR_FIRSH + 2, code.a1);
        buf[ADDR_FIRSH + 4] = ')';
        buf[ADDR_FIRSH + 5] = ',';
        buf[ADDR_FIRSH + 7] = 'Y';
        break;
    case SFC_AM_IND:
        // XXX ($ABCD)
        buf[ADDR_FIRSH + 0] = '(';
        buf[ADDR_FIRSH + 1] = '$';
        sfc_btoh(buf + ADDR_FIRSH + 2, code.a2);
        sfc_btoh(buf + ADDR_FIRSH + 4, code.a1);
        buf[ADDR_FIRSH + 6] = ')';
        break;
    case SFC_AM_REL:
        // XXX $AB(-085)
        // XXX $ABCD
        buf[ADDR_FIRSH + 0] = '$';
        //const uint16_t target = base + int8_t(data.a1);
        //sfc_btoh(buf + ADDR_FIRSH + 1, uint8_t(target >> 8));
        //sfc_btoh(buf + ADDR_FIRSH + 3, uint8_t(target & 0xFF));
        sfc_btoh(buf + ADDR_FIRSH + 1, code.a1);
        buf[ADDR_FIRSH + 3] = '(';
        sfc_btod(buf + ADDR_FIRSH + 4, code.a1);
        buf[ADDR_FIRSH + 8] = ')';
        break;
    }
}
