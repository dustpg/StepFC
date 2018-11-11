#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include "sfc_mapper_helper.h"
#include <assert.h>
#include <string.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

// ------------------------------- MAPPER 002 - UxROM


/// <summary>
/// StepFC: MAPPER 002 - UxROM 重置
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static sfc_ecode sfc_mapper_02_reset(sfc_famicom_t* famicom) {
    const uint32_t count_prgrom16kb = famicom->rom_info.size_prgrom >> 14;
    // 你用UxROM居然没有32KB PRG-ROM?
    assert(count_prgrom16kb > 2 && "bad count");
    // PRG-ROM
    sfc_load_prgrom_8k(famicom, 0, 0);
    sfc_load_prgrom_8k(famicom, 1, 1);
    const int last = count_prgrom16kb * 2;
    sfc_load_prgrom_8k(famicom, 2, last-2);
    sfc_load_prgrom_8k(famicom, 3, last-1);
    // CHR-ROM 没有 是RAM
    for (int i = 0; i != 8; ++i)
        sfc_load_chrrom_1k(famicom, i, i);
    return SFC_ERROR_OK;
}


/// <summary>
/// SFCs the mapper 02 write high.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="address">The address.</param>
/// <param name="value">The value.</param>
static void sfc_mapper_02_write_high(sfc_famicom_t* famicom, uint16_t address, uint8_t value) {
    const uint32_t count_prgrom16kb = famicom->rom_info.size_prgrom >> 14;
    const int bank = (value % count_prgrom16kb) * 2;
    sfc_load_prgrom_8k(famicom, 0, bank + 0);
    sfc_load_prgrom_8k(famicom, 1, bank + 1);
}

/// <summary>
/// SFCs the load mapper 01.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline sfc_ecode sfc_load_mapper_02(sfc_famicom_t* famicom) {
    // 初始化回调接口
    famicom->mapper.reset = sfc_mapper_02_reset;
    famicom->mapper.write_high = sfc_mapper_02_write_high;
    return SFC_ERROR_OK;
}