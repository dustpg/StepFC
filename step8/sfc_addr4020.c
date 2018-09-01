#include "sfc_6502.h"
#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include <assert.h>
#include <string.h>

/// <summary>
/// StepFC: 读取CPU地址数据4020
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline uint8_t sfc_read_cpu_address4020(uint16_t address, sfc_famicom_t* famicom) {
    uint8_t data = 0;
    switch (address & (uint16_t)0x1f)
    {
    case 0x16:
        // 手柄端口#1
        data = (famicom->button_states+0)[famicom->button_index_1 & famicom->button_index_mask];
        ++famicom->button_index_1;
        break;
    case 0x17:
        // 手柄端口#2
        data = (famicom->button_states+8)[famicom->button_index_2 & famicom->button_index_mask];
        ++famicom->button_index_2;
        break;
    }
    return data;
}


/// <summary>
/// StepFC: 获取DMA地址
/// </summary>
/// <param name="data">The data.</param>
/// <returns></returns>
static inline const uint8_t* sfc_get_dma_address(uint8_t data, const sfc_famicom_t* famicom) {
    const uint16_t offset = ((uint16_t)(data & 0x07) << 8);
    switch (data >> 5)
    {
    default:
    case 1:
        // PPU寄存器
        assert(!"PPU REG!");
    case 2:
        // 扩展区
        assert(!"TODO");
    case 0:
        // 系统内存
        return famicom->main_memory + offset;
    case 3:
        // 存档 SRAM区
        return famicom->save_memory + offset;
    case 4: case 5: case 6: case 7:
        // 高一位为1, [$8000, $10000) 程序PRG-ROM区
        return famicom->prg_banks[data >> 5] + offset;
    }
}



/// <summary>
/// StepFC: 写入CPU地址数据4020
/// </summary>
/// <param name="address">The address.</param>
/// <param name="data">The data.</param>
/// <param name="famicom">The famicom.</param>
extern inline void sfc_write_cpu_address4020(uint16_t address, uint8_t data, sfc_famicom_t* famicom) {
    switch (address & (uint16_t)0x1f)
    {
    case 0x00:
        /*
            $4000 - 方波 1
            DDLC NNNN 
        */
        famicom->apu.square1.ctrl = data;
        //printf("4000: %d", data);
        break;
    case 0x01:
        /*
            $4001 - 方波 1
            EPPP NSSS
        */
        famicom->apu.square1.sweep = data;
        break;
    case 0x02:
        /*
            $4002 - 方波 1
            TTTT TTTT
        */
        famicom->apu.square1.fine = data;
        break;
    case 0x03:
        /*
            $4002 - 方波 1
            LLLL LTTT
        */
        famicom->apu.square1.coarse = data;
        break;
    case 0x14:
        // 精灵RAM直接储存器访问
        if (famicom->ppu.oamaddr) {
            uint8_t* dst = famicom->ppu.sprites;
            const uint8_t len = famicom->ppu.oamaddr;
            const uint8_t* src = sfc_get_dma_address(data, famicom);
            // 需要换行
            memcpy(dst, src + len, len);
            memcpy(dst + len, src, 256 - len);
        }
        else memcpy(famicom->ppu.sprites, sfc_get_dma_address(data, famicom), 256);
        famicom->cpu_cycle_count += 513;
        famicom->cpu_cycle_count += famicom->cpu_cycle_count & 1;
        break;
    case 0x15:
        // 状态寄存器

        break;
    case 0x16:
        // 手柄端口
        famicom->button_index_mask = (data & 1) ? 0x0 : 0x7;
        if (data & 1) {
            famicom->button_index_1 = 0;
            famicom->button_index_2 = 0;
        }
        break;
    }
}