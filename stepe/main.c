#include "sfc_famicom.h"
#include "sfc_cpu.h"
#include "sfc_play.h"
#include "../common/d2d_interface3.h"
#include "../common/xa2_interface1.h"
#include "sfc_audio_filter.h"
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    NTSC_CPU_RATE = 1789773,
    //SAMPLES_PER_SEC = 44100,
    SAMPLES_PER_SEC = 48000,
    VRC7_OUTPUT_PER_SEC = 49716,
    SAMPLES_PER_FRAME = SAMPLES_PER_SEC / 60,
    SAMPLES_ALIGNED = 8,
    SAMPLES_PER_FRAME_ALIGNED = ((SAMPLES_PER_FRAME + SAMPLES_ALIGNED - 1) / SAMPLES_ALIGNED ) * SAMPLES_ALIGNED,
    BASIC_BUFFER_COUNT = 16,
    BUFFER_MAX = 4
};

sfc_famicom_t* g_famicom = NULL;
static float s_buffer[SAMPLES_PER_FRAME_ALIGNED * (BASIC_BUFFER_COUNT + 1)];

/// <summary>
/// 由于各种外部原因需要额外插入一帧缓存
/// </summary>
void extra_buffer() {
    float* const buffer
        = s_buffer
        + SAMPLES_PER_FRAME_ALIGNED
        * (g_famicom->frame_counter % BASIC_BUFFER_COUNT)
        ;
    const float last = buffer[SAMPLES_PER_FRAME - 1];

    float* const send = s_buffer + SAMPLES_PER_FRAME_ALIGNED * BASIC_BUFFER_COUNT;

    if (last != 0.f) printf("<LAST:%f>", last);

    for (int i = 0; i != SAMPLES_PER_FRAME; ++i)
        send[i] = last;

    xa2_submit_buffer(send, SAMPLES_PER_FRAME);
}


extern void sfc_render_frame_easy(
    sfc_famicom_t* famicom, 
    uint8_t* buffer
);

enum {
    IB_IS_RECORD = 0,
    IB_IS_REPLAY = 0,
};

struct interface_audio_state {
    uint8_t*                        input_buffer_1mb;
    uint32_t                        input_length;
    uint32_t                        last_cycle;

    uint8_t                         change_2a03;
    uint8_t                         change_mmc5;

    uint8_t                         change_vrc7_custompatch;


    sfc_1storder_rc_lopass_filter_t lp_14kHz;
    sfc_1storder_rc_hipass_filter_t hp_440Hz;
    sfc_1storder_rc_hipass_filter_t hp__90Hz;
    sfc_1storder_rc_lopass_filter_t fds_lp2k;


    sfc_2a03_smi_ctx_t              ctx_2a03;
    sfc_vrc6_smi_ctx_t              ctx_vrc6;
    sfc_vrc7_smi_ctx_t              ctx_vrc7;
    sfc_fds1_smi_ctx_t              ctx_fds1;
    sfc_mmc5_smi_ctx_t              ctx_mmc5;
    sfc_n163_smi_ctx_t              ctx_n163;
    sfc_fme7_smi_ctx_t              ctx_fme7;


    float                           max_vol;

    float                           ch_weight[SFC_CHANNEL_COUNT];

    sfc_visualizers_t               visualizers[SFC_CHANNEL_COUNT];

    float                           fds1_wave_table[64];
    float                           n163_wave_table[256];
    float                           vrc7_wave_table[128 * 16];


    float                           current_chn[SAMPLES_PER_FRAME * SFC_VIS_PCM_COUNT];
} g_states;



void update_mask(uint8_t exsound) {
    for (int i = 0; i != SFC_CHANNEL_COUNT; ++i)
        g_states.visualizers[i].mask = 0;
    // 2A03  x 5
    for (int i = 0; i != 5; ++i)
        g_states.visualizers[SFC_2A03_Square1 + i].mask = 1;
    // VRC6  x 3
    if (exsound & SFC_NSF_EX_VCR6)
        for (int i = 0; i != 3; ++i)
            g_states.visualizers[SFC_VRC6_Square1 + i].mask = 1;
    // VRC7  x 6
    if (exsound & SFC_NSF_EX_VCR7)
        for (int i = 0; i != 6; ++i)
            g_states.visualizers[SFC_VRC7_FM0 + i].mask = 1;
    // FDS1  x 1
    if (exsound & SFC_NSF_EX_FDS1)
        g_states.visualizers[SFC_FDS1_Wavefrom].mask = 1;
    // MMC5  x 3
    if (exsound & SFC_NSF_EX_MMC5)
        for (int i = 0; i != 3; ++i)
            g_states.visualizers[SFC_MMC5_Square1 + i].mask = 1;
    // N163  x 8
    if (exsound & SFC_NSF_EX_N163)
        for (int i = 0; i != 8; ++i)
            g_states.visualizers[SFC_N163_Wavefrom0 + i].mask = 1;
    // FME7  x 3
    if (exsound & SFC_NSF_EX_FME7)
        for (int i = 0; i != 3; ++i)
            g_states.visualizers[SFC_FME7_ChannelA + i].mask = 1;
}

#define LP14kHz(x) sfc_filter_rclp(&g_states.lp_14kHz, x)
#define HP440Hz(x) sfc_filter_rchp(&g_states.hp_440Hz, x)
#define HP_90Hz(x) sfc_filter_rchp(&g_states.hp__90Hz, x)


static inline float filter_this(float x) {
    return HP_90Hz(HP440Hz(LP14kHz(x)));
}


static inline void ib_set_keys(uint32_t pos, uint8_t data) {
    g_states.input_length = pos;
    const uint32_t mask = (1 << 20) - 1;
    g_states.input_buffer_1mb[pos & mask] = data;
}

static inline uint8_t ib_pack_u8x8(const uint8_t data[8]) {
    uint8_t value = 0;
    value |= ((data[0] & 1) << 0);
    value |= ((data[1] & 1) << 1);
    value |= ((data[2] & 1) << 2);
    value |= ((data[3] & 1) << 3);
    value |= ((data[4] & 1) << 4);
    value |= ((data[5] & 1) << 5);
    value |= ((data[6] & 1) << 6);
    value |= ((data[7] & 1) << 7);
    return value;
}

static inline void ib_unpack_u8x8(uint8_t value, uint8_t data[8]) {
    data[0] = (value >> 0) & 1;
    data[1] = (value >> 1) & 1;
    data[2] = (value >> 2) & 1;
    data[3] = (value >> 3) & 1;
    data[4] = (value >> 4) & 1;
    data[5] = (value >> 5) & 1;
    data[6] = (value >> 6) & 1;
    data[7] = (value >> 7) & 1;
}

