#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com

#include <stdint.h>


/// <summary>
/// The SFC disassembly buf length
/// </summary>
enum { SFC_DISASSEMBLY_BUF_LEN = 32 };

/// <summary>
/// StepFC: 6502机器码
/// </summary>
typedef union {
    // 用32位保存数据
    uint32_t    data;
    // 4个8位数据
    struct {
        // 操作码 
        uint8_t op;
        // 地址码1
        uint8_t a1;
        // 地址码2
        uint8_t a2;
        // 显示控制
        uint8_t ctrl;
    };

} sfc_6502_code_t;


// 反汇编
void sfc_6502_disassembly(sfc_6502_code_t, char buf[SFC_DISASSEMBLY_BUF_LEN]);


/// <summary>
/// StepFC: 6502指令
/// </summary>
enum sfc_6502_instruction {
    SFC_INS_UNK = 0,   // 未知指令
    SFC_INS_LDA,       // LDA--由存储器取数送入累加器A    M -> A
    SFC_INS_LDX,       // LDX--由存储器取数送入寄存器X    M -> X
    SFC_INS_LDY,       // LDY--由存储器取数送入寄存器Y    M -> Y
    SFC_INS_STA,       // STA--将累加器A的数送入存储器    A -> M
    SFC_INS_STX,       // STX--将寄存器X的数送入存储器    X -> M
    SFC_INS_STY,       // STY--将寄存器Y的数送入存储器    Y -> M
    SFC_INS_TAX,       // 将累加器A的内容送入变址寄存器X
    SFC_INS_TXA,       // 将变址寄存器X的内容送入累加器A
    SFC_INS_TAY,       // 将累加器A的内容送入变址寄存器Y
    SFC_INS_TYA,       // 将变址寄存器Y的内容送入累加器A
    SFC_INS_TSX,       // 堆栈指针S的内容送入变址寄存器X
    SFC_INS_TXS,       // 变址寄存器X的内容送入堆栈指针S
    SFC_INS_ADC,       // ADC--累加器,存储器,进位标志C相加,结果送累加器A  A+M+C -> A 
    SFC_INS_SBC,       // SBC--从累加器减去存储器和进位标志C取反,结果送累加器 A-M-(1-C) -> A
    SFC_INS_INC,       // INC--存储器单元内容增1  M+1 -> M
    SFC_INS_DEC,       // DEC--存储器单元内容减1  M-1 -> M
    SFC_INS_INX,       // INX--X寄存器+1 X+1 -> X
    SFC_INS_DEX,       // DEX--X寄存器-1 X-1 -> X
    SFC_INS_INY,       // INY--Y寄存器+1 Y+1 -> Y
    SFC_INS_DEY,       // DEY--Y寄存器-1 Y-1 -> Y
    SFC_INS_AND,       // AND--存储器与累加器相与,结果送累加器  A∧M -> A
    SFC_INS_ORA,       // ORA--存储器与累加器相或,结果送累加器  A∨M -> A
    SFC_INS_EOR,       // EOR--存储器与累加器异或,结果送累加器  A≮M -> A
    SFC_INS_CLC,       // CLC--清除进位标志C         0 -> C
    SFC_INS_SEC,       // SEC--设置进位标志C         1 -> C
    SFC_INS_CLD,       // CLD--清除十进标志D         0 -> D
    SFC_INS_SED,       // SED--设置十进标志D         1 -> D
    SFC_INS_CLV,       // CLV--清除溢出标志V         0 -> V
    SFC_INS_CLI,       // CLI--清除中断禁止V         0 -> I
    SFC_INS_SEI,       // SEI--设置中断禁止V         1 -> I
    SFC_INS_CMP,       // CMP--累加器和存储器比较
    SFC_INS_CPX,       // CPX--寄存器X的内容和存储器比较
    SFC_INS_CPY,       // CPY--寄存器Y的内容和存储器比较
    SFC_INS_BIT,       // BIT--位测试
    SFC_INS_ASL,       // ASL--算术左移 储存器
    SFC_INS_ASLA,      // ASL--算术左移 累加器
    SFC_INS_LSR,       // LSR--算术右移 储存器
    SFC_INS_LSRA,      // LSR--算术右移 累加器
    SFC_INS_ROL,       // ROL--循环算术左移 储存器
    SFC_INS_ROLA,      // ROL--循环算术左移 累加器
    SFC_INS_ROR,       // ROR--循环算术右移 储存器
    SFC_INS_RORA,      // ROR--循环算术右移 累加器
    SFC_INS_PHA,       // PHA--累加器进栈
    SFC_INS_PLA,       // PLA--累加器出栈
    SFC_INS_PHP,       // PHP--标志寄存器P进栈
    SFC_INS_PLP,       // PLP--标志寄存器P出栈
    SFC_INS_JMP,       // JMP--无条件跳转
    SFC_INS_BEQ,       // 如果标志位Z = 1则转移，否则继续
    SFC_INS_BNE,       // 如果标志位Z = 0则转移，否则继续
    SFC_INS_BCS,       // 如果标志位C = 1则转移，否则继续
    SFC_INS_BCC,       // 如果标志位C = 0则转移，否则继续
    SFC_INS_BMI,       // 如果标志位N = 1则转移，否则继续
    SFC_INS_BPL,       // 如果标志位N = 0则转移，否则继续
    SFC_INS_BVS,       // 如果标志位V = 1则转移，否则继续
    SFC_INS_BVC,       // 如果标志位V = 0则转移，否则继续
    SFC_INS_JSR,       // 跳转到子程序
    SFC_INS_RTS,       // 返回到主程序
    SFC_INS_NOP,       // 无操作
    SFC_INS_BRK,       // 强制中断
    SFC_INS_RTI,       // 从中断返回
    // --------  组合指令  ----------
    SFC_INS_ALR,       // [Unofficial&Combo] AND+LSR
    SFC_INS_ASR = SFC_INS_ALR,// 有消息称是叫这个
    SFC_INS_ANC,       // [Unofficial&Combo] AND+N2C?
    SFC_INS_AAC = SFC_INS_ANC,// 差不多一个意思
    SFC_INS_ARR,       // [Unofficial&Combo] AND+ROR [类似]
    SFC_INS_AXS,       // [Unofficial&Combo] AND+XSB?
    SFC_INS_SBX = SFC_INS_AXS,// 一个意思
    SFC_INS_LAX,       // [Unofficial&Combo] LDA+TAX
    SFC_INS_SAX,       // [Unofficial&Combo] STA&STX [类似]
    // -------- 读改写指令 ----------
    SFC_INS_DCP,       // [Unofficial& RMW ] DEC+CMP
    SFC_INS_ISC,       // [Unofficial& RMW ] INC+SBC
    SFC_INS_ISB = SFC_INS_ISC,// 差不多一个意思
    SFC_INS_RLA,       // [Unofficial& RMW ] ROL+AND
    SFC_INS_RRA,       // [Unofficial& RMW ] ROR+AND
    SFC_INS_SLO,       // [Unofficial& RMW ] ASL+ORA
    SFC_INS_SRE,       // [Unofficial& RMW ] LSR+EOR
    // -------- 卧槽 ----
    SFC_INS_LAS,
    SFC_INS_XAA,
    SFC_INS_AHX,
    SFC_INS_TAS,
    SFC_INS_SHX,
    SFC_INS_SHY,
};


/// <summary>
/// StepFC: 寻址方式
/// </summary>
enum sfc_6502_addressing_mode {
    SFC_AM_UNK = 0,     // 未知寻址
    SFC_AM_ACC,         // 操累加器A: Op Accumulator
    SFC_AM_IMP,         // 隐含 寻址: Implied    Addressing
    SFC_AM_IMM,         // 立即 寻址: Immediate  Addressing
    SFC_AM_ABS,         // 直接 寻址: Absolute   Addressing
    SFC_AM_ABX,         // 直接X变址: Absolute X Addressing
    SFC_AM_ABY,         // 直接Y变址: Absolute Y Addressing
    SFC_AM_ZPG,         // 零页 寻址: Zero-Page  Addressing
    SFC_AM_ZPX,         // 零页X变址: Zero-PageX Addressing
    SFC_AM_ZPY,         // 零页Y变址: Zero-PageY Addressing
    SFC_AM_INX,         // 间接X变址:  Pre-indexed Indirect Addressing
    SFC_AM_INY,         // 间接Y变址: Post-indexed Indirect Addressing
    SFC_AM_IND,         // 间接 寻址: Indirect   Addressing
    SFC_AM_REL,         // 相对 寻址: Relative   Addressing
};

