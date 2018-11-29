#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include "sfc_mapper_helper.h"
#include <assert.h>
#include <string.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

// ------------------------------- MAPPER 005 - MMC5


/// <summary>
/// MMC5 ZeroNT: 原来8 KiB WRAM - 偏移2 KiB
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint8_t* sfc_mmc5_zero_nt(sfc_famicom_t* famicom) {
    return famicom->save_memory + 1024 * 2;
}

/// <summary>
/// MMC5 FillNT: 原来8 KiB WRAM - 偏移3 KiB
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint8_t* sfc_mmc5_fill_nt(sfc_famicom_t* famicom) {
    return famicom->save_memory + 1024 * 3;
}


/// <summary>
/// MMC5 5xxx: 原来8 KiB WRAM - 偏移4 KiB
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint8_t* sfc_mmc5_5xxx(sfc_famicom_t* famicom) {
    return famicom->save_memory + 1024 * 4;
}


/// <summary>
/// MMC5 ExRAM: 原来8 KiB WRAM - 偏移7 KiB(包含在5xxx)
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline uint8_t* sfc_mmc5_exram(sfc_famicom_t* famicom) {
    return famicom->save_memory + 1024 * 7;
}


/// <summary>
/// 
/// </summary>
typedef struct {
    // PRG 切换模式
    uint8_t     prg_mode;
    // CHR 切换模式
    uint8_t     chr_mode;
    // RAM写入保护
    uint8_t     ram_wp;
    // 扩展RAM模式
    uint8_t     exram_mode;
    // CHR-BANK高两位 储存在高2位
    uint8_t     chrbank_hi;
    // IRQ 扫描线
    uint8_t     irq_scanline;
    // IRQ 状态-Pending
    uint8_t     irq_status_p;
    // IRQ 状态-InFrame
    uint8_t     irq_status_f;
    // IRQ 使能
    uint8_t     irq_enable;
    // 扩展区 写入掩码-PPU
    uint8_t     exram_write_mask_ppu;
    // 扩展区 写入掩码-MMC5
    uint8_t     exram_write_mask_mmc5;
    // 未使用
    uint8_t     unused[25];
    // 16位乘法器
    uint8_t     factor[2];
    // 16位乘法器
    uint16_t    product;
    // CHR-BANK-8x8 模式
    uint16_t    chrbank_x8[8];
    // CHR-BANK-8x16模式
    uint16_t    chrbank_16[4];
} sfc_mapper05_t;


#ifndef NDEBUG
extern uint16_t dbg_scanline = 0;
#endif

enum {
    SFC_MMC5_RAM_WRITE = 6,
    SFC_MMC5_InFrame = 0x40,
    SFC_MMC5_IRQPending = 0x80,
};

/// <summary>
/// SFCs the mapper.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline sfc_mapper05_t* sfc_mapper(sfc_famicom_t* famicom) {
    return (sfc_mapper05_t*)famicom->mapper_buffer.mapper05;
}

#define MAPPER sfc_mapper05_t* const mapper = sfc_mapper(famicom);



/// <summary>
/// SFCs the MMC5 get 5130 hi.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline uint8_t sfc_mmc5_get_5130_hi(sfc_famicom_t* famicom) {
    MAPPER; return mapper->chrbank_hi;
}

// IRQ - 中断请求 - 确认
extern inline void sfc_operation_IRQ_acknowledge(sfc_famicom_t* famicom);
// 尝试触发
extern inline void sfc_operation_IRQ_try(sfc_famicom_t* famicom);

/// <summary>
/// SFCs the mapper 05 reset.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static sfc_ecode sfc_mapper_05_reset(sfc_famicom_t* famicom) {
    MAPPER;
    // 支持MMC5
    famicom->rom_info.extra_sound = SFC_NSF_EX_MMC5;
    // PRG模式3
    mapper->prg_mode = 3;
    // CHR模式3
    mapper->chr_mode = 0;
    // 16位乘法器
    mapper->factor[0] = 0xff;
    mapper->factor[1] = 0xff;
    mapper->product = 0xff * 0xff;
    // 载入最后的BANK
    const uint32_t count_prgrom8kb = famicom->rom_info.size_prgrom >> 13;
    //sfc_load_prgrom_8k(famicom, 0, count_prgrom8kb - 1);
    //sfc_load_prgrom_8k(famicom, 1, count_prgrom8kb - 1);
    //sfc_load_prgrom_8k(famicom, 2, count_prgrom8kb - 1);
    sfc_load_prgrom_8k(famicom, 3, count_prgrom8kb - 1);
    // 零数据名称表
    memset(sfc_mmc5_zero_nt(famicom), 0, 1024);
    // CHR-ROM
    for (int i = 0; i != 8; ++i)
        sfc_load_chrrom_1k(famicom, i, i);
    // 扩展5xxx区域
    famicom->prg_banks[5] = sfc_mmc5_5xxx(famicom);
    return SFC_ERROR_OK;
}

