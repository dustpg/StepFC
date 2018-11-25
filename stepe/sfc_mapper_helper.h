#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com

#include "sfc_famicom.h"

/// <summary>
/// 实用函数-StepFC: 载入4k PRG-ROM
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="des">The DES.</param>
/// <param name="src">The source.</param>
static inline void sfc_load_prgrom_4k(
    sfc_famicom_t* famicom, int des, int src) {
    famicom->prg_banks[8 + des] = famicom->rom_info.data_prgrom + 4 * 1024 * src;
}

/// <summary>
/// 实用函数-StepFC: 载入8k PRG-ROM
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="des">The DES.</param>
/// <param name="src">The source.</param>
static inline void sfc_load_prgrom_8k(
    sfc_famicom_t* famicom, int des, int src) {
    sfc_load_prgrom_4k(famicom, des * 2 + 0, src * 2 + 0);
    sfc_load_prgrom_4k(famicom, des * 2 + 1, src * 2 + 1);
}

/// <summary>
/// 实用函数-StepFC: 载入4k PRG-ROM - 原型
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="des">The DES.</param>
/// <param name="src">The source.</param>
static inline void sfc_load_prgrom_4kpt(
    sfc_famicom_t* famicom, int des, int src) {
    famicom->prg_banks[des] = famicom->rom_info.data_prgrom + 4 * 1024 * src;
}

/// <summary>
/// 实用函数-StepFC: 载入8k PRG-ROM - 原型
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="des">The DES.</param>
/// <param name="src">The source.</param>
static inline void sfc_load_prgrom_8kpt(
    sfc_famicom_t* famicom, int des, int src) {
    sfc_load_prgrom_4kpt(famicom, des * 2 + 0, src * 2 + 0);
    sfc_load_prgrom_4kpt(famicom, des * 2 + 1, src * 2 + 1);
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



/// <summary>
/// 实用函数-StepFC: 载入4kbx2 PRG-RAM
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="des">The DES.</param>
/// <param name="ram">The ram.</param>
static inline void sfc_load_prgram_8kb(sfc_famicom_t* famicom, int des, uint8_t* ram) {
    famicom->prg_banks[des * 2 + 0] = ram + 4 * 1024 * 0;
    famicom->prg_banks[des * 2 + 1] = ram + 4 * 1024 * 1;
}