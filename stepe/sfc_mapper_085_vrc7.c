#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include "sfc_mapper_helper.h"
#include <assert.h>
#include <string.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

// 控制宏:

// AM参数模仿EMU2413
//#define SFC_AM_SAMEAS_EMU2413

// FM使用浮点
//#define SFC_FM_FLOAT


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

// 算子修改
static void sfc_vrc7_operator_changed(
    sfc_famicom_t*,
    sfc_vrc7_ch_t*,
    sfc_vrc7_operator_t*, 
    uint8_t carrier
);

// 敲键盘!
static inline void sfc_vrc7_audio_trigger(
    sfc_vrc7_operator_t* op,
    uint8_t changed, 
    uint8_t trigger
);

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
        famicom->interfaces.audio_changed(famicom->argument, famicom->cpu_cycle_count, SFC_VRC7_VRC7);
        sfc_get_vrc7_patch(famicom)[selected] = value;
    }
    // 10-15: 声道低八位频率
    else if (selected >= 0x10 && selected <= 0x15) {
        famicom->interfaces.audio_changed(famicom->argument, famicom->cpu_cycle_count, SFC_VRC7_FM0 + (selected & 0x7));
        sfc_vrc7_ch_t* const ch = &famicom->apu.vrc7.ch[selected & 0x7];
        ch->freq = (ch->freq & 0xff00) | (uint16_t)value;
        sfc_vrc7_operator_changed(famicom, ch, &ch->modulator, 0);
        sfc_vrc7_operator_changed(famicom, ch, &ch->carrier, 1);
    }
    // 20-25: 控制信息
    else if (selected >= 0x20 && selected <= 0x25) {
        famicom->interfaces.audio_changed(famicom->argument, famicom->cpu_cycle_count, SFC_VRC7_FM0 + (selected & 0x7));
        // --ST OOOH
        sfc_vrc7_ch_t* const ch = &famicom->apu.vrc7.ch[selected & 0x7];
        ch->freq = (ch->freq & 0xff) | ((uint16_t)(value & 1) << 8);
        ch->octave = (value >> 1) & 7;
        ch->sustain = (value >> 5) & 1;
        ch->low4bit = value & 0xf;
        const uint8_t trigger = (value >> 4) & 1;
        const uint8_t changed = trigger ^ ch->trigger;
        ch->trigger = trigger;
        sfc_vrc7_audio_trigger(&ch->carrier, changed, trigger);
        sfc_vrc7_audio_trigger(&ch->modulator, changed, trigger);
        sfc_vrc7_operator_changed(famicom, ch, &ch->modulator, 0);
        sfc_vrc7_operator_changed(famicom, ch, &ch->carrier, 1);
    }
    // 30-35: 乐器音量
    else if (selected >= 0x30 && selected <= 0x35) {
        famicom->interfaces.audio_changed(famicom->argument, famicom->cpu_cycle_count, SFC_VRC7_FM0 + (selected & 0x7));
        // IIII VVVV
        sfc_vrc7_ch_t* const ch = &famicom->apu.vrc7.ch[selected & 0x7];
        ch->volume = value & 0xf;
        ch->instrument8 = (value & 0xf0) >> 1;
        sfc_vrc7_operator_changed(famicom, ch, &ch->modulator, 0);
        sfc_vrc7_operator_changed(famicom, ch, &ch->carrier, 1);
    }
}

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
        if (famicom->rom_info.extra_sound & SFC_NSF_EX_VCR7) {
            famicom->interfaces.audio_changed(famicom->argument, famicom->cpu_cycle_count, SFC_VRC7_VRC7);
            famicom->rom_info.extra_sound &= ~SFC_NSF_EX_VCR7;
            const uint8_t last = famicom->apu.vrc7.selected;
            memset(&famicom->apu.vrc7, 0, sizeof(famicom->apu.vrc7));
            famicom->apu.vrc7.selected = last;
        }
    }
    // 设置VRC7
    else {
        if (!(famicom->rom_info.extra_sound & SFC_NSF_EX_VCR7)) {
            famicom->interfaces.audio_changed(famicom->argument, famicom->cpu_cycle_count, SFC_VRC7_VRC7);
            famicom->rom_info.extra_sound |= SFC_NSF_EX_VCR7;
        }
    }
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
        if (address & 0x20) 
            sfc_mapper_55_regwrite(famicom, value);
        // Audio Register Select ($9010)
        else 
            famicom->apu.vrc7.selected = value;
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
//                   VRC7 Integer Mode
// -----------------------------------------------------