/// <summary>
/// SFCs the MMC5 update chrbank x8.
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mmc5_update_chrbank_x8(sfc_famicom_t* famicom) {
    MAPPER;
    switch (mapper->chr_mode)
    {
    case 0:
        // 8 KiB模式
        sfc_load_chrrom_1k(famicom, 0, 8 * mapper->chrbank_x8[7] + 0);
        sfc_load_chrrom_1k(famicom, 1, 8 * mapper->chrbank_x8[7] + 1);
        sfc_load_chrrom_1k(famicom, 2, 8 * mapper->chrbank_x8[7] + 2);
        sfc_load_chrrom_1k(famicom, 3, 8 * mapper->chrbank_x8[7] + 3);
        sfc_load_chrrom_1k(famicom, 4, 8 * mapper->chrbank_x8[7] + 5);
        sfc_load_chrrom_1k(famicom, 5, 8 * mapper->chrbank_x8[7] + 5);
        sfc_load_chrrom_1k(famicom, 6, 8 * mapper->chrbank_x8[7] + 6);
        sfc_load_chrrom_1k(famicom, 7, 8 * mapper->chrbank_x8[7] + 7);
        break;
    case 1:
        // 4 KiB模式
        sfc_load_chrrom_1k(famicom, 0, 4 * mapper->chrbank_x8[3] + 0);
        sfc_load_chrrom_1k(famicom, 1, 4 * mapper->chrbank_x8[3] + 1);
        sfc_load_chrrom_1k(famicom, 2, 4 * mapper->chrbank_x8[3] + 2);
        sfc_load_chrrom_1k(famicom, 3, 4 * mapper->chrbank_x8[3] + 3);
        sfc_load_chrrom_1k(famicom, 4, 4 * mapper->chrbank_x8[7] + 0);
        sfc_load_chrrom_1k(famicom, 5, 4 * mapper->chrbank_x8[7] + 1);
        sfc_load_chrrom_1k(famicom, 6, 4 * mapper->chrbank_x8[7] + 2);
        sfc_load_chrrom_1k(famicom, 7, 4 * mapper->chrbank_x8[7] + 3);
        break;
    case 2:
        // 2 KiB模式
        sfc_load_chrrom_1k(famicom, 0, 2 * mapper->chrbank_x8[1] + 0);
        sfc_load_chrrom_1k(famicom, 1, 2 * mapper->chrbank_x8[1] + 1);
        sfc_load_chrrom_1k(famicom, 2, 2 * mapper->chrbank_x8[3] + 0);
        sfc_load_chrrom_1k(famicom, 3, 2 * mapper->chrbank_x8[3] + 1);
        sfc_load_chrrom_1k(famicom, 4, 2 * mapper->chrbank_x8[5] + 0);
        sfc_load_chrrom_1k(famicom, 5, 2 * mapper->chrbank_x8[5] + 1);
        sfc_load_chrrom_1k(famicom, 6, 2 * mapper->chrbank_x8[7] + 0);
        sfc_load_chrrom_1k(famicom, 7, 2 * mapper->chrbank_x8[7] + 1);
        break;
    case 3:
        // 1 KiB模式
        sfc_load_chrrom_1k(famicom, 0, mapper->chrbank_x8[0]);
        sfc_load_chrrom_1k(famicom, 1, mapper->chrbank_x8[1]);
        sfc_load_chrrom_1k(famicom, 2, mapper->chrbank_x8[2]);
        sfc_load_chrrom_1k(famicom, 3, mapper->chrbank_x8[3]);
        sfc_load_chrrom_1k(famicom, 4, mapper->chrbank_x8[4]);
        sfc_load_chrrom_1k(famicom, 5, mapper->chrbank_x8[5]);
        sfc_load_chrrom_1k(famicom, 6, mapper->chrbank_x8[6]);
        sfc_load_chrrom_1k(famicom, 7, mapper->chrbank_x8[7]);
        break;
    }
}


