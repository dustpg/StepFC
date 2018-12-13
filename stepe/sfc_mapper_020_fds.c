#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include "sfc_mapper_helper.h"
#include <assert.h>
#include <string.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

// 内部处理模式(由于调制可能存在样本内部误差)
#define SFC_FDS_PER_SAMPLE

// ------------------------------- MAPPER 020 FDS

/// <summary>
/// The SFC modtbl lut
/// </summary>
static const int8_t sfc_modtbl_lut[] = {
    /*
        0 = %000 -->  0
        1 = %001 --> +1
        2 = %010 --> +2
        3 = %011 --> +4
        4 = %100 --> reset to 0
        5 = %101 --> -4
        6 = %110 --> -2
        7 = %111 --> -1
*/
    0, 2, 4, 8, 1, -8, -4, -2
};

/// <summary>
/// The SFC master vol lut
/// </summary>
static const uint8_t sfc_master_vol_lut[] = {
    // 2/2
    30,
    // 2/3
    20,
    // 2/4
    15,
    // 2/5
    12
};

/// <summary>
/// SFCs the FDS calculate envrate.
/// </summary>
/// <param name="env">The env.</param>
/// <param name="master">The master.</param>
/// <returns></returns>
static inline uint32_t sfc_fds1_calc_envtpc(uint32_t env, uint32_t master) {
    return 8 * (env + 1) * master;
}

/// <summary>
/// SFCs the FDS update volrate.
/// </summary>
/// <param name="famicom">The famicom.</param>
static inline void sfc_fds1_update_voltpc(sfc_famicom_t* famicom) {
    const uint32_t env = famicom->apu.fds.volenv_4080 & 0x3f;
    const uint32_t master = famicom->apu.fds.masenv_speed;
    famicom->apu.fds.volenv_tpc = sfc_fds1_calc_envtpc(env, master);
}

/// <summary>
/// SFCs the FDS update modrate.
/// </summary>
/// <param name="famicom">The famicom.</param>
static inline void sfc_fds1_update_modtpc(sfc_famicom_t* famicom) {
    const uint32_t env = famicom->apu.fds.modenv_4084 & 0x3f;
    const uint32_t master = famicom->apu.fds.masenv_speed;
    famicom->apu.fds.volenv_tpc = sfc_fds1_calc_envtpc(env, master);
}


/// <summary>
/// SFCs the FDS update volenv gain.
/// </summary>
/// <param name="famicom">The famicom.</param>
static inline void sfc_fds1_update_volenv_gain(sfc_famicom_t* famicom) {
    famicom->bus_memory[0x90] = famicom->apu.fds.volenv_gain | 0x40;
}

/// <summary>
/// SFCs the FDS update modenv gain.
/// </summary>
/// <param name="famicom">The famicom.</param>
static inline void sfc_fds1_update_modenv_gain(sfc_famicom_t* famicom) {
    famicom->bus_memory[0x92] = famicom->apu.fds.modenv_gain | 0x40;
}


/// <summary>
/// SFCs the FDS update freq gained.
/// </summary>
/// <param name="fds">The FDS.</param>
static inline void sfc_fds1_update_freq_gained(sfc_fds1_data_t* fds) {
    const int16_t gained = fds->freq + fds->freq_gain;
    fds->freq_gained = gained < 0 ? 0 : gained;
}

/// <summary>
/// StepFC: MAPPER 000 - NROM 重置
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern sfc_ecode sfc_mapper_00_reset(sfc_famicom_t* famicom);



/// <summary>
/// StepFC: FDS 获取调制单元频率增益
/// </summary>
/// <param name="pitch">$4082/4083 (12-bit unsigned pitch value)</param>
/// <param name="counter">$4085 (7-bit signed mod counter)</param>
/// <param name="gain">$4084 (6-bit unsigned mod gain)</param>
/// <seealso cref="https://wiki.nesdev.com/w/index.php/FDS_audio#Unit_tick"/>
/// <returns></returns>
static int16_t sfc_fds1_get_mod_pitch_gain(uint16_t pitch, int8_t counter, int8_t gain) {
    // pitch   = $4082/4083 (12-bit unsigned pitch value)
    // counter = $4085 (7-bit signed mod counter)
    // gain    = $4084 (6-bit unsigned mod gain)

    // 1. multiply counter by gain, lose lowest 4 bits of result but "round" in a strange way
    int16_t temp = counter * gain;
    uint8_t remainder = temp & 0xF;
    temp >>= 4;
    if ((remainder > 0) && ((temp & 0x80) == 0))
    {
        if (counter < 0) temp -= 1;
        else temp += 2;
    }

    // 2. wrap if a certain range is exceeded
    if (temp >= 192) temp -= 256;
    else if (temp < -64) temp += 256;

    // 3. multiply result by pitch, then round to nearest while dropping 6 bits
    temp = pitch * temp;
    remainder = temp & 0x3F;
    temp >>= 6;
    if (remainder >= 32) temp += 1;

    // final mod result is in temp
    return temp;
}



