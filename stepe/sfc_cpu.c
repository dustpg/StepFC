#include "sfc_6502.h"
#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include <assert.h>
#include <string.h>


// BYTE -> HEX
extern inline void sfc_btoh(char o[], uint8_t b);
// get opcode ins-len
extern inline uint8_t sfc_get_inslen(uint8_t);

/// <summary>
/// StepFC: 读取PRG数据
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline uint8_t sfc_read_prgdata(uint16_t address, const sfc_famicom_t* famicom) {
    assert(((address & (uint16_t)0x8000) == (uint16_t)0x8000) || (address>>13) == 0 || (address >= 0x4100 && address < 0x4200));
    assert(!(address >= 0x0800 && address < 0x2000) && "OVERFLOW");
    const uint16_t prgaddr = address;
    return famicom->prg_banks[prgaddr >> 12][prgaddr & (uint16_t)0x0fff];
}

/// <summary>
/// StepFC: 指定地方反汇编
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
/// <param name="buf">The buf.</param>
void sfc_fc_disassembly(uint16_t address, const sfc_famicom_t* famicom, char buf[]) {
    enum {
        OFFSET_M = SFC_DISASSEMBLY_BUF_LEN2 - SFC_DISASSEMBLY_BUF_LEN,
        OFFSET = 8
    };
    static_assert(OFFSET < OFFSET_M, "LESS!");
    memset(buf, ' ', OFFSET);
    buf[0] = '$';
    sfc_btoh(buf + 1, (uint8_t)(address >> 8));
    sfc_btoh(buf + 3, (uint8_t)(address));

    sfc_6502_code_t code;
    code.data = 0;
    code.op = sfc_read_prgdata(address, famicom);
    // 获取指令长度
    switch (sfc_get_inslen(code.op))
    {
    case 3:
        code.a2 = sfc_read_prgdata(address + 2, famicom);
    case 2:
        code.a1 = sfc_read_prgdata(address + 1, famicom);
    }
    // 反汇编
    sfc_6502_disassembly(code, buf + OFFSET);
}

// StepFC: 读取CPU地址数据4020
extern inline uint8_t sfc_read_cpu_address4020(uint16_t, sfc_famicom_t*);
// StepFC: 写入CPU地址数据4020
extern inline void    sfc_write_cpu_address4020(uint16_t, uint8_t, sfc_famicom_t*);

/// <summary>
/// StepFC: 读取CPU地址数据
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
uint8_t sfc_read_cpu_address(uint16_t address, sfc_famicom_t* famicom) {
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
        return sfc_read_ppu_register_via_cpu(address, &famicom->ppu);
    case 2:
        // 高三位为2, [$4000, $6000): pAPU寄存器 扩展ROM区
        if (address < 0x4020)
            return sfc_read_cpu_address4020(address, famicom);
        //else assert(!"NOT IMPL");
        return 0;
    case 3:
        // 高三位为3, [$6000, $8000): 存档 SRAM区
        //printf("$%04X[$%02X]\n", address, famicom->save_memory[address & (uint16_t)0x1fff]);
        return famicom->save_memory[address & (uint16_t)0x1fff];
    case 4: case 5: case 6: case 7:
        // 高一位为1, [$8000, $10000) 程序PRG-ROM区
        return famicom->prg_banks[address >> 12][address & (uint16_t)0x0fff];
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
        sfc_write_ppu_register_via_cpu(address, data, &famicom->ppu);
        return;
    case 2:
        // 高三位为2, [$4000, $6000): pAPU寄存器 扩展ROM区
        // 前0x20字节为APU, I / O寄存器
        if (address < 0x4020) sfc_write_cpu_address4020(address, data, famicom);
        else famicom->mapper.write_low(famicom, address, data);
        return;
    case 3:
        // 高三位为3, [$6000, $8000): 存档 SRAM区
        //printf("$%04X = $%02X\n", address, data);
        famicom->save_memory[address & (uint16_t)0x1fff] = data;
        return;
    case 4: case 5: case 6: case 7:
        // 高一位为1, [$8000, $10000) 程序PRG-ROM区
        famicom->mapper.write_high(famicom, address, data);
        //assert(!"WARNING: PRG-ROM");
        //famicom->prg_banks[address >> 13][address & (uint16_t)0x1fff] = data;
        return;
    }
    assert(!"invalid address");
}

/// <summary>
/// StepFC: NSF用长跳转
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
void sfc_cpu_long_jmp(uint16_t address, sfc_famicom_t* famicom) {
    famicom->registers.program_counter = 0x4100;
    const uint32_t loop_point = 0x4103;
    // JSR
    famicom->bus_memory[0x100] = 0x20;
    famicom->bus_memory[0x101] = (uint8_t)(address & 0xff);
    famicom->bus_memory[0x102] = (uint8_t)(address >> 8);
    // JMP $4103
    famicom->bus_memory[0x103] = 0x4c;
    famicom->bus_memory[0x104] = (uint8_t)(loop_point & 0xff);
    famicom->bus_memory[0x105] = (uint8_t)(loop_point >> 8);
}