/// <summary>
/// SFCs the MMC5 update chrbank 16.
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mmc5_update_chrbank_16(sfc_famicom_t* famicom) {
    MAPPER;
    switch (mapper->chr_mode)
    {
    case 0:
        // 8 KiB模式
        sfc_load_chrrom_1k(famicom, 0, 8 * mapper->chrbank_16[3] + 0);
        sfc_load_chrrom_1k(famicom, 1, 8 * mapper->chrbank_16[3] + 1);
        sfc_load_chrrom_1k(famicom, 2, 8 * mapper->chrbank_16[3] + 2);
        sfc_load_chrrom_1k(famicom, 3, 8 * mapper->chrbank_16[3] + 3);
        sfc_load_chrrom_1k(famicom, 4, 8 * mapper->chrbank_16[3] + 5);
        sfc_load_chrrom_1k(famicom, 5, 8 * mapper->chrbank_16[3] + 5);
        sfc_load_chrrom_1k(famicom, 6, 8 * mapper->chrbank_16[3] + 6);
        sfc_load_chrrom_1k(famicom, 7, 8 * mapper->chrbank_16[3] + 7);
        break;
    case 1:
        // 4 KiB模式
        sfc_load_chrrom_1k(famicom, 0, 4 * mapper->chrbank_16[3] + 0);
        sfc_load_chrrom_1k(famicom, 1, 4 * mapper->chrbank_16[3] + 1);
        sfc_load_chrrom_1k(famicom, 2, 4 * mapper->chrbank_16[3] + 2);
        sfc_load_chrrom_1k(famicom, 3, 4 * mapper->chrbank_16[3] + 3);
        sfc_load_chrrom_1k(famicom, 4, 4 * mapper->chrbank_16[3] + 0);
        sfc_load_chrrom_1k(famicom, 5, 4 * mapper->chrbank_16[3] + 1);
        sfc_load_chrrom_1k(famicom, 6, 4 * mapper->chrbank_16[3] + 2);
        sfc_load_chrrom_1k(famicom, 7, 4 * mapper->chrbank_16[3] + 3);
        break;
    case 2:
        // 2 KiB模式
        sfc_load_chrrom_1k(famicom, 0, 2 * mapper->chrbank_16[1] + 0);
        sfc_load_chrrom_1k(famicom, 1, 2 * mapper->chrbank_16[1] + 1);
        sfc_load_chrrom_1k(famicom, 2, 2 * mapper->chrbank_16[3] + 0);
        sfc_load_chrrom_1k(famicom, 3, 2 * mapper->chrbank_16[3] + 1);
        sfc_load_chrrom_1k(famicom, 4, 2 * mapper->chrbank_16[1] + 0);
        sfc_load_chrrom_1k(famicom, 5, 2 * mapper->chrbank_16[1] + 1);
        sfc_load_chrrom_1k(famicom, 6, 2 * mapper->chrbank_16[3] + 0);
        sfc_load_chrrom_1k(famicom, 7, 2 * mapper->chrbank_16[3] + 1);
        break;
    case 3:
        // 1 KiB模式
        sfc_load_chrrom_1k(famicom, 0, mapper->chrbank_16[0]);
        sfc_load_chrrom_1k(famicom, 1, mapper->chrbank_16[1]);
        sfc_load_chrrom_1k(famicom, 2, mapper->chrbank_16[2]);
        sfc_load_chrrom_1k(famicom, 3, mapper->chrbank_16[3]);
        sfc_load_chrrom_1k(famicom, 4, mapper->chrbank_16[0]);
        sfc_load_chrrom_1k(famicom, 5, mapper->chrbank_16[1]);
        sfc_load_chrrom_1k(famicom, 6, mapper->chrbank_16[2]);
        sfc_load_chrrom_1k(famicom, 7, mapper->chrbank_16[3]);
        break;
    }
}

#include <stdbool.h>

