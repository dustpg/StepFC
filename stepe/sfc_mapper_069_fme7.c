#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include "sfc_mapper_helper.h"
#include <assert.h>
#include <string.h>

#ifndef NDEBUG
#include <stdio.h>
#endif


// ------------------------------- MAPPER 069 - Sunsoft FME-7
//                                            - Sunsoft 5A
//                                            - Sunsoft 5B


typedef struct {
    // IRQ 计数器
    uint16_t    irq_counter;
    // IRQ 控制码
    uint8_t     irq_control;
    // 命令码
    uint8_t     cmd_code;
} sfc_mapper45_t;


enum {
    SFC_FME7_IRQEnable          = 0x01,
    SFC_FME7_IRQCounterEnable   = 0x80,
};

/// <summary>
/// SFCs the mapper.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline sfc_mapper45_t* sfc_mapper(sfc_famicom_t* famicom) {
    return (sfc_mapper45_t*)famicom->mapper_buffer.mapper45;
}

#define MAPPER sfc_mapper45_t* const mapper = sfc_mapper(famicom);

// IRQ - 中断请求 - 确认
extern inline void sfc_operation_IRQ_acknowledge(sfc_famicom_t* famicom);
// 尝试触发
extern inline void sfc_operation_IRQ_try(sfc_famicom_t* famicom);


/// <summary>
/// SFCs the fme7 load chrrom.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="id">The identifier.</param>
/// <param name="bank">The bank.</param>
static inline void sfc_fme7_load_chrrom(sfc_famicom_t* famicom, uint8_t id, uint8_t bank) {
    const uint8_t banks = (famicom->rom_info.size_chrrom >> 10) - 1;
    // TODO: 非二次幂的情况
    sfc_load_chrrom_1k(famicom, id, bank & banks);
}

/// <summary>
/// SFCs the fme load prgbank0.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="value">The value.</param>
static inline void sfc_fme_load_prgbank0(sfc_famicom_t* famicom, uint8_t value) {
    /*
    7  bit  0
    ---- ----
    ERbB BBBB
    |||| ||||
    ||++-++++- The bank number to select at CPU $6000 - $7FFF
    |+------- RAM / ROM Select Bit
    |         0 = PRG ROM
    |         1 = PRG RAM
    +-------- RAM Enable Bit (6264 +CE line)
              0 = PRG RAM Disabled
              1 = PRG RAM Enabled
    */

    // TODO: PRG-RAM 写入保护
    
    // RAM
    if (value & 0x40) {
        // TODO: 512kb PRG-RAM
        const uint8_t bank = value & 0x3f;
    }
    // ROM
    else {
        // TODO: PRG-ROM 写入保护
        const uint8_t bank = value & 0x3f;
        const uint8_t banks = (famicom->rom_info.size_prgrom >> 13);
        sfc_load_prgrom_8kpt(famicom, 3, bank % banks);
    }
}

/// <summary>
/// SFCs the fme7 load chrrom.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="id">The identifier.</param>
/// <param name="bank">The bank.</param>
static inline void sfc_fme7_load_prgrom(sfc_famicom_t* famicom, uint8_t id, uint8_t bank) {
    // TODO: 非二次幂的情况
    const uint8_t banks = (famicom->rom_info.size_prgrom >> 13);
    sfc_load_prgrom_8k(famicom, id, bank % banks);
}


/// <summary>
/// StepFC: FME-7 更新名称表镜像模式
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="id">The identifier.</param>
static inline void sfc_fme7_update_ntm(sfc_famicom_t* famicom, uint8_t id) {
    sfc_nametable_mirroring_mode mode;
    // 0 = [2]Vertical
    // 1 = [3]Horizontal
    // 2 = [0]One Screen Mirroring from $2000("1ScA")
    // 3 = [1]One Screen Mirroring from $2400("1ScB")
    mode = id & 3;
    mode ^= 2;
    sfc_switch_nametable_mirroring(famicom, mode);
}

#ifndef NDEBUG
extern uint16_t dbg_scanline;
#endif

