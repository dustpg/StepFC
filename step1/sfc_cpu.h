#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com

#include <stdint.h>

// StepFC::Famicom
struct sfc_famicom;
typedef struct sfc_famicom sfc_famicom_t;

/// <summary>
/// NES的CPU将最后几个地址称为向量(vector)
/// </summary>
enum sfc_cpu_vector {
    SFC_VERCTOR_NMI     = 0xFFFA,   // 不可屏蔽中断
    SFC_VERCTOR_RESET   = 0xFFFC,   // 重置CP指针地址
    SFC_VERCTOR_IRQBRK  = 0xFFFE,   // 中断重定向
};

// read cpu address
uint8_t sfc_read_cpu_address(uint16_t, const sfc_famicom_t*);
// write cpu address
void sfc_write_cpu_address(uint16_t, uint8_t, sfc_famicom_t*);