/// <summary>
/// SFCs the MMC5 ppu rendering.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="value">The value.</param>
/// <returns></returns>
static inline bool sfc_mmc5_ppu_rendering(sfc_famicom_t*famicom, uint32_t value) {
    return (famicom->ppu.data.mask & (SFC_PPU2001_Back | SFC_PPU2001_Sprite))
        ;
}

/// <summary>
/// SFCs the mapper 05 hsync.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="value">The value.</param>
static void sfc_mapper_05_hsync(sfc_famicom_t* famicom, uint16_t value) {
    MAPPER;
#ifndef NDEBUG
    //static uint8_t debug_sp16 = 0;
    //const uint8_t sp16 = famicom->ppu.data.ctrl & SFC_PPU2000_Sp8x16;
    //if (debug_sp16 != sp16) {
    //    debug_sp16 = sp16;
    //    printf("Sprite 8x16 Mode: O%s\n", sp16 ? "n" : "ff");
    //}
    dbg_scanline = value;
#endif
    mapper->irq_status_f = 0;
    mapper->exram_write_mask_ppu = 0x00;
    // 不可见扫描线
    if (value >= SFC_HEIGHT) return ;
    mapper->irq_status_f = SFC_MMC5_InFrame;
    mapper->exram_write_mask_ppu 
        = sfc_mmc5_ppu_rendering(famicom, value)
        ? 0xff : 0x00
        ;
    // 写入N就会在第N+1条扫描线前端触发
    // 这里是每条扫描线后触发水平同步所以就直接判断就行
    if (value) {
        if ((uint16_t)mapper->irq_scanline == value) {
            mapper->irq_status_p = SFC_MMC5_IRQPending;
            if (mapper->irq_enable) 
                sfc_operation_IRQ_try(famicom);
        }
    }
    // 第一行清除'Pending'
    else mapper->irq_status_p = 0;
    
}




/// <summary>
/// SFCs the MMC5 update nametable.
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mmc5_update_nametable(sfc_famicom_t* famicom, uint8_t value) {
    /*
     - 0 -自带的VRAM-前1kb
     - 1 -自带的VRAM-后1kb
     - 2 -内部扩展RAM, 不过$5104必须是模式00或者01, 否则全部读取到0
     - 3 -填充模式数据
    */
    MAPPER;
    uint8_t** base = famicom->ppu.banks + 8;
    for (int i = 0; i != 4; ++i) {
        uint8_t* ptr = NULL;
        switch (value & 3)
        {
        case 0:
            ptr = famicom->video_memory + 1024 * 0;
            break;
        case 1:
            ptr = famicom->video_memory + 1024 * 1;
            break;
        case 2:
            //ptr = (mapper->exram_mode & 2) ? sfc_mmc5_zero_nt(famicom) : sfc_mmc5_exram(famicom);
            ptr = sfc_mmc5_exram(famicom);
            break;
        case 3:
            ptr = sfc_mmc5_fill_nt(famicom);
            break;
        }
        value >>= 2;
        base[i] = ptr;
    }
    // 镜像
    famicom->ppu.banks[0xc] = famicom->ppu.banks[0x8];
    famicom->ppu.banks[0xd] = famicom->ppu.banks[0x9];
    famicom->ppu.banks[0xe] = famicom->ppu.banks[0xa];
    famicom->ppu.banks[0xf] = famicom->ppu.banks[0xb];
}

/// <summary>
/// SFCs the MMC5 get rambank.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="value">The value.</param>
/// <returns></returns>
static inline uint8_t* sfc_mmc5_get_rambank(sfc_famicom_t* famicom, uint8_t value) {
    assert((value & 6) != 6);
    const uint8_t bank = ((value & 4) >> 1) ^ (value & 3);
    uint8_t* const ptr = 8 * 1024 * bank + famicom->expansion_ram32;
    return ptr;
}


/// <summary>
/// SFCs the MMC5 load rombank.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="des">The DES.</param>
/// <param name="value">The value.</param>
static inline void sfc_mmc5_load_rombank(sfc_famicom_t* famicom, int des, uint8_t value) {
    const uint8_t count_prgrom8kb = (uint8_t)(famicom->rom_info.size_prgrom >> 13);
    sfc_load_prgrom_8kpt(famicom, des, (value & 0x7f) % count_prgrom8kb);
}

