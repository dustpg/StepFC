#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com

#include <stdint.h>


// 方波寄存器
struct sfc_square_data_t {
    // 控制寄存器
    uint8_t     ctrl;
    // 扫描单元寄存器
    uint8_t     sweep;
    // 微调寄存器
    uint32_t    fine;
    // 粗调寄存器
    uint32_t    coarse;
};

// CPU寄存器
typedef struct {
    // 方波 #1
    struct sfc_square_data_t    square1;

} sfc_apu_register_t;

