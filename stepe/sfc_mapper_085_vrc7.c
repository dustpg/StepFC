#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include "sfc_mapper_helper.h"
#include <assert.h>
#include <string.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

// Mapper-085 VRC7 (Submapper2, 1  -> VRC7a, VRC7b)


// 内部PATCH表
const uint8_t sfc_vrc7_internal_patch_set[128] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Custom
    0x03, 0x21, 0x05, 0x06, 0xB8, 0x81, 0x42, 0x27, // Buzzy Bell
    0x13, 0x41, 0x13, 0x0D, 0xD8, 0xD6, 0x23, 0x12, // Guitar
    0x31, 0x11, 0x08, 0x08, 0xFA, 0x9A, 0x22, 0x02, // Wurly
    0x31, 0x61, 0x18, 0x07, 0x78, 0x64, 0x30, 0x27, // Flute
    0x22, 0x21, 0x1E, 0x06, 0xF0, 0x76, 0x08, 0x28, // Clarinet
    0x02, 0x01, 0x06, 0x00, 0xF0, 0xF2, 0x03, 0xF5, // Synth
    0x21, 0x61, 0x1D, 0x07, 0x82, 0x81, 0x16, 0x07, // Trumpet
    0x23, 0x21, 0x1A, 0x17, 0xCF, 0x72, 0x25, 0x17, // Organ
    0x15, 0x11, 0x25, 0x00, 0x4F, 0x71, 0x00, 0x11, // Bells
    0x85, 0x01, 0x12, 0x0F, 0x99, 0xA2, 0x40, 0x02, // Vibes
    0x07, 0xC1, 0x69, 0x07, 0xF3, 0xF5, 0xA7, 0x12, // Vibraphone
    0x71, 0x23, 0x0D, 0x06, 0x67, 0x75, 0x23, 0x16, // Tutti
    0x01, 0x02, 0xD3, 0x05, 0xA3, 0x92, 0xF7, 0x52, // Fretless
    0x61, 0x63, 0x0C, 0x00, 0x94, 0xAF, 0x34, 0x06, // Synth Bass
    0x21, 0x72, 0x0D, 0x00, 0xC1, 0xA0, 0x54, 0x16, // Sweep
};



// IRQ - 中断请求 - 确认
extern inline void sfc_operation_IRQ_acknowledge(sfc_famicom_t* famicom);
// 尝试触发
extern inline void sfc_operation_IRQ_try(sfc_famicom_t* famicom);
// 使用VRC6 相同的水平同步逻辑
extern void sfc_mapper_18_hsyc(sfc_famicom_t* famicom, uint16_t line);

typedef struct {
    // IRQ contrl
    uint8_t         irq_control;
    // IRQ contrl
    uint8_t         irq_reload;
    // IRQ counter
    uint8_t         irq_counter;
    // IRQ enable
    uint8_t         irq_enable;

} sfc_mapper55_t;


/// <summary>
/// SFCs the mapper.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline sfc_mapper55_t* sfc_mapper(sfc_famicom_t* famicom) {
    return (sfc_mapper55_t*)famicom->mapper_buffer.mapper04;
}

#define MAPPER sfc_mapper55_t* const mapper = sfc_mapper(famicom);



/// <summary>
/// SFCs the mapper 04 reset.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
sfc_ecode sfc_mapper_55_reset(sfc_famicom_t* famicom) {
    const uint32_t count_prgrom8kb = famicom->rom_info.size_prgrom >> 13;

    // 支持VRC7
    famicom->rom_info.extra_sound = SFC_NSF_EX_VCR7;
    // 载入最后的BANK
    sfc_load_prgrom_8k(famicom, 3, count_prgrom8kb - 1);

    sfc_load_prgrom_8k(famicom, 0, 0);
    sfc_load_prgrom_8k(famicom, 1, 0);
    sfc_load_prgrom_8k(famicom, 2, 0);

    for (int i = 0; i != 8 ; ++i)
        sfc_load_chrrom_1k(famicom, i, i);

    return SFC_ERROR_OK;
}


// 默认写入
extern void sfc_mapper_wrts_defualt(const sfc_famicom_t* famicom);
// 默认读取
extern void sfc_mapper_rrfs_defualt(sfc_famicom_t* famicom);


