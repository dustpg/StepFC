#include "sfc_famicom.h"
#include "sfc_cpu.h"
#include "sfc_play.h"
#include "../common/d2d_interface2.h"
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
    SAMPLES_PER_FRAME_ALIGNED = (SAMPLES_PER_FRAME / SAMPLES_ALIGNED + 1) * SAMPLES_ALIGNED,
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
    float                           square1_cycle;
    float                           square2_cycle;
    float                           triangle_cycle;
    float                           noise_cycle;
    float                           dmc_cycle;
    sfc_square_channel_state_t      square1;
    sfc_square_channel_state_t      square2;
    sfc_triangle_channel_state_t    triangle;
    sfc_noise_channel_state_t       noise;

    sfc_dmc_data_t           dmc;

    uint16_t                        lfsr;
    uint8_t                         square1_seq_index;
    uint8_t                         square2_seq_index;
    uint8_t                         triangle_seq_index;
    uint8_t                         dmc_enable;

    sfc_1storder_rc_lopass_filter_t lp_14kHz;
    sfc_1storder_rc_hipass_filter_t hp_440Hz;
    sfc_1storder_rc_hipass_filter_t hp__90Hz;


    sfc_1storder_rc_lopass_filter_t fds_lp2k;

    sfc_vrc6_data_t                 vrc6;

    float vrc6_squ1_cycle       ;
    float vrc6_squ2_cycle       ;
    float vrc6_saw__cycle       ;

    float                           vrc7_index;
    float                           max_vol;
} g_states;


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

static const uint8_t sq_seq_mask[] = {
    0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00,
    0x0f, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
};


static const uint8_t tri_seq[] = {
    15, 14, 13, 12, 11, 10,  9,  8,
    7,  6,  5,  4,  3,  2,  1,  0,
    0,  1,  2,  3,  4,  5,  6,  7,
    8,  9, 10, 11, 12, 13, 14, 15
};

static void do_lfsr_count(int count, uint8_t mode) {
    if (mode) {
        for (int i = 0; i != count; ++i) 
            g_states.lfsr = sfc_lfsr_short(g_states.lfsr);
    }
    else {
        for (int i = 0; i != count; ++i) 
            g_states.lfsr = sfc_lfsr_long(g_states.lfsr);
    }
}

// 读取PRG-ROM数据
extern inline uint8_t sfc_read_prgdata(uint16_t, const sfc_famicom_t*);

static void do_dmc() {
    // 禁止DMC
    if (!g_states.dmc_enable) return;
    // 还有DMC样本
    if (g_states.dmc.lenleft && !g_states.dmc.count) {
        g_states.dmc.count = 8;
        g_states.dmc.data = sfc_read_prgdata(g_states.dmc.curaddr, g_famicom);
        g_states.dmc.curaddr = (uint16_t)(g_states.dmc.curaddr + 1) | (uint16_t)0x8000;
        --g_states.dmc.lenleft;
    }
    // 还有比特数
    if (g_states.dmc.count) {
        --g_states.dmc.count;
        if (g_states.dmc.data & 1) {
            if (g_states.dmc.value <= 125)
                g_states.dmc.value += 2;
        }
        else {
            if (g_states.dmc.value >= 2)
                g_states.dmc.value -= 2;
        }
        g_states.dmc.data >>= 1;
        // 循环
        if ((g_states.dmc.irq_loop & 1) && !g_states.dmc.lenleft) {
            g_states.dmc.curaddr = g_states.dmc.orgaddr;
            g_states.dmc.lenleft = g_states.dmc.length;
        }
    }
}

extern int32_t sfc_vrc7_49716hz(sfc_famicom_t*);

