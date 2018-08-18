#include "sfc_famicom.h"
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
    getchar();
    sfc_famicom_uninit(&famicom);
    return 0;
}