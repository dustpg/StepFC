#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include "sfc_mapper_helper.h"
#include <assert.h>
#include <string.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

// ------------------------------- MAPPER 003 - CNROM


/// <summary>
/// StepFC: MAPPER 000 - NROM 重置
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern sfc_ecode sfc_mapper_00_reset(sfc_famicom_t* famicom);

/// <summary>
/// SFCs the mapper 03 write high.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="address">The address.</param>
/// <param name="value">The value.</param>
static void sfc_mapper_03_write_high(sfc_famicom_t* famicom, uint16_t address, uint8_t value) {
    const uint32_t count_prgrom8kb = famicom->rom_info.size_prgrom >> 13;
    const int bank = (value % count_prgrom8kb) * 8;
    for (int i = 0; i != 8; ++i)
        sfc_load_chrrom_1k(famicom, i, bank +i );
}

/// <summary>
/// SFCs the load mapper 03.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline sfc_ecode sfc_load_mapper_03(sfc_famicom_t* famicom) {
    // 初始化回调接口
    // 重置可以直接使用Mapper000的
    famicom->mapper.reset = sfc_mapper_00_reset;
    famicom->mapper.write_high = sfc_mapper_03_write_high;
    return SFC_ERROR_OK;
}