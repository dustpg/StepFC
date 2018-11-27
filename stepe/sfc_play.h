#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com

#include <stdint.h>



// -------------------------------------------------------
//                         2A03
// -------------------------------------------------------

/// <summary>
/// 方波状态
/// </summary>
typedef struct {
    // 方波 周期
    uint16_t    period;
    // 方波 音量
    uint8_t     volume;
    // 方波 占空比
    uint8_t     duty;

} sfc_square_ch_state_t;

typedef sfc_square_ch_state_t sfc_square_channel_state_t;


/// <summary>
/// 三角波状态
/// </summary>
typedef union {
    // 结构体
    struct {
        // 三角波 周期
        uint16_t    period;
        // 三角波 递增掩码
        uint8_t     inc_mask;
        // 三角波 播放掩码
        uint8_t     play_mask;
    };
    // U32数据
    uint32_t        u32;

} sfc_triangle_channel_state_t;


/// <summary>
/// 噪声状态
/// </summary>
typedef union {
    // 结构体
    struct {
        // 噪声 周期
        uint16_t    period;
        // 噪声 模式
        uint8_t     mode;
        // 三角波 音量
        uint8_t     volume;
    };
    // U32数据
    uint32_t        u32;

} sfc_noise_channel_state_t;

// typedef
struct sfc_famicom;
typedef struct sfc_famicom sfc_famicom_t;

// 获取方波#1状态
extern sfc_square_channel_state_t 
sfc_check_square1_state(const sfc_famicom_t*);

// 获取方波#2状态
extern sfc_square_channel_state_t
sfc_check_square2_state(const sfc_famicom_t*);

// 获取三角波状态
extern sfc_triangle_channel_state_t
sfc_check_triangle_state(const sfc_famicom_t*);

// 获取噪声状态
extern sfc_noise_channel_state_t
sfc_check_noise_state(const sfc_famicom_t*);




// -------------------------------------------------------
//                         FDS
// -------------------------------------------------------



// FDS 高级接口1
float sfc_fds_get_output(sfc_famicom_t*);
void sfc_fds_per_cpu_clock(sfc_famicom_t*);
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
} sfc_fds_ctx_t;
float sfc_fds_per_sample(sfc_famicom_t*, sfc_fds_ctx_t*, float cps);
void sfc_fds_samplemode_begin(sfc_famicom_t*, sfc_fds_ctx_t*, float cps);
void sfc_fds_samplemode_end(sfc_famicom_t*, sfc_fds_ctx_t*);



// -------------------------------------------------------
//                         MMC5
// -------------------------------------------------------

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