/// <summary>
/// StepFC: FDS Tick一次音量包络
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_fds1_tick_volenv(sfc_famicom_t* famicom) {
    sfc_fds1_data_t* const fds = &famicom->apu.fds;
    assert(fds->flags_4083 == 0);
    assert((fds->volenv_4080 & SFC_FDS_4080_GainMode) == 0);
    // 增
    if (fds->volenv_4080 & SFC_FDS_4080_Increase) {
        if (fds->volenv_gain < 32) {
            fds->volenv_gain++;
            fds->volenv_gain_clamped = fds->volenv_gain;
            sfc_fds1_update_volenv_gain(famicom);
        }
    }
    // 减
    else {
        if (fds->volenv_gain) {
            fds->volenv_gain--;
            fds->volenv_gain_clamped = fds->volenv_gain;
            sfc_fds1_update_volenv_gain(famicom);
        }
    }
}

/// <summary>
/// StepFC: FDS Tick一次调制包络
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_fds1_tick_modenv(sfc_famicom_t* famicom) {
    sfc_fds1_data_t* const fds = &famicom->apu.fds;
    assert(fds->flags_4083 == 0);
    assert((fds->modenv_4084 & SFC_FDS_4084_GainMode) == 0);
    // 增
    if (fds->modenv_4084 & SFC_FDS_4084_Increase) {
        if (fds->modenv_gain < 32) {
            fds->modenv_gain++;
            sfc_fds1_update_modenv_gain(famicom);
        }
    }
    // 减
    else {
        if (fds->modenv_gain) {
            fds->modenv_gain--;
            sfc_fds1_update_modenv_gain(famicom);
        }
    }
}


/// <summary>
/// StepFC: FDS Tick一次波输出
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_fds1_tick_waveout(sfc_famicom_t* famicom) {
    assert((famicom->apu.fds.flags_4083 & SFC_FDS_4083_HaltWave) == 0);
    const uint8_t* const table = sfc_get_fds1_wavtbl(famicom);
    const uint8_t value = table[famicom->apu.fds.wavtbl_index++] & 0x3f;
    if (!famicom->apu.fds.write_enable)
        famicom->apu.fds.waveout = value;
    famicom->apu.fds.wavtbl_index &= 0x3f;
}


/// <summary>
/// StepFC: FDS Tick一次调制单元
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_fds1_tick_modunit(sfc_famicom_t* famicom) {
    assert(famicom->apu.fds.mod_enabled);
    const uint8_t* const table = sfc_get_fds1_modtbl(famicom);
    sfc_fds1_data_t* const fds = &famicom->apu.fds;
    const int8_t value = table[famicom->apu.fds.modtbl_index++];
    fds->modtbl_index &= 0x3f;
    if (value & 1) fds->mod_counter_x2 = 0;
    else  fds->mod_counter_x2 += value;
    fds->freq_gain = sfc_fds1_get_mod_pitch_gain(
        fds->freq,
        fds->mod_counter_x2 / 2,
        fds->modenv_gain
    );
    sfc_fds1_update_freq_gained(fds);
}