/// <summary>
/// StepFC: FME-7 更新命令
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_fme7_update_command(sfc_famicom_t* famicom, uint8_t parameter) {
    MAPPER;
    uint8_t code; 
    switch (code = mapper->cmd_code & 0xf)
    {
    case 0x0:
    case 0x1:
    case 0x2:
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
        // CHR Bank 0-7 ($0-7)
        sfc_fme7_load_chrrom(famicom, code, parameter);
        break;
    case 0x8:
        // PRG Bank 0 ($8)
#ifndef NDEBUG
        //printf("FME-7:  PRG Bank 0 ($8) = %02x\n", parameter);
#endif
        sfc_fme_load_prgbank0(famicom, parameter);
        break;
    case 0x9:
    case 0xA:
    case 0xB:
        // PRG Bank 1-3 ($9-B)
        sfc_fme7_load_prgrom(famicom, code - 9, parameter & 0x3f);
        break;
    case 0xC:
        // Name Table Mirroring ($C)
        sfc_fme7_update_ntm(famicom, parameter);
        break;
    case 0xD:
        // IRQ Control ($D)
        mapper->irq_control = parameter;
        sfc_operation_IRQ_acknowledge(famicom);
        break;
    case 0xE:
        // IRQ Counter Low Byte ($E)
        mapper->irq_counter = (mapper->irq_counter & (uint16_t)0xFF00) | (uint16_t)parameter;
        break;
    case 0xF:
        // IRQ Counter High Byte ($F)
        mapper->irq_counter = (mapper->irq_counter & (uint16_t)0x00FF) | ((uint16_t)parameter << 8);
        break;
    }
}

// 写入YM2149F内部寄存器
static void sfc_fme7_ym2149f_write(sfc_famicom_t* famicom, uint8_t value);

/// <summary>
/// SFCs the mapper 45 write high.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="address">The address.</param>
/// <param name="value">The value.</param>
void sfc_mapper_45_write_high(sfc_famicom_t* famicom, uint16_t address, uint8_t value) {
    MAPPER;
    switch ((address >> 13) & 3)
    {
    case 0:
        // Command Register ($8000-$9FFF)
        //printf("command = %x\n", value);
        mapper->cmd_code = value;
        break;
    case 1:
        // Parameter Register ($A000-$BFFF)
        //printf("value = %02x\n", value);
        sfc_fme7_update_command(famicom, value);
        break;
    case 2:
        // Audio Register Select ($C000-$DFFF)
        famicom->apu.fme7.ch[0].a0_reg_select = value & 0xf;
        break;
    case 3:
        // Audio Register Write ($E000-$FFFF)
        sfc_fme7_ym2149f_write(famicom, value);
        break;
    }
}

//void sfc_fme7_per_cycle(sfc_famicom_t* famicom) {
//    MAPPER;
//    if (!(mapper->irq_control & SFC_FME7_IRQCounterEnable)) return;
//    --mapper->irq_counter;
//    if (!mapper->irq_counter && (mapper->irq_control & SFC_FME7_IRQEnable)) {
//        sfc_operation_IRQ_try(famicom);
//    }
//}
//
//void sfc_fme7_cycle(sfc_famicom_t* famicom, uint32_t cycle_add) {
//    for (uint32_t i = 0; i != cycle_add; ++i)
//        sfc_fme7_per_cycle(famicom);
//}


/// <summary>
/// SFCs the mapper 45 hsync.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="line">The line.</param>
static void sfc_mapper_45_hsync(sfc_famicom_t* famicom, uint16_t line) {
#ifndef NDEBUG
    dbg_scanline = line;
#endif
    MAPPER;
    if (mapper->irq_control & SFC_FME7_IRQCounterEnable) {
        // 尝试触发
        if (mapper->irq_control & SFC_FME7_IRQEnable) {
            // 提前半行(四舍五入)触发
            const uint32_t counter = 113 * 3 / 2;
            if (mapper->irq_counter <= counter) {
#ifndef NDEBUG
                //printf("[%5d][FME-7] IRQ@ scanline: %3d\n", famicom->frame_counter, line);
#endif
                sfc_operation_IRQ_try(famicom);
                mapper->irq_counter = 0xffff;
                return;
            }
        }
        // 每根扫描线可以视为113.66周期
        const uint16_t count = (line % 3) ? 114 : 113;
        mapper->irq_counter -= count;
    }
}


