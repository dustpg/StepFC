#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com

#include "sfc_famicom.h"


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