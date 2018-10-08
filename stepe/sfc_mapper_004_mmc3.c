#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include "sfc_mapper_helper.h"
#include <assert.h>
#include <string.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

/// <summary>
/// 
/// </summary>
typedef struct {
    // 寄存器编号
    uint8_t     regid;
    // RPG-ROM 切换模式
    uint8_t     prmode;
    // CHR-ROM 切换模式
    uint8_t     crmode;
    // 未使用
    uint8_t     unused[1];
    // 寄存器列表
    uint8_t     regs[8];
    // 重载值
    uint8_t     reload;
    // 计数器
    uint8_t     counter;
    // IRQ使能
    uint8_t     irq_enable;
    // 未使用#2
    uint8_t     unused2[1];

} sfc_mapper04_t;


/// <summary>
/// SFCs the mapper.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline sfc_mapper04_t* sfc_mapper(sfc_famicom_t* famicom) {
    return (sfc_mapper04_t*)famicom->mapper_buffer.mapper04;
}

#define MAPPER sfc_mapper04_t* const mapper = sfc_mapper(famicom);
#define R0 (mapper->regs[0])
#define R1 (mapper->regs[1])
#define R2 (mapper->regs[2])
#define R3 (mapper->regs[3])
#define R4 (mapper->regs[4])
#define R5 (mapper->regs[5])
#define R6 (mapper->regs[6])
#define R7 (mapper->regs[7])
// ------------------------------- MAPPER 004 - TxROM

/// <summary>
/// SFCs the mapper 04 update banks.
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_04_update_banks(sfc_famicom_t* famicom) {
    MAPPER;
    if (mapper->crmode) {
        // R2R3R4R5R0r0R1r1
        sfc_load_chrrom_1k(famicom, 0, R2);
        sfc_load_chrrom_1k(famicom, 1, R3);
        sfc_load_chrrom_1k(famicom, 2, R4);
        sfc_load_chrrom_1k(famicom, 3, R5);
        sfc_load_chrrom_1k(famicom, 4, R0 & 0xFE);
        sfc_load_chrrom_1k(famicom, 5, R0 | 1);
        sfc_load_chrrom_1k(famicom, 6, R1 & 0xFE);
        sfc_load_chrrom_1k(famicom, 7, R1 | 1);
        //static int r1 = 0;
        //if (r1 != R1) {
        //    r1 = R1;
        //    printf("R1: %d\n", R1);
        //}
    }
    else {
        // R0r0R1r1R2R3R4R5
        sfc_load_chrrom_1k(famicom, 0, R0 & 0xFE);
        sfc_load_chrrom_1k(famicom, 1, R0 | 1);
        sfc_load_chrrom_1k(famicom, 2, R1 & 0xFE);
        sfc_load_chrrom_1k(famicom, 3, R1 | 1);
        sfc_load_chrrom_1k(famicom, 4, R2);
        sfc_load_chrrom_1k(famicom, 5, R3);
        sfc_load_chrrom_1k(famicom, 6, R4);
        sfc_load_chrrom_1k(famicom, 7, R5);
    }
    
    const int last = famicom->rom_info.count_prgrom16kb * 2;
    if (mapper->prmode) {
        // (-2) R7 R6 (-1)
        sfc_load_prgrom_8k(famicom, 0, last - 2);
        sfc_load_prgrom_8k(famicom, 1, R7);
        sfc_load_prgrom_8k(famicom, 2, R6);
        //sfc_load_prgrom_8k(famicom, 3, last - 1);
    }
    else {
        // R6 R7 (-2) (-1)
        sfc_load_prgrom_8k(famicom, 0, R6);
        sfc_load_prgrom_8k(famicom, 1, R7);
        sfc_load_prgrom_8k(famicom, 2, last - 2);
        //sfc_load_prgrom_8k(famicom, 3, last - 1);
    }
    //static int r7 = 0;
    //if (r7 != R7) {
    //    r7 = R7;
    //    if (r7 == 11) {
    //        int bk = 9;
    //    }
    //    printf("R7: %d\n", r7);
    //}

}