/// <summary>
/// VRC7: 写入RAM到流
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_55_write_ram(const sfc_famicom_t* famicom) {
    // 保存VRC7 PATCH表
    famicom->interfaces.sl_write_stream(
        famicom->argument,
        sfc_get_vrc7_patchc(famicom),
        sizeof(sfc_vrc7_internal_patch_set)
    );
    // CHR-RAM数据写入流[拉格朗日点使用了CHR-RAM]
    sfc_mapper_wrts_defualt(famicom);
}

/// <summary>
/// VRC7: 从流读取至RAM
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_55_read_ram(sfc_famicom_t* famicom) {
    // 读取VRC7 PATCH表
    famicom->interfaces.sl_read_stream(
        famicom->argument,
        sfc_get_vrc7_patch(famicom),
        sizeof(sfc_vrc7_internal_patch_set)
    );
    // 流中读取至CHR-RAM[拉格朗日点使用了CHR-RAM]
    sfc_mapper_rrfs_defualt(famicom);
}

// 写入寄存器
static void sfc_mapper_55_regwrite(sfc_famicom_t* famicom, uint8_t value);

/// <summary>
/// StepFC: VRC7 镜像控制
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="value">The value.</param>
static void sfc_mapper_55_mirroring(sfc_famicom_t* famicom, uint8_t value) {
    /*
    7  bit  0
    ---------
    RS.. ..MM
    ||     ||
    ||     ++- Mirroring (0: vertical; 1: horizontal;
    ||                        2: one-screen, lower bank; 3: one-screen, upper bank)
    |+-------- Silence expansion sound if set
    +--------- WRAM enable (1: enable WRAM, 0: protect)

    0: 2(SFC_NT_MIR_Vertical)
    1: 3(SFC_NT_MIR_Horizontal)
    2: 1(SFC_NT_MIR_SingleLow)
    3: 2(SFC_NT_MIR_SingleHigh)
    */

    const sfc_nametable_mirroring_mode mode = ((value & 2) ^ 2) | (value & 1);
    sfc_switch_nametable_mirroring(famicom, mode);

    // 静音VRC7
    if (value & 0x40) {
        famicom->rom_info.extra_sound &= ~SFC_NSF_EX_VCR7;

    }
    // 设置VRC7
    else
        famicom->rom_info.extra_sound |= SFC_NSF_EX_VCR7;
}

/// <summary>
/// SFCs the mapper 55 write high.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="address">The address.</param>
/// <param name="value">The value.</param>
void sfc_mapper_55_write_high(sfc_famicom_t* famicom, uint16_t address, uint8_t value) {
    uint16_t base;
    //printf("[$%04X] = $%02X\n", address, value);

    // VRC7: A12 A13 A14
    // VRC7a:  A4
    // VRC7b:  A3
    const uint16_t vrc7a = address >> 4;
    const uint16_t vrc7b = address >> 3;
    const uint16_t basic = ((address >> 11) & 0xfffe) | ((vrc7a | vrc7b) & 1);

    switch (base = basic & 0xf)
    {
        sfc_mapper55_t* mapper;
        uint16_t banks;
    case 0x0:
        // PRG Select 0 ($8000)
    case 0x1:
        // PRG Select 1 ($8010, $8008)
    case 0x2:
        // PRG Select 2 ($9000)
        banks = famicom->rom_info.size_prgrom >> 13;
        sfc_load_prgrom_8k(famicom, base, value % banks);
        break;
    case 0x3:
        // Audio Register Write ($9030)
        if (address & 0x20) sfc_mapper_55_regwrite(famicom, value);
        // Audio Register Select ($9010)
        else famicom->apu.vrc7.selected = value;
        break;
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
    case 0x8:
    case 0x9:
    case 0xa:
    case 0xb:
        // CHR Select 0…7 ($A000…$DFFF)
        banks = (famicom->rom_info.size_chrrom >> 10) - 1;
        sfc_load_chrrom_1k(famicom, base - 4, value & banks);
        break;
    case 0xc:
        // Mirroring Control ($E000)
        sfc_mapper_55_mirroring(famicom, value);
        break;
    case 0xd:
        // $E008, $E010:  IRQ Latch
        mapper = sfc_mapper(famicom);
        mapper->irq_reload = value;
        break;
    case 0xe:
        // $F000:  IRQ Control
        mapper = sfc_mapper(famicom);
        mapper->irq_control = value;
        mapper->irq_enable = value & 2;
        if (mapper->irq_enable)
            mapper->irq_counter = mapper->irq_reload;
        break;
    case 0xf:
        // $F008, $F010:  IRQ Acknowledge
        mapper = sfc_mapper(famicom);
        mapper->irq_enable = mapper->irq_control & 1;
        //mapper->irq_control 
        //    = (mapper->irq_control & 5)
        //    | ((mapper->irq_control & 1) << 1)
        //    ;
        sfc_operation_IRQ_acknowledge(famicom);
        break;
    }
}