static void ib_try_record_replay() {
    // 记录
    if (IB_IS_RECORD) {
        const uint8_t state = ib_pack_u8x8(g_famicom->button_states);
        ib_set_keys(g_famicom->frame_counter, state);
    }
    // 回放
    else if (IB_IS_REPLAY) {
        const uint8_t state = g_states.input_buffer_1mb[g_famicom->frame_counter];
        ib_unpack_u8x8(state, g_famicom->button_states);
    }
}


static void ib_try_save_record() {
    if (IB_IS_RECORD) {
        FILE* const file = fopen("record.rep", "wb");
        if (!file);
        fwrite(g_states.input_buffer_1mb, g_states.input_length, 1, file);
        fclose(file);
    }
}

static void ib_try_load_record() {
    if (IB_IS_REPLAY) {
        FILE* const file = fopen("record.rep", "rb");
        if (!file);
        const size_t len = fread(g_states.input_buffer_1mb, 1, 1 << 20, file);
        g_states.input_length = len;
        fclose(file);
    }
}

void log_this_sample(uint32_t i) {
    // 记录2A03-NOI
    g_states.current_chn[SFC_VIS_PCM_NOISE * SAMPLES_PER_FRAME + i] = g_states.ctx_2a03.noi_output;
    // 记录2A03-DMC
    g_states.current_chn[SFC_VIS_PCM_DMC * SAMPLES_PER_FRAME + i] = g_states.ctx_2a03.dmc_output;
    // 记录MMC5-PCM
    g_states.current_chn[SFC_VIS_PCM_PCM * SAMPLES_PER_FRAME + i] = g_states.ctx_mmc5.pcm_output;
    // 记录VRC7-FMC
    //for (int j = 0; j != 6; ++j) {
    //    g_states.current_chn[(SFC_VRC7_FM0 + j) * SAMPLES_PER_FRAME + i] = g_states.ctx_vrc7.output[j];
    //}
    // 记录N163-WAV
    //for (int j = 0; j != 8; ++j) {
    //    g_states.current_chn[(SFC_N163_Wavefrom0 + j) * SAMPLES_PER_FRAME + i] = (float)g_famicom->apu.n163.ch_output[j];
    //}
    // 记录FDS1
    //g_states.current_chn[SFC_FDS1_Wavefrom * SAMPLES_PER_FRAME + i] = g_states.ctx_fds1.output;
}

// 生成样本
static void make_samples(const uint32_t begin, const uint32_t end) {
    if (begin >= end) return;

    float* const buffer 
        = s_buffer 
        + SAMPLES_PER_FRAME_ALIGNED 
        *  (g_famicom->frame_counter % BASIC_BUFFER_COUNT)
        ;

    // TODO: 移动至合适位置
    const sfc_fixed_t cps_fixed = sfc_fixed_make(NTSC_CPU_RATE, SAMPLES_PER_SEC);
    // TODO: 移动至合适位置
    sfc_n163_smi_update_subweight(g_famicom, &g_states.ctx_n163);

    const uint8_t extra_sound = g_famicom->rom_info.extra_sound;

    for (uint32_t i = begin; i != end; ++i) {

        float output = 0.f;
        // 2A03
        {
            sfc_2a03_smi_ctx_t* const ctx = &g_states.ctx_2a03;
            const float* const weight_list = g_states.ch_weight + SFC_2A03_Square1;
            sfc_2a03_smi_sample(g_famicom, ctx, weight_list, cps_fixed);
            const float squ = sfc_mix_square(ctx->sq1_output, ctx->sq2_output);
            const float tnd = sfc_mix_tnd(ctx->tri_output, ctx->noi_output, ctx->dmc_output);
            output += squ + tnd;
        }
        // VRC6
        if (extra_sound & SFC_NSF_EX_VCR6) {
            const float* const weight_list = g_states.ch_weight + SFC_VRC6_Square1;
            sfc_vrc6_smi_ctx_t* const ctx = &g_states.ctx_vrc6;
            sfc_vrc6_smi_sample(g_famicom, ctx, weight_list, cps_fixed);
            const float vrc6 = ctx->square1_output + ctx->square2_output + ctx->sawtooth_output;
            output += (0.2583f / 30.f) * vrc6;
        }
        // VRC7
        if (extra_sound & SFC_NSF_EX_VCR7) {
            const float* const weight_list = g_states.ch_weight + SFC_VRC7_FM0;
            sfc_vrc7_smi_sample(g_famicom, &g_states.ctx_vrc7, weight_list, cps_fixed);
            const float weight = (float)(0.1494 * 3.5 * 8 * 0.5);
            output += g_states.ctx_vrc7.mixed * weight;
        }
        // FDS1
        if (extra_sound & SFC_NSF_EX_FDS1) {
            const float* const weight_list = g_states.ch_weight + SFC_FDS1_Wavefrom;
            sfc_fds1_smi_sample(g_famicom, &g_states.ctx_fds1, weight_list, cps_fixed);
            float out = g_states.ctx_fds1.output * (2.4f * 0.1494f / 63.0f);
            out = sfc_filter_rclp(&g_states.fds_lp2k, out);
            output += out;
        }
        // MMC5
        if (extra_sound & SFC_NSF_EX_MMC5) {
            sfc_mmc5_smi_ctx_t* const ctx = &g_states.ctx_mmc5;
            const float* const weight_list = g_states.ch_weight + SFC_MMC5_Square1;
            sfc_mmc5_smi_sample(g_famicom, ctx, weight_list, cps_fixed);
            const float squ = sfc_mix_square(ctx->sq1_output, ctx->sq2_output);
            const float pcm = 0.002f * ctx->pcm_output;
            output += squ + pcm;
        }
        // N163
        if (extra_sound & SFC_NSF_EX_N163) {
            // N声道模式
            const uint8_t mode = 7;
            const float* const weight_list = g_states.ch_weight + SFC_N163_Wavefrom0;
            sfc_n163_smi_ctx_t* const ctx = &g_states.ctx_n163;
            sfc_n163_smi_sample(g_famicom, ctx, weight_list, cps_fixed, mode);
            output += ctx->output * ctx->subweight * (0.1494f / 225.f);
        }
        // FME7
        if (extra_sound & SFC_NSF_EX_FME7) {
            sfc_fme7_smi_ctx_t* const ctx = &g_states.ctx_fme7;
            const float* const weight_list = g_states.ch_weight + SFC_FME7_ChannelA;
            sfc_fme7_smi_sample(g_famicom, ctx, weight_list, cps_fixed);
            // FME7 权重近似0.1494*2.4*3 近似 = 1
            output += ctx->output[0] + ctx->output[1] + ctx->output[2];
        }
        // 记录当前
        log_this_sample(i);
        // 限制最大音量
        if (output > g_states.max_vol) g_states.max_vol = output;
        buffer[i] = output / g_states.max_vol;
    }
}


