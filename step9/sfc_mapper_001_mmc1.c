#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include <assert.h>
#include <string.h>


/// <summary>
/// 
/// </summary>
typedef struct  {
    // 移位寄存器
    uint8_t     shifter;
    // RPG-ROM 切换模式
    uint8_t     prmode;
    // CHR-ROM 切换模式
    uint8_t     crmode;

    // 未使用
    uint8_t     unused[1];

} sfc_mapper01_t;


/// <summary>
/// SFCs the mapper.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline sfc_mapper01_t* sfc_mapper(sfc_famicom_t* famicom) {
    return (sfc_mapper01_t*)famicom->mapper_buffer.mapper01;
}

#define MAPPER sfc_mapper01_t* const mapper = sfc_mapper(famicom);


/// <summary>
/// 实用函数-StepFC: 载入8k PRG-ROM
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="des">The DES.</param>
/// <param name="src">The source.</param>
static inline void sfc_load_prgrom_8k(
    sfc_famicom_t* famicom, int des, int src) {
    famicom->prg_banks[4 + des] = famicom->rom_info.data_prgrom + 8 * 1024 * src;
}


/// <summary>
/// 实用函数-StepFC: 载入1k CHR-ROM
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="des">The DES.</param>
/// <param name="src">The source.</param>
static inline void sfc_load_chrrom_1k(
    sfc_famicom_t* famicom, int des, int src) {
    famicom->ppu.banks[des] = famicom->rom_info.data_chrrom + 1024 * src;
}


// ------------------------------- MAPPER 001 - MMC1 - SxROM


/// <summary>
/// SFCs the mapper 01 write control.
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_01_write_control(sfc_famicom_t* famicom) {
    MAPPER;
    const uint8_t data = mapper->shifter;
    // D0D1 - 镜像模式
    const sfc_nametable_mirroring_mode mode = data & (uint8_t)0x3;
    sfc_switch_nametable_mirroring(famicom, mode);
    // D2D3 - PRG ROM bank 模式
    const uint8_t prmode = (data >> 2) & (uint8_t)0x3;
    mapper->prmode = prmode;
    // D5 - CHR ROM bank 模式
    const uint8_t crmode = data >> 4;
    mapper->crmode = crmode;
}

/// <summary>
/// SFCs the mapper 01 write register.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="address">The address.</param>
static void sfc_mapper_01_write_register(sfc_famicom_t* famicom, uint16_t address) {
    switch ((address & (uint16_t)0x7FFF) >> 13)
    {
    case 0:
        // $8000-$9FFF Control
        sfc_mapper_01_write_control(famicom);
        break;
    case 1:
        // $A000-$BFFF CHR bank 0
        assert(!"NOT IMPL");
        break;
    case 2:
        // $C000-$DFFF CHR bank 1
        assert(!"NOT IMPL");
        break;
    case 3:
        // $E000-$FFFF PRG bank
        assert(!"NOT IMPL");
        break;
    }
}

/// <summary>
/// StepFC: MAPPER 001 - NROM 重置
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static sfc_ecode sfc_mapper_01_reset(sfc_famicom_t* famicom) {
    // 你用MMC1居然没有32KB PRG-ROM?
    assert(famicom->rom_info.count_prgrom16kb > 2 && "bad count");
    // PRG-ROM
    sfc_load_prgrom_8k(famicom, 0, 0);
    sfc_load_prgrom_8k(famicom, 1, 1);
    const int last = famicom->rom_info.count_prgrom16kb * 2;
    sfc_load_prgrom_8k(famicom, 2, last-2);
    sfc_load_prgrom_8k(famicom, 3, last-1);
    // CHR-ROM
    for (int i = 0; i != 8; ++i)
        sfc_load_chrrom_1k(famicom, i, i);
    return SFC_ERROR_OK;
}


/// <summary>
/// SFCs the mapper 01 write high.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="address">The address.</param>
/// <param name="value">The value.</param>
static void sfc_mapper_01_write_high(sfc_famicom_t* famicom, uint16_t address, uint8_t value) {
    MAPPER;
    // D7 = 1 -> 重置移位寄存器
    if (value & (uint8_t)0x80) {
        mapper->shifter = 0x10;
    }
    // D7 = 0 -> 写入D0到移位寄存器
    else {
        const uint8_t finished = mapper->shifter & 1;
        mapper->shifter >>= 1;
        mapper->shifter |= (value & 1) << 4;
        if (finished) sfc_mapper_01_write_register(famicom, address);
    }
}

/// <summary>
/// SFCs the load mapper 01.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline sfc_ecode sfc_load_mapper_01(sfc_famicom_t* famicom) {
    enum { MAPPER_01_SIZE_IMPL =  sizeof(sfc_mapper01_t) };
    static_assert(SFC_MAPPER_01_SIZE == MAPPER_01_SIZE_IMPL, "SAME");
    // 初始化回调接口
    famicom->mapper.reset = sfc_mapper_01_reset;
    famicom->mapper.write_high = sfc_mapper_01_write_high;
    // 初始化数据
    MAPPER;
    memset(mapper, 0, MAPPER_01_SIZE_IMPL);
    mapper->shifter = 0x10;

    return SFC_ERROR_OK;
}