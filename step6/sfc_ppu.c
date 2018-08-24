#include "sfc_ppu.h"
#include <assert.h>
#include <string.h>

/// <summary>
/// StepFC: 读取PPU地址空间
/// </summary>
/// <param name="address">The address.</param>
/// <param name="data">The data.</param>
/// <param name="ppu">The ppu.</param>
uint8_t sfc_read_ppu_address(uint16_t address, sfc_ppu_t* ppu) {
    const uint16_t real_address = address & (uint16_t)0x3FFF;
    // 使用BANK读取
    if (real_address < (uint16_t)0x3F00) {
        const uint16_t index = real_address >> 10;
        const uint16_t offset = real_address & (uint16_t)0x3FF;
        assert(ppu->banks[index]);
        const uint8_t data = ppu->pseudo;
        ppu->pseudo = ppu->banks[index][offset];
        return data;
    }
    // 调色板索引
    else return ppu->pseudo = ppu->spindexes[real_address & (uint16_t)0x1f];
}

/// <summary>
/// StepFC: 写入PPU地址空间
/// </summary>
/// <param name="address">The address.</param>
/// <param name="data">The data.</param>
/// <param name="ppu">The ppu.</param>
void sfc_write_ppu_address(uint16_t address, uint8_t data, sfc_ppu_t* ppu) {
    const uint16_t real_address = address & (uint16_t)0x3FFF;
    // 使用BANK写入
    if (real_address < (uint16_t)0x3F00) {
        assert(real_address >= 0x2000);
        const uint16_t index = real_address >> 10;
        const uint16_t offset = real_address & (uint16_t)0x3FF;
        assert(ppu->banks[index]);
        ppu->banks[index][offset] = data;
    }
    // 调色板索引
    else {
        // 独立地址
        if (real_address & (uint16_t)0x03) {
            ppu->spindexes[real_address & (uint16_t)0x1f] = data;
        }
        // 镜像$3F00/$3F04/$3F08/$3F0C
        else {
            const uint16_t offset = real_address & (uint16_t)0x0f;
            ppu->spindexes[offset] = data;
            ppu->spindexes[offset | (uint16_t)0x10] = data;
        }
    }
}


/// <summary>
/// StepFC: 使用CPU地址空间读取PPU寄存器
/// </summary>
/// <param name="address">The address.</param>
/// <param name="ppu">The ppu.</param>
/// <returns></returns>
uint8_t sfc_read_ppu_register_via_cpu(uint16_t address, sfc_ppu_t* ppu) {
    uint8_t data = 0x00;
    // 8字节镜像
    switch (address & (uint16_t)0x7)
    {
    case 0:
        // 0x2000: Controller ($2000) > write
        // 只写寄存器
    case 1:
        // 0x2001: Mask ($2001) > write
        // 只写寄存器
        assert(!"write only!");
        break;
    case 2:
        // 0x2002: Status ($2002) < read
        // 只读状态寄存器
        data = ppu->status;
        // 读取后会清除VBlank状态
        ppu->status &= ~(uint8_t)SFC_PPUFLAG_VBlank;
        break;
    case 3:
        // 0x2003: OAM address port ($2003) > write
        // 只写寄存器
        assert(!"write only!");
        break;
    case 4:
        // 0x2004: OAM data ($2004) <> read/write
        // 读写寄存器
        data = ppu->sprites[ppu->oamaddr++];
        break;
    case 5:
        // 0x2005: Scroll ($2005) >> write x2
        // 双写寄存器
    case 6:
        // 0x2006: Address ($2006) >> write x2
        // 双写寄存器
        assert(!"write only!");
        break;
    case 7:
        // 0x2007: Data ($2007) <> read/write
        // PPU VRAM读写端口
        data = sfc_read_ppu_address(ppu->vramaddr, ppu);
        ppu->vramaddr += (uint16_t)((ppu->ctrl & SFC_PPUFLAG_VINC32) ? 32 : 1);
        break;
    }
    return data;
}


