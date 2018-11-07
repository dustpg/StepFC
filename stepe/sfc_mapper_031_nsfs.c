#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include "sfc_mapper_helper.h"
#include <assert.h>
#include <string.h>

// Mapper 031 - NSF 子集
enum {
    MAPPER_031_BANK_WINDOW = 4 * 1024,
};

/// <summary>
/// StepFC: MAPPER 031 重置
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern sfc_ecode sfc_mapper_1F_reset(sfc_famicom_t* famicom) {
    assert(famicom->rom_info.count_prgrom16kb && "bad count");
    // PRG-ROM
    const int last = famicom->rom_info.count_prgrom16kb * 4;
    sfc_load_prgrom_4k(famicom, 7, last - 1);
    // CHR-ROM
    for (int i = 0; i != 8; ++i)
        sfc_load_chrrom_1k(famicom, i, i);
    return SFC_ERROR_OK;
}


/// <summary>
/// Mapper - 031 - 写入低地址($4020, $6000)
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="addr">The addr.</param>
/// <param name="data">The data.</param>
static void sfc_mapper_1F_write_low(sfc_famicom_t*famicom, uint16_t addr, uint8_t data) {
    // PRG bank select $5000-$5FFF
    if (addr >= 0x5000) {
        // 0101 .... .... .AAA  --    PPPP PPPP
        const uint16_t count = famicom->rom_info.count_prgrom16kb * 4;
        const uint16_t src = data;
        sfc_load_prgrom_4k(famicom, addr & 0x07, src % count);
    }
    // ???
    else {
        assert(!"NOT IMPL");
    }
}

/// <summary>
/// SFCs the load mapper 1F
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline sfc_ecode sfc_load_mapper_1F(sfc_famicom_t* famicom) {
    famicom->mapper.reset = sfc_mapper_1F_reset;
    famicom->mapper.write_low = sfc_mapper_1F_write_low;
    return SFC_ERROR_OK;
}