enum sfc_vrc7_const {
    // 最大衰减值(23bit max + 1)
    SFC_VRC7_AttenuationMax = 1 << 23
    // 单位衰减
#define SFC_VRC7_AttenuationUnit ((double)SFC_VRC7_AttenuationMax / 48.0)
#define SFC_PI 3.14159265358979323846
#define SFC_2PI 6.28318530717958647692
};


// 倍乘因子查找表
static const uint8_t sfc_vrc7_multi_lut[] = {
    1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30
};

// Key Scale用表
static const int32_t sfc_vrc7_ks_lut[] = {
    (int32_t)(SFC_VRC7_AttenuationUnit * 0.0 + 0.5),
    (int32_t)(SFC_VRC7_AttenuationUnit * 18.0 + 0.5),
    (int32_t)(SFC_VRC7_AttenuationUnit * 24.0 + 0.5),
    (int32_t)(SFC_VRC7_AttenuationUnit * 27.75 + 0.5),
    (int32_t)(SFC_VRC7_AttenuationUnit * 30.00 + 0.5),
    (int32_t)(SFC_VRC7_AttenuationUnit * 32.25 + 0.5),
    (int32_t)(SFC_VRC7_AttenuationUnit * 33.75 + 0.5),
    (int32_t)(SFC_VRC7_AttenuationUnit * 35.25 + 0.5),
    (int32_t)(SFC_VRC7_AttenuationUnit * 36.00 + 0.5),
    (int32_t)(SFC_VRC7_AttenuationUnit * 37.50 + 0.5),
    (int32_t)(SFC_VRC7_AttenuationUnit * 38.25 + 0.5),
    (int32_t)(SFC_VRC7_AttenuationUnit * 39.00 + 0.5),
    (int32_t)(SFC_VRC7_AttenuationUnit * 39.75 + 0.5),
    (int32_t)(SFC_VRC7_AttenuationUnit * 40.50 + 0.5),
    (int32_t)(SFC_VRC7_AttenuationUnit * 41.25 + 0.5),
    (int32_t)(SFC_VRC7_AttenuationUnit * 42.00 + 0.5),
};


#include <math.h>

enum sfc_vrc7_config {
    // 半正弦
    SFC_VRC7_HALFSINE_LEN = 1 << SFC_VRC7_HALF_SINE_LUT_BIT,
    SFC_VRC7_HS_RS_BIT = 17 - SFC_VRC7_HALF_SINE_LUT_BIT,
    SFC_VRC7_HS_MASK = SFC_VRC7_HALFSINE_LEN - 1,
    // AM
    SFC_VRC7_AM_LUTLEN = 1 << SFC_VRC7_AM_LUT_BIT,
    SFC_VRC7_AM_RS_BIT = 20 - SFC_VRC7_AM_LUT_BIT,
    SFC_VRC7_AM_MASK = SFC_VRC7_AM_LUTLEN - 1,
    // FM
    SFC_VRC7_FM_LUTLEN = 1 << SFC_VRC7_FM_LUT_BIT,
    SFC_VRC7_FM_RS_BIT = 20 - SFC_VRC7_FM_LUT_BIT,
    SFC_VRC7_FM_MASK = SFC_VRC7_FM_LUTLEN - 1,
    SFC_VRC7_INT_BITWIDTH = 18,
    // Attack 输出
    SFC_VRC7_AO_LUTLEN = 1 << SFC_VRC7_ATKOUT_LUT_BIT,
    SFC_VRC7_AO_RS_BIT = 23 - SFC_VRC7_ATKOUT_LUT_BIT,
    SFC_VRC7_AO_MASK = SFC_VRC7_AO_LUTLEN - 1,
    // 衰减转线性
    SFC_VRC7_AL_LUTLEN = 1 << SFC_VRC7_A2L_LUT_BIT,
    SFC_VRC7_AL_RS_BIT = 23 - SFC_VRC7_A2L_LUT_BIT,
};