/// <summary>
/// SFCs the fme7 initialize.
/// </summary>
/// <param name="famicom">The famicom.</param>
extern inline void sfc_fme7_init(sfc_famicom_t* famicom) {
    famicom->apu.fme7.lfsr = 1;
    famicom->apu.fme7.noise_period = 0x1f;
    famicom->apu.fme7.env_period = 0x0fff;
    famicom->apu.fme7.ch[0].period = 0xfff;
    famicom->apu.fme7.ch[1].period = 0xfff;
    famicom->apu.fme7.ch[1].period = 0xfff;
    famicom->apu.fme7.evn_shape = 0x80;
    famicom->apu.fme7.evn_index = 0;

    famicom->apu.fme7.ch[0].cpu_period_step = 0xffff;
    famicom->apu.fme7.ch[1].cpu_period_step = 0xffff;
    famicom->apu.fme7.ch[2].cpu_period_step = 0xffff;
    famicom->apu.fme7.noise_period_step = 0x2ff;
    famicom->apu.fme7.env_period_step = 0xffff;
}

/// <summary>
/// SFCs the mapper 15 reset.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static sfc_ecode sfc_mapper_45_reset(sfc_famicom_t* famicom) {
    MAPPER;
    // 支持FME-7
    famicom->rom_info.extra_sound = SFC_NSF_EX_FME7;
    sfc_fme7_init(famicom);
    // 载入最后的BANK
    const uint32_t count_prgrom8kb = famicom->rom_info.size_prgrom >> 13;
    //sfc_load_prgrom_8k(famicom, 0, count_prgrom8kb - 4);
    //sfc_load_prgrom_8k(famicom, 1, count_prgrom8kb - 3);
    //sfc_load_prgrom_8k(famicom, 2, count_prgrom8kb - 2);
    sfc_load_prgrom_8k(famicom, 3, count_prgrom8kb - 1);

    for (int i = 0; i != 8; ++i)
        sfc_load_chrrom_1k(famicom, i, i);

    return SFC_ERROR_OK;
}


/// <summary>
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline sfc_ecode sfc_load_mapper_45(sfc_famicom_t* famicom) {
    enum { MAPPER_45_SIZE_IMPL = sizeof(sfc_mapper45_t) };
    static_assert(SFC_MAPPER_45_SIZE == MAPPER_45_SIZE_IMPL, "SAME");
    // 初始化回调接口
    famicom->mapper.reset = sfc_mapper_45_reset;
    famicom->mapper.hsync = sfc_mapper_45_hsync;
    //famicom->mapper.read_low = sfc_mapper_45_read_low;
    //famicom->mapper.write_low = sfc_mapper_45_write_low;
    famicom->mapper.write_high = sfc_mapper_45_write_high;
    //famicom->mapper.write_ram_to_stream = sfc_mapper_45_write_ram;
    //famicom->mapper.read_ram_from_stream = sfc_mapper_45_read_ram;
    // 初始化数据
    //MAPPER;
    //memset(mapper, 0, MAPPER_45_SIZE_IMPL);
    return SFC_ERROR_OK;
}

// ----------------------------------------------------------------------------
//                                   YM2149F 
// ----------------------------------------------------------------------------

#include "sfc_play.h"
#include "stdbool.h"

static uint16_t sfc_fme7_env_lut[32*4];
static uint16_t sfc_fme7_vol_lut[16];


enum { SFC_FME7_CH_MAX = 0x2000, SFC_FME7_CH3_MAX = SFC_FME7_CH_MAX * 3 };

