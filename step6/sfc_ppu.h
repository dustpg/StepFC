#pragma once
#include <stdint.h>

// PPU用标志
enum sfc_ppu_flag {
    SFC_PPUFLAG_NMIGen  = 0x80, // [0x2000]VBlank期间是否产生NMI
    SFC_PPUFLAG_Sp8x16  = 0x20, // [0x2000]精灵为8x16(1), 还是8x8(0)
    SFC_PPUFLAG_BgTabl  = 0x10, // [0x2000]背景调色板表地址$1000(1), $0000(0)
    SFC_PPUFLAG_SpTabl  = 0x08, // [0x2000]精灵调色板表地址$1000(1), $0000(0), 8x16模式下被忽略
    SFC_PPUFLAG_VINC32  = 0x04, // [0x2000]VRAM读写增加值32(1), 1(0)
        
    SFC_PPUFLAG_VBlank  = 0x80, // [0x2002]垂直空白间隙标志
    SFC_PPUFLAG_Sp0Hit  = 0x40, // [0x2002]零号精灵命中标志
    SFC_PPUFLAG_SpOver  = 0x20, // [0x2002]精灵溢出标志
};

/// <summary>
/// 
/// </summary>
typedef struct {
    // 内存地址库
    uint8_t*        banks[0x4000 / 0x0400];
    // VRAM 地址
    uint16_t        vramaddr;
    // 寄存器 PPUCTRL      @$2000
    uint8_t         ctrl;
    // 寄存器 PPUMASK      @$2001
    uint8_t         mask;
    // 寄存器 PPUSTATUS    @$2002
    uint8_t         status;
    // 寄存器 OAMADDR      @$2003
    uint8_t         oamaddr;
    // 滚动偏移
    uint8_t         scroll[2];
    // 滚动偏移双写位置记录
    uint8_t         writex2;
    // 显存读取缓冲值
    uint8_t         pseudo;
    // 精灵调色板索引
    uint8_t         spindexes[0x20];
    // 精灵数据: 256B
    uint8_t         sprites[0x100];
} sfc_ppu_t;


// read ppu register via cpu address space
uint8_t sfc_read_ppu_register_via_cpu(uint16_t, sfc_ppu_t*);
// write ppu register via cpu address space
void sfc_write_ppu_register_via_cpu(uint16_t, uint8_t, sfc_ppu_t*);

