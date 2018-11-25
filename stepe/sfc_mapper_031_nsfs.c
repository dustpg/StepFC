#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include "sfc_mapper_helper.h"
#include <assert.h>
#include <string.h>

// Mapper 031 - NSF 子集
enum {
    MAPPER_031_BANK_WINDOW = 4 * 1024,
};


/// <summary>
/// SFCs the NSF switch.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="addr">The addr.</param>
/// <param name="data">The data.</param>
static void sfc_nsf_switch(sfc_famicom_t* famicom, uint16_t addr, uint8_t data) {
    // 0101 .... .... .AAA  --    PPPP PPPP
    const uint16_t count = (famicom->rom_info.size_prgrom + 0x0fff)>>12;
    const uint16_t src = data;
    sfc_load_prgrom_4k(famicom, addr & 0x07, src%count);
}
 
// 初始化FDS
extern void sfc_fds_init(sfc_famicom_t* famicom);


/// <summary>
/// StepFC: MAPPER 031 重置
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern sfc_ecode sfc_mapper_1F_reset(sfc_famicom_t* famicom) {
    const uint32_t size_prgrom = famicom->rom_info.size_prgrom;
    assert(size_prgrom && "bad size");
    // NSF的场合
    if (famicom->rom_info.song_count) {
        uint8_t* const bs_init = famicom->rom_info.bankswitch_init;
        uint64_t bankswi; memcpy(&bankswi, bs_init, sizeof(bankswi));
        // 计算起点
        uint16_t i = famicom->rom_info.load_addr >> 12;
        if (i < 8) {
            // 目前不支持载入FDS
            assert(!"UNSUPPORTED");
            return SFC_ERROR_UNSUPPORTED;
        }
        i &= 7;
        // 使用切换
        if (bankswi) {
            for (; i != 8; ++i)
                sfc_nsf_switch(famicom, i, bs_init[i]);
        }
        // 直接载入
        else {
            // 终点
            uint16_t count = ((size_prgrom + 0xfff) >> 12) + i;
            if (count > 8) count = 8;
            // 处理
            for (uint8_t data = 0; i != count; ++i, ++data)
                sfc_nsf_switch(famicom, i, data);
        }
    }
    // Mapper-031
    else {
        // PRG-ROM
        const int last = famicom->rom_info.size_prgrom >> 12;
        sfc_load_prgrom_4k(famicom, 7, last - 1);
    }
    // CHR-ROM
    for (int i = 0; i != 8; ++i)
        sfc_load_chrrom_1k(famicom, i, i);
    // FDS
    if (famicom->rom_info.extra_sound & SFC_NSF_EX_FDS1) {
        sfc_fds_init(famicom);
    }
    return SFC_ERROR_OK;
}



// VRC6
extern void sfc_mapper_18_write_high(sfc_famicom_t*, uint16_t, uint8_t);
// VRC7
extern void sfc_mapper_55_write_high(sfc_famicom_t*, uint16_t, uint8_t);
// FDS1
extern void sfc_mapper_14_write_low(sfc_famicom_t*, uint16_t, uint8_t);


#include <stdbool.h>


/// <summary>
/// Mapper - 031 - 写入低地址($4020, $6000)
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="addr">The addr.</param>
/// <param name="data">The data.</param>
static void sfc_mapper_1F_write_low(sfc_famicom_t*famicom, uint16_t addr, uint8_t data) {
    // PRG bank select $5000-$5FFF
    if (addr >= 0x5000) {
        sfc_nsf_switch(famicom, addr, data);
    }
    // FDS
    else if (famicom->rom_info.extra_sound & SFC_NSF_EX_FDS1) {
        sfc_mapper_14_write_low(famicom, addr, data);
    }
}

/// <summary>
/// SFCs the mapper 1 f write high.
/// </summary>
/// <param name="f">The f.</param>
/// <param name="d">The d.</param>
/// <param name="v">The v.</param>
static void sfc_mapper_1F_write_high(sfc_famicom_t*f, uint16_t d, uint8_t v) {
    //assert(!"CANNOT WRITE PRG-ROM");
    // VRC6
    if (f->rom_info.extra_sound & SFC_NSF_EX_VCR6) {
        // $9000 - $9003(if VRC6 is enabled)
        // $A000 - $A002(if VRC6 is enabled)
        // $B000 - $B002(if VRC6 is enabled)
        const bool r0 = d >= 0x9000 && d <= 0x9003;
        const bool r1 = d >= 0xA000 && d <= 0xA002;
        const bool r2 = d >= 0xB000 && d <= 0xB002;
        if (r0 | r1 | r2)
            sfc_mapper_18_write_high(f, d, v);
    }
    // VRC7
    if (f->rom_info.extra_sound & SFC_NSF_EX_VCR7) {
        // $9010 (if VRC7 is enabled)
        // $9030 (if VRC7 is enabled)
        const bool r0 = d == 0x9010;
        const bool r1 = d == 0x9030;
        if (r0 | r1)
            sfc_mapper_55_write_high(f, d, v);
    }
}


