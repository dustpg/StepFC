#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com

#include <stdint.h>

// StepFC::Famicom
struct sfc_famicom;
typedef struct sfc_famicom sfc_famicom_t;

// 状态寄存器标记
enum sfc_status_index {
    SFC_INDEX_C = 0,
    SFC_INDEX_Z = 1,
    SFC_INDEX_I = 2,
    SFC_INDEX_D = 3,
    SFC_INDEX_B = 4,
    SFC_INDEX_R = 5,
    SFC_INDEX_V = 6,
    SFC_INDEX_S = 7,
    SFC_INDEX_N = SFC_INDEX_S,
};

// 状态寄存器标记
enum sfc_status_flag {
    SFC_FLAG_C = 1 << 0,    // 进位标记(Carry flag)
    SFC_FLAG_Z = 1 << 1,    // 零标记 (Zero flag)
    SFC_FLAG_I = 1 << 2,    // 禁止中断(Irq disabled flag)
    SFC_FLAG_D = 1 << 3,    // 十进制模式(Decimal mode flag)
    SFC_FLAG_B = 1 << 4,    // 软件中断(BRK flag)
    SFC_FLAG_R = 1 << 5,    // 保留标记(Reserved), 一直为1
    SFC_FLAG_V = 1 << 6,    // 溢出标记(Overflow  flag)
    SFC_FLAG_S = 1 << 7,    // 信号标记(Sign flag)
    SFC_FLAG_N = SFC_FLAG_S,// 又叫(Negative Flag)
};

// CPU寄存器
typedef struct {
    // 指令计数器 Program Counter
    uint16_t    program_counter;
    // 状态寄存器 Status Register
    uint8_t     status;
    // 累加寄存器 Accumulator
    uint8_t     accumulator;
    // X 变址寄存器 X Index Register
    uint8_t     x_index;
    // Y 变址寄存器 Y Index Register
    uint8_t     y_index;
    // 栈指针  Stack Pointer
    uint8_t     stack_pointer;
    // 保留对齐用
    uint8_t     unused;

} sfc_cpu_register_t;

/// <summary>
/// NES的CPU将最后几个地址称为向量(vector)
/// </summary>
enum sfc_cpu_vector {
    SFC_VERCTOR_NMI     = 0xFFFA,   // 不可屏蔽中断
    SFC_VERCTOR_RESET   = 0xFFFC,   // 重置CP指针地址
    SFC_VERCTOR_BRK     = 0xFFFE,   // 中断重定向
    SFC_VERCTOR_IRQ     = 0xFFFE,   // 中断重定向
};

// read cpu address
uint8_t sfc_read_cpu_address(uint16_t, sfc_famicom_t*);
// write cpu address
void sfc_write_cpu_address(uint16_t, uint8_t, sfc_famicom_t*);
// sfc cpu execute one instruction
void sfc_cpu_execute_one(sfc_famicom_t*);