// 生成样本
static void make_samples(const uint32_t begin, const uint32_t end) {
    if (begin >= end) return;
    /*
    output = square_out + tnd_out
    
                          95.88
    square_out = -----------------------
                        8128
                 ----------------- + 100
                 square1 + square2

                          159.79
    tnd_out = ------------------------------
                          1
              ------------------------ + 100
              triangle   noise    dmc
              -------- + ----- + -----
                8227     12241   22638
    */

    float* const buffer 
        = s_buffer 
        + SAMPLES_PER_FRAME_ALIGNED 
        *  (g_famicom->frame_counter % BASIC_BUFFER_COUNT)
        ;

    const float cpu_cycle_per_sample 
        = (float)NTSC_CPU_RATE 
        / (float)SAMPLES_PER_SEC
        ;
    assert(g_states.square1.period);
    assert(g_states.square2.period);
    assert(g_states.triangle.period);
    assert(g_states.noise.period);
    assert(g_states.dmc.period);

    const float square1p = g_states.square1.period;
    const float square2p = g_states.square2.period;
    const float trianglep = g_states.triangle.period;
    const float noise_p = g_states.noise.period;

    const float dmc_p = g_states.dmc.period;

    sfc_fds1_ctx_t fds1_ctx;
    sfc_mmc5_ctx_t mmc5_ctx;
    sfc_n163_ctx_t n163_ctx;
    sfc_fme7_ctx_t fme7_ctx;
    sfc_fds1_samplemode_begin(g_famicom, &fds1_ctx, cpu_cycle_per_sample);
    sfc_mmc5_samplemode_begin(g_famicom, &mmc5_ctx);
    sfc_n163_samplemode_begin(g_famicom, &n163_ctx);
    sfc_fme7_samplemode_begin(g_famicom, &fme7_ctx);

    for (uint32_t i = begin; i != end; ++i) {
        // 方波#1, APU频率驱动
        {
            g_states.square1_cycle += cpu_cycle_per_sample * 0.5f;
            const int count1 = (int)(g_states.square1_cycle / square1p);
            g_states.square1_cycle -= (float)count1 * square1p;
            g_states.square1_seq_index += (uint8_t)count1;
            g_states.square1_seq_index = g_states.square1_seq_index & 7;
        }
        const float square1
            = g_states.square1.volume 
            & sq_seq_mask[g_states.square1_seq_index | (g_states.square1.duty << 3)]
            ;
        // 方波#2, APU频率驱动
        {
            g_states.square2_cycle += cpu_cycle_per_sample * 0.5f;
            const int count2 = (int)(g_states.square2_cycle / square2p);
            g_states.square2_cycle -= (float)count2 * square2p;
            g_states.square2_seq_index += (uint8_t)count2;
            g_states.square2_seq_index = g_states.square2_seq_index & 7;
        }
        const float square2
            = g_states.square2.volume
            & sq_seq_mask[g_states.square2_seq_index | (g_states.square2.duty << 3)]
            ;
        // 三角波, CPU频率驱动
        {
            g_states.triangle_cycle += cpu_cycle_per_sample;
            const int count3 = (int)(g_states.triangle_cycle / trianglep);
            g_states.triangle_cycle -= (float)count3 * trianglep;
            g_states.triangle_seq_index 
                += g_states.triangle.inc_mask 
                & (uint8_t)count3
                ;
            g_states.triangle_seq_index = g_states.triangle_seq_index & 31;
        }
        const float triangle 
            = tri_seq[g_states.triangle_seq_index]
            & g_states.triangle.play_mask
            //& 0
            ;
        // 噪声, CPU频率驱动?
        {
            g_states.noise_cycle += cpu_cycle_per_sample ;
            const int count4 = (int)(g_states.noise_cycle / noise_p);
            g_states.noise_cycle -= (float)count4 * noise_p;
            do_lfsr_count(count4, g_states.noise.mode);
        }
        uint8_t mask = (uint8_t)(g_states.lfsr & 1); --mask;
        const float noise 
            = g_states.noise.volume
            & mask
            //& 0
            ;
        // DMC, CPU频率驱动
        {
            g_states.dmc_cycle += cpu_cycle_per_sample;
            // 最高是33kHz, 目前采样率是44.1kHz, 能保证每样本最多一次
            // 如果采样率低于33kHz, 则可能需要每次采样多次
            if (g_states.dmc_cycle > dmc_p) {
                g_states.dmc_cycle -= dmc_p;
                do_dmc();
            }
        }
        const float dmc = g_states.dmc.value;

        const float square_out = 95.88f / ((8128.f / (square1 + square2)) + 100.f);
        const float tnd_out 
            = 159.79f 
            / (1.f / (triangle / 8227.f + noise / 12241.f + dmc / 22638.f) + 100.f)
            ;
        // 2A03
        float output 
            = square_out
            //* 0.f
            + tnd_out
            //* 0.f
            ;

        // VRC6
        if (g_famicom->rom_info.extra_sound & SFC_NSF_EX_VCR6) {
            sfc_vrc6_data_t* const vrc6_data = &g_states.vrc6;
            //sfc_vrc6_data_t* const vrc6_data = &g_famicom->apu.vrc6;


            const float vrc6_squ1p = vrc6_data->square1.period;
            const float vrc6_squ2p = vrc6_data->square2.period;
            const float vrc6_raw_p = vrc6_data->saw.period;




            uint8_t vrc6 = 0;




            if (vrc6_data->square1.enable) {



                g_states.vrc6_squ1_cycle += cpu_cycle_per_sample;
                const int count1 = (int)(g_states.vrc6_squ1_cycle / vrc6_squ1p);
                g_states.vrc6_squ1_cycle -= (float)count1 * vrc6_squ1p;
                vrc6_data->square1.index += (uint8_t)count1;
                vrc6_data->square1.index &= 0xf;


                vrc6 += vrc6_data->square1.index <= vrc6_data->square1.duty
                    ? vrc6_data->square1.volume : 0;
            }

            if (vrc6_data->square2.enable) {

                g_states.vrc6_squ2_cycle += cpu_cycle_per_sample;
                const int count2 = (int)(g_states.vrc6_squ2_cycle / vrc6_squ2p);
                g_states.vrc6_squ2_cycle -= (float)count2 * vrc6_squ2p;
                vrc6_data->square2.index += (uint8_t)count2;
                vrc6_data->square2.index &= 0xf;

                vrc6 += vrc6_data->square2.index <= vrc6_data->square2.duty
                    ? vrc6_data->square2.volume : 0;
            }

            if (vrc6_data->saw.enable) {


                g_states.vrc6_saw__cycle += cpu_cycle_per_sample * 0.5f;
                const int count3 = (int)(g_states.vrc6_saw__cycle / vrc6_raw_p);
                g_states.vrc6_saw__cycle -= (float)count3 * vrc6_raw_p;
                vrc6_data->saw.accumulator += (uint8_t)((int)vrc6_data->saw.rate * count3);
                vrc6_data->saw.count += count3;
                if (vrc6_data->saw.count >= 7) {
                    vrc6_data->saw.count = 0;
                    vrc6_data->saw.accumulator = 0;
                }

                vrc6 += vrc6_data->saw.accumulator >> 3;
            }

            output += 0.00752f * (float)vrc6;
        }

        // VRC7
        if (g_famicom->rom_info.extra_sound & SFC_NSF_EX_VCR7) {
            const double adper = ((double)VRC7_OUTPUT_PER_SEC / (double)SAMPLES_PER_SEC);
            g_states.vrc7_index += (float)(adper - 1.0);
            int32_t vrc7 = sfc_vrc7_49716hz(g_famicom);
            if (g_states.vrc7_index >= 1.f) {
                g_states.vrc7_index -= 1.f;
                const int32_t newvrc7 = sfc_vrc7_49716hz(g_famicom);
                vrc7 += newvrc7;
                vrc7 /= 2;
            }
            const double weight = 2.0;
            output += (float)((double)vrc7 * weight / (double)(1 << 23));
        }
        // FDS
        if (g_famicom->rom_info.extra_sound & SFC_NSF_EX_FDS1) {
            float out = sfc_fds1_per_sample(g_famicom, &fds1_ctx, cpu_cycle_per_sample);
            //void debug_putaudio(float);
            //debug_putaudio(out);
            out = sfc_filter_rclp(&g_states.fds_lp2k, out * 1.41f);
            output += out;
        }

        // MMC5
        if (g_famicom->rom_info.extra_sound & SFC_NSF_EX_MMC5) {
            sfc_mmc5_per_sample(g_famicom, &mmc5_ctx, cpu_cycle_per_sample);
            const float squ = 0.00752f * (mmc5_ctx.squ1.output + mmc5_ctx.squ2.output);
            const float pcm = 0.00752f * 0.25f * (float)g_famicom->apu.mmc5.pcm_output;
            output += squ + pcm;
        }
        // N163
        if (g_famicom->rom_info.extra_sound & SFC_NSF_EX_N163) {
            // 3声道模式
            const float out = sfc_n163_per_sample(g_famicom, &n163_ctx, cpu_cycle_per_sample, 3);
            output += out * (0.00752f / 16.f) * n163_ctx.subweight;
        }
        // FME7
        if (g_famicom->rom_info.extra_sound & SFC_NSF_EX_FME7) {
            const float out = sfc_fme7_per_sample(g_famicom, &fme7_ctx, cpu_cycle_per_sample);
            output = out;
        }
        if (output > g_states.max_vol) g_states.max_vol = output;

        buffer[i] = output / g_states.max_vol
            //* 2.f
            ;
    }
    // 
    sfc_fds1_samplemode_end(g_famicom, &fds1_ctx);
    sfc_mmc5_samplemode_end(g_famicom, &mmc5_ctx);
    sfc_n163_samplemode_end(g_famicom, &n163_ctx);
    sfc_fme7_samplemode_end(g_famicom, &fme7_ctx);
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
    make_samples(old_index, now_index);
    g_states.last_cycle = cycle;


    switch (type)
    {
    case SFC_Overview:
        g_states.dmc = g_famicom->apu.dmc;
        g_states.dmc_enable = g_famicom->apu.status_write & SFC_APU4015_WRITE_EnableDMC;
    case SFC_FrameCounter:
        g_states.square1 = sfc_check_square1_state(g_famicom);
        g_states.square2 = sfc_check_square2_state(g_famicom);
        g_states.triangle.u32 = sfc_check_triangle_state(g_famicom).u32;
        g_states.noise.u32 = sfc_check_noise_state(g_famicom).u32;
        break;
    case SFC_2A03_Square1:
        g_states.square1 = sfc_check_square1_state(g_famicom);
        // 写入了$4003
        if (!g_famicom->apu.square1.seq_index) {
            g_famicom->apu.square1.seq_index = 1;
            g_states.square1_seq_index = 0;
        }
        break;
    case SFC_2A03_Square2:
        g_states.square2 = sfc_check_square2_state(g_famicom);
        // 写入了$4004
        if (!g_famicom->apu.square2.seq_index) {
            g_famicom->apu.square2.seq_index = 1;
            g_states.square2_seq_index = 0;
        }
        break;
    case SFC_2A03_Triangle:
        g_states.triangle.u32 = sfc_check_triangle_state(g_famicom).u32;
        break;
    case SFC_2A03_Noise:
        g_states.noise.u32 = sfc_check_noise_state(g_famicom).u32;
        break;
    case SFC_2A03_DMC:
        g_states.dmc = g_famicom->apu.dmc;
        break;
    case SFC_VRC6_Square1:
        g_famicom->apu.vrc6.square1.index = g_states.vrc6.square1.index;
        g_states.vrc6.square1 = g_famicom->apu.vrc6.square1;
        g_states.vrc6.halt = g_famicom->apu.vrc6.halt;
        break;
    case SFC_VRC6_Square2:
        g_famicom->apu.vrc6.square2.index = g_states.vrc6.square2.index;
        g_states.vrc6.square2 = g_famicom->apu.vrc6.square2;
        break;
    case SFC_VRC6_Saw:
        g_famicom->apu.vrc6.saw.count = g_states.vrc6.saw.count;
        g_famicom->apu.vrc6.saw.accumulator = g_states.vrc6.saw.accumulator;
        g_states.vrc6.saw = g_famicom->apu.vrc6.saw;
        break;
    }
    // 替换当前的DMC
    g_famicom->apu.dmc = g_states.dmc;
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

    g_states.square1.period = 1;
    g_states.square2.period = 1;
    g_states.triangle.period = 1;
    g_states.noise.period = 4;
    g_states.lfsr = 1;

    g_states.dmc.period = 500;

    g_states.max_vol = 1.f;
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
int main() {
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
    if (sfc_famicom_init(&famicom, NULL, &interfaces)) return 1;
    //qload();

    printf(
        "ROM: PRG-ROM: %d x 16kb   CHR-ROM %d x 8kb   Mapper: %03d\n",
        (int)famicom.rom_info.size_prgrom / (16*1024),
        (int)famicom.rom_info.size_chrrom / (8 *1024),
        (int)famicom.rom_info.mapper_number
    );
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
        // 状态

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
    FILE* const file = fopen("31_test_16.nes", "rb");
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
    if (g_debug_line < 0x77f70 || g_debug_return) return;
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