/// <summary>
/// SFCs the MMC5 load bank.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="des">The DES.</param>
/// <param name="value">The value.</param>
static inline void sfc_mmc5_load_bank(sfc_famicom_t* famicom, int des, uint8_t value) {
    if (value & 0x80)
        sfc_mmc5_load_rombank(famicom, des, value);
    else 
        sfc_load_prgram_8kb(famicom, des, sfc_mmc5_get_rambank(famicom, value));
}

/// <summary>
/// SFCs the MMC5 load go on 8k.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="des">The DES.</param>
static inline void sfc_mmc5_load_go_on_8k(sfc_famicom_t* famicom, int des) {
    const int index = des * 2;
    uint8_t* ptr = famicom->prg_banks[index];
    famicom->prg_banks[index + 2] = ptr + 4 * 1024 * 2;
    famicom->prg_banks[index + 3] = ptr + 4 * 1024 * 3;
}


// 方波#0
extern inline sfc_square_set0(sfc_square_data_t* sq, uint8_t value);
// 方波#1
extern inline sfc_square_set1(sfc_square_data_t* sq, uint8_t value);
// 方波#2
extern inline sfc_square_set2(sfc_square_data_t* sq, uint8_t value);
// 方波#3
extern inline sfc_square_set3(sfc_square_data_t* sq, uint8_t value, uint8_t ok);


