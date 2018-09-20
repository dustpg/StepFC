#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com

#include <stdint.h>


/// <summary>
/// 方波状态
/// </summary>
typedef union  {
    // 结构体
    struct {
        // 方波 周期
        uint16_t    period;
        // 方波 音量
        uint8_t     volume;
        // 方波 占空比
        uint8_t     duty;
    };
    // U32数据
    uint32_t        u32;

} sfc_square_channel_state_t;


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