static uint32_t sfc_vrc7_hslut[SFC_VRC7_HALFSINE_LEN];
static uint32_t sfc_vrc7_amlut[SFC_VRC7_AM_LUTLEN];
static sfc_vrc7_fm_t sfc_vrc7_fmlut[SFC_VRC7_FM_LUTLEN];
static uint32_t sfc_vrc7_aolut[SFC_VRC7_AO_LUTLEN];
static int32_t  sfc_vrc7_allut[SFC_VRC7_AL_LUTLEN];


/// <summary>
/// SFCs the VRC7 initialize lut.
/// </summary>
void sfc_vrc7_init_lut(void) {
    // 初始化半正弦查找表
    sfc_vrc7_hslut[0] = SFC_VRC7_AttenuationMax;
    for (int i = 1; i != SFC_VRC7_HALFSINE_LEN; ++i) {
        // dB = -20 * log10( Linear ) * scale
        const double sinx = sin(SFC_PI * i / SFC_VRC7_HALFSINE_LEN);
        const double fx = -20.0 * log10(sinx) * SFC_VRC7_AttenuationUnit;
        sfc_vrc7_hslut[i] 
            = (fx >= (double)SFC_VRC7_AttenuationMax)
            ? SFC_VRC7_AttenuationMax
            : (uint32_t)fx
            ;
    }
    // 初始化AM相位表
    // sinx = sin(2 * PI * counter / (1<<20))
    // AM_output = (1.0 + sinx) * 0.6 dB (emu2413 使用的是 1.2 dB
    for (int i = 0; i != SFC_VRC7_AM_LUTLEN; ++i) {
        const double sinx = sin(SFC_2PI * i / SFC_VRC7_AM_LUTLEN);
        const double out
            = (1.0 + sinx)
            * SFC_VRC7_AttenuationUnit
#ifdef SFC_AM_SAMEAS_EMU2413
            * 1.2
#else
            * 0.6
#endif
            ;
        sfc_vrc7_amlut[i] = (uint32_t)out;
    }

    // 初始化FM相位表
    // FM_output = 2 ^ (13.75 / 1200 * sinx)
    for (int i = 0; i != SFC_VRC7_FM_LUTLEN; ++i) {
        const double sinx = sin(SFC_2PI * i / SFC_VRC7_FM_LUTLEN);
        const double out = pow(2.0, 13.75 / 1200.0 * sinx);
#ifdef SFC_FM_FLOAT
        sfc_vrc7_fmlut[i] = (float)out;
#else
        const int32_t out2 = (int32_t)((out - 1.0) * (double)(1<< SFC_VRC7_INT_BITWIDTH));
        sfc_vrc7_fmlut[i] = out2;
#endif
    }
    // Attack输出
    // AO(EGC) = 48 dB - (48 dB * ln(EGC) / ln(1<<23))
    const double base = log(SFC_VRC7_AttenuationMax >> SFC_VRC7_AO_RS_BIT);
    for (int i = 0; i != SFC_VRC7_AO_LUTLEN; ++i) {
        const double d48 = SFC_VRC7_AttenuationUnit * 48.0;
        const double out = (d48 * log((double)i)) / base;
        sfc_vrc7_aolut[i] = (uint32_t)d48 - (uint32_t)out;
    }
    // 衰减转线性
    // Linear = 10 ^ (dB / -20 / scale)
    // 输出20bit
    const double outmax = 1 << 20;
    const double scale = SFC_VRC7_AttenuationUnit / (1 << SFC_VRC7_AL_RS_BIT);
    for (int i = 0; i != SFC_VRC7_AL_LUTLEN; ++i) {
        const double out = pow(10.0, (double)i / -20.0 / scale) ;
        sfc_vrc7_allut[i] = (int32_t)(out * outmax);
    }
}


/// <summary>
/// StepFC: VRC7 半正弦
/// </summary>
/// <param name="x">The x.</param>
/// <returns></returns>
static inline uint32_t sfc_vrc7_half_sine(uint32_t x) {
    // 17bit + 1bit负相位
    return sfc_vrc7_hslut[(x >> SFC_VRC7_HS_RS_BIT)&SFC_VRC7_HS_MASK];
}