/// <summary>
/// 初始化FME-7用LUT
/// </summary>
extern void sfc_fme7_init_lut(void) {
    double table[32];
    // 1 / 2^(0.25)
    const double qqrt2 = 0.84089641525;
    table[0] = 0;
    table[31] = SFC_FME7_CH_MAX;
    for (int i = 0; i != 30; ++i)
        table[30 - i] = table[31 - i] * qqrt2;
    uint16_t* const table_as_u16 = (uint16_t*)table;
    // 变成U16
    for (int i = 0; i != 32; ++i)
        table_as_u16[i] = (uint16_t)(table[i] + 0.5);
    // 建立LUT-A
    sfc_fme7_vol_lut[0] = 0;
    for (int i = 1; i != 16; ++i) {
        sfc_fme7_vol_lut[i] = table_as_u16[i * 2 + 1];
    }
    // 建立LUT-B 0->0
    for (int i = 0; i != 32; ++i) 
        sfc_fme7_env_lut[i] = 0;
    // 建立LUT-B 0->1
    for (int i = 0; i != 32; ++i)
        sfc_fme7_env_lut[i+32] = table_as_u16[i];
    // 建立LUT-B 1->0
    for (int i = 0; i != 32; ++i)
        sfc_fme7_env_lut[i+64] = table_as_u16[31 - i];
    // 建立LUT-B 1->1
    for (int i = 0; i != 32; ++i)
        sfc_fme7_env_lut[i+96] = SFC_FME7_CH_MAX;
}


/*
   值  |  续 |  起  | 换  |持|    形状
-------|-----|-----|-----|--|------------
$00-$03|  0  |  0  |  x  |x |   \_______
$04-$07|  0  |  1  |  x  |x |   /_______
$08    |  1  |  0  |  0  |0 |   \\\\\\\\
$09    |  1  |  0  |  0  |1 |   \_______
$0A    |  1  |  0  |  1  |0 |   \/\/\/\/
$0B    |  1  |  0  |  1  |1 |   \¯¯¯¯¯¯¯
$0C    |  1  |  1  |  0  |0 |   ////////
$0D    |  1  |  1  |  0  |1 |   /¯¯¯¯¯¯¯
$0E    |  1  |  1  |  1  |0 |   /\/\/\/\
$0F    |  1  |  1  |  1  |1 |   /_______
*/


// 5B用 包络形状
#define FME7_SHAPE(a, b, c, d, e, f, g, h) \
 (a<<7) | (b<<6) | (c<<5) | (d<<4) | (e<<3) | (f<<2) | (g<<1) | (h<<0)


/// <summary>
/// FME-7 包络形状查找表
/// </summary>
static const uint8_t sfc_fme7_evn_shape_lut[] = {
    // $00 \___
    FME7_SHAPE(1, 0, 0, 0, 0, 0, 0, 0),
    // $01 \___
    FME7_SHAPE(1, 0, 0, 0, 0, 0, 0, 0),
    // $02 \___
    FME7_SHAPE(1, 0, 0, 0, 0, 0, 0, 0),
    // $03 \___
    FME7_SHAPE(1, 0, 0, 0, 0, 0, 0, 0),
    // $04 /___
    FME7_SHAPE(0, 1, 0, 0, 0, 0, 0, 0),
    // $05 /___
    FME7_SHAPE(0, 1, 0, 0, 0, 0, 0, 0),
    // $06 /___
    FME7_SHAPE(0, 1, 0, 0, 0, 0, 0, 0),
    // $07 /___
    FME7_SHAPE(0, 1, 0, 0, 0, 0, 0, 0),
    // $08 \\\\ 
    FME7_SHAPE(1, 0, 1, 0, 1, 0, 1, 0),
    // $09 \___
    FME7_SHAPE(1, 0, 0, 0, 0, 0, 0, 0),
    // $0A \/\/
    FME7_SHAPE(1, 0, 0, 1, 1, 0, 0, 1),
    // $0B \¯¯¯
    FME7_SHAPE(1, 0, 1, 1, 1, 1, 1, 1),
    // $0C ////
    FME7_SHAPE(0, 1, 0, 1, 0, 1, 0, 1),
    // $0D /¯¯¯
    FME7_SHAPE(0, 1, 1, 1, 1, 1, 1, 1),
    // $0E /\/\ 
    FME7_SHAPE(0, 1, 1, 0, 0, 1, 1, 0),
    // $0F /__
    FME7_SHAPE(0, 1, 0, 0, 0, 0, 0, 0),
};


/// <summary>
/// StepFC: FME7 声道事件
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ch">The ch.</param>
static inline void sfc_fme7_ch_change(sfc_famicom_t* famicom, uint8_t ch) {
    famicom->interfaces.audio_change(
        famicom->argument, 
        famicom->cpu_cycle_count, 
        SFC_FME7_ChannelA + ch
    );
}


