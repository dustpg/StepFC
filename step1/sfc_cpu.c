#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include <assert.h>
#include <string.h>

/// <summary>
/// StepFC: 读取CPU地址数据
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
uint8_t sfc_read_cpu_address(uint16_t address, const sfc_famicom_t* famicom) {
    /* 
    CPU 地址空间
    +---------+-------+-------+-----------------------+
    | 地址    | 大小  | 标记  |         描述          |
    +---------+-------+-------+-----------------------+
    | $0000   | $800  |       | RAM                   |
    | $0800   | $800  | M     | RAM                   |
    | $1000   | $800  | M     | RAM                   |
    | $1800   | $800  | M     | RAM                   |
    | $2000   | 8     |       | Registers             |
    | $2008   | $1FF8 | R     | Registers             |
    | $4000   | $20   |       | Registers             |
    | $4020   | $1FDF |       | Expansion ROM         |
    | $6000   | $2000 |       | SRAM                  |
    | $8000   | $4000 |       | PRG-ROM               |
    | $C000   | $4000 |       | PRG-ROM               |
    +---------+-------+-------+-----------------------+
    标记图例: M = $0000的镜像
              R = $2000-2008 每 8 bytes 的镜像
            (e.g. $2008=$2000, $2018=$2000, etc.)
    */
    switch (address >> 13)
    {
    case 0:
        // 高三位为0: [$0000, $2000): 系统主内存, 4次镜像
        return famicom->main_memory[address & (uint16_t)0x07ff];
    case 1:
        // 高三位为1, [$2000, $4000): PPU寄存器, 8字节步进镜像
        assert(!"NOT IMPL");
        return 0;
    case 2:
        // 高三位为2, [$4000, $6000): pAPU寄存器 扩展ROM区
        assert(!"NOT IMPL");
        return 0;
    case 3:
        // 高三位为3, [$6000, $8000): 存档 SRAM区
        return famicom->save_memory[address & (uint16_t)0x1fff];
    case 4: case 5: case 6: case 7:
        // 高一位为1, [$8000, $10000) 程序PRG-ROM区
        return famicom->prg_banks[address >> 13][address & (uint16_t)0x1fff];
    }
    assert(!"invalid address");
    return 0;
}


/// <summary>
/// SFCs the write cpu address.
/// </summary>
/// <param name="address">The address.</param>
/// <param name="data">The data.</param>
/// <param name="famicom">The famicom.</param>
void sfc_write_cpu_address(uint16_t address, uint8_t data, sfc_famicom_t* famicom) {
    /* 
    CPU 地址空间
    +---------+-------+-------+-----------------------+
    | 地址    | 大小  | 标记  |         描述          |
    +---------+-------+-------+-----------------------+
    | $0000   | $800  |       | RAM                   |
    | $0800   | $800  | M     | RAM                   |
    | $1000   | $800  | M     | RAM                   |
    | $1800   | $800  | M     | RAM                   |
    | $2000   | 8     |       | Registers             |
    | $2008   | $1FF8 | R     | Registers             |
    | $4000   | $20   |       | Registers             |
    | $4020   | $1FDF |       | Expansion ROM         |
    | $6000   | $2000 |       | SRAM                  |
    | $8000   | $4000 |       | PRG-ROM               |
    | $C000   | $4000 |       | PRG-ROM               |
    +---------+-------+-------+-----------------------+
    标记图例: M = $0000的镜像
              R = $2000-2008 每 8 bytes 的镜像
            (e.g. $2008=$2000, $2018=$2000, etc.)
    */
    switch (address >> 13)
    {
    case 0:
        // 高三位为0: [$0000, $2000): 系统主内存, 4次镜像
        famicom->main_memory[address & (uint16_t)0x07ff] = data;
        return;
    case 1:
        // 高三位为1, [$2000, $4000): PPU寄存器, 8字节步进镜像
        assert(!"NOT IMPL");
        return;
    case 2:
        // 高三位为2, [$4000, $6000): pAPU寄存器 扩展ROM区
        assert(!"NOT IMPL");
        return;
    case 3:
        // 高三位为3, [$6000, $8000): 存档 SRAM区
        famicom->save_memory[address & (uint16_t)0x1fff] = data;
        return;
    case 4: case 5: case 6: case 7:
        // 高一位为1, [$8000, $10000) 程序PRG-ROM区
        assert(!"WARNING: PRG-ROM");
        famicom->prg_banks[address >> 13][address & (uint16_t)0x1fff] = data;
        return;
    }
    assert(!"invalid address");
}