/// <summary>
/// SFCs the mapper 05 write low.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="address">The address.</param>
/// <param name="value">The value.</param>
extern void sfc_mapper_05_write_low(sfc_famicom_t* famicom, uint16_t address, uint8_t value) {
    MAPPER;
    switch (address)
    {
        uint8_t color;
#if 0
    case 0x5800:
        // Unknown Register ($5800, write only)
        color = 0;
        break;
#endif
    case 0x5000:
        // Pulse 1 ($5000-$5003)
        famicom->interfaces.audio_change(famicom->argument, famicom->cpu_cycle_count, SFC_MMC5_Square1);
        sfc_square_set0(&famicom->apu.mmc5.square1, value);
        break;
    case 0x5002:
        // Pulse 1 ($5000-$5003)
        famicom->interfaces.audio_change(famicom->argument, famicom->cpu_cycle_count, SFC_MMC5_Square1);
        sfc_square_set2(&famicom->apu.mmc5.square1, value);
        break;
    case 0x5003:
        // Pulse 1 ($5000-$5003)
        famicom->interfaces.audio_change(famicom->argument, famicom->cpu_cycle_count, SFC_MMC5_Square1);
        sfc_square_set3(&famicom->apu.mmc5.square1, value, famicom->apu.mmc5.square1.unused__mmc5_5015);
        break;
    case 0x5004:
        // Pulse 2 ($5004-$5007)
        //{
        //    static uint8_t debug_old = 0;
        //    if (debug_old != value) printf("%02x\n", value);
        //    debug_old = value;
        //}
        famicom->interfaces.audio_change(famicom->argument, famicom->cpu_cycle_count, SFC_MMC5_Square2);
        sfc_square_set0(&famicom->apu.mmc5.square2, value);
        break;
    case 0x5006:
        // Pulse 2 ($5004-$5007)
        famicom->interfaces.audio_change(famicom->argument, famicom->cpu_cycle_count, SFC_MMC5_Square2);
        sfc_square_set2(&famicom->apu.mmc5.square2, value);
        break;
    case 0x5007:
        // Pulse 2 ($5004-$5007)
        famicom->interfaces.audio_change(famicom->argument, famicom->cpu_cycle_count, SFC_MMC5_Square2);
        sfc_square_set3(&famicom->apu.mmc5.square2, value, famicom->apu.mmc5.square2.unused__mmc5_5015);
        //printf("%02x\n", value);
        break;
    case 0x5001:
        // Pulse 1 扫描单元
    case 0x5005:
        // Pulse 2 扫描单元
        break;
    case 0x5010:
        // PCM Mode/IRQ ($5010)
        famicom->apu.mmc5.pcm_mask = value & 1 ? 0x00 : 0xff;
        famicom->apu.mmc5.pcm_irq_enable = value & 0x80;
        break;
    case 0x5011:
        // Raw PCM ($5011)
        famicom->interfaces.audio_change(famicom->argument, famicom->cpu_cycle_count, SFC_MMC5_PCM);
        famicom->apu.mmc5.pcm_output = value & famicom->apu.mmc5.pcm_mask;
        break;
    case 0x5015:
        // Status ($5015, read/write)
        if (!(famicom->apu.mmc5.square1.unused__mmc5_5015 = value & 1))
            famicom->apu.mmc5.square1.length_counter = 0;
        if (!(famicom->apu.mmc5.square2.unused__mmc5_5015 = value & 2))
            famicom->apu.mmc5.square2.length_counter = 0;
        break;
    case 0x5100:
        // PRG mode ($5100)
        mapper->prg_mode = value & 3;
        break;
    case 0x5101:
        // CHR mode ($5101)
        mapper->chr_mode = value & 3;
        break;
    case 0x5102:
        // PRG RAM Protect 1 ($5102)
        mapper->ram_wp = (mapper->ram_wp & 0xc) | (value & 3);
        break;
    case 0x5103:
        // PRG RAM Protect 2 ($5103)
        mapper->ram_wp = (mapper->ram_wp & 0x3) | ((value << 2) & 0xc);
        break;
    case 0x5104:
        // Extended RAM mode ($5104)
#ifndef NDEBUG
        printf("[%5d]MMC5: Extended RAM mode ($5104) = %02x\n", famicom->frame_counter, value & 3);
#endif
        mapper->exram_mode = value & 3;
        mapper->exram_write_mask_mmc5 = 0x00;
        famicom->ppu.data.ppu_mode = SFC_EZPPU_Normal;
        if (mapper->exram_mode == 1) 
            famicom->ppu.data.ppu_mode = SFC_EXPPU_ExGrafix;
        else if (mapper->exram_mode == 2)
            mapper->exram_write_mask_mmc5 = 0xff;
        break;
    case 0x5105:
        // Nametable mapping ($5105)
#ifndef NDEBUG
        //printf("[%5d]MMC5: Nametable mapping ($5105) = %02x\n", famicom->frame_counter, value);
#endif
        sfc_mmc5_update_nametable(famicom, value);
        break;
    case 0x5106:
        // Fill-mode tile ($5106)
        memset(sfc_mmc5_fill_nt(famicom) + 0x000, value, 0x3C0);
        break;
    case 0x5107:
        // Fill-mode color ($5107)
        color = value & 3;
        color = color | (color << 2) | (color << 4) | (color << 6);
        memset(sfc_mmc5_fill_nt(famicom) + 0x3C0, color, 0x040);
        break;
    case 0x5113:
        // PRG Bank 3, RAM Only ($5113)
        sfc_load_prgram_8kb(famicom, 3, sfc_mmc5_get_rambank(famicom, value));
#ifndef NDEBUG
        //{
        //    static uint8_t value_old_debug = 0;
        //    if (value_old_debug != value) {
        //        value_old_debug = value;
        //        printf("MMC5: $6000-$7FFF change to %d\n", value);
        //    }
        //}
#endif
        break;
    case 0x5114:
        // PRG Bank 4, ROM/RAM ($5114)
        switch (mapper->prg_mode)
        {
        case 3:
            // Mode 3 - Select an 8KB PRG bank at $8000-$9FFF
            sfc_mmc5_load_bank(famicom, 4, value);
        }
        break;
    case 0x5115:
        // PRG Bank 5, ROM/RAM ($5115)
        switch (mapper->prg_mode)
        {
        case 1:
            // Mode 1 - Select a 16KB PRG bank at $8000-$BFFF (ignore bottom bit)
        case 2:
            // Mode 2 - Select a 16KB PRG bank at $8000-$BFFF (ignore bottom bit)
            sfc_mmc5_load_bank(famicom, 4, value & 0xfe);
            sfc_mmc5_load_go_on_8k(famicom, 4);
            break;
        case 3:
            // Mode 3 - Select an 8KB PRG bank at $A000-$BFFF
            sfc_mmc5_load_bank(famicom, 5, value);
            break;
        }
        break;
    case 0x5116:
        // PRG Bank 6, ROM/RAM ($5116)
        // Mode 2 - Select an 8KB PRG bank at $C000-$DFFF
        // Mode 3 - Select an 8KB PRG bank at $C000-$DFFF
        if (mapper->prg_mode & 2)
            sfc_mmc5_load_bank(famicom, 6, value);
        break;
    case 0x5117:
        // PRG Bank 7, ROM Only ($5117)
        switch (mapper->prg_mode)
        {
        case 0:
            // 模式 0 - 选择 32KB PRG-ROM bank @ $8000-$FFFF (忽略低2位)
            sfc_mmc5_load_rombank(famicom, 4, value & 0xfc);
            sfc_mmc5_load_go_on_8k(famicom, 4);
            sfc_mmc5_load_go_on_8k(famicom, 5);
            sfc_mmc5_load_go_on_8k(famicom, 6);
            break;
        case 1:
            // 模式 1 - 选择 16KB PRG-ROM bank @ $C000-$FFFF (忽略最低1位)
            sfc_mmc5_load_rombank(famicom, 6, value & 0xfe);
            sfc_mmc5_load_go_on_8k(famicom, 6);
            break;
        case 2:
            // 模式 2 - 选择  8KB PRG-ROM bank @ $E000-$FFFF
        case 3:
            // 模式 3 - 选择  8KB PRG-ROM bank @ $E000-$FFFF
            sfc_mmc5_load_rombank(famicom, 7, value);
            break;
        }
        break;
    case 0x5120:
    case 0x5121:
    case 0x5122:
    case 0x5123:
    case 0x5124:
    case 0x5125:
    case 0x5126:
    case 0x5127:
        // CHR Bankswitching A ($5120-$5137)
        mapper->chrbank_x8[address & 7] = (uint16_t)value | ((uint16_t)mapper->chrbank_hi << 2);
        sfc_mmc5_update_chrbank_x8(famicom);
        //if (debug_irq_ed)
        //    printf(
        //        "[%5d@%3d]$%04x = %02x\n", 
        //        famicom->frame_counter, 
        //        debug_scanline,
        //        address, value
        //    );
        break;
    case 0x5128:
    case 0x5129:
    case 0x512A:
    case 0x512B:
        // CHR Bankswitching B ($5128-$513B)
        // TODO: 实现MMC5'黑科技'
        mapper->chrbank_16[address & 3] = (uint16_t)value | ((uint16_t)mapper->chrbank_hi << 2);
        //sfc_mmc5_update_chrbank_16(famicom);
        //printf("[%5d]$%04x = %02x\n", famicom->frame_counter, address, value);
        //if (debug_irq_ed)
        //    printf(
        //        "[%5d@%3d]$%04x = %02x\n",
        //        famicom->frame_counter,
        //        debug_scanline,
        //        address, value
        //    );
        break;
    case 0x5130:
        // Upper CHR Bank bits ($5130)
        // 为了配个ExGrafix模式, 将数据移动到高位
        mapper->chrbank_hi = (value & 3) << 6;
        break;
    case 0x5200:
        // Vertical Split Mode ($5200)
    case 0x5201:
        // Vertical Split Scroll ($5201)
    case 0x5202:
        // Vertical Split Bank ($5202)
        // TODO: 实现Split模式
        assert(value == 0 && "NOT IMPL");
        break;
    case 0x5203:
        // IRQ Counter ($5203)
        mapper->irq_scanline = value;
        //printf("[%5d]IRQ Counter ($5203) = %02x\n", famicom->frame_counter, value);
        break;
    case 0x5204:
        // IRQ Status ($5204, read/write)
        mapper->irq_enable = value & 0x80;
        break;
    case 0x5205:
        // Unsigned 8x8 to 16 Multiplier ($5205, $5206 read/write)
        mapper->factor[0] = value;
        mapper->product = (uint16_t)mapper->factor[0] * (uint16_t)mapper->factor[1];
        break;
    case 0x5206:
        // Unsigned 8x8 to 16 Multiplier ($5205, $5206 read/write)
        mapper->factor[1] = value;
        mapper->product = (uint16_t)mapper->factor[0] * (uint16_t)mapper->factor[1];
        break;
    default:
        if (address >= 0x5C00) {
            // XXX: 写入保护?
            assert(mapper->exram_mode != 3);
            //if (mapper->exram_mode != 3)
            const uint8_t mask
                = mapper->exram_write_mask_mmc5
                | mapper->exram_write_mask_ppu
                ;
            sfc_mmc5_exram(famicom)[address & 0x3ff] = value & mask;
        }
        else assert(!"NOT IMPL");
        break;
    }
}

