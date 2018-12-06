#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com

#include <stdint.h>

//#define SFC_SMF_DEFINED

// 目前使用16.16定点小数, 可以换成6.10~8.8的16bit
typedef uint32_t sfc_fixed_t;

#define SFC_FIXED(x) (x << 16)

// 创建
static inline sfc_fixed_t sfc_fixed_make(uint32_t a, uint32_t b) {
    // 防溢出操作

    // 整数部分
    const uint32_t part0 = a / b;
    const uint32_t hi16 = part0 << 16;
    // 小数部分
    const uint32_t part1 = a % b;
    const uint32_t lo16 = (part1 << 16) / b;
    return hi16 | lo16;
}

// 增加
static inline uint32_t sfc_fixed_add(sfc_fixed_t* a, sfc_fixed_t b) {
    *a += b; const uint32_t rv = *a >> 16; *a &= 0xffff; return rv;
}


// -------------------------------------------------------
//                         2A03
// -------------------------------------------------------

/*
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


/// <summary>
/// StepFC: 混方波
/// </summary>
/// <param name="sq1">The SQ1.</param>
/// <param name="sq2">The SQ2.</param>
/// <returns></returns>
static inline float sfc_mix_square(float sq1, float sq2) {
    return 95.88f / ((8128.f / (sq1 + sq2)) + 100.f);
}

/// <summary>
/// StepFC: 混TND
/// </summary>
/// <param name="triangle">The triangle.</param>
/// <param name="noise">The noise.</param>
/// <param name="dmc">The DMC.</param>
/// <returns></returns>
static inline float sfc_mix_tnd(float triangle, float noise, float dmc) {
    return 159.79f / (1.f / (triangle / 8227.f + noise / 12241.f + dmc / 22638.f) + 100.f);
}

/// <summary>
/// 方波状态
/// </summary>
typedef struct {
    // 方波 周期 预乘2
    uint16_t    period_x2;
    // 方波 音量
    uint8_t     volume;
    // 方波 占空比
    uint8_t     duty;

} sfc_square_ch_state_t;

typedef sfc_square_ch_state_t sfc_square_channel_state_t;


/// <summary>
/// 三角波状态
/// </summary>
typedef struct {
    // 三角波 周期
    uint16_t    period;
    // 三角波 递增掩码
    uint8_t     inc_mask;
    // 三角波 播放掩码
    uint8_t     play_mask;

} sfc_triangle_ch_state_t;


/// <summary>
/// 噪声状态
/// </summary>
typedef struct {
    // 噪声 周期
    uint16_t    period;
    // 噪声 右移位数 6[短模式], 1[长模式]
    uint8_t     count;
    // 三角波 音量
    uint8_t     volume;

} sfc_noise_ch_state_t;

// typedef
struct sfc_famicom;
typedef struct sfc_famicom sfc_famicom_t;


/// <summary>
/// StepFC: 2A03 用 样本模式整型上下文
/// </summary>
typedef struct {
    // 方波#1 输出
    float                   sq1_output;
    // 方波#2 输出
    float                   sq2_output;
    // 三角波 输出
    float                   tri_output;
    // 噪音 输出
    float                   noi_output;
    // DMC 输出
    float                   dmc_output;
    // 方波#1 状态
    sfc_square_ch_state_t   sq1_state;
    // 方波#2 状态
    sfc_square_ch_state_t   sq2_state;
    // 三角波状态
    sfc_triangle_ch_state_t tri_state;
    // 噪音状态
    sfc_noise_ch_state_t    noi_state;
    // 定点小数 方波#1 
    sfc_fixed_t             sq1_clock;
    // 定点小数 方波#2
    sfc_fixed_t             sq2_clock;
    // 定点小数 三角波澜
    sfc_fixed_t             tri_clock;
    // 定点小数 噪音
    sfc_fixed_t             noi_clock;
    // 定点小数 DMC
    sfc_fixed_t             dmc_clock;

} sfc_2a03_smi_ctx_t;



// 2A03 整型采样模式 - 采样
void sfc_2a03_smi_sample(sfc_famicom_t*, sfc_2a03_smi_ctx_t*, const float[], sfc_fixed_t cps);
// 2A03 整型采样模式 - 更新方波#1
void sfc_2a03_smi_update_sq1(const sfc_famicom_t*, sfc_2a03_smi_ctx_t*);
// 2A03 整型采样模式 - 更新方波#2
void sfc_2a03_smi_update_sq2(const sfc_famicom_t*, sfc_2a03_smi_ctx_t*);
// 2A03 整型采样模式 - 更新三角波
void sfc_2a03_smi_update_tri(const sfc_famicom_t*, sfc_2a03_smi_ctx_t*);
// 2A03 整型采样模式 - 更新噪音
void sfc_2a03_smi_update_noi(const sfc_famicom_t*, sfc_2a03_smi_ctx_t*);

// -------------------------------------------------------
//                         VRC6
// -------------------------------------------------------


/// <summary>
/// StepFC: VRC7 用 样本模式整型上下文
/// </summary>
typedef struct {
    // 方波#1 输出
    float           square1_output;
    // 方波#2 输出
    float           square2_output;
    // 锯齿波输出
    float           sawtooth_output;
    // 方波#1 定点小数
    sfc_fixed_t     sq1_clock;
    // 方波#2 定点小数
    sfc_fixed_t     sq2_clock;
    // 锯齿波 定点小数
    sfc_fixed_t     saw_clock;
} sfc_vrc6_smi_ctx_t;


// VRC6 整型采样模式 - 采样
void sfc_vrc6_smi_sample(sfc_famicom_t*, sfc_vrc6_smi_ctx_t*, const float[], sfc_fixed_t cps);

// -------------------------------------------------------
//                         VRC7
// -------------------------------------------------------


/// <summary>
/// StepFC: VRC7 用 样本模式整型上下文
/// </summary>
typedef struct {
    // 输出
    float           output[6];
    // 合成
    float           mixed;
    // 定点小数
    sfc_fixed_t     clock;
} sfc_vrc7_smi_ctx_t;


// VRC7 整型采样模式 - 采样
void sfc_vrc7_smi_sample(sfc_famicom_t*, sfc_vrc7_smi_ctx_t*, const float[], sfc_fixed_t cps);



// -------------------------------------------------------
//                         FDS1
// -------------------------------------------------------


/// <summary>
/// StepFC: FDS1 用 样本模式整型上下文
/// </summary>
typedef struct {
    // 输出
    float           output;
    // 定点小数 音量包络
    sfc_fixed_t     volenv_clock;
    // 定点小数 调制包络
    sfc_fixed_t     modenv_clock;
    // 定点小数 波输出
    sfc_fixed_t     wavout_clock;
    // 定点小数 调制器
    sfc_fixed_t     mdunit_clock;
} sfc_fds1_smi_ctx_t;


// FDS1 整型采样模式 - 采样
void sfc_fds1_smi_sample(sfc_famicom_t*, sfc_fds1_smi_ctx_t*, const float[], sfc_fixed_t cps);

// 浮点版本
#ifdef SFC_SMF_DEFINED

// FDS 高级接口1
float sfc_fds1_get_output(sfc_famicom_t*);
void sfc_fds1_per_cpu_clock(sfc_famicom_t*);
// FDS 高级接口2
typedef struct {
    float       volenv_clock;
    float       volenv_tps;
    float       modenv_clock;
    float       modenv_tps;
    float       wavout_clock;
    float       mdunit_clock;
    float       mdunit_rate;

    float       cycle_remain;
} sfc_fds1_ctx_t;
float sfc_fds1_per_sample(sfc_famicom_t*, sfc_fds1_ctx_t*, float cps);
void sfc_fds1_samplemode_begin(sfc_famicom_t*, sfc_fds1_ctx_t*);
void sfc_fds1_samplemode_end(sfc_famicom_t*, sfc_fds1_ctx_t*);

#endif


// -------------------------------------------------------
//                         MMC5
// -------------------------------------------------------

/// <summary>
/// StepFC: MMC5 用 样本模式整型上下文
/// </summary>
typedef struct {
    // 方波#1 输出
    float                   sq1_output;
    // 方波#2 输出
    float                   sq2_output;
    // PCM 输出
    float                   pcm_output;
    // 方波#1 状态
    sfc_square_ch_state_t   sq1_state;
    // 方波#2 状态
    sfc_square_ch_state_t   sq2_state;
    // 定点小数 方波#1 
    sfc_fixed_t             sq1_clock;
    // 定点小数 方波#2
    sfc_fixed_t             sq2_clock;

} sfc_mmc5_smi_ctx_t;


// MMC5 整型采样模式 - 采样
void sfc_mmc5_smi_sample(sfc_famicom_t*, sfc_mmc5_smi_ctx_t*, const float[], sfc_fixed_t cps);
// MMC5 整型采样模式 - 更新方波#1
void sfc_mmc5_smi_update_sq1(const sfc_famicom_t*, sfc_mmc5_smi_ctx_t*);
// MMC5 整型采样模式 - 更新方波#2
void sfc_mmc5_smi_update_sq2(const sfc_famicom_t*, sfc_mmc5_smi_ctx_t*);
// MMC5 整型采样模式 - 更新PCM
//void sfc_mmc5_smi_update_pcm(const sfc_famicom_t*, sfc_mmc5_smi_ctx_t*);

// 浮点版本
#ifdef SFC_SMF_DEFINED
typedef struct {
    // 声道状态
    sfc_square_ch_state_t   state;
    // 方波 时钟周期 - 浮点误差修正
    float                   clock;
    // 方波 浮点周期
    float                   period;
    // 方波 输出音量
    float                   output;
}sfc_mmc5_square_ctx_t;

typedef struct {
    sfc_mmc5_square_ctx_t   squ1;
    sfc_mmc5_square_ctx_t   squ2;
} sfc_mmc5_ctx_t;

void sfc_mmc5_per_sample(sfc_famicom_t*, sfc_mmc5_ctx_t*, float cps);
void sfc_mmc5_samplemode_begin(sfc_famicom_t*, sfc_mmc5_ctx_t*);
void sfc_mmc5_samplemode_end(sfc_famicom_t*, sfc_mmc5_ctx_t*);

#endif

// -------------------------------------------------------
//                         N163
// -------------------------------------------------------



/// <summary>
/// StepFC: FME7用 样本模式整型上下文
/// </summary>
/// <remarks>
/// 各个声道直接访问 .apu.fme7 数据
/// </remarks>
typedef struct {
    // 输出
    float           output;
    // 副Mapper指定权重
    float           subweight;
    // 定点小数 包络用时钟数
    sfc_fixed_t     clock;

} sfc_n163_smi_ctx_t;

// N163 整型采样模式 - 采样
void sfc_n163_smi_sample(sfc_famicom_t*, sfc_n163_smi_ctx_t*, const float[], sfc_fixed_t cps, uint8_t mode);
// N163 整型采样模式 - 更新副权重
void sfc_n163_smi_update_subweight(const sfc_famicom_t*, sfc_n163_smi_ctx_t* );

// 浮点版本
#ifdef SFC_SMF_DEFINED
typedef struct {
    float               clock;
    float               subweight;
} sfc_n163_ctx_t;

float sfc_n163_per_sample(sfc_famicom_t*, sfc_n163_ctx_t*, float cps, uint8_t mode);
void sfc_n163_samplemode_begin(sfc_famicom_t*, sfc_n163_ctx_t*);
void sfc_n163_samplemode_end(sfc_famicom_t*, sfc_n163_ctx_t*);

#endif


// -------------------------------------------------------
//                         FME-7(5B)
// -------------------------------------------------------

/// <summary>
/// StepFC: FME7用 样本模式整型上下文
/// </summary>
typedef struct {
    // 输出
    float           output[3];
    // 声道用时钟数 定点小数
    sfc_fixed_t     chn_clock[3];
    // 噪音用时钟数 定点小数
    sfc_fixed_t     noi_clock;
    // 包络用时钟数 定点小数
    sfc_fixed_t     env_clock;
} sfc_fme7_smi_ctx_t;


// FME7 整型采样模式 - 采样
void sfc_fme7_smi_sample(sfc_famicom_t*, sfc_fme7_smi_ctx_t*, const float[], sfc_fixed_t cps);



// 浮点版本
#ifdef SFC_SMF_DEFINED
typedef struct {
    struct sfc_fme7_tone_s {
        float           period;
        float           clock;
    }                   ch[3];
    float               noise_period;
    float               noise_clock;
    float               env_period;
    float               env_clock;
} sfc_fme7_ctx_t;

float sfc_fme7_per_sample(sfc_famicom_t*, sfc_fme7_ctx_t*, float cps);
void sfc_fme7_samplemode_begin(sfc_famicom_t*, sfc_fme7_ctx_t*);
void sfc_fme7_samplemode_end(sfc_famicom_t*, sfc_fme7_ctx_t*);
#endif

