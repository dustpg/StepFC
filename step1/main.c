#include "sfc_famicom.h"
#include "sfc_cpu.h"
#include <stdio.h>

/// <summary>
/// 应用程序入口
/// </summary>
/// <returns></returns>
int main() {
    sfc_famicom_t famicom;
    sfc_famicom_init(&famicom, NULL, NULL);
    printf(
        "ROM: PRG-RPM: %d x 16kb   CHR-ROM %d x 8kb   Mapper: %03d\n", 
        (int)famicom.rom_info.count_prgrom16kb,
        (int)famicom.rom_info.count_chrrom_8kb,
        (int)famicom.rom_info.mapper_number
    );
    // V0 - NMI
    uint16_t v0 = sfc_read_cpu_address(SFC_VERCTOR_NMI + 0, &famicom);
    v0 |= sfc_read_cpu_address(SFC_VERCTOR_NMI + 1, &famicom) << 8;
    // V1 - RESET
    uint16_t v1 = sfc_read_cpu_address(SFC_VERCTOR_RESET + 0, &famicom);
    v1 |= sfc_read_cpu_address(SFC_VERCTOR_RESET + 1, &famicom) << 8;
    // V2 - IRQ/BRK
    uint16_t v2 = sfc_read_cpu_address(SFC_VERCTOR_IRQBRK + 0, &famicom);
    v2 |= sfc_read_cpu_address(SFC_VERCTOR_IRQBRK + 1, &famicom) << 8;

    printf(
        "ROM: NMI: $%04X  RESET: $%04X  IRQ/BRK: $%04X\n",
        (int)v0, (int)v1, (int)v2
    );
    getchar();
    sfc_famicom_uninit(&famicom);
    return 0;
}