/// <summary>
/// SFCs the mapper 04 write bank select.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="value">The value.</param>
static inline void sfc_mapper_04_write_bank_select(sfc_famicom_t* famicom, uint8_t value) {
    // D2D1D0: Specify which bank register to update on next write to Bank Data register
    //  -  0: Select 2 KB CHR bank at PPU $0000 - $07FF(or $1000 - $17FF);
    //  -  1: Select 2 KB CHR bank at PPU $0800 - $0FFF(or $1800 - $1FFF);
    //  -  2: Select 1 KB CHR bank at PPU $1000 - $13FF(or $0000 - $03FF);
    //  -  3: Select 1 KB CHR bank at PPU $1400 - $17FF(or $0400 - $07FF);
    //  -  4: Select 1 KB CHR bank at PPU $1800 - $1BFF(or $0800 - $0BFF);
    //  -  5: Select 1 KB CHR bank at PPU $1C00 - $1FFF(or $0C00 - $0FFF);
    //  -  6: Select 8 KB PRG ROM bank at $8000 - $9FFF(or $C000 - $DFFF);
    //  -  7: Select 8 KB PRG ROM bank at $A000 - $BFFF
    // D5: Nothing on the MMC3, MMC6: [PRG RAM enable]
    // D6: PRG ROM bank mode
    // D7: CHR A12 inversion
    MAPPER;
    mapper->regid = value & 7;
    mapper->prmode = (value >> 6) & 1;
    mapper->crmode = (value >> 7) & 1;
    sfc_mapper_04_update_banks(famicom);
}

/// <summary>
/// SFCs the mapper 04 write bank data.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="value">The value.</param>
static inline void sfc_mapper_04_write_bank_data(sfc_famicom_t* famicom, uint8_t value) {
    MAPPER;
    mapper->regs[mapper->regid] = value;
    //switch (mapper->regid)
    //{
    //case 6:
    //    break;
    //case 7:
    //    // $A000-$BFFF
    //    sfc_load_prgrom_8k(famicom, 1, R7);
    //    break;
    //}
    sfc_mapper_04_update_banks(famicom);
}


/// <summary>
/// SFCs the mapper 04 write mirroring.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="value">The value.</param>
static inline void sfc_mapper_04_write_mirroring(sfc_famicom_t* famicom, uint8_t value) {
    // D0: Nametable mirroring (0: vertical; 1: horizontal)
    const sfc_nametable_mirroring_mode mode
        = (value & 1)
        ? SFC_NT_MIR_Horizontal
        : SFC_NT_MIR_Vertical
        ;
    sfc_switch_nametable_mirroring(famicom, mode);
}

/// <summary>
/// SFCs the mapper 04 write ram protect.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="value">The value.</param>
static inline void sfc_mapper_04_write_ram_protect(sfc_famicom_t* famicom, uint8_t value) {
    // 是否实现?
}


/// <summary>
/// SFCs the mapper 04 write irq latch.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="value">The value.</param>
static inline void sfc_mapper_04_write_irq_latch(sfc_famicom_t* famicom, uint8_t value) {
    MAPPER;
    mapper->reload = value;
}

/// <summary>
/// SFCs the mapper 04 write irq reload.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="value">The value.</param>
static inline void sfc_mapper_04_write_irq_reload(sfc_famicom_t* famicom, uint8_t value) {
    MAPPER;
    mapper->counter = 0;
}


// IRQ - 中断请求 - 确认
extern inline void sfc_operation_IRQ_acknowledge(sfc_famicom_t* famicom);

/// <summary>
/// SFCs the mapper 04 write irq disable.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="value">The value.</param>
static inline void sfc_mapper_04_write_irq_disable(sfc_famicom_t* famicom, uint8_t value) {
    MAPPER;
    mapper->irq_enable = 0;
    sfc_operation_IRQ_acknowledge(famicom);
    //mapper->counter = mapper->reload;
    //printf("[IRQ-D]@%d\n", famicom->cpu_cycle_count);
}