/// <summary>
/// 音频事件接口
/// </summary>
/// <param name="arg">The argument.</param>
/// <param name="cycle">The cycle.</param>
/// <param name="type">The type.</param>
static void this_audio_event(void* arg, uint32_t cycle, enum sfc_channel_index type) {
    const uint32_t old_index = g_states.last_cycle * SAMPLES_PER_SEC / NTSC_CPU_RATE;
    const uint32_t now_index = cycle * SAMPLES_PER_SEC / NTSC_CPU_RATE;
    // 将目前的状态覆盖 区间[old_index, now_index)

    // 更新状态
    if (now_index > old_index && (g_states.change_2a03 | g_states.change_mmc5)) {
        if (g_states.change_2a03 & 1) 
            sfc_2a03_smi_update_sq1(g_famicom, &g_states.ctx_2a03);
        if (g_states.change_2a03 & 2)
            sfc_2a03_smi_update_sq2(g_famicom, &g_states.ctx_2a03);
        if (g_states.change_2a03 & 4)
            sfc_2a03_smi_update_tri(g_famicom, &g_states.ctx_2a03);
        if (g_states.change_2a03 & 8)
            sfc_2a03_smi_update_noi(g_famicom, &g_states.ctx_2a03);

        if (g_states.change_mmc5 & 1)
            sfc_mmc5_smi_update_sq1(g_famicom, &g_states.ctx_mmc5);
        if (g_states.change_mmc5 & 2)
            sfc_mmc5_smi_update_sq2(g_famicom, &g_states.ctx_mmc5);

        g_states.change_2a03 = 0;
        g_states.change_mmc5 = 0;
    }
    // 处理修改
    const uint8_t extra_sound = g_famicom->rom_info.extra_sound;
    switch (type)
    {
    case SFC_Overview:
    case SFC_FrameCounter:
        g_states.change_2a03 = 0xf;
        if (type != SFC_Overview && (extra_sound & SFC_NSF_EX_MMC5)) {
    case SFC_MMC5_MMC5:
        g_states.change_mmc5 = 0xf;
        }
        break;
    case SFC_2A03_Square1:
        g_states.change_2a03 |= 1;
        break;
    case SFC_2A03_Square2:
        g_states.change_2a03 |= 2;
        break;
    case SFC_2A03_Triangle:
        g_states.change_2a03 |= 3;
        break;
    case SFC_2A03_Noise:
        g_states.change_2a03 |= 4;
        break;
    case SFC_2A03_DMC:
        break;
    case SFC_MMC5_Square1:
        g_states.change_mmc5 |= 1;
        break;
    case SFC_MMC5_Square2:
        g_states.change_mmc5 |= 2;
        break;
    case SFC_MMC5_PCM:
        break;
    case SFC_VRC7_VRC7:
        g_states.change_vrc7_custompatch = 1;
        break;
    }



    make_samples(old_index, now_index);

    g_states.last_cycle = cycle;
}


static inline float vol2alpha(float vol) {
    vol *= 2.f; return vol > 1.f ? 1.f : vol;
}


void sfc_make_2a03_squ(const sfc_square_ch_state_t* state, sfc_visualizers_t* data) {
    data->key_on = 0;
    if (state->volume) {
        data->key_on = 1;
        data->color[3] = vol2alpha((float)state->volume / 15.f);
        data->freq = g_famicom->config.cpu_clock_f / 8.f / (float)state->period_x2;
        data->volume = state->volume;
        data->ud.sq_duty = state->duty >> 3;
    }
}


extern inline sfc_get_triangle_data(sfc_famicom_t* famicom);

void sfc_make_2a03_tri(const sfc_triangle_ch_state_t* state, sfc_visualizers_t* data) {
    data->key_on = 0;
    data->volume = state->play_mask ? sfc_get_triangle_data(g_famicom) : 0;
    if (state->play_mask & state->inc_mask) {
        data->volume = 15;
        data->key_on = 1;
        data->freq = g_famicom->config.cpu_clock_f / 32.f / (float)state->period;
    }
}

void sfc_make_2a03_noi(const sfc_noise_ch_state_t* state, sfc_visualizers_t* data) {
    //data->key_on = 0;
    data->key_on = !!state->volume;
    data->volume = state->volume;
    data->ud.noi_mode = state->count - 1;
    data->freq = g_famicom->config.cpu_clock_f / (float)state->period;
}


void sfc_make_2a03_dmc(const sfc_dmc_data_t* state, sfc_visualizers_t* data) {
    data->key_on = !!(g_famicom->apu.status_write & SFC_APU4015_WRITE_EnableDMC);
    data->freq = g_famicom->config.cpu_clock_f / (float)state->period;
}


void sfc_make_vrc6_squ(const sfc_vrc6_square_data_t* state, sfc_visualizers_t* data) {
    data->key_on = 0;
    data->volume = state->volume;
    data->ud.sq_duty = state->duty;
    if (state->enable && state->volume) {
        data->key_on = 1;
        data->color[3] = vol2alpha((float)state->volume / 15.f);
        data->freq = g_famicom->config.cpu_clock_f / 16.f / (float)state->period;
    }
}


void sfc_make_vrc6_saw(const sfc_vrc6_saw_data_t* state, sfc_visualizers_t* data) {
    data->key_on = 0;
    data->volume = state->rate;
    if (state->enable && state->rate) {
        data->key_on = 1;
        data->color[3] = vol2alpha((float)state->rate / 42.f);
        data->freq = g_famicom->config.cpu_clock_f / 14.f / (float)state->period;
    }
}


void sfc_vrc7_wavetable_gen(sfc_famicom_t* famicom, float* const out, uint8_t instrument);

void make_inited_vrc7_wavtable(void) {
    for (uint8_t i = 1; i != 16; ++i) {
        sfc_vrc7_wavetable_gen(g_famicom, g_states.vrc7_wave_table + 128 * i, i);
    }
}



void sfc_make_vrc7_fmc(const sfc_vrc7_ch_t* state, sfc_visualizers_t* data) {
    data->key_on = state->trigger;
    /*
              49716 Hz * freq
        F = -----------------
             2^(19 - octave)
    */
    const uint32_t a = VRC7_OUTPUT_PER_SEC * state->freq;
    const uint32_t b = 1 << (19 - state->octave);
    data->freq = (float)((double)a / (double)b);
    // VRC7的音量是非线性的
    data->color[3] = vol2alpha((float)state->volume / 15.f);

    data->volume = state->volume;

    data->ex.vrc7.instrument = state->instrument8 >> 3;
    data->ex.vrc7.mod_state = state->modulator.state;
    data->ex.vrc7.car_state = state->carrier.state;
    // AM/FM
    const uint8_t* const patch = sfc_get_vrc7_patch(g_famicom) + state->instrument8;
    data->ex.vrc7.mod_am = patch[0] & 0x80;
    data->ex.vrc7.mod_fm = patch[0] & 0x40;
    data->ex.vrc7.car_am = patch[1] & 0x80;
    data->ex.vrc7.car_fm = patch[1] & 0x40;
}