/// <summary>
/// StepFC: FDS写入音频相关寄存器
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="address">The address.</param>
/// <param name="value">The value.</param>
void sfc_mapper_14_write_low(sfc_famicom_t* famicom, uint16_t address, uint8_t value) {
    // 前段数据
    if (address < 0x4040) {
        // Master I/O enable ($4023)
    }
    // 波形数据区
    if (address >= 0x4040 && address < 0x4080) {
        sfc_fds1_data_t* const fds = &famicom->apu.fds;
        if (fds->write_enable) {
            famicom->interfaces.audio_change(famicom->argument, famicom->cpu_cycle_count, SFC_FDS1_FDS1);
            sfc_get_fds1_wavtbl(famicom)[address & 0x3f] = (value & 0x3f) | 0x40;
            fds->wavtbl_index = 0;
            fds->waveout = sfc_get_fds1_wavtbl(famicom)[0] & 0x3f;
        }
    }
    // 后段数据
    else if (address <= 0x408a) {
        famicom->interfaces.audio_change(famicom->argument, famicom->cpu_cycle_count, SFC_FDS1_Wavefrom);
        sfc_fds1_data_t* const fds = &famicom->apu.fds;
        switch (address & 0xf)
        {
            int8_t modtblv;
        case 0:
            // Volume envelope ($4080)
            /*
                7  bit  0  (write; read through $4090)
                ---- ----
                MDVV VVVV
                |||| ||||
                ||++-++++- (M=0) Volume envelope speed
                ||         (M=1) Volume gain and envelope speed.
                |+-------- Volume change direction (0: decrease; 1: increase)
                +--------- Volume envelope mode (0: on; 1: off)
            */
            fds->volenv_4080 = value;
            if (value & SFC_FDS_4080_GainMode)
                fds->volenv_gain = value & 0x3f;
            fds->volenv_gain_clamped 
                = fds->volenv_gain > 32
                ? 32
                : fds->volenv_gain
                ;
            sfc_fds1_update_volenv_gain(famicom);
            sfc_fds1_update_voltpc(famicom);
            fds->volenv_clock = fds->volenv_tpc;
            break;
        case 2:
            // Frequency low ($4082)
            fds->freq
                = (fds->freq & 0xf00)
                | (uint16_t)value
                ;
            sfc_fds1_update_freq_gained(fds);
            break;
        case 3:
            // Frequency high ($4083)
            /*
                7  bit  0  (write)
                ---- ----
                MExx FFFF
                ||   ||||
                ||   ++++- Bits 8-11 of frequency
                |+-------- Disable volume and sweep envelopes (but not modulation)
                +--------- Halt waveform and reset phase to 0, disable envelopes
            */
            fds->freq
                = (fds->freq & 0xff)
                | (((uint16_t)value << 8) & 0xf00)
                ;
            fds->flags_4083 = value & 0xc0;
            if (!fds->freq) fds->flags_4083 |= SFC_FDS_4083_HaltWave;
            if (value & SFC_FDS_4083_HaltWave) {
                // 输出第一个波数据
                fds->wavout_clock = 0;
                fds->wavtbl_index = 0;
                fds->waveout = sfc_get_fds1_wavtbl(famicom)[0] & 0x3f;
            }
            // Bit 6 halts just the envelopes without halting the waveform, 
            // and also resets both of their timers.
            if (value & SFC_FDS_4083_DisableEnv) {
                fds->volenv_clock = fds->volenv_tpc;
                fds->modenv_clock = fds->modenv_tpc;
            }
            sfc_fds1_update_freq_gained(fds);
            break;
        case 4:
            // Mod envelope ($4084)
            fds->modenv_4084 = value;
            if (value & SFC_FDS_4084_GainMode)
                fds->modenv_gain = value & 0x3f;
            sfc_fds1_update_modenv_gain(famicom);
            sfc_fds1_update_modtpc(famicom);
            fds->modenv_clock = fds->modenv_tpc;
            break;
        case 5:
            // Mod counter ($4085)
            fds->mod_counter_x2 = (int8_t)(value << 1);
            break;
        case 6:
            // Mod frequency low ($4086)
            fds->mod_freq
                = (fds->mod_freq & 0xf00)
                | (uint16_t)value
                ;
            break;
        case 7:
            // Mod frequency high ($4087)
            /*
                7  bit  0  (write)
                ---- ----
                Dxxx FFFF
                |    ||||
                |    ++++- Bits 8-11 of modulation frequency
                +--------- Disable modulation
            */
            fds->mod_freq
                = (fds->mod_freq & 0xff)
                | (((uint16_t)value << 8) & 0xf00)
                ;
            fds->mod_enabled = ((~value) >> 7) & 1;
            if (!fds->mod_freq) fds->mod_enabled = 0;
            // 重置相关
            if (value & 0x80) {
                fds->mdunit_clock = 0;
                fds->freq_gain = 0;
            }
            sfc_fds1_update_freq_gained(fds);
            break;
        case 8:
            // Mod table write ($4088)
            if (!fds->mod_enabled) {
                modtblv = sfc_modtbl_lut[value & 7];
                sfc_get_fds1_modtbl(famicom)[fds->modtbl_index++] = modtblv;
                fds->modtbl_index &= 0x3f;
                sfc_get_fds1_modtbl(famicom)[fds->modtbl_index++] = modtblv;
                fds->modtbl_index &= 0x3f;
            }
            break;
        case 9:
            // Wave write / master volume ($4089)
            /*
                7  bit  0  (write)
                ---- ----
                Wxxx xxVV
                |      ||
                |      ++- Master volume (0: full; 1: 2/3; 2: 2/4; 3: 2/5)
                |          Output volume = current volume (see $4080 above) * master volume
                +--------- Wavetable write enable
                           (0: write protect RAM; 1: write enable RAM and hold channel)
            */
            fds->master_volume = sfc_master_vol_lut[value & 0x3];
            fds->write_enable = value >> 7;
            break;
        case 0xa:
            // Envelope speed ($408A)
            fds->masenv_speed = value;
            sfc_fds1_update_voltpc(famicom);
            sfc_fds1_update_modtpc(famicom);
            break;
        }
    }
}

