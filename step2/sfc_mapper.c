#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include <assert.h>
#include <string.h>

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

// mapper000 - NROM
static inline sfc_ecode sfc_load_mapper_00(sfc_famicom_t* famicom);


#define SFC_CASE_LOAD_MAPPER(id) case 0x##id: return sfc_load_mapper_##id(famicom);

/// <summary>
/// StepFC: 加载Mapper
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="id">The identifier.</param>
/// <returns></returns>
extern sfc_ecode sfc_load_mapper(sfc_famicom_t* famicom, uint8_t id) {
    switch (id)
    {
    SFC_CASE_LOAD_MAPPER(00);
    }
    return SFC_ERROR_MAPPER_NOT_FOUND;
}



// ------------------------------- MAPPER 000 - NROM

/// <summary>
/// StepFC: MAPPER 000 - NROM 重置
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static sfc_ecode sfc_mapper_00_reset(sfc_famicom_t* famicom) {
    assert(famicom->rom_info.count_prgrom16kb && "bad count");
    assert(famicom->rom_info.count_prgrom16kb <= 2 && "bad count");
    // 16KB -> 载入 $8000-$BFFF, $C000-$FFFF 为镜像
    const int id2 = famicom->rom_info.count_prgrom16kb & 2;
    // 32KB -> 载入 $8000-$FFFF
    sfc_load_prgrom_8k(famicom, 0, 0);
    sfc_load_prgrom_8k(famicom, 1, 1);
    sfc_load_prgrom_8k(famicom, 2, id2 + 0);
    sfc_load_prgrom_8k(famicom, 3, id2 + 1);
    return SFC_ERROR_OK;
}

/// <summary>
/// SFCs the load mapper 00.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
sfc_ecode sfc_load_mapper_00(sfc_famicom_t* famicom) {
    famicom->mapper.reset = sfc_mapper_00_reset;
    return SFC_ERROR_OK;
}