/// <summary>
/// SFCs the mapper 05 write high.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="address">The address.</param>
/// <param name="value">The value.</param>
static void sfc_mapper_05_write_high(sfc_famicom_t* famicom, uint16_t address, uint8_t value) {
    assert(address < 0xe000);
    famicom->prg_banks[address >> 12][address & (uint16_t)0x0fff] = value;
}

/// <summary>
/// SFCs the mapper 05 read low.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="address">The address.</param>
/// <returns></returns>
static uint8_t sfc_mapper_05_read_low(sfc_famicom_t* famicom, uint16_t address) {
    MAPPER;
    switch (address)
    {
        uint8_t value;
    case 0x5010:
        value = (famicom->apu.mmc5.pcm_mask & 1) | famicom->apu.mmc5.pcm_irq_tri;
        famicom->apu.mmc5.pcm_irq_tri = 0;
        return value;
    case 0x5015:
        // Status ($5015, read/write)
        value = 0;
        if (famicom->apu.mmc5.square1.length_counter) value |= 1;
        if (famicom->apu.mmc5.square2.length_counter) value |= 2;
        return value;
    case 0x5204:
        // IRQ Status ($5204, read/write)
        value = mapper->irq_status_f | mapper->irq_status_p;
        mapper->irq_status_p = 0;
        sfc_operation_IRQ_acknowledge(famicom);
        return value;
    case 0x5205:
        // Unsigned 8x8 to 16 Multiplier ($5205, $5206 read/write)
        return (uint8_t)(mapper->product & 0xff);
    case 0x5206:
        // Unsigned 8x8 to 16 Multiplier ($5205, $5206 read/write)
        return (uint8_t)((mapper->product >> 8) & 0xff);
    default:
        if (address >= 0x5C00) 
            return sfc_mmc5_exram(famicom)[address & 0x3ff];
    }
    assert(!"NOT IMPL");
    return 0;
}