/// <summary>
/// StepFC: VRC7 衰减值转线性值
/// </summary>
/// <param name="a">a.</param>
/// <returns></returns>
static inline int32_t sfc_vrc7_a2l(uint32_t a) {
    // output: 20bit
    return sfc_vrc7_allut[a >> SFC_VRC7_AL_RS_BIT];
}

/// <summary>
/// StepFC: VRC7 AM输出计算
/// </summary>
/// <param name="a">a.</param>
/// <returns></returns>
static inline uint32_t sfc_vrc7_am_calc(uint32_t phase) {
    // 20bit
    return sfc_vrc7_amlut[(phase >> SFC_VRC7_AM_RS_BIT) & SFC_VRC7_AM_MASK];
}

/// <summary>
/// StepFC: VRC7 FM输出计算
/// </summary>
/// <param name="a">a.</param>
/// <returns></returns>
static inline sfc_vrc7_fm_t sfc_vrc7_fm_calc(uint32_t phase) {
    // 用于20bit
    return sfc_vrc7_fmlut[(phase >> SFC_VRC7_FM_RS_BIT) & SFC_VRC7_FM_MASK];
}

/// <summary>
/// StepFC: VRC7 FM计算
/// </summary>
/// <param name="left">The left.</param>
/// <param name="right">The right.</param>
/// <returns></returns>
static inline uint32_t sfc_vrc7_fm_do(uint32_t left_x4, sfc_vrc7_fm_t right) {
#ifdef SFC_FM_FLOAT
    return (uint32_t)((double)left_x4 * (double)right * 0.25);
#else
    const int32_t ileft = (int32_t)(left_x4);
    const int32_t extra = (ileft * right) / (1 << SFC_VRC7_INT_BITWIDTH);
    return (uint32_t)(ileft + extra) >> 2;
#endif
}


/// <summary>
/// StepFC: VRC7 Attack阶段输出
/// </summary>
/// <param name="egc">The egc.</param>
/// <returns></returns>
static inline uint32_t sfc_vrc7_attack_output(uint32_t egc) {
    // 23bit
    return sfc_vrc7_aolut[(egc >> SFC_VRC7_AO_RS_BIT)&SFC_VRC7_AO_MASK];
}



// 包络阶段
enum sfc_envelope_phase_state {
    SFC_VRC7_Idle = 0,
    SFC_VRC7_Attack,
    SFC_VRC7_Decay,
    SFC_VRC7_Sustain,
    SFC_VRC7_Release,
};




/// <summary>
/// VRC7: Audio 触发
/// </summary>
/// <remarks>
/// 敲下
/// - EGC=0
/// - 18bit的相位计数器归零
/// - 进入 Attack阶段
///
/// 弹起：
/// - 如果当前为 Attack阶段, EGC置为输出: EGC=AO(EGC)
/// - 进入 Release阶段
/// </remarks>
/// <param name="op">The op.</param>
/// <param name="changed">The changed.</param>
static inline void sfc_vrc7_audio_trigger(
    sfc_vrc7_operator_t* op,
    uint8_t changed, uint8_t trigger) {
    if (!changed) return;
    if (trigger) {
        op->egc = 0;
        op->phase = 0;
        op->state = SFC_VRC7_Attack;
    }
    else {
        if (op->state == SFC_VRC7_Attack)
            op->egc = sfc_vrc7_attack_output(op->egc);
        op->state = SFC_VRC7_Release;
    }
}



/// <summary>
/// StepFC: VRC7 ADSR包络处理
/// </summary>
/// <param name="op">The op.</param>
static uint32_t sfc_vrc7_envelope(sfc_vrc7_operator_t* op) {
    uint32_t rv = SFC_VRC7_AttenuationMax;
    switch (op->state)
    {
    case SFC_VRC7_Attack:
        rv = sfc_vrc7_attack_output(op->egc);
        op->egc += op->attack_rate;
        if (op->egc >= SFC_VRC7_AttenuationMax) {
            op->egc = 0;
            op->state = SFC_VRC7_Decay;
        }
        break;
    case SFC_VRC7_Decay:
        rv = op->egc;
        op->egc += op->decay_rate;
        if (op->egc >= op->sustain_level) {
            op->egc = op->sustain_level;
            op->state = SFC_VRC7_Sustain;
        }
        break;
    case SFC_VRC7_Sustain:
        rv = op->egc;
        op->egc += op->sustain_rate;
        if (op->egc >= SFC_VRC7_AttenuationMax) {
            op->egc = SFC_VRC7_AttenuationMax;
            op->state = SFC_VRC7_Idle;
        }
        break;
    case SFC_VRC7_Release:
        rv = op->egc;
        op->egc += op->release_rate;
        if (op->egc >= SFC_VRC7_AttenuationMax) {
            op->egc = SFC_VRC7_AttenuationMax;
            op->state = SFC_VRC7_Idle;
        }
        break;
    }
    return rv;
}


