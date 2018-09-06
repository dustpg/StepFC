#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com

#include <stdint.h>


enum {
    SFC_MAPPER_00_SIZE = 1,
    SFC_MAPPER_01_SIZE = 4,
};

// Mapper用缓存
typedef union {
    // 对齐用指针
    void*           unused;
    // Mapper 00
    uint8_t         mapper00[SFC_MAPPER_00_SIZE];
    // Mapper 01
    uint8_t         mapper01[2];

} sfc_mapper_buffer_t;



// typedef
struct sfc_famicom;
typedef struct sfc_famicom sfc_famicom_t;


typedef enum {
    SFC_NT_MIR_SingleLow = 0,
    SFC_NT_MIR_SingleHigh,
    SFC_NT_MIR_Vertical,
    SFC_NT_MIR_Horizontal,
    SFC_NT_MIR_FourScreen,
} sfc_nametable_mirroring_mode;

// 切换名称表模式
void sfc_switch_nametable_mirroring(sfc_famicom_t*, sfc_nametable_mirroring_mode);