// 默认写入
extern void sfc_mapper_wrts_defualt(const sfc_famicom_t* famicom);
// 默认读取
extern void sfc_mapper_rrfs_defualt(sfc_famicom_t* famicom);

/// <summary>
/// MMC5: 写入RAM到流
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_05_write_ram(const sfc_famicom_t* famicom) {
    // 保存MMC5 PRG-RAM
    famicom->interfaces.sl_write_stream(
        famicom->argument,
        famicom->expansion_ram32,
        sizeof(famicom->expansion_ram32)
    );
    // 将可能的CHR-RAM数据写入流
    sfc_mapper_wrts_defualt(famicom);
}

/// <summary>
/// MMC5: 从流读取至RAM
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_05_read_ram(sfc_famicom_t* famicom) {
    // 读取MMC5 PRG-RAM
    famicom->interfaces.sl_read_stream(
        famicom->argument,
        famicom->expansion_ram32,
        sizeof(famicom->expansion_ram32)
    );
    // 将流中读取至可能的CHR-RAM
    sfc_mapper_rrfs_defualt(famicom);
}


/// <summary>
/// StepFC: 载入Mapper005-MMC5
/// </summary>
/// <param name="famicom">The famicom.</param>
extern inline sfc_ecode sfc_load_mapper_05(sfc_famicom_t* famicom) {
    enum { MAPPER_05_SIZE_IMPL = sizeof(sfc_mapper05_t) };
    static_assert(SFC_MAPPER_05_SIZE == MAPPER_05_SIZE_IMPL, "SAME");
    // 初始化回调接口
    famicom->mapper.reset = sfc_mapper_05_reset;
    famicom->mapper.hsync = sfc_mapper_05_hsync;
    famicom->mapper.read_low = sfc_mapper_05_read_low;
    famicom->mapper.write_low = sfc_mapper_05_write_low;
    famicom->mapper.write_high = sfc_mapper_05_write_high;
    famicom->mapper.write_ram_to_stream = sfc_mapper_05_write_ram;
    famicom->mapper.read_ram_from_stream = sfc_mapper_05_read_ram;
    // 初始化数据
    MAPPER;
    memset(mapper, 0, MAPPER_05_SIZE_IMPL);
    // 可能超过8KiB的SRAM
    if (famicom->rom_info.save_ram_flags)
        famicom->rom_info.save_ram_flags |= SFC_ROMINFO_SRAM_More8KiB;
    return SFC_ERROR_OK;
}