extern void sfc_fds1_update_wavetable(sfc_famicom_t*, float*);

void update_fds1_wavtbl() {
    sfc_fds1_update_wavetable(g_famicom, g_states.fds1_wave_table);

}

void sfc_make_fds1_wav(const sfc_fds1_data_t* state, sfc_visualizers_t* data) {
    data->key_on = 0;
    if (!(state->flags_4083 & SFC_FDS_4083_HaltWave)) {
        //  CPU * current_picth / 65536 / 64
        data->key_on = 1;
        const float freq = g_famicom->config.cpu_clock_f / 64.f / 65536.f * (float)state->freq_gained;
        data->freq = freq;

        data->volume = state->volenv_gain_clamped * state->master_volume;
        const float volb = (float)data->volume;
        data->color[3] = vol2alpha(volb / (32.f * 30.f));
    }
}

typedef struct { uint32_t freq; uint16_t len; uint8_t vol; uint8_t addr; } sfc_n163info_t;
extern sfc_n163info_t sfc_n163_get_info(sfc_famicom_t* famicom, uint8_t ch);

extern void sfc_n163_update_wavetable(sfc_famicom_t*, float*, uint16_t);

void update_n163_wavtbl(uint16_t l) {
    sfc_n163_update_wavetable(g_famicom, g_states.n163_wave_table, l);
}


void sfc_make_n163_wav(uint8_t i, sfc_visualizers_t* data, uint16_t* maxl) {
    data->key_on = 0;
    if (i < g_famicom->apu.n163.n163_lowest_id) return;
    /*
        f = wave frequency
        l = wave length
        c = number of channels
        p = 18-bit frequency value
        n = CPU clock rate (≈1789773 Hz)

        f = (n * p) / (15 * 65536 * l * c)
    */
    const sfc_n163info_t info = sfc_n163_get_info(g_famicom, i);
    data->ex.n163.wavtbl_len = info.len;
    data->ex.n163.wavtbl_off = info.addr;
    data->volume = (uint8_t)info.vol;
    // 当前尺度
    const uint16_t current = info.len + (uint16_t)info.addr;
    if (current > *maxl) *maxl = current;
    // 有效
    if (info.vol) {
        data->key_on = 1;
        const uint32_t c = g_famicom->apu.n163.n163_count;
        const double a = g_famicom->config.cpu_clock_f * info.freq;
        const double b = 15 * 65536 * info.len * c;
        data->freq = (float)(a / b);
        data->color[3] = vol2alpha((float)info.vol / 15.f);
    }
}




void sfc_make_fme7_squ(
    const sfc_fme7_data_t* fme7,
    const sfc_sunsoft5b_ch_t* state, 
    sfc_visualizers_t* data) {
    data->key_on = 0;
    data->ud.sq_duty = 2;
    data->ex.fme7.noi = state->noise;
    data->ex.fme7.env = state->env;
    data->ex.fme7.tone = state->tone;
    // 包络模式
    if (state->env) {
        /*
        包络频率: f1 = CPU / (2 * 256 * Period)
        适用于: $08/$0C
        包络频率: f2 = CPU / (2 * 256 * Period * 2)
        适用于: $0A/$0E
        */
        const float period = (float)(fme7->env_period_step * 32);
        float base = g_famicom->config.cpu_clock_f / period;
        const uint8_t step3 = (fme7->evn_shape >> 2) & 3;
        const uint8_t step4 = (fme7->evn_shape >> 0) & 3;
        if (step3 != step4) base *= 0.5f;
        data->key_on = 1;
        data->freq = base;
    }
    // Tone模式
    else {
        // f = CPU / (2 * 16 * Period)
        if (state->volume) {
            data->volume = state->vol_raw;
            data->key_on = 1;
            const float freq = g_famicom->config.cpu_clock_f / (float)(state->cpu_period_step * 2);
            data->freq = freq;
            data->color[3] = vol2alpha((float)state->volume / 15.f);
        }
    }
}

void update_infomation(void) {
    // 2A03 方波#1
    sfc_make_2a03_squ(&g_states.ctx_2a03.sq1_state, g_states.visualizers + SFC_2A03_Square1);
    // 2A03 方波#2
    sfc_make_2a03_squ(&g_states.ctx_2a03.sq2_state, g_states.visualizers + SFC_2A03_Square2);
    // 2A03 三角波
    sfc_make_2a03_tri(&g_states.ctx_2a03.tri_state, g_states.visualizers + SFC_2A03_Triangle);
    //  2A03 噪音
    sfc_make_2a03_noi(&g_states.ctx_2a03.noi_state, g_states.visualizers + SFC_2A03_Noise);
    //  2A03 DMC
    sfc_make_2a03_dmc(&g_famicom->apu.dmc, g_states.visualizers + SFC_2A03_DMC);

    // VRC6 方波#1
    sfc_make_vrc6_squ(&g_famicom->apu.vrc6.square1, g_states.visualizers + SFC_VRC6_Square1);
    // VRC6 方波#2
    sfc_make_vrc6_squ(&g_famicom->apu.vrc6.square2, g_states.visualizers + SFC_VRC6_Square2);
    // VRC6 锯齿波
    sfc_make_vrc6_saw(&g_famicom->apu.vrc6.saw, g_states.visualizers + SFC_VRC6_Saw);

    // VRC7
    for (int i = 0; i != 6; ++i) 
        sfc_make_vrc7_fmc(g_famicom->apu.vrc7.ch + i, g_states.visualizers + SFC_VRC7_FM0 + i);
    // VRC7 自定义乐器
    if (g_states.change_vrc7_custompatch) {
        g_states.change_vrc7_custompatch = 0;
        sfc_vrc7_wavetable_gen(g_famicom, g_states.vrc7_wave_table, 0);
    }

    // FDS1
    sfc_make_fds1_wav(&g_famicom->apu.fds, g_states.visualizers + SFC_FDS1_Wavefrom);
    // TODO: 更新时更新
    update_fds1_wavtbl();


    // MMC5 方波#1
    sfc_make_2a03_squ(&g_states.ctx_mmc5.sq1_state, g_states.visualizers + SFC_MMC5_Square1);
    // MMC5 方波#2
    sfc_make_2a03_squ(&g_states.ctx_mmc5.sq2_state, g_states.visualizers + SFC_MMC5_Square2);

    // N163 波表调制
    uint16_t max_len = 0;
    for (uint8_t i = 0; i != 8; ++i) {
        sfc_make_n163_wav(i, g_states.visualizers + SFC_N163_Wavefrom0 + i, &max_len);
    }
    // TODO: 更新时更新
    update_n163_wavtbl(max_len);


    // FME7 声道
    for(int i = 0; i != 3; ++i)
        sfc_make_fme7_squ(&g_famicom->apu.fme7, g_famicom->apu.fme7.ch + i, g_states.visualizers + SFC_FME7_ChannelA + i);
}