/// <summary>
/// StepFC: 使用CPU地址空间写入PPU寄存器
/// </summary>
/// <param name="address">The address.</param>
/// <param name="data">The data.</param>
/// <param name="ppu">The ppu.</param>
/// <returns></returns>
void sfc_write_ppu_register_via_cpu(uint16_t address, uint8_t data, sfc_ppu_t* ppu) {
    switch (address & (uint16_t)0x7)
    {
    case 0:
        // PPU 控制寄存器
        // 0x2000: Controller ($2000) > write
        ppu->ctrl = data;
        break;
    case 1:
        // PPU 掩码寄存器
        // 0x2001: Mask ($2001) > write
        ppu->mask = data;
        break;
    case 2:
        // 0x2002: Status ($2002) < read
        // 只读
        assert(!"read only");
        break;
    case 3:
        // 0x2003: OAM address port ($2003) > write
        // PPU OAM 地址端口
        ppu->oamaddr = data;
        break;
    case 4:
        // 0x2004: OAM data ($2004) <> read/write
        // PPU OAM 数据端口
        ppu->sprites[ppu->oamaddr++] = data;
        break;
    case 5:
        // 0x2005: Scroll ($2005) >> write x2
        // PPU 滚动位置寄存器 - 双写
        ppu->scroll[ppu->writex2 & 1] = data;
        ++ppu->writex2;
        break;
    case 6:
        // 0x2006: Address ($2006) >> write x2
        // PPU 地址寄存器 - 双写
        // 写入高字节
        if (ppu->writex2 & 1) {
            ppu->vramaddr = (ppu->vramaddr & (uint16_t)0xFF00) | (uint16_t)data;
        }
        // 写入低字节
        else {
            ppu->vramaddr = (ppu->vramaddr & (uint16_t)0x00FF) | ((uint16_t)data << 8);
        }
        ++ppu->writex2;
        break;
    case 7:
        // 0x2007: Data ($2007) <> read/write
        // PPU VRAM数据端
        sfc_write_ppu_address(ppu->vramaddr, data, ppu);
        ppu->vramaddr += (uint16_t)((ppu->ctrl & SFC_PPUFLAG_VINC32) ? 32 : 1);
        break;
    }
}


/// <summary>
/// 调色板数据
/// </summary>
const union sfc_palette_data {
    struct { uint8_t r, g, a, x; };
    uint32_t    data;
} sfc_stdalette[64] = {
    { 0x7F, 0x7F, 0x7F, 0xFF }, { 0x20, 0x00, 0xB0, 0xFF }, { 0x28, 0x00, 0xB8, 0xFF }, { 0x60, 0x10, 0xA0, 0xFF },
    { 0x98, 0x20, 0x78, 0xFF }, { 0xB0, 0x10, 0x30, 0xFF }, { 0xA0, 0x30, 0x00, 0xFF }, { 0x78, 0x40, 0x00, 0xFF },
    { 0x48, 0x58, 0x00, 0xFF }, { 0x38, 0x68, 0x00, 0xFF }, { 0x38, 0x6C, 0x00, 0xFF }, { 0x30, 0x60, 0x40, 0xFF },
    { 0x30, 0x50, 0x80, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF },

    { 0xBC, 0xBC, 0xBC, 0xFF }, { 0x40, 0x60, 0xF8, 0xFF }, { 0x40, 0x40, 0xFF, 0xFF }, { 0x90, 0x40, 0xF0, 0xFF },
    { 0xD8, 0x40, 0xC0, 0xFF }, { 0xD8, 0x40, 0x60, 0xFF }, { 0xE0, 0x50, 0x00, 0xFF }, { 0xC0, 0x70, 0x00, 0xFF },
    { 0x88, 0x88, 0x00, 0xFF }, { 0x50, 0xA0, 0x00, 0xFF }, { 0x48, 0xA8, 0x10, 0xFF }, { 0x48, 0xA0, 0x68, 0xFF },
    { 0x40, 0x90, 0xC0, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF },

    { 0xFF, 0xFF, 0xFF, 0xFF }, { 0x60, 0xA0, 0xFF, 0xFF }, { 0x50, 0x80, 0xFF, 0xFF }, { 0xA0, 0x70, 0xFF, 0xFF },
    { 0xF0, 0x60, 0xFF, 0xFF }, { 0xFF, 0x60, 0xB0, 0xFF }, { 0xFF, 0x78, 0x30, 0xFF }, { 0xFF, 0xA0, 0x00, 0xFF },
    { 0xE8, 0xD0, 0x20, 0xFF }, { 0x98, 0xE8, 0x00, 0xFF }, { 0x70, 0xF0, 0x40, 0xFF }, { 0x70, 0xE0, 0x90, 0xFF },
    { 0x60, 0xD0, 0xE0, 0xFF }, { 0x60, 0x60, 0x60, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF },

    { 0xFF, 0xFF, 0xFF, 0xFF }, { 0x90, 0xD0, 0xFF, 0xFF }, { 0xA0, 0xB8, 0xFF, 0xFF }, { 0xC0, 0xB0, 0xFF, 0xFF },
    { 0xE0, 0xB0, 0xFF, 0xFF }, { 0xFF, 0xB8, 0xE8, 0xFF }, { 0xFF, 0xC8, 0xB8, 0xFF }, { 0xFF, 0xD8, 0xA0, 0xFF },
    { 0xFF, 0xF0, 0x90, 0xFF }, { 0xC8, 0xF0, 0x80, 0xFF }, { 0xA0, 0xF0, 0xA0, 0xFF }, { 0xA0, 0xFF, 0xC8, 0xFF },
    { 0xA0, 0xFF, 0xF0, 0xFF }, { 0xA0, 0xA0, 0xA0, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }
};