/// <summary>
/// StepFC: FME7 音频事件
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ch">The ch.</param>
static inline void sfc_fme7_audio_change(sfc_famicom_t* famicom) {
    famicom->interfaces.audio_change(
        famicom->argument,
        famicom->cpu_cycle_count,
        SFC_FME7_Sun5B
    );
}

/// <summary>
/// SFCs the fme7 make ch bool.
/// </summary>
/// <param name="ch">The ch.</param>
/// <param name="flag">The flag.</param>
static inline void sfc_fme7_make_ch_bool(sfc_sunsoft5b_ch_t* ch, uint32_t flag) {
    // --CB Acba 
    // Noise disable on channels C/B/A, Tone disable on channels c/b/a
    const uint8_t flag_r = ~flag;
    ch->tone = flag_r & 1;
    ch->noise = (flag_r >> 3) & 1;
    ch->disable = ~(ch->tone | ch->noise) & 1;
}

/// <summary>
/// StepFC: FME-7 写入YM2149F内部寄存器
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="value">The value.</param>
static void sfc_fme7_ym2149f_write(sfc_famicom_t* famicom, uint8_t value) {
    sfc_fme7_data_t* const fme7 = &famicom->apu.fme7;
    const uint8_t selected = fme7->ch[0].a0_reg_select;
    switch (selected)
    {
        sfc_sunsoft5b_ch_t* ch;
    case 0x0:
    case 0x2:
    case 0x4:
        // Channel A/B/C low period
        sfc_fme7_ch_change(famicom, selected >> 1);
        ch = &fme7->ch[selected >> 1];
        ch->period = (ch->period & 0xf00) | (uint16_t)value;
        // 转换成CPU周期(STEP)
        ch->cpu_period_step = (ch->period ? ch->period : 1) * 16;
        break;
    case 0x1:
    case 0x3:
    case 0x5:
        // Channel A/B/C high period
        sfc_fme7_ch_change(famicom, selected >> 1);
        ch = &fme7->ch[selected >> 1];
        ch->period = (ch->period & 0xff) | ((uint16_t)(value & 0xf) << 8);
        // 转换成CPU周期(STEP)
        ch->cpu_period_step = (ch->period ? ch->period : 1) * 16;
        break;
    case 0x6:
        // Noise period
        sfc_fme7_audio_change(famicom);
        fme7->noise_period = value & 0x1f;
        // 噪音  Clock / (2 * 16 * Period)
        fme7->noise_period_step = (fme7->noise_period ? fme7->noise_period : 1) * 32;
        break;
    case 0x7:
        // Noise disable/ Tone disable
        // --CB Acba
        sfc_fme7_audio_change(famicom);
        sfc_fme7_make_ch_bool(fme7->ch + 0, value >> 0);
        sfc_fme7_make_ch_bool(fme7->ch + 1, value >> 1);
        sfc_fme7_make_ch_bool(fme7->ch + 2, value >> 2);
        break;
    case 0x8:
    case 0x9:
    case 0xA:
        // Channel A/B/C envelope enable (E), volume (V)
        sfc_fme7_ch_change(famicom, selected - 8);
        ch = &fme7->ch[selected - 8];
        ch->volume = sfc_fme7_vol_lut[ch->vol_raw = value & 0xf];
        ch->env = value & 0x10;
        break;
    case 0xB:
        // Envelope low period
        sfc_fme7_audio_change(famicom);
        fme7->env_period
            = (fme7->env_period & (uint16_t)0xff00)
            | (uint16_t)value
            ;
        // 包络  Clock / (2 * 256 * Period), 每步/32
        fme7->env_period_step = (fme7->env_period ? fme7->env_period : 1) * 16;
        break;
    case 0xC:
        // Envelope high period
        sfc_fme7_audio_change(famicom);
        fme7->env_period
            = (fme7->env_period & (uint16_t)0x00ff)
            | ((uint16_t)value << 8)
            ;
        // 包络  Clock / (2 * 256 * Period), 每步/32
        fme7->env_period_step = (fme7->env_period ? fme7->env_period : 1) * 16;
        break;
    case 0xD:
        // Envelope reset and shape: ---- CAaH
        sfc_fme7_audio_change(famicom);
        fme7->evn_shape = sfc_fme7_evn_shape_lut[value & 0xf];
        fme7->evn_index = 0;
        fme7->evn_repeat = 0;
        break;
    }
}