/// <summary>
/// SFCs the mapper 04 write irq enable.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="value">The value.</param>
static inline void sfc_mapper_04_write_irq_enable(sfc_famicom_t* famicom, uint8_t value) {
    MAPPER;
    mapper->irq_enable = 1;
    //printf("[IRQ-E]@%d\n", famicom->cpu_cycle_count);
}

/// <summary>
/// SFCs the mapper 04 reset.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static sfc_ecode sfc_mapper_04_reset(sfc_famicom_t* famicom) {
    // 你用MMC4居然没有32KB PRG-ROM?
    assert(famicom->rom_info.count_prgrom16kb > 2 && "bad count");
    // PRG-ROM
    sfc_load_prgrom_8k(famicom, 0, 0);
    sfc_load_prgrom_8k(famicom, 1, 1);
    const int last = famicom->rom_info.count_prgrom16kb * 2;
    sfc_load_prgrom_8k(famicom, 2, last - 2);
    sfc_load_prgrom_8k(famicom, 3, last - 1);
    // CHR-ROM
    for (int i = 0; i != 8; ++i)
        sfc_load_chrrom_1k(famicom, i, i);
    return SFC_ERROR_OK;
}

// 尝试触发
extern inline void sfc_operation_IRQ_try(sfc_famicom_t* famicom);


/// <summary>
/// SFCs the mapper 04 hsync.
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_04_hsync(sfc_famicom_t* famicom) {
    //  打开渲染时?
    if (!(famicom->ppu.data.mask & (
        ((uint8_t)SFC_PPU2001_Back |
        (uint8_t)SFC_PPU2001_Sprite)
        ))) return;

    MAPPER;
    if (mapper->counter) {
        --mapper->counter;
        if (!mapper->counter && mapper->irq_enable) {
            sfc_operation_IRQ_try(famicom);
        }
    }
    else mapper->counter = mapper->reload;
}


/// <summary>
/// SFCs the mapper 04 write high.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="address">The address.</param>
/// <param name="value">The value.</param>
static void sfc_mapper_04_write_high(sfc_famicom_t* famicom, uint16_t address, uint8_t value) {
    // D14 D13 D0
    const uint16_t register_id 
        = (((address & (uint16_t)0x6000)) >> 12) 
        | (address & 1)
        ;
    switch (register_id)
    {
    case 0:
        // $8000-$9FFE Bank select
        sfc_mapper_04_write_bank_select(famicom, value);
        break;
    case 1:
        // $8001-$9FFF Bank data
        sfc_mapper_04_write_bank_data(famicom, value);
        break;
    case 2:
        // $A000-$BFFE Mirroring
        sfc_mapper_04_write_mirroring(famicom, value);
        break;
    case 3:
        // $A001-$BFFF PRG RAM protect
        sfc_mapper_04_write_ram_protect(famicom, value);
        break;
    case 4:
        // $C000-$DFFE IRQ latch
        sfc_mapper_04_write_irq_latch(famicom, value);
        break;
    case 5:
        // $C001-$DFFF IRQ reload
        sfc_mapper_04_write_irq_reload(famicom, value);
        break;
    case 6:
        // $E000-$FFFE IRQ disable
        sfc_mapper_04_write_irq_disable(famicom, value);
        break;
    case 7:
        // $E001-$FFFF IRQ enable
        sfc_mapper_04_write_irq_enable(famicom, value);
        break;
    }
}


/// <summary>
/// SFCs the load mapper 04
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline sfc_ecode sfc_load_mapper_04(sfc_famicom_t* famicom) {
    enum { MAPPER_04_SIZE_IMPL = sizeof(sfc_mapper04_t) };
    static_assert(SFC_MAPPER_04_SIZE == MAPPER_04_SIZE_IMPL, "SAME");
    // 初始化回调接口
    // 重置可以直接使用Mapper000的
    famicom->mapper.reset = sfc_mapper_04_reset;
    famicom->mapper.hsync = sfc_mapper_04_hsync;
    famicom->mapper.write_high = sfc_mapper_04_write_high;
    // 初始化数据
    MAPPER;
    memset(mapper, 0, MAPPER_04_SIZE_IMPL);
    return SFC_ERROR_OK;
}