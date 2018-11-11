#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include "sfc_mapper_helper.h"
#include <assert.h>
#include <string.h>

// Mapper 074 - 魔改MMC3 - $08 $09是CHR-RAM

enum {
    MAPPER_074_BANKS_UNIT = 1 * 1024,
    MAPPER_074_CHRRAM_POS = 8,
    MAPPER_074_CHRRAM_LEN = 2,
};

/// <summary>
/// SFCs the mapper WRTS.
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_wrts_074(const sfc_famicom_t* famicom) {
    // PRG-RAM 不考虑

    // $08 $09是CHR-RAM

    assert(famicom->rom_info.size_chrrom >= 16*1024);
    famicom->interfaces.sl_write_stream(
        famicom->argument,
        famicom->rom_info.data_chrrom 
        + MAPPER_074_CHRRAM_POS * MAPPER_074_BANKS_UNIT,
        MAPPER_074_CHRRAM_LEN * MAPPER_074_BANKS_UNIT
    );
}

/// <summary>
/// SFCs the mapper WRTS.
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_rrfs_074(sfc_famicom_t* famicom) {
    // PRG-RAM 不考虑

    // $08 $09是CHR-RAM
    assert(famicom->rom_info.size_chrrom >= 16 * 1024);
    famicom->interfaces.sl_read_stream(
        famicom->argument,
        famicom->rom_info.data_chrrom
        + MAPPER_074_CHRRAM_POS * MAPPER_074_BANKS_UNIT,
        MAPPER_074_CHRRAM_LEN * MAPPER_074_BANKS_UNIT
    );
}


// Mapper 004
extern inline sfc_ecode sfc_load_mapper_04(sfc_famicom_t* famicom);

/// <summary>
/// SFCs the load mapper 4A
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline sfc_ecode sfc_load_mapper_4A(sfc_famicom_t* famicom) {
    sfc_load_mapper_04(famicom);
    famicom->mapper.write_ram_to_stream = sfc_mapper_wrts_074;
    famicom->mapper.read_ram_from_stream = sfc_mapper_rrfs_074;
    return SFC_ERROR_OK;
}