/// <summary>
/// StepFC: VRC7 计算Sustain输出值
/// </summary>
/// <param name="egc">The egc.</param>
/// <returns></returns>
static inline uint32_t sfc_vrc7_sustain_level(uint32_t egc) {
    const uint32_t unit = (uint32_t)(3.0 * SFC_VRC7_AttenuationUnit);
    return unit * egc;
}


/// <summary>
/// StepFC: VRC7 算子数据修改
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ch">The ch.</param>
/// <param name="op">The op.</param>
/// <param name="carrier">The carrier.</param>
static void sfc_vrc7_operator_changed(
    sfc_famicom_t* famicom,
    sfc_vrc7_ch_t* ch,
    sfc_vrc7_operator_t* op, uint8_t carrier) {
    /*
    $00/$01 MMMM  $0  $1  $2  $3  $4  $5  $6  $7  $8  $9  $A  $B  $C  $D  $E  $F
    Multiplier    1/2  1   2   3   4   5   6   7   8   9  10  10  12  12  15  15

    phase += F * (1<<O) * M * V / 2

    F = $2X:D0  $1X 组成的9bit数据
    O = $2X:D1-D3 3bit的八度信息
    M = $0/$1 经过查表的倍乘因子. 由于第一个是乘以二分之一, 可以考虑LUT预乘2, 最后再除以2
    V = vibrato(FM)输出. 如果vibrato=0, V=1. 后面说明AM/FM的情况
    */
    const uint8_t* const patch = sfc_get_vrc7_patch(famicom) + ch->instrument8;
    const uint8_t first = patch[carrier];
    const uint32_t phase_rate_x
        = (uint32_t)ch->freq
        * (uint32_t)(1 << ch->octave)
        * (uint32_t)sfc_vrc7_multi_lut[first&0xf]
        ;
    op->phase_rate_x4 = phase_rate_x;
    /*
    这里列出一些数据之后会用上:

     - OF: $2X的低四位, 由八度和频率最高位组成的4bit数据
     - K: $0$1的'key rate scaling'
     - KO: 'K ? OF : OF >> 2'
     - R: 基础速率, 每个阶段不同, $04-$08的数据
     - RKS: 'R*4 + KO'
     - RH: 'RKS >> 2', 超过15则被钳制到15
     - RL: 'RKS & 3'

     Attack 阶段:

     - 'R'是对应的 Attack 速率
     - 'EGC += (12 * (RL+4)) << RH'
     - 'EGC'超过23bit范围, 归零, 进入 Decay 阶段.
     -  Attack 阶段输出是: 'AO(EGC) = 48 dB - (48 dB * ln(EGC) / ln(1<<23))'

     Decay 阶段:

     - 'R'是对应的 Decay 速率
     - 'EGC += (RL+4) << (RH-1)'
     - 当EGC达到对应的 Sustain 值时, 进入 Sustain 阶段.
     - 具体是'EGC >= (3 * Sustain * (1<<23) / 48)'

     Sustain 阶段:

     - 如果' $0$1: sustain'为1, 'R'是对应的 Release 速率
     - 否则, 'R=0'
     - 'EGC += (RL+4) << (RH-1)'
     - 如果超过EGC本身范围, 进入 Idle 阶段
 
     Release 阶段:

     - '$2X:S'为1的话, 'R=5'
     - 另外, ' $0$1: sustain'为1的话, 'R'是对应的 Release 速率
     - 否则, 'R=7'
     - 'EGC += (RL+4) << (RH-1)'
     - 如果超过EGC本身范围, 进入 Idle 阶段


    $04  |  AAAA DDDD  |  Modulator attack  (A), decay   (D)
    $05  |  AAAA DDDD  |  Carrier   attack  (A), decay   (D)
    $06  |  SSSS RRRR  |  Modulator sustain (S), release (R)
    $07  |  SSSS RRRR  |  Carrier   sustain (S), release (R)
    */

    const uint32_t OF = ch->low4bit;
    const uint32_t KO = (first & 0x10) ? OF : OF >> 2;
    // Attack: (12 * (RL+4)) << RH
    if ((op->attack_rate = patch[4 | carrier] >> 4)) {
        const uint32_t R = op->attack_rate;
        const uint32_t RKS = R * 4 + KO;
        const uint32_t RH = RKS > 0x3f ? 0xf : RKS >> 2;
        const uint32_t RL = RKS & 3;
        op->attack_rate = (12 * (RL + 4)) << RH;
    }
    // Decay:  (RL+4) << (RH-1)
    if ((op->decay_rate = patch[4 | carrier] & 0xf)) {
        const uint32_t R = op->decay_rate;
        const uint32_t RKS = R * 4 + KO;
        const uint32_t RH = RKS > 0x3f ? 0xf : RKS >> 2;
        const uint32_t RL = RKS & 3;
        op->decay_rate = (RL + 4) << (RH - 1);
    }
    // Sustain: (RL+4) << (RH-1)
    if ((op->sustain_rate = (first & 0x20) ? 0 : patch[6 | carrier] & 0xf)) {
        const uint32_t R = op->sustain_rate;
        const uint32_t RKS = R * 4 + KO;
        const uint32_t RH = RKS > 0x3f ? 0xf : RKS >> 2;
        const uint32_t RL = RKS & 3;
        op->sustain_rate = (RL + 4) << (RH - 1);
    }
    // Release: (RL+4) << (RH-1)
    if ((op->release_rate = ch->sustain ? 5 : ((first & 0x20) ? 7 : patch[6 | carrier] & 0xf))){
        const uint32_t R = op->release_rate;
        const uint32_t RKS = R * 4 + KO;
        const uint32_t RH = RKS > 0x3f ? 0xf : RKS >> 2;
        const uint32_t RL = RKS & 3;
        op->release_rate = (RL + 4) << (RH - 1);
    }

    // 计算延音输出(Sustain level)
    op->sustain_level = sfc_vrc7_sustain_level(patch[6 | carrier] >> 4);

    // 第二个值
    const uint8_t sec = patch[2 | carrier];


    // 计算base值
    if (carrier) {
        // 3.00dB * V 
        const uint32_t unit = (uint32_t)(3.0 * SFC_VRC7_AttenuationUnit);
        op->base = unit * ch->volume;
    }
    else {
        // 0.75dB * L
        const uint32_t unit = (uint32_t)(0.75 * SFC_VRC7_AttenuationUnit);
        op->base = unit * (sec & 0x3f);
    }
    // 计算 key_scale

    /*
        F: 9bit的频率数据
        Oct: 八度

        IF K==0, THEN
          key_scale = 0
        ELSE
          A = table[ F >> 5 ] - 6 * (7-Oct)
          IF A < 0, THEN
            key_scale = 0
          ELSE
            key_scale = A >> (3-K)
          ENDIF
        ENDIF
    */
    const uint8_t key_scale_bits = sec >> 6;
    if (!key_scale_bits)
        op->key_scale = 0;
    else {
        const int32_t unit = (int32_t)(6.0 * SFC_VRC7_AttenuationUnit);
        const int32_t a = sfc_vrc7_ks_lut[ch->freq >> 5] - unit * (7 - (int32_t)ch->octave);
        if (a <= 0)
            op->key_scale = 0;
        else
            op->key_scale = (uint32_t)a >> (3 - key_scale_bits);

        //op->key_scale = ((uint32_t)a << key_scale_bits) >> 3;
    }

}