/// <summary>
/// SFCs the FDS initialize.
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_fds1_init(sfc_famicom_t* famicom) {
    // 主包络速率: $E8
    famicom->apu.fds.masenv_speed = 0xE8;
    // 禁用位为1
    famicom->apu.fds.flags_4083 = 0xc0;
    famicom->apu.fds.volenv_4080 = 0xc0;
    famicom->apu.fds.modenv_4084 = 0xc0;
}

/// <summary>
/// SFCs the load mapper 01.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline sfc_ecode sfc_load_mapper_14(sfc_famicom_t* famicom) {
    // 初始化回调接口

    // 重置可以直接使用Mapper000的
    famicom->mapper.reset = sfc_mapper_00_reset;
    famicom->mapper.write_low = sfc_mapper_14_write_low;
    // TODO: FDS 支持, 相关初始化代码移至RESET

    // FDS 扩展音源
    famicom->rom_info.extra_sound = SFC_NSF_EX_FDS1;
    // 初始化FDS
    sfc_fds1_init(famicom);
    return SFC_ERROR_OK;
}


// ----------------------------------------------------------------
//                            输出
// ----------------------------------------------------------------
#include "sfc_play.h"


/// <summary>
/// SFCs the FDS get output u6.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
uint8_t sfc_fds1_get_output_u6(sfc_famicom_t* famicom) {
    const uint16_t volgain = famicom->apu.fds.volenv_gain_clamped;
    const uint16_t waveout = famicom->apu.fds.waveout;
    const uint16_t masterv = famicom->apu.fds.master_volume;
    const uint16_t fds_out = volgain * waveout * masterv;
    return (fds_out / SFC_FDS_MASTER_VOL_LCM) >> 5;
}

/// <summary>
/// StepFC: FDS高级接口[1] - 获取浮点输出
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="hold">The hold.</param>
/// <returns></returns>
float sfc_fds1_get_output(sfc_famicom_t* famicom) {
    const uint16_t volgain = famicom->apu.fds.volenv_gain_clamped;
    const uint16_t waveout = famicom->apu.fds.waveout;
    const uint16_t masterv = famicom->apu.fds.master_volume;
    const uint16_t fds_out = volgain * waveout * masterv;
    const double weight = 2.4 * 0.1494 / 63.0;
    const double factor = 1.0 / ((1 << 5) * SFC_FDS_MASTER_VOL_LCM / weight);
    return (float)fds_out * (float)factor;
}

#ifdef SFC_SMF_DEFINED

