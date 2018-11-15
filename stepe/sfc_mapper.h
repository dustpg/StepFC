#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com

#include <stdint.h>


enum {
    SFC_MAPPER_01_SIZE = 4,
    SFC_MAPPER_04_SIZE = 16,
    SFC_MAPPER_18_SIZE = 16,
    SFC_MAPPER_55_SIZE = 4,
};

// Mapper用缓存
typedef union {
    // 对齐用指针
    void*           unused;
    // Mapper 01
    uint8_t         mapper01[SFC_MAPPER_01_SIZE];
    // Mapper 04
    uint8_t         mapper04[SFC_MAPPER_04_SIZE];
    // Mapper 18
    uint8_t         mapper18[SFC_MAPPER_18_SIZE];
    // Mapper 55
    uint8_t         mapper55[SFC_MAPPER_55_SIZE];

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