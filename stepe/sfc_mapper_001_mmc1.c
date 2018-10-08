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
typedef struct  {
    // 移位寄存器
    uint8_t     shifter;
    // RPG-ROM 切换模式
    uint8_t     prmode;
    // CHR-ROM 切换模式
    uint8_t     crmode;
    // 控制寄存器
    uint8_t     control;

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

// ------------------------------- MAPPER 001 - MMC1 - SxROM


/// <summary>
/// SFCs the mapper 01 write control.
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_01_write_control(sfc_famicom_t* famicom, uint8_t data) {
    MAPPER;
    mapper->control = data;
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
/// SFCs the mapper 01 write chrbank0.
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_01_write_chrbank0(sfc_famicom_t* famicom) {
    MAPPER;
    const uint8_t data = mapper->shifter;
    const uint8_t mode = mapper->crmode;
    // 8KB模式?
    if (!mode) {
        const int bank = (data & (uint8_t)0x0E) * 4;
        sfc_load_chrrom_1k(famicom, 0, bank + 0);
        sfc_load_chrrom_1k(famicom, 1, bank + 1);
        sfc_load_chrrom_1k(famicom, 2, bank + 2);
        sfc_load_chrrom_1k(famicom, 3, bank + 3);
        sfc_load_chrrom_1k(famicom, 4, bank + 4);
        sfc_load_chrrom_1k(famicom, 5, bank + 5);
        sfc_load_chrrom_1k(famicom, 6, bank + 6);
        sfc_load_chrrom_1k(famicom, 7, bank + 7);
    }
    // 4KB模式
    else {
        const int bank = data * 4;
        sfc_load_chrrom_1k(famicom, 0, bank + 0);
        sfc_load_chrrom_1k(famicom, 1, bank + 1);
        sfc_load_chrrom_1k(famicom, 2, bank + 2);
        sfc_load_chrrom_1k(famicom, 3, bank + 3);
    }
}


/// <summary>
/// SFCs the mapper 01 write chrbank1.
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_01_write_chrbank1(sfc_famicom_t* famicom) {
    MAPPER;
    const uint8_t data = mapper->shifter;
    const uint8_t mode = mapper->crmode;
    // 8KB模式?
    if (!mode) return;
    // 4KB模式
    const int bank = data * 4;
    sfc_load_chrrom_1k(famicom, 4, bank + 0);
    sfc_load_chrrom_1k(famicom, 5, bank + 1);
    sfc_load_chrrom_1k(famicom, 6, bank + 2);
    sfc_load_chrrom_1k(famicom, 7, bank + 3);
}


/// <summary>
/// SFCs the mapper 01 write prgbank.
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_01_write_prgbank(sfc_famicom_t* famicom) {
    MAPPER;
    const uint8_t data = mapper->shifter;
    const uint8_t mode = mapper->prmode;
    // PRG RAM使能
    // TODO: PRG-RAM
    const uint8_t prgram = data & (uint8_t)0x10;
    const uint8_t bankid = data & (uint8_t)0x0f;
    switch (mode)
    {
        int bank;
        int last;
    case 0: case 1:
        // 32KB 模式
        bank = (bankid & (uint8_t)0xE) * 2;
        sfc_load_prgrom_8k(famicom, 0, bank + 0);
        sfc_load_prgrom_8k(famicom, 1, bank + 1);
        sfc_load_prgrom_8k(famicom, 2, bank + 2);
        sfc_load_prgrom_8k(famicom, 3, bank + 3);
#ifndef NDEBUG
        //printf("switch: PRG-ROM 32KB to:$%02X\n", bank / 2);
#endif
        break;
    case 2:
        // 固定低16KB到最后 切换高16KB
        bank = bankid * 2;
        sfc_load_prgrom_8k(famicom, 0, 0);
        sfc_load_prgrom_8k(famicom, 1, 1);
        sfc_load_prgrom_8k(famicom, 2, bank + 0);
        sfc_load_prgrom_8k(famicom, 3, bank + 1);
#ifndef NDEBUG
        //printf("switch: HI-PRG-ROM 16KB to:$%02X\n", bank / 2);
#endif
        break;
    case 3:
        // 固定高16KB到最后 切换低16KB
        bank = bankid * 2;
        last = famicom->rom_info.count_prgrom16kb * 2;
        sfc_load_prgrom_8k(famicom, 0, bank + 0);
        sfc_load_prgrom_8k(famicom, 1, bank + 1);
        sfc_load_prgrom_8k(famicom, 2, last - 2);
        sfc_load_prgrom_8k(famicom, 3, last - 1);
#ifndef NDEBUG
        //printf("switch: LO-PRG-ROM 16KB to:$%02X\n", bank / 2);
#endif
        break;
    }
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
        sfc_mapper_01_write_control(famicom, sfc_mapper(famicom)->shifter);
        break;
    case 1:
        // $A000-$BFFF CHR bank 0
        sfc_mapper_01_write_chrbank0(famicom);
        break;
    case 2:
        // $C000-$DFFF CHR bank 1
        sfc_mapper_01_write_chrbank1(famicom);
        break;
    case 3:
        // $E000-$FFFF PRG bank
        sfc_mapper_01_write_prgbank(famicom);
        break;
    }
}

/// <summary>
/// StepFC: MAPPER 001 - SxROM 重置
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
        sfc_mapper_01_write_control(famicom, mapper->control | (uint8_t)0x0C);
    }
    // D7 = 0 -> 写入D0到移位寄存器
    else {
        const uint8_t finished = mapper->shifter & 1;
        mapper->shifter >>= 1;
        mapper->shifter |= (value & 1) << 4;
        if (finished) {
            sfc_mapper_01_write_register(famicom, address);
            mapper->shifter = 0x10;
        }
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