/// <summary>
/// 提交当前缓冲区
/// </summary>
void submit_now_buffer(void*rgba) {
    float* const buffer
        = s_buffer
        + SAMPLES_PER_FRAME_ALIGNED
        * (g_famicom->frame_counter % BASIC_BUFFER_COUNT)
        ;

    //for (int i = 0; i != SAMPLES_PER_FRAME; ++i)
    //    buffer[i] = filter_this(buffer[i]);

    //static FILE* file = 0;
    //if (!file) file = fopen("output.raw", "wb");
    //fwrite(buffer, 4, SAMPLES_PER_FRAME, file);

    // 跳过视频帧
    if (rgba) d2d_submit_wave(buffer, SAMPLES_PER_FRAME);
    xa2_submit_buffer(buffer, SAMPLES_PER_FRAME);
}


void debug_putaudio(float x) {
    static float buf[1024];
    static int index = 0;
    static FILE* file = 0;
    buf[index++] = x;
    if (index == 1024) {
        index = 0;
        if (!file) file = fopen("output.raw", "wb");
        fwrite(buf, 4, 1024, file);
    }
}

/// <summary>
/// 播放该帧音频
/// </summary>
static void play_audio(void* rgba) {
    const unsigned left = xa2_buffer_left();
    // 太低
    if (!left) {
        // 可以跳过视频帧, 这里直接加入空白音频帧
        extra_buffer();
        printf("<buffer empty>\n");
    }
    // 太高
    else if (left > BUFFER_MAX) {
        printf("<buffer overflow[%d]>\n", left);
        // 可以让图像接口空等一两帧, 这里直接睡过去
        //Sleep(10);
        // 这里还能直接return跳过该音频帧
        return;
    }
    // 收尾
    const uint32_t old_index = g_states.last_cycle * SAMPLES_PER_SEC / NTSC_CPU_RATE;
    make_samples(old_index, SAMPLES_PER_FRAME);
    g_states.last_cycle = 0;
    // 更新当前信息
    update_infomation();
    // 提交当前缓存
    submit_now_buffer(rgba);
}


// 标准调色板
extern uint32_t sfc_stdpalette[];


/// <summary>
/// SFCs the play NSF once.
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_play_nsf_once(sfc_famicom_t* famicom) {
    ++famicom->frame_counter;
    const uint32_t cpu_cycle_per_frame = famicom->config.cpu_cycle_per_frame;
    const uint32_t cpu_cycle_per_frame_4 = famicom->config.cpu_cycle_per_frame / 4;

    while (famicom->cpu_cycle_count < cpu_cycle_per_frame) {
        const uint32_t cycle = sfc_cpu_execute_one(famicom);
        // PLAY
        if (famicom->nsf.play_clock < cycle) {
            famicom->nsf.play_clock += famicom->rom_info.clock_per_play;
            sfc_famicom_nsf_play(famicom);
        }
        // FRAME COUNTER
        if (famicom->nsf.framecounter_clock < cycle) {
            famicom->nsf.framecounter_clock += cpu_cycle_per_frame_4;
            sfc_trigger_frame_counter(famicom);
        }
        famicom->nsf.play_clock -= cycle;
        famicom->nsf.framecounter_clock -= cycle;
    }

    // END
    famicom->cpu_cycle_count = 0;
}


extern uint8_t sp_backup[32];


/// <summary>
/// 主渲染
/// </summary>
/// <param name="rgba">The RGBA.</param>
extern void main_render(void* rgba) {
    uint8_t buffer[(256+8) * 256];

    // 记录信息
    ib_try_record_replay();
    if (g_famicom->rom_info.song_count)
        sfc_play_nsf_once(g_famicom);
    else
        sfc_render_frame_easy(g_famicom, buffer);

    // 数据有效

    if (rgba) {
        uint32_t* data = rgba;
        // 生成调色板数据
        uint32_t palette[32];

        for (int i = 0; i != 32; ++i) {
            const uint8_t spindex = g_famicom->ppu.data.spindexes[i];
            //const uint8_t spindex = sp_backup[i];
            
            palette[i] = sfc_stdpalette[spindex];
        }
        // 镜像数据
        palette[4 * 1] = palette[0];
        palette[4 * 2] = palette[0];
        palette[4 * 3] = palette[0];
        palette[4 * 4] = palette[0];
        palette[4 * 5] = palette[0];
        palette[4 * 6] = palette[0];
        palette[4 * 7] = palette[0];

        for (int y = 0; y != 240; ++y) {
            for (int x = 0; x != 256; ++x) {
                *data = palette[buffer[y*(256 + 8) + x] >> 1];
                ++data;
            }
        }

    }
    play_audio(rgba);
}


/// <summary>
/// 生成文件
/// </summary>
/// <param name="buffer">The buffer.</param>
static void sfc_make_file_name(
    const sfc_rom_info_t* info, 
    const char* folder, 
    char buffer[256]) {
    snprintf(buffer, 256, "%s/%08X.sfc", folder, info->prgrom_crc32b);
}


/// <summary>
/// Thises the length of the get all.
/// </summary>
/// <param name="s">The s.</param>
/// <param name="c">The c.</param>
/// <returns></returns>
static inline uint32_t this_get_all_len(const sfc_data_set_t* s, uint32_t c) {
    uint32_t len = 0;
    for (uint32_t i = 0; i != c; ++i) len += s[i].length;
    return len;
}

/// <summary>
/// Thises the load sram.
/// </summary>
/// <param name="arg">The argument.</param>
/// <param name="info">The information.</param>
/// <param name="sets">The sets.</param>
/// <param name="len">The length.</param>
void this_load_sram(void*arg, const sfc_rom_info_t* info, const sfc_data_set_t* sets, uint32_t len) {
    char buffer[256]; sfc_make_file_name(info, "save", buffer);
    FILE* const file = fopen(buffer, "rb");
    if (!file) return;
    fseek(file, 0, SEEK_END);
    const uint32_t file_len = ftell(file);
    fseek(file, 0, SEEK_SET);
    // 长度匹配
    if (file_len == this_get_all_len(sets, len)) {
        for (uint32_t i = 0; i != len; ++i) {
            const size_t read_count = fread(sets[i].address, sets[i].length, 1, file);
            assert(read_count && "ERROR");
        }
    }
    else assert(!"BAD LENGTH");
    fclose(file);
}

#if defined _MSC_VER
#include <direct.h>
#elif defined __GNUC__
#include <sys/types.h>
#include <sys/stat.h>
#endif

/// <summary>
/// SFCs the create dir.
/// </summary>
/// <param name="dir">The dir.</param>
void sfc_create_dir(const char* dir) {
#if defined _MSC_VER
    _mkdir(dir);
#elif defined __GNUC__
    mkdir(dir/*, 0777*/);
#endif
}