/// <summary>
/// SFCs the mapper 18 hsyc.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="line">The line.</param>
void sfc_mapper_55_hsyc(sfc_famicom_t* famicom, uint16_t line) {
    MAPPER;
    if (!mapper->irq_enable) return;
    // 扫描线模式
    assert((mapper->irq_control & (1 << 2)) == 0 && "UNSUPPORTED");
    // 触发
    if (mapper->irq_counter == (uint8_t)0xff) {
        mapper->irq_counter = mapper->irq_reload;
        sfc_operation_IRQ_try(famicom);
    }
    // +1
    else ++mapper->irq_counter;
}

/// SFCs the load mapper 1A.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline sfc_ecode sfc_load_mapper_55(sfc_famicom_t* famicom) {
    enum { MAPPER_55_SIZE_IMPL = sizeof(sfc_mapper55_t) };
    static_assert(SFC_MAPPER_55_SIZE == MAPPER_55_SIZE_IMPL, "SAME");
    famicom->mapper.reset = sfc_mapper_55_reset;
    famicom->mapper.hsync = sfc_mapper_55_hsyc;
    famicom->mapper.write_high = sfc_mapper_55_write_high;
    famicom->mapper.write_ram_to_stream = sfc_mapper_55_write_ram;
    famicom->mapper.read_ram_from_stream = sfc_mapper_55_read_ram;
    MAPPER;
    memset(mapper, 0, MAPPER_55_SIZE_IMPL);
    return SFC_ERROR_OK;
}

/// <summary>
/// SFCs the VRC7 reset.
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_vrc7_reset(sfc_famicom_t* famicom) {
    memcpy(
        sfc_get_vrc7_patch(famicom), 
        sfc_vrc7_internal_patch_set, 
        sizeof(sfc_vrc7_internal_patch_set)
    );
}


// -----------------------------------------------------


/// <summary>
/// VRC7: Audio 触发
/// </summary>
/// <param name="op">The op.</param>
/// <param name="changed">The changed.</param>
static inline void sfc_vrc7_audio_trigger(
    sfc_vrc7_operator_t* op, 
    uint8_t changed, uint8_t trigger) {
    if (!changed) return;

}


/// <summary>
/// SFCs the mapper 55 regwrite.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="value">The value.</param>
static void sfc_mapper_55_regwrite(sfc_famicom_t* famicom, uint8_t value) {
    sfc_vrc7_data_t* const vrc7 = &famicom->apu.vrc7;
    const uint8_t selected = vrc7->selected;
    // 00-07: 自定义PATCH
    if (selected < 0x08) {
        sfc_get_vrc7_patch(famicom)[selected] = value;
    }
    // 10-15: 声道低八位频率
    else if (selected >= 0x10 && selected <= 0x15) {
        sfc_vrc7_ch_t* const ch = &famicom->apu.vrc7.ch[selected & 0x7];
        ch->freq = (ch->freq & 0x100) | (uint16_t)value;
    }
    // 20-25: 控制信息
    else if (selected >= 0x20 && selected <= 0x25) {
        // --ST OOOH
        sfc_vrc7_ch_t* const ch = &famicom->apu.vrc7.ch[selected & 0x7];
        ch->freq = (ch->freq & 0xff) | ((uint16_t)(value & 1) << 8);
        ch->octave = (value >> 1) & 7;
        ch->sustain = (value >> 5) & 1;
        const uint8_t trigger = (value >> 4) & 1;
        const uint8_t changed = trigger ^ ch->trigger;
        ch->trigger = trigger;
        sfc_vrc7_audio_trigger(&ch->carrier, changed, trigger);
        sfc_vrc7_audio_trigger(&ch->modulator, changed, trigger);
    }
    // 30-35: 乐器音量
    else if (selected >= 0x30 && selected <= 0x35) {
        // IIII VVVV
        sfc_vrc7_ch_t* const ch = &famicom->apu.vrc7.ch[selected & 0x7];
        ch->volume = value & 0xf;
        ch->instrument = (value >> 4) & 0xf;
    }
}


// 倍乘因子查找表(需要再除以2)
static const uint8_t sfc_vrc7_multi_lut[] = {
    1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30
};