/// <summary>
/// StepFC: FDS高级接口[1] - 处理每个CPU周期
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
void sfc_fds1_per_cpu_clock(sfc_famicom_t* famicom) {
    sfc_fds1_data_t* const fds = &famicom->apu.fds;
    // 处理包络
    if (!fds->flags_4083) {
        // 音量包络
        if (!(fds->volenv_4080 & SFC_FDS_4080_GainMode)) {
            if (fds->volenv_clock) --fds->volenv_clock;
            else {
                fds->volenv_clock = fds->volenv_tpc;
                sfc_fds1_tick_volenv(famicom);
            }
        }
        // 调制包络
        if (!(fds->modenv_4084 & SFC_FDS_4084_GainMode)) {
            if (fds->modenv_clock) --fds->modenv_clock;
            else {
                fds->modenv_clock = fds->modenv_tpc;
                sfc_fds1_tick_modenv(famicom);
            }
        }
    }
    // 处理波输出
    if (!(fds->flags_4083 & SFC_FDS_4083_HaltWave)) {
        fds->wavout_clock += fds->freq_gained;
        if (fds->wavout_clock >> 16) {
            fds->wavout_clock &= 0xffff;
            sfc_fds1_tick_waveout(famicom);
        }
    }
    // 处理调制
    if (fds->mod_enabled) {
        fds->mdunit_clock += fds->mod_freq;
        if (fds->mdunit_clock >> 16) {
            fds->mdunit_clock &= 0xffff;
            sfc_fds1_tick_modunit(famicom);
        }
    }
}

/// <summary>
/// StepFC: FDS高级接口[2] - 输出每个样本
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
/// <param name="cps">clock per sample</param>
/// <returns></returns>
float sfc_fds1_per_sample(sfc_famicom_t* famicom, sfc_fds1_ctx_t* ctx, float cps) {
#ifdef SFC_FDS_PER_SAMPLE
    sfc_fds1_data_t* const fds = &famicom->apu.fds;
    // 处理包络
    if (!fds->flags_4083) {
        // 音量包络
        if (!(fds->volenv_4080 & SFC_FDS_4080_GainMode)) {
            ctx->volenv_clock -= cps;
            while (ctx->volenv_clock <= 0.f) {
                ctx->volenv_clock += ctx->volenv_tps;
                sfc_fds1_tick_volenv(famicom);
            }
        }
        // 调制包络
        if (!(fds->modenv_4084 & SFC_FDS_4084_GainMode)) {
            ctx->modenv_clock -= cps;
            while (ctx->modenv_clock <= 0.f) {
                ctx->modenv_clock += ctx->modenv_tps;
                sfc_fds1_tick_modenv(famicom);
            }
        }
    }
    float output = 0.f;
    float avgcount = 1.f;
    // 处理波输出
    if (!(fds->flags_4083 & SFC_FDS_4083_HaltWave)) {
        const float wavout_rate = cps * (float)fds->freq_gained;
        ctx->wavout_clock += wavout_rate;
        while (ctx->wavout_clock >= (float)0x10000) {
            ctx->wavout_clock -= (float)0x10000;
            ++avgcount;
            output += sfc_fds1_get_output(famicom);
            sfc_fds1_tick_waveout(famicom);
        }
    }
    // 处理调制
    if (fds->mod_enabled) {
        ctx->mdunit_clock += ctx->mdunit_rate * cps;
        while (ctx->mdunit_clock >= (float)0x10000) {
            ctx->mdunit_clock -= (float)0x10000;
            sfc_fds1_tick_modunit(famicom);
        }
    }
    output += sfc_fds1_get_output(famicom);
    return output / avgcount;
#else
    ctx->cycle_remain += cps;
    float out = 0.f;
    float count = 0.f;
    while (ctx->cycle_remain >= 1.f) {
        ctx->cycle_remain -= 1.f;
        sfc_fds1_per_cpu_clock(famicom);
        out += sfc_fds1_get_output(famicom);
        count++;
    }
    return out / count;
#endif
}

/// <summary>
/// StepFC: FDS高级接口[2] - 样本模式开始
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
void sfc_fds1_samplemode_begin(sfc_famicom_t* famicom, sfc_fds1_ctx_t* ctx) {
#ifdef SFC_FDS_PER_SAMPLE
    ctx->volenv_clock = (float)famicom->apu.fds.volenv_clock;
    ctx->volenv_tps   = (float)famicom->apu.fds.volenv_tpc;
    ctx->modenv_clock = (float)famicom->apu.fds.modenv_clock;
    ctx->modenv_tps   = (float)famicom->apu.fds.modenv_tpc;

    ctx->wavout_clock = (float)famicom->apu.fds.wavout_clock;
    ctx->mdunit_clock = (float)famicom->apu.fds.mdunit_clock;
    ctx->mdunit_rate  = (float)famicom->apu.fds.mod_freq;
#else
    ctx->cycle_remain = 0.f;
#endif
}