/// <summary>
/// StepFC: VRC7 算子输出
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ch">The ch.</param>
/// <param name="op">The op.</param>
/// <param name="in">The in.</param>
/// <param name="carrier">The carrier.</param>
/// <returns></returns>
static int32_t sfc_vrc7_operator_ouput(
    sfc_famicom_t* famicom,
    sfc_vrc7_ch_t* ch,
    sfc_vrc7_operator_t* op, 
    uint32_t adj,
    uint8_t carrier) {
    // Idle??
    if (op->state == SFC_VRC7_Idle) return 0;
    const uint8_t* const patch = sfc_get_vrc7_patch(famicom) + ch->instrument8;
    // 相位处理
    // phase += [F * (1 << B) * M] * V

    
    // 使用FM
    if (patch[carrier] & 0x40)
        op->phase += sfc_vrc7_fm_do(op->phase_rate_x4, famicom->apu.vrc7.fm_output);
    else
        op->phase += op->phase_rate_x4 >> 2;

    const uint32_t phase_secondary = op->phase + adj;

    // 基础输出
    // TOTAL = half_sine_table[I] + base + key_scale + envelope + AM

    const uint32_t envelope = sfc_vrc7_envelope(op);
    const uint32_t am = patch[carrier] & 0x80 ? famicom->apu.vrc7.am_output : 0;

    const uint32_t total
        = sfc_vrc7_half_sine(phase_secondary)
        + op->base
        + op->key_scale
        + envelope
        + am
        ;

    // 衰到家
    if (total >= SFC_VRC7_AttenuationMax) return 0;

    // 末尾处理
    /*
     1. 到这里, 每个算子的输出就计算好了. 不过这是一个衰减值, 需要转换成一个初步的线性输出值, 20bit.
     2. 根据'R'与'waveform'的值决定是否钳制为0.
     3. 还要通过一个滤波器, 不过很简单, 只需要和前一个输出做平均就行
     4. 输出
    */
    int32_t preliminary = sfc_vrc7_a2l(total);
    // R: 1 下半部分
    if (phase_secondary & (1 << 17)) {
        // ---Q W---: C(Q) M(W)
        if (patch[3] & (0x8 << carrier))
            preliminary = 0;
        else
            preliminary = -preliminary;
    }

    const int32_t prev_out = op->prev_output;
    op->prev_output = preliminary;
    return op->prev_final = (prev_out + preliminary) / 2;
}

