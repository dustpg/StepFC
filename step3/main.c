#include "sfc_famicom.h"
#include "sfc_cpu.h"
#include <stdio.h>


void sfc_log_exec(void* arg, sfc_famicom_t* famicom) {
    static int line = 0;
    line++;
    char buf[SFC_DISASSEMBLY_BUF_LEN2];
    const uint16_t pc = famicom->registers.program_counter;
    sfc_fc_disassembly(pc, famicom, buf);
    printf(
        "%4d - %s   A:%02X X:%02X Y:%02X P:%02X SP:%02X\n",
        line, buf,
        (int)famicom->registers.accumulator,
        (int)famicom->registers.x_index,
        (int)famicom->registers.y_index,
        (int)famicom->registers.status,
        (int)famicom->registers.stack_pointer
    );
}

/// <summary>
/// 应用程序入口
/// </summary>
/// <returns></returns>
int main() {
    sfc_interface_t interfaces = { NULL };
    interfaces.before_execute = sfc_log_exec;

    sfc_famicom_t famicom;
    sfc_famicom_init(&famicom, NULL, &interfaces);
    printf(
        "ROM: PRG-RPM: %d x 16kb   CHR-ROM %d x 8kb   Mapper: %03d\n", 
        (int)famicom.rom_info.count_prgrom16kb,
        (int)famicom.rom_info.count_chrrom_8kb,
        (int)famicom.rom_info.mapper_number
    );
    do {
        for(int i = 0; i != 1000; ++i) 
            sfc_cpu_execute_one(&famicom);
    } while (getchar() != -1);

    sfc_famicom_uninit(&famicom);
    return 0;
}