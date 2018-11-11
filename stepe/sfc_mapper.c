#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include "sfc_mapper_helper.h"
#include <assert.h>
#include <string.h>


// mapper000 - NROM
static inline sfc_ecode sfc_load_mapper_00(sfc_famicom_t* famicom);
// mapper001 - SxROM
extern inline sfc_ecode sfc_load_mapper_01(sfc_famicom_t* famicom);
// mapper002 - UxROM
extern inline sfc_ecode sfc_load_mapper_02(sfc_famicom_t* famicom);
// mapper003 - CNROM
extern inline sfc_ecode sfc_load_mapper_03(sfc_famicom_t* famicom);
// mapper004 - TxROM
extern inline sfc_ecode sfc_load_mapper_04(sfc_famicom_t* famicom);
// mapper031 - NSF 子集
extern inline sfc_ecode sfc_load_mapper_1F(sfc_famicom_t* famicom);
// mapper074 - MMC3 魔改
extern inline sfc_ecode sfc_load_mapper_4A(sfc_famicom_t* famicom);

#define SFC_CASE_LOAD_MAPPER(id) case 0x##id: return sfc_load_mapper_##id(famicom);


/// <summary>
/// SFCs the mapper hsync defualt.
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_hsync_defualt(sfc_famicom_t* famicom) {
}

/// <summary>
/// SFCs the mapper WRTS.
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_wrts_defualt(const sfc_famicom_t* famicom) {
    // PRG-RAM 不考虑

    // 没有CHR-ROM则表明全是CHR-RAM
    if (!famicom->rom_info.size_chrrom) {
        famicom->interfaces.sl_write_stream(
            famicom->argument,
            famicom->rom_info.data_chrrom,
            8 * 1024
        );
    }
}

/// <summary>
/// SFCs the mapper WRTS.
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_rrfs_defualt(sfc_famicom_t* famicom) {
    // PRG-RAM 不考虑

    // 没有CHR-ROM则表明全是CHR-RAM
    if (!famicom->rom_info.size_chrrom) {
        famicom->interfaces.sl_read_stream(
            famicom->argument,
            famicom->rom_info.data_chrrom,
            8 * 1024
        );
    }
}

/// <summary>
/// SFCs the mapper 00 write high.
/// </summary>
/// <param name="f">The f.</param>
/// <param name="d">The d.</param>
/// <param name="v">The v.</param>
static void sfc_mapper_00_write_high(sfc_famicom_t*f, uint16_t d, uint8_t v) {
    assert(!"CANNOT WRITE PRG-ROM");
}



/// <summary>
/// StepFC: 加载Mapper
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="id">The identifier.</param>
/// <returns></returns>
extern sfc_ecode sfc_load_mapper(sfc_famicom_t* famicom, uint8_t id) {
    memset(&famicom->mapper, 0, sizeof(famicom->mapper));
    // 载入默认接口
    famicom->mapper.write_low = sfc_mapper_00_write_high;
    famicom->mapper.write_high = sfc_mapper_00_write_high;
    famicom->mapper.hsync = sfc_mapper_hsync_defualt;
    famicom->mapper.write_ram_to_stream = sfc_mapper_wrts_defualt;
    famicom->mapper.read_ram_from_stream = sfc_mapper_rrfs_defualt;
    switch (id)
    {
        SFC_CASE_LOAD_MAPPER(00);
        SFC_CASE_LOAD_MAPPER(01);
        SFC_CASE_LOAD_MAPPER(02);
        SFC_CASE_LOAD_MAPPER(03);
        SFC_CASE_LOAD_MAPPER(04);
        SFC_CASE_LOAD_MAPPER(1F);
        SFC_CASE_LOAD_MAPPER(4A);
    }
    assert(!"NO MAPPER");
    return SFC_ERROR_MAPPER_NOT_FOUND;
}



// ------------------------------- MAPPER 000 - NROM


/// <summary>
/// StepFC: MAPPER 000 - NROM 重置
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern sfc_ecode sfc_mapper_00_reset(sfc_famicom_t* famicom) {
    const uint32_t count_prgrom16kb = famicom->rom_info.size_prgrom >> 14;
    assert(count_prgrom16kb && "bad count");
    assert(count_prgrom16kb <= 2 && "bad count");
    // PRG-ROM

    // 16KB -> 载入 $8000-$BFFF, $C000-$FFFF 为镜像
    const int id2 = count_prgrom16kb & 2;
    // 32KB -> 载入 $8000-$FFFF
    sfc_load_prgrom_8k(famicom, 0, 0);
    sfc_load_prgrom_8k(famicom, 1, 1);
    sfc_load_prgrom_8k(famicom, 2, id2 + 0);
    sfc_load_prgrom_8k(famicom, 3, id2 + 1);

    // CHR-ROM
    for (int i = 0; i != 8; ++i) 
        sfc_load_chrrom_1k(famicom, i, i);
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