/// <summary>
/// Thises the save sram.
/// </summary>
/// <param name="arg">The argument.</param>
/// <param name="info">The information.</param>
/// <param name="sets">The sets.</param>
/// <param name="len">The length.</param>
void this_save_sram(void*arg, const sfc_rom_info_t* info, const sfc_data_set_t* sets, uint32_t len) {
    sfc_create_dir("save");
    char buffer[256]; sfc_make_file_name(info, "save", buffer);
    FILE* const file = fopen(buffer, "wb");
    for (uint32_t i = 0; i != len; ++i) {
        const size_t read_count = fwrite(sets[i].address, sets[i].length, 1, file);
        assert(read_count && "ERROR");
    }
    fclose(file);
}

void sfc_sl_write_stream(void*, const uint8_t*, uint32_t);
void sfc_sl_read_stream(void*, uint8_t*, uint32_t);


void init_global() {
    memset(&g_states, 0, sizeof(g_states));
    sfc_make_rclp(&g_states.lp_14kHz, SAMPLES_PER_SEC, 14000);
    sfc_make_rchp(&g_states.hp_440Hz, SAMPLES_PER_SEC, 440);
    sfc_make_rchp(&g_states.hp__90Hz, SAMPLES_PER_SEC, 90);

    sfc_make_rclp(&g_states.fds_lp2k, SAMPLES_PER_SEC, 2000);

    //g_states.square1.period_x2 = 2;
    //g_states.square2.period_x2 = 2;
    //g_states.triangle.period = 1;
    //g_states.noise.period = 4;
    //g_states.lfsr = 1;
    //g_states.dmc.period = 500;

    g_states.ctx_2a03.sq1_state.period_x2 = 2;
    g_states.ctx_2a03.sq2_state.period_x2 = 2;
    g_states.ctx_2a03.tri_state.period = 1;
    g_states.ctx_2a03.noi_state.period = 1;
    g_states.ctx_2a03.noi_state.count = 1;

    g_states.ctx_mmc5.sq1_state.period_x2 = 2;
    g_states.ctx_mmc5.sq2_state.period_x2 = 2;



    g_states.max_vol = 1.f;

    //srand(233);
    void ui_function_hsla_to_rgba_float32(const float hsla[4], float rgba[4]);


    float hsla[4];
    hsla[1] = 1.0f;
    hsla[2] = 0.7f;
    hsla[3] = 1.f;

    for (int i = 0; i != SFC_CHANNEL_COUNT; ++i) {
        g_states.ch_weight[i] = 1.f;

        float* const color = g_states.visualizers[i].color;
        hsla[0] = (float)((360 * i * 9 / SFC_CHANNEL_COUNT) % 360);
        ui_function_hsla_to_rgba_float32(hsla, color);
    }

    // 

    d2d_set_visualizers(
        g_states.visualizers, SFC_CHANNEL_COUNT,
        g_states.n163_wave_table,
        g_states.fds1_wave_table,
        g_states.vrc7_wave_table, 128,
        g_states.current_chn, SAMPLES_PER_FRAME
    );

    //g_states.ch_weight[SFC_2A03_Square1] = 1.f;
    //g_states.ch_weight[SFC_2A03_Square2] = 1.f;
    //g_states.ch_weight[SFC_2A03_Triangle] = 1.f;
    //g_states.ch_weight[SFC_2A03_Noise] = 1.f;
    //g_states.ch_weight[SFC_2A03_DMC] = 1.f;

    //g_states.ch_weight[SFC_VRC6_Square1] = 1.f;
    //g_states.ch_weight[SFC_VRC6_Square2] = 1.f;
    //g_states.ch_weight[SFC_VRC6_Saw] = 1.f;


    //g_states.ch_weight[SFC_VRC7_FM0] = 1.f;
    //g_states.ch_weight[SFC_VRC7_FM1] = 1.f;
    //g_states.ch_weight[SFC_VRC7_FM2] = 1.f;
    //g_states.ch_weight[SFC_VRC7_FM3] = 1.f;
    //g_states.ch_weight[SFC_VRC7_FM4] = 1.f;
    //g_states.ch_weight[SFC_VRC7_FM5] = 1.f;


    //g_states.ch_weight[SFC_FDS1_Wavefrom] = 1.f;

    //g_states.ch_weight[SFC_N163_Wavefrom0] = 1.f;
    //g_states.ch_weight[SFC_N163_Wavefrom1] = 1.f;
    //g_states.ch_weight[SFC_N163_Wavefrom2] = 1.f;
    //g_states.ch_weight[SFC_N163_Wavefrom3] = 1.f;
    //g_states.ch_weight[SFC_N163_Wavefrom4] = 1.f;
    //g_states.ch_weight[SFC_N163_Wavefrom5] = 1.f;
    //g_states.ch_weight[SFC_N163_Wavefrom6] = 1.f;
    //g_states.ch_weight[SFC_N163_Wavefrom7] = 1.f;

    //g_states.ch_weight[SFC_FME7_ChannelA] = 1.f;
    //g_states.ch_weight[SFC_FME7_ChannelB] = 1.f;
    //g_states.ch_weight[SFC_FME7_ChannelC] = 1.f;
}



enum {
    PATH_BUFLEN = 1024
};

void clear_input_buffer() {
    int c;  while ((c = getchar()) != '\n' && c != EOF);
}

void get_cso_file_path(char path_input[PATH_BUFLEN]) {
    // 上次输入的东西?
    FILE* const last_input_file = fopen("last_input.ini", "rb");
    if (last_input_file) {
        const size_t len = fread(path_input, 1, PATH_BUFLEN-1, last_input_file);
        path_input[len] = 0;
        fclose(last_input_file);
        printf("Last path: [%s], enter 'N' to rewrite, other to skip\n", path_input);
        return;
        const int ch = getchar();
        if (!(ch == 'N' || ch == 'n'))  return;
        // 清除输入缓存
        clear_input_buffer();
    }
    // 输入shader文件地址
    printf("Input shader file(*.cso) path: ");
#ifdef _MSC_VER
    gets_s(path_input, PATH_BUFLEN);
#else
    gets(path_input);
#endif
    const uint32_t len = strlen(path_input);
    {
        // 保存输入
        FILE* const this_input_file = fopen("last_input.ini", "wb");
        if (this_input_file) {
            fwrite(path_input, 1, len, this_input_file);
            fclose(this_input_file);
        }
    }
}

sfc_ecode this_load_rom(void* arg, sfc_rom_info_t* info);
sfc_ecode this_free_rom(void* arg, sfc_rom_info_t* info);