/// <summary>
/// StepFC: FDS高级接口[2] - 样本模式结束
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
void sfc_fds1_samplemode_end(sfc_famicom_t* famicom, sfc_fds1_ctx_t* ctx) {
#ifdef SFC_FDS_PER_SAMPLE
    famicom->apu.fds.volenv_clock = (uint32_t)ctx->volenv_clock;
    famicom->apu.fds.modenv_clock = (uint32_t)ctx->modenv_clock;
    famicom->apu.fds.wavout_clock = (uint32_t)ctx->wavout_clock;
    famicom->apu.fds.mdunit_clock = (uint32_t)ctx->mdunit_clock;
#else
    if (ctx->cycle_remain >= 0.5f)
        sfc_fds1_per_cpu_clock(famicom);
#endif
}


#endif

static sfc_fixed_t s_clock = 0;

/// <summary>
/// StepFC: FDS1 整型采样模式 - 采样
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
/// <param name="chw">The CHW.</param>
/// <param name="cps">The CPS.</param>
void sfc_fds1_smi_sample(sfc_famicom_t* famicom, sfc_fds1_smi_ctx_t* ctx, const float chw[], sfc_fixed_t cps) {
#ifdef SFC_FDS_PER_SAMPLE
    sfc_fds1_data_t* const fds = &famicom->apu.fds;
    // 处理包络
    if (!fds->flags_4083) {
        // 音量包络
        if (!(fds->volenv_4080 & SFC_FDS_4080_GainMode)) {
            const uint32_t clock = sfc_fixed_add(&ctx->volenv_clock, cps);
            // 每次不足就tick
            while (fds->volenv_clock <= clock) {
                fds->volenv_clock += fds->volenv_tpc;
                sfc_fds1_tick_volenv(famicom);
            }
            fds->volenv_clock -= clock;
        }
        // 调制包络
        if (!(fds->modenv_4084 & SFC_FDS_4084_GainMode)) {
            const uint32_t clock = sfc_fixed_add(&ctx->modenv_clock, cps);
            // 每次不足就tick
            while (fds->modenv_clock <= clock) {
                fds->modenv_clock += fds->modenv_tpc;
                sfc_fds1_tick_modenv(famicom);
            }
            fds->modenv_clock -= clock;
        }
    }
    //float output = 0.f;
    //float avgcount = 1.f;
    // 处理波输出
    if (!(fds->flags_4083 & SFC_FDS_4083_HaltWave)) {
        const uint32_t clock = sfc_fixed_add(&ctx->wavout_clock, cps);

        const uint32_t rate = clock * fds->freq_gained;
        fds->wavout_clock += rate;

        while (fds->wavout_clock >= 0x10000) {
            fds->wavout_clock -= 0x10000;
            //++avgcount;
            //output += sfc_fds1_get_output(famicom);
            sfc_fds1_tick_waveout(famicom);
        }
    }
    // 处理调制
    if (fds->mod_enabled) {
        const uint32_t clock = sfc_fixed_add(&ctx->mdunit_clock, cps);

        const uint32_t rate = clock * famicom->apu.fds.mod_freq;
        fds->mdunit_clock += rate;
        while (fds->mdunit_clock >= 0x10000) {
            fds->mdunit_clock -= 0x10000;
            sfc_fds1_tick_modunit(famicom);
        }
    }
    // 输出
    //output += sfc_fds1_get_output(famicom);
    //ctx->output =  output * chw[0] / avgcount;

    ctx->output = (float)sfc_fds1_get_output_u6(famicom);
    ctx->output *= chw[0];


#else
    assert(!"NOT IMPL");
#endif
}


/// <summary>
/// StepFC: FDS更新波表
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="table">The table.</param>
void sfc_fds1_update_wavetable(sfc_famicom_t* famicom, float* const out) {
    const uint8_t* const table = sfc_get_fds1_wavtbl(famicom);
    for (int i = 0; i != 64; ++i) out[i] = (float)(table[i] & 0x3f);
}