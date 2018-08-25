#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com

#include <stdint.h>


/// <summary>
/// StepFC: 配置信息
/// </summary>
typedef struct {
    // 屏幕刷新率
    uint16_t        refresh_rate;
    // 每条扫描线CPU执行周期
    uint16_t        cpu_cycle_per_scanline;
    // 每条扫描线渲染CPU执行周期
    uint16_t        cpu_cycle_per_drawline;
    // 每条扫描线水平空白CPU执行周期
    uint16_t        cpu_cycle_per_hblank;
    // 可见扫描线
    uint16_t        visible_scanline;
    // 垂直空白扫描线
    uint16_t        vblank_scanline;

} sfc_config_t;

// NTSC
extern const sfc_config_t SFC_CONFIG_NTSC;
// PAL
//extern const sfc_config_t SFC_CONFIG_PAL;