/// <summary>
/// StepFC: YM2149F 触发噪音
/// </summary>
/// <param name="famicom">The famicom.</param>
static inline void sfc_ym2149f_tick_noise(sfc_famicom_t* famicom) {
    famicom->apu.fme7.lfsr = sfc_lfsr_fme7(famicom->apu.fme7.lfsr);
}


/// <summary>
/// StepFC: YM2149F 触发包络
/// </summary>
/// <param name="famicom">The famicom.</param>
static inline void sfc_ym2149f_tick_evn_times(sfc_famicom_t* famicom, uint8_t times) {
    // 不能太大
    //assert(times < 64);
    sfc_fme7_data_t* const fme7 = &famicom->apu.fme7;
    fme7->evn_index += times;
    fme7->evn_repeat |= fme7->evn_index & 64;
    fme7->evn_index &= 63;
    fme7->evn_index |= fme7->evn_repeat;
}

/// <summary>
/// StepFC: YM2149F 获取包络音量
/// </summary>
/// <param name="shape">The shape.</param>
/// <param name="index">The index.</param>
static inline uint16_t sfc_ym2149f_get_evn_value(uint8_t shape, uint8_t index) {
    const uint8_t shift = (index & 0x60) >> 4;
    shape <<= shift;
    shape &= 0xc0;
    index &= 0x1f;
    index <<= 1;
    const uint8_t* const lut = (const uint8_t*)sfc_fme7_env_lut;
    const uint8_t* const data = &lut[shape | index];
    return *(uint16_t*)data;
}




#ifdef SFC_SMF_DEFINED

/// <summary>
/// StepFC: FME7 采样模式 - 输出每个样本
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
/// <param name="cps">The CPS.</param>
/// <returns></returns>
float sfc_fme7_per_sample(sfc_famicom_t* famicom, sfc_fme7_ctx_t* ctx, float cps) {
    sfc_fme7_data_t* const fme7 = &famicom->apu.fme7;
    // 声道
    for (int i = 0; i != 3; ++i) {
        ctx->ch[i].clock += cps;
        const uint8_t count = (uint8_t)(ctx->ch[i].clock / ctx->ch[i].period);
        fme7->ch[i].square ^= count;
        ctx->ch[i].clock -= (float)count * ctx->ch[i].period;
    }
    // 噪音
    ctx->noise_clock += cps;
    while (ctx->noise_clock >= ctx->noise_period) {
        ctx->noise_clock -= ctx->noise_period;
        sfc_ym2149f_tick_noise(famicom);
    }
    // 包络
    ctx->env_clock += cps;
    const uint8_t count = (uint8_t)(ctx->env_clock / ctx->env_period);
    ctx->env_clock -= (float)count * ctx->env_period;
    sfc_ym2149f_tick_evn_times(famicom, count);
    fme7->env_volume = sfc_ym2149f_get_evn_value(fme7->evn_shape, fme7->evn_index);
    // 合并输出
    uint16_t output = 0;
    const uint8_t lfsr = fme7->lfsr & 1;
    for (int i = 0; i != 3; ++i) {
        const uint8_t disable = fme7->ch[i].disable;
        const uint8_t square  = fme7->ch[i].square;
        const uint8_t noise   = fme7->ch[i].noise;
        const uint8_t tone    = fme7->ch[i].tone;
        // 输出
        bool flag;
        if (tone & noise) flag = square & lfsr & 1;
        else flag = disable | (tone & square) | (noise & lfsr);
        // 检测
        if (flag) output += fme7->ch[i].env ? fme7->env_volume : fme7->ch[i].volume;
    }
    return (float)output / (float)SFC_FME7_CH3_MAX;
}