/// <summary>
/// 应用程序入口
/// </summary>
/// <returns></returns>
int main(int argc, const char* argv[]) {
    char cso_path[PATH_BUFLEN]; char png_path[PATH_BUFLEN];
    printf("Battle Control Online! \n");
    init_global();
    get_cso_file_path(cso_path);
    // 把CSO后缀换成png
    strcpy(png_path, cso_path);
    char* const ext  = strstr(png_path, ".cso");
    if (ext) {
        ext[1] = 'p';
        ext[2] = 'n';
        ext[3] = 'g';
    }

    // 申请1MB作为按键缓存
    g_states.input_buffer_1mb = malloc(1 << 20);
    if (!g_states.input_buffer_1mb) return -1;
    memset(g_states.input_buffer_1mb, 0, 1 << 20);
    ib_try_load_record();

    memset(s_buffer, 0, sizeof(s_buffer));
    sfc_interface_t interfaces = { NULL };

    interfaces.audio_change = this_audio_event;
    interfaces.load_sram = this_load_sram;
    interfaces.save_sram = this_save_sram;
    interfaces.sl_write_stream = sfc_sl_write_stream;
    interfaces.sl_read_stream = sfc_sl_read_stream;
    interfaces.load_rom = this_load_rom;
    interfaces.free_rom = this_free_rom;


    sfc_famicom_t famicom;
    g_famicom = &famicom;

    const char* const filepath = argc > 1 ? argv[1] : NULL;
    if (sfc_famicom_init(&famicom, (void*)filepath, &interfaces)) return 1;
    //qload();

    update_mask(famicom.rom_info.extra_sound);


    printf(
        "ROM: PRG-ROM: %d x 16kb   CHR-ROM %d x 8kb   Mapper: %03d\n",
        (int)famicom.rom_info.size_prgrom / (16*1024),
        (int)famicom.rom_info.size_chrrom / (8 *1024),
        (int)famicom.rom_info.mapper_number
    );

    //
    make_inited_vrc7_wavtable();

    //getchar();

    xa2_init(SAMPLES_PER_SEC);
    main_cpp(cso_path, png_path);
    xa2_clean();
    ib_try_save_record();
    free(g_states.input_buffer_1mb);
    sfc_famicom_uninit(&famicom);
    printf("Battle Control Terminated! \n");
    return 0;
}



/// <summary>
/// 接口: 写入流
/// </summary>
/// <param name="arg">The argument.</param>
/// <param name="data">The data.</param>
/// <param name="len">The length.</param>
void sfc_sl_write_stream(void* arg, const uint8_t* data, uint32_t len) {
    FILE* const file = arg;
    // TODO: 错误处理
    fwrite(data, len, 1, file);
}

/// <summary>
/// 接口: 从流读
/// </summary>
/// <param name="arg">The argument.</param>
/// <param name="data">The data.</param>
/// <param name="len">The length.</param>
void sfc_sl_read_stream(void* arg, uint8_t* data, uint32_t len) {
    FILE* const file = arg;
    // TODO: 错误处理
    fread(data, len, 1, file);
}

/// <summary>
/// 快速存档
/// </summary>
void qsave() {
    sfc_create_dir("qsave");
    const sfc_rom_info_t* const info = &g_famicom->rom_info;
    char buffer[256]; sfc_make_file_name(info, "qsave", buffer);
    FILE* const file = fopen(buffer, "wb");
    if (!file) return;
    void* const old_arg = g_famicom->argument;
    g_famicom->argument = file;
    sfc_famicom_save_state(g_famicom);
    g_famicom->argument = old_arg;
    fclose(file);
}

/// <summary>
/// 快速读档
/// </summary>
void qload() {
    const sfc_rom_info_t* const info = &g_famicom->rom_info;
    char buffer[256]; sfc_make_file_name(info, "qsave", buffer);
    FILE* const file = fopen(buffer, "rb");
    if (!file) return;
    void* const old_arg = g_famicom->argument;
    g_famicom->argument = file;
    sfc_famicom_load_state(g_famicom);
    g_famicom->argument = old_arg;
    fclose(file);
}

/// <summary>
/// Thises the is be.
/// </summary>
/// <returns></returns>
static inline uint8_t this_is_be() {
    const uint16_t data = 0x0100;
    return *(const uint8_t*)(&data);
}

/// <summary>
/// Thises the load NSF.
/// </summary>
/// <param name="info">The information.</param>
/// <returns></returns>
sfc_ecode this_load_nsf(sfc_rom_info_t* info, FILE* file) {
    sfc_nsf_header_t header;
    const size_t count = fread(&header, sizeof(header), 1, file);
    // 交换大小端
    if (this_is_be()) sfc_nsf_swap_endian(&header);
    // 检测合法性
    union {
        // NESM
        char        nesm[4];
        // NESM
        uint32_t    nesm_u32;
    } data;
    data.nesm[0] = 'N';
    data.nesm[1] = 'E';
    data.nesm[2] = 'S';
    data.nesm[3] = 'M';
    // 开头匹配
    if (header.nesm_u32 == data.nesm_u32 && header.u8_1a == 0x1a) {
        // 获取ROM长度
        fseek(file, 0, SEEK_END);
        const long len = ftell(file) - sizeof(header);
        const size_t offset = header.load_addr_le & 0x0fff;
        const size_t alloclen = len + offset + 8 * 1024;
        uint8_t* const data_ptr = malloc(alloclen);
        if (!data_ptr) return SFC_ERROR_OUT_OF_MEMORY;
        memset(data_ptr, 0, alloclen);
        fseek(file, sizeof(header), SEEK_SET);
        fread(data_ptr + offset, len, 1, file);
        // 填写信息
        info->data_prgrom = data_ptr;
        info->data_chrrom = data_ptr + len + offset;
        info->size_prgrom = len;
        info->size_chrrom = 0;
        info->mapper_number = 0x1f;
        info->vmirroring = 0;
        info->save_ram_flags = 0;
        info->four_screen = 0;
        // 播放速度提示
        info->clock_per_play_n = (uint64_t)1789773 * (uint64_t)header.play_speed_ntsc_le / (uint64_t)1000000;
        info->clock_per_play_p = (uint64_t)1662607 * (uint64_t)header.play_speed__pal_le / (uint64_t)1000000;
        info->clock_per_play = info->clock_per_play_n;
        printf("NSF NTSC RATE: %.02fHz\n", 1000000.0 / header.play_speed_ntsc_le);
        // NSF
        memcpy(info->name, &header.name, sizeof(info->name) * 3);
        memcpy(info->bankswitch_init, &header.bankswitch_init, sizeof(header.bankswitch_init));
        info->song_count = header.count;
        info->load_addr = header.load_addr_le;
        info->init_addr = header.init_addr_le;
        info->play_addr = header.play_addr_le;
        info->pal_ntsc_bits = header.pal_ntsc_bits;
        info->extra_sound = header.extra_sound;
        info->start_play = header.start;
        // 状态
        //info->start_play = 5;
        // 播放曲目
        return SFC_ERROR_OK;
    }
    fseek(file, 0, SEEK_SET);
    return SFC_ERROR_ILLEGAL_FILE;
}