/// <summary>
/// StepFC: VRC7 获取调制器的参数adj
/// </summary>
/// <param name="ch">The ch.</param>
/// <returns></returns>
static inline uint32_t sfc_vrc7_modulator_adj(sfc_famicom_t* famicom, sfc_vrc7_ch_t* ch) {
    const uint8_t* const patch = sfc_get_vrc7_patch(famicom) + ch->instrument8;
    // - 当F为0: adj=0
    // - 其他情况, adj = previous_output_of_modulator >> (8 - F)
    const uint8_t f = patch[3] & 7;
    return  f ? (uint32_t)ch->modulator.prev_final >> (8 - f) : 0;
}


/// <summary>
/// StepFC: VRC7 每周期(49716Hz)
/// </summary>
/// <param name="famicom">The famicom.</param>
int32_t sfc_vrc7_49716hz(sfc_famicom_t* famicom) {
    sfc_vrc7_data_t* const vrc7 = &famicom->apu.vrc7;
    // 计算AM/FM
    /*
        AM:
         - rate = 78
         - AM_output = (1.0 + sinx) * 0.6 dB   (emu2413 使用的是 1.2 dB)

        FM:
         - rate = 105
         - FM_output = 2 ^ (13.75 / 1200 * sinx)   
         - '^'是指指数运算
    */
    vrc7->am_phase += 78;
    vrc7->am_output = sfc_vrc7_am_calc(vrc7->am_phase);
    vrc7->fm_phase += 105;
    vrc7->fm_output = sfc_vrc7_fm_calc(vrc7->fm_phase);

    int32_t output = 0;
    // 处理所有声道
    for (int i = 0; i != 6; ++i) {
        sfc_vrc7_ch_t* const ch = vrc7->ch + i;
        const uint32_t ma = sfc_vrc7_modulator_adj(famicom, ch);
        const uint32_t mo = sfc_vrc7_operator_ouput(famicom, ch, &ch->modulator, ma, 0);
        const  int32_t co = sfc_vrc7_operator_ouput(famicom, ch, &ch->carrier, mo, 1);
        output += co;
    }
    return output;
}



// -----------------------------------------------------
//                   VRC7 Float Mode
// -----------------------------------------------------