// 默认写入
extern void sfc_mapper_wrts_defualt(const sfc_famicom_t* famicom);
// 默认读取
extern void sfc_mapper_rrfs_defualt(sfc_famicom_t* famicom);

/// <summary>
/// NSFs: 写入RAM到流
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_1F_write_ram(const sfc_famicom_t* famicom) {
    // NSF场合
    if (famicom->rom_info.song_count) {
        // 保存BUS
        famicom->interfaces.sl_write_stream(
            famicom->argument,
            famicom->bus_memory,
            sizeof(famicom->bus_memory)
        );
    }
    // CHR-RAM -> 流
    sfc_mapper_wrts_defualt(famicom);
}

/// <summary>
/// NSFs: 从流读取至RAM
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_1F_read_ram(sfc_famicom_t* famicom) {
    // NSF场合
    if (famicom->rom_info.song_count) {
        // 读取BUS
        famicom->interfaces.sl_read_stream(
            famicom->argument,
            famicom->bus_memory,
            sizeof(famicom->bus_memory)
        );
    }
    // 流 -> CHR-RAM
    sfc_mapper_rrfs_defualt(famicom);
}

/// <summary>
/// SFCs the load mapper 1F
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline sfc_ecode sfc_load_mapper_1F(sfc_famicom_t* famicom) {
    famicom->mapper.reset = sfc_mapper_1F_reset;
    famicom->mapper.write_low = sfc_mapper_1F_write_low;
    famicom->mapper.write_high = sfc_mapper_1F_write_high;
    famicom->mapper.read_ram_from_stream = sfc_mapper_1F_read_ram;
    famicom->mapper.write_ram_to_stream = sfc_mapper_1F_write_ram;
    return SFC_ERROR_OK;
}


/// <summary>
/// SFCs the famicom NSF initialize.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="index">The index.</param>
/// <param name="pal">The pal.</param>
void sfc_famicom_nsf_init(sfc_famicom_t* famicom, uint8_t index, uint8_t pal) {
    assert(index < famicom->rom_info.song_count && "out of range");
    // 清空主内存与工作内存
    memset(famicom->main_memory, 0, sizeof(famicom->main_memory));
    memset(famicom->save_memory, 0, sizeof(famicom->save_memory));
    // $4000-$4013写入$00, $4015先后写入$00,$0F
    for (uint16_t addr = 0x4000; addr != 0x4014; ++addr)
        sfc_write_cpu_address(addr, 0, famicom);
    sfc_write_cpu_address(0x4015, 0x00, famicom);
    sfc_write_cpu_address(0x4015, 0x0f, famicom);
    // 4步模式
    sfc_write_cpu_address(0x4017, 0x40, famicom);
    // 累加器A为索引
    famicom->registers.accumulator = index;
    // 变址器X为模式
    famicom->registers.x_index = pal;
    // PLAY时钟周期
    famicom->nsf.play_clock = 0xffffffff;
    // 调用INIT程序
    const uint16_t address = famicom->rom_info.init_addr;
    famicom->registers.program_counter = 0x4106;
    const uint32_t loop_point = 0x410A;
    // JSR
    famicom->bus_memory[0x106] = 0x20;
    famicom->bus_memory[0x107] = (uint8_t)(address & 0xff);
    famicom->bus_memory[0x108] = (uint8_t)(address >> 8);
    // (HACK) HK2
    famicom->bus_memory[0x109] = 0x02;
    // JMP $410A
    famicom->bus_memory[0x10A] = 0x4c;
    famicom->bus_memory[0x10B] = (uint8_t)(loop_point & 0xff);
    famicom->bus_memory[0x10C] = (uint8_t)(loop_point >> 8);
}

/// <summary>
/// SFCs the famicom NSF play.
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_famicom_nsf_play(sfc_famicom_t* famicom) {
    const uint16_t address = famicom->rom_info.play_addr;
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