/// <summary>
/// 加载ROM
/// </summary>
/// <param name="arg">The argument.</param>
/// <param name="info">The information.</param>
/// <returns></returns>
sfc_ecode this_load_rom(void* arg, sfc_rom_info_t* info) {
    assert(info->data_prgrom == NULL && "FREE FIRST");
    const char* const filename = arg ? arg : "31_test_16.nes";
    FILE* const file = fopen(filename, "rb");
    
    // 文本未找到
    if (!file) return SFC_ERROR_FILE_NOT_FOUND;
    //sfc_ecode code = SFC_ERROR_ILLEGAL_FILE;
    sfc_ecode code = this_load_nsf(info, file);
    // 读取文件头
    sfc_nes_header_t nes_header;
    if (fread(&nes_header, sizeof(nes_header), 1, file)) {
        // 开头4字节
        union { uint32_t u32; uint8_t id[4]; } this_union;
        this_union.id[0] = 'N';
        this_union.id[1] = 'E';
        this_union.id[2] = 'S';
        this_union.id[3] = '\x1A';
        // 比较这四字节
        if (this_union.u32 == nes_header.id) {
            const uint32_t prgrom16
                = (uint32_t)nes_header.count_prgrom16kb
                | (((uint32_t)nes_header.upper_rom_size & 0x0F) << 8)
                ;
            const uint32_t chrrom8
                = (uint32_t)nes_header.count_chrrom_8kb
                | (((uint32_t)nes_header.upper_rom_size & 0xF0) << 4)
                ;
            const size_t size1 = 16 * 1024 * prgrom16;
            // 允许没有CHR-ROM(使用CHR-RAM代替)
            const size_t size2 = 8 * 1024 * (chrrom8 | 1);
            const size_t size3 = 8 * 1024 * chrrom8;
            // 计算实际长度
            const size_t malloc_len = size1 + size2;
            const size_t fread_len = size1 + size3;

            uint8_t* const ptr = (uint8_t*)malloc(malloc_len);
            // 内存申请成功
            if (ptr) {
                // 清空
                memset(ptr, 0, malloc_len);
                code = SFC_ERROR_OK;
                // TODO: 实现Trainer
                // 跳过Trainer数据
                if (nes_header.control1 & SFC_NES_TRAINER) fseek(file, 512, SEEK_CUR);
                // 这都错了就不关我的事情了
                fread(ptr, fread_len, 1, file);

                // 填写info数据表格
                info->data_prgrom = ptr;
                info->data_chrrom = ptr + size1;
                info->size_prgrom = prgrom16 * 16*1024;
                info->size_chrrom = chrrom8 * 8 * 1024;
                info->mapper_number
                    = (nes_header.control1 >> 4)
                    | (nes_header.control2 & 0xF0)
                    ;
                info->vmirroring = (nes_header.control1 & SFC_NES_VMIRROR) > 0;
                info->four_screen = (nes_header.control1 & SFC_NES_4SCREEN) > 0;
                info->save_ram_flags = (nes_header.control1 & SFC_NES_SAVERAM) > 0 ? SFC_ROMINFO_SRAM_HasSRAM : 0;
                assert(!(nes_header.control1 & SFC_NES_TRAINER) && "unsupported");
                assert(!(nes_header.control2 & SFC_NES_VS_UNISYSTEM) && "unsupported");
                assert(!(nes_header.control2 & SFC_NES_Playchoice10) && "unsupported");

                d2d_set_visualizers(0, 0, 0, 0, 0, 0, 0, 0);
            }
            // 内存不足
            else code = SFC_ERROR_OUT_OF_MEMORY;
        }
        // 非法文件
    }
    fclose(file);
    return code;
}

/// <summary>
/// 释放ROM
/// </summary>
/// <param name="arg">The argument.</param>
/// <param name="info">The information.</param>
/// <returns></returns>
sfc_ecode this_free_rom(void* arg, sfc_rom_info_t* info) {
    // 释放动态申请的数据
    free(info->data_prgrom);
    info->data_prgrom = NULL;

    return SFC_ERROR_OK;
}

int g_debug_line = 0;
int g_debug_return = 1;


/// <summary>
/// Users the input.
/// </summary>
/// <param name="index">The index.</param>
/// <param name="data">The data.</param>
void user_input(int index, unsigned char data) {
    assert(index >= 0 && index < 16);
    if (IB_IS_REPLAY) return;
    g_famicom->button_states[index] = data;
}

void sfc_before_execute(void* ctx, sfc_famicom_t* famicom) {
    g_debug_line++;
    if (g_debug_line < 0x5555 || g_debug_return) return;
    char buf[SFC_DISASSEMBLY_BUF_LEN2];
    const uint16_t pc = famicom->registers.program_counter;
    sfc_fc_disassembly(pc, famicom, buf);
    printf(
        "%4d - %s   A:%02X X:%02X Y:%02X P:%02X SP:%02X\n",
        g_debug_line, buf,
        (int)famicom->registers.accumulator,
        (int)famicom->registers.x_index,
        (int)famicom->registers.y_index,
        (int)famicom->registers.status,
        (int)famicom->registers.stack_pointer
    );
}


#include <math.h>

/// <summary>
/// 以A4为基础计算键ID
/// </summary>
/// <param name="freq">The freq.</param>
/// <returns></returns>
float calc_key_id_a4(float freq) {
    /*
    换底公式
                 log(c, b)
    log(a, b) = ------------
                 log(c, a)
    */
    const float a4 = 440.f;
    return logf(freq / a4) * 17.3123404907f;
}



/// <summary>
/// UIs the function HSL to RGB float32.
/// </summary>
/// <param name="data">The data.</param>
void ui_function_hsla_to_rgba_float32(const float hsla[4], float rgba[4]) {
    // 将A弄过去
    rgba[3] = hsla[3];
    // 准备数据
    const float h = hsla[0];
    assert(0.f <= h && h < 360.f && "out of range");
    const float s = hsla[1];
    const float l = hsla[2];
    float *r = rgba + 0;
    float *g = rgba + 1;
    float *b = rgba + 2;
    // Q
    const float v = (l <= 0.5f) ? (l * (1.0f + s)) : (l + s - l * s);
    // 黑色
    if (v <= 0.0f) {
        *r = *g = *b = 0.f;
    }
    // 其他
    else {
        const float m = l + l - v;
        const float sv = (v - m) / v;
        const int sextant = (int)(h / 60.0f);
        const float fract = h / 60.0f - sextant;
        const float vsf = v * sv * fract;
        const float mid1 = m + vsf;
        const float  mid2 = v - vsf;
        switch (sextant) {
        case 0: *r = v; *g = mid1; *b = m; break;
        case 1: *r = mid2; *g = v; *b = m; break;
        case 2: *r = m; *g = v; *b = mid1; break;
        case 3: *r = m; *g = mid2; *b = v; break;
        case 4: *r = mid1; *g = m; *b = v; break;
        case 5: *r = v; *g = m; *b = mid2; break;
        }
    }
}