/// <summary>
/// StepFC: FME7 采样模式 - 开始
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
void sfc_fme7_samplemode_begin(sfc_famicom_t* famicom, sfc_fme7_ctx_t* ctx) {
    sfc_fme7_data_t* const fme7 = &famicom->apu.fme7;
    //  5B使用了一个倍分频器, 所以多除以了2

    // 音调  Clock / (2 * 16 * Period), 修改频率加倍
    for (int i = 0; i != 3; ++i) {
        ctx->ch[i].clock = (float)fme7->ch[i].cpu_clock;
        const uint16_t period = fme7->ch[i].period;
        ctx->ch[i].period = (float)fme7->ch[i].cpu_period_step;
    }
    // 噪音  Clock / (2 * 16 * Period)
    ctx->noise_period = (float)fme7->noise_period_step;
    ctx->noise_clock = (float)fme7->noise_clock;
    // 包络  Clock / (2 * 256 * Period), 每步/32
    ctx->env_period = (float)fme7->env_period_step;
    ctx->env_clock = (float)fme7->env_clock;

}

/// <summary>
/// StepFC: FME7 采样模式 - 结束
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
void sfc_fme7_samplemode_end(sfc_famicom_t* famicom, sfc_fme7_ctx_t* ctx) {
    sfc_fme7_data_t* const fme7 = &famicom->apu.fme7;
    // 音调
    fme7->ch[0].cpu_clock = (uint16_t)ctx->ch[0].clock;
    fme7->ch[1].cpu_clock = (uint16_t)ctx->ch[1].clock;
    fme7->ch[2].cpu_clock = (uint16_t)ctx->ch[2].clock;
    // 噪音
    fme7->noise_clock = (uint16_t)ctx->noise_clock;
    // 包络
    fme7->env_clock = (uint32_t)ctx->env_clock;

}

#endif



/// <summary>
/// StepFC: FME7 整型采样模式 - 每个样本
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
/// <param name="chw">The CHW.</param>
/// <param name="cps">The CPS.</param>
/// <returns></returns>
void sfc_fme7_smi_sample(sfc_famicom_t* famicom, sfc_fme7_smi_ctx_t* ctx, const float chw[], sfc_fixed_t cps) {
    sfc_fme7_data_t* const fme7 = &famicom->apu.fme7;
    // 声道
    for (int i = 0; i != 3; ++i) {
        const uint16_t clock = sfc_fixed_add(ctx->chn_clock + i, cps);
        sfc_sunsoft5b_ch_t* const ch = fme7->ch + i;
        ch->cpu_clock += clock;

        const uint8_t count = ch->cpu_clock / ch->cpu_period_step;
        ch->square ^= count;
        ch->cpu_clock -= count * ch->cpu_period_step;
    }

    // 噪音
    fme7->noise_clock += sfc_fixed_add(&ctx->noi_clock, cps);

    while (fme7->noise_clock >= fme7->noise_period_step) {
        fme7->noise_clock -= fme7->noise_period_step;
        sfc_ym2149f_tick_noise(famicom);
    }
    // 包络
    fme7->env_clock += sfc_fixed_add(&ctx->env_clock, cps);

    const uint32_t count = fme7->env_clock / fme7->env_period_step;
    fme7->env_clock -= count * fme7->env_period_step;
    sfc_ym2149f_tick_evn_times(famicom, (uint8_t)count);
    fme7->env_volume = sfc_ym2149f_get_evn_value(fme7->evn_shape, fme7->evn_index);

    // 合并输出
    const uint8_t lfsr = fme7->lfsr & 1;
    for (int i = 0; i != 3; ++i) {
        const uint8_t disable = fme7->ch[i].disable;
        const uint8_t square = fme7->ch[i].square;
        const uint8_t noise = fme7->ch[i].noise;
        const uint8_t tone = fme7->ch[i].tone;
        // 输出
        bool flag;
        if (tone & noise) flag = square & lfsr & 1;
        else flag = disable | (tone & square) | (noise & lfsr);
        // 检测
        float output = 0.f;
        if (flag) {
            const uint16_t cho = fme7->ch[i].env ? fme7->env_volume : fme7->ch[i].volume;
            output = (float)cho / (float)SFC_FME7_CH3_MAX * chw[i];
        }
        ctx->output[i] = output;
    }
}
