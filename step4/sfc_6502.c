#include "sfc_6502.h"
#include "sfc_famicom.h"
#include <assert.h>
#include <string.h>

// 实用宏定义

// 寄存器
#define SFC_REG (famicom->registers)
#define SFC_PC (SFC_REG.program_counter)
#define SFC_SP (SFC_REG.stack_pointer)
#define SFC_A (SFC_REG.accumulator)
#define SFC_X (SFC_REG.x_index)
#define SFC_Y (SFC_REG.y_index)
#define SFC_P (SFC_REG.status)

// if中判断用FLAG
#define SFC_CF (SFC_P & (uint8_t)SFC_FLAG_C)
#define SFC_ZF (SFC_P & (uint8_t)SFC_FLAG_Z)
#define SFC_IF (SFC_P & (uint8_t)SFC_FLAG_I)
#define SFC_DF (SFC_P & (uint8_t)SFC_FLAG_D)
#define SFC_BF (SFC_P & (uint8_t)SFC_FLAG_B)
#define SFC_VF (SFC_P & (uint8_t)SFC_FLAG_V)
#define SFC_SF (SFC_P & (uint8_t)SFC_FLAG_S)
// 将FLAG将变为1
#define SFC_CF_SE (SFC_P |= (uint8_t)SFC_FLAG_C)
#define SFC_ZF_SE (SFC_P |= (uint8_t)SFC_FLAG_Z)
#define SFC_IF_SE (SFC_P |= (uint8_t)SFC_FLAG_I)
#define SFC_DF_SE (SFC_P |= (uint8_t)SFC_FLAG_D)
#define SFC_BF_SE (SFC_P |= (uint8_t)SFC_FLAG_B)
#define SFC_RF_SE (SFC_P |= (uint8_t)SFC_FLAG_R)
#define SFC_VF_SE (SFC_P |= (uint8_t)SFC_FLAG_V)
#define SFC_SF_SE (SFC_P |= (uint8_t)SFC_FLAG_S)
// 将FLAG将变为0
#define SFC_CF_CL (SFC_P &= ~(uint8_t)SFC_FLAG_C)
#define SFC_ZF_CL (SFC_P &= ~(uint8_t)SFC_FLAG_Z)
#define SFC_IF_CL (SFC_P &= ~(uint8_t)SFC_FLAG_I)
#define SFC_DF_CL (SFC_P &= ~(uint8_t)SFC_FLAG_D)
#define SFC_BF_CL (SFC_P &= ~(uint8_t)SFC_FLAG_B)
#define SFC_VF_CL (SFC_P &= ~(uint8_t)SFC_FLAG_V)
#define SFC_SF_CL (SFC_P &= ~(uint8_t)SFC_FLAG_S)
// 将FLAG将变为0或者1
#define SFC_CF_IF(x) (x ? SFC_CF_SE : SFC_CF_CL);
#define SFC_ZF_IF(x) (x ? SFC_ZF_SE : SFC_ZF_CL);
#define SFC_OF_IF(x) (x ? SFC_IF_SE : SFC_IF_CL);
#define SFC_DF_IF(x) (x ? SFC_DF_SE : SFC_DF_CL);
#define SFC_BF_IF(x) (x ? SFC_BF_SE : SFC_BF_CL);
#define SFC_VF_IF(x) (x ? SFC_VF_SE : SFC_VF_CL);
#define SFC_SF_IF(x) (x ? SFC_SF_SE : SFC_SF_CL);

// 实用函数
#define SFC_READ(a) sfc_read_cpu_address(a, famicom)
#define SFC_PUSH(a) (famicom->main_memory + 0x100)[SFC_SP--] = a;
#define SFC_POP() (famicom->main_memory + 0x100)[++SFC_SP];
#define SFC_WRITE(a, v) sfc_write_cpu_address(a, v, famicom)
#define CHECK_ZSFLAG(x) { SFC_SF_IF(x & (uint8_t)0x80); SFC_ZF_IF(x == 0); }
// 指令实现
#define OP(n, a, o) \
case 0x##n:\
{           \
    const uint16_t address = sfc_addressing_##a(famicom);\
    sfc_operation_##o(address, famicom);\
    break;\
}


// ---------------------------------- 寻址


/// <summary>
/// 寻址方式: 未知
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_UNK(sfc_famicom_t* famicom) {
    assert(!"UNKNOWN ADDRESSING MODE");
    return 0;
}

/// <summary>
/// 寻址方式: 累加器
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_ACC(sfc_famicom_t* famicom) {
    return 0;
}

/// <summary>
/// 寻址方式: 隐含寻址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_IMP(sfc_famicom_t* famicom) {
    return 0;
}

/// <summary>
/// 寻址方式: 立即寻址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_IMM(sfc_famicom_t* famicom) {
    const uint16_t address = SFC_PC; 
    SFC_PC++;
    return address;
}

/// <summary>
/// 寻址方式: 绝对寻址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_ABS(sfc_famicom_t* famicom) {
    const uint8_t address0 = SFC_READ(SFC_PC++);
    const uint8_t address1 = SFC_READ(SFC_PC++);
    return (uint16_t)address0 | (uint16_t)((uint16_t)address1 << 8);
}

/// <summary>
/// 寻址方式: 绝对X变址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_ABX(sfc_famicom_t* famicom) {
    const uint16_t base = sfc_addressing_ABS(famicom);
    return base + SFC_X;
}

/// <summary>
/// 寻址方式: 绝对Y变址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_ABY(sfc_famicom_t* famicom) {
    const uint16_t base = sfc_addressing_ABS(famicom);
    return base + SFC_Y;
}


/// <summary>
/// 寻址方式: 零页寻址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_ZPG(sfc_famicom_t* famicom) {
    const uint16_t address = SFC_READ(SFC_PC++);
    return address;
}

/// <summary>
/// 寻址方式: 零页X变址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_ZPX(sfc_famicom_t* famicom) {
    const uint16_t base = sfc_addressing_ZPG(famicom);
    const uint16_t index = base + SFC_X;
    return index & (uint16_t)0x00FF;
}

/// <summary>
/// 寻址方式: 零页Y变址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_ZPY(sfc_famicom_t* famicom) {
    const uint16_t base = sfc_addressing_ZPG(famicom);
    const uint16_t index = base + SFC_Y;
    return index & (uint16_t)0x00FF;
}

/// <summary>
/// 寻址方式: 间接X变址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_INX(sfc_famicom_t* famicom) {
    uint8_t base = SFC_READ(SFC_PC++) + SFC_X;
    const uint8_t address0 = SFC_READ(base++);
    const uint8_t address1 = SFC_READ(base++);
    return (uint16_t)address0 | (uint16_t)((uint16_t)address1 << 8);
}

/// <summary>
/// 寻址方式: 间接Y变址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_INY(sfc_famicom_t* famicom) {
    uint8_t base = SFC_READ(SFC_PC++);
    const uint8_t address0 = SFC_READ(base++);
    const uint8_t address1 = SFC_READ(base++);
    const uint16_t address 
        = (uint16_t)address0 
        | (uint16_t)((uint16_t)address1 << 8)
        ;
    return address + SFC_Y;
}

/// <summary>
/// 寻址方式: 间接寻址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_IND(sfc_famicom_t* famicom) {
    // 读取地址
    const uint16_t base1 = sfc_addressing_ABS(famicom);
    // 刻意实现6502的BUG
    const uint16_t base2
        = (base1 & (uint16_t)0xFF00)
        | ((base1 + 1) & (uint16_t)0x00FF)
        ;
    // 读取地址
    const uint16_t address 
        = (uint16_t)SFC_READ(base1)
        | (uint16_t)((uint16_t)SFC_READ(base2) << 8)
        ;
    return address;
}

/// <summary>
/// 寻址方式: 相对寻址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_REL(sfc_famicom_t* famicom) {
    const uint8_t data = SFC_READ(SFC_PC++);
    const uint16_t address = SFC_PC + (int8_t)data;
    return address;
}


// ---------------------------------- 指令

/// <summary>
/// UNK: Unknown
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_UNK(uint16_t address, sfc_famicom_t* famicom) {
    assert(!"UNKNOWN INS");
}

/// <summary>
/// SHY
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SHY(uint16_t address, sfc_famicom_t* famicom) {
    sfc_operation_UNK(address, famicom);
}

/// <summary>
/// SHX
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SHX(uint16_t address, sfc_famicom_t* famicom) {
    sfc_operation_UNK(address, famicom);
}

/// <summary>
/// TAS
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_TAS(uint16_t address, sfc_famicom_t* famicom) {
    sfc_operation_UNK(address, famicom);
}

/// <summary>
/// AHX
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_AHX(uint16_t address, sfc_famicom_t* famicom) {
    sfc_operation_UNK(address, famicom);
}

/// <summary>
/// XAA
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_XAA(uint16_t address, sfc_famicom_t* famicom) {
    sfc_operation_UNK(address, famicom);
}

/// <summary>
/// LAS
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_LAS(uint16_t address, sfc_famicom_t* famicom) {
    sfc_operation_UNK(address, famicom);
}

/// <summary>
/// SRE: Shift Right then "Exclusive-Or" - LSR + EOR
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SRE(uint16_t address, sfc_famicom_t* famicom) {
    // LSR
    uint8_t data = SFC_READ(address);
    SFC_CF_IF(data & 1);
    data >>= 1;
    SFC_WRITE(address, data);
    // EOR
    SFC_A ^= data;
    CHECK_ZSFLAG(SFC_A);
}

/// <summary>
/// SLO - Shift Left then 'Or' - ASL + ORA
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SLO(uint16_t address, sfc_famicom_t* famicom) {
    // ASL
    uint8_t data = SFC_READ(address);
    SFC_CF_IF(data & (uint8_t)0x80);
    data <<= 1;
    SFC_WRITE(address, data);
    // ORA
    SFC_A |= data;
    CHECK_ZSFLAG(SFC_A);
}

/// <summary>
/// RRA: Rotate Right then Add with Carry - ROR + ADC
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_RRA(uint16_t address, sfc_famicom_t* famicom) {
    // ROR
    uint16_t result16_ror = SFC_READ(address);
    result16_ror |= ((uint16_t)SFC_CF) << (8 - SFC_INDEX_C);
    const uint16_t tmpcf = result16_ror & 1;
    result16_ror >>= 1;
    const uint8_t result8_ror = (uint8_t)result16_ror;
    SFC_WRITE(address, result8_ror);
    // ADC
    const uint8_t src = result8_ror;
    const uint16_t result16 = (uint16_t)SFC_A + (uint16_t)src + tmpcf;
    SFC_CF_IF(result16 >> 8);
    const uint8_t result8 = (uint8_t)result16;
    SFC_VF_IF(!((SFC_A ^ src) & 0x80) && ((SFC_A ^ result8) & 0x80));
    SFC_A = result8;
    CHECK_ZSFLAG(SFC_A);
}

/// <summary>
/// RLA: Rotate Left then 'And' - ROL + AND
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_RLA(uint16_t address, sfc_famicom_t* famicom) {
    // ROL
    uint16_t result16 = SFC_READ(address);
    result16 <<= 1;
    result16 |= ((uint16_t)SFC_CF) >> (SFC_INDEX_C);
    SFC_CF_IF(result16 & (uint16_t)0x100);
    const uint8_t result8 = (uint8_t)result16;
    SFC_WRITE(address, result8);
    // AND
    SFC_A &= result8;
    CHECK_ZSFLAG(SFC_A);
}

/// <summary>
/// ISB: Increment memory then Subtract with Carry - INC + SBC
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ISB(uint16_t address, sfc_famicom_t* famicom) {
    // INC
    uint8_t data = SFC_READ(address);
    ++data;
    SFC_WRITE(address, data);
    // SBC
    const uint8_t src = data;
    const uint16_t result16 = (uint16_t)SFC_A - (uint16_t)src - (SFC_CF ? 0 : 1);
    SFC_CF_IF(!(result16 >> 8));
    const uint8_t result8 = (uint8_t)result16;
    SFC_VF_IF(((SFC_A ^ src) & 0x80) && ((SFC_A ^ result8) & 0x80));
    SFC_A = result8;
    CHECK_ZSFLAG(SFC_A);
}

/// <summary>
/// ISC
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ISC(uint16_t address, sfc_famicom_t* famicom) {
    sfc_operation_UNK(address, famicom);
}

/// <summary>
/// DCP: Decrement memory then Compare with A - DEC + CMP
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_DCP(uint16_t address, sfc_famicom_t* famicom) {
    // DEC
    uint8_t data = SFC_READ(address);
    --data;
    SFC_WRITE(address, data);
    // CMP
    const uint16_t result16 = (uint16_t)SFC_A - (uint16_t)data;
    SFC_CF_IF(!(result16 & (uint16_t)0x8000));
    CHECK_ZSFLAG((uint8_t)result16);
}

/// <summary>
/// SAX: Store A 'And' X - 
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SAX(uint16_t address, sfc_famicom_t* famicom) {
    SFC_WRITE(address, SFC_A & SFC_X);
}

/// <summary>
/// LAX: Load 'A' then Transfer X - LDA  + TAX
/// </summary>
/// <remarks>
/// 非法指令
/// </remarks>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_LAX(uint16_t address, sfc_famicom_t* famicom) {
    SFC_A = SFC_READ(address);
    SFC_X = SFC_A;
    CHECK_ZSFLAG(SFC_X);
}

/// <summary>
/// SBX
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SBX(uint16_t address, sfc_famicom_t* famicom) {
    sfc_operation_UNK(address, famicom);
}

/// <summary>
/// AXS
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_AXS(uint16_t address, sfc_famicom_t* famicom) {
    sfc_operation_UNK(address, famicom);
}

/// <summary>
/// ARR
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ARR(uint16_t address, sfc_famicom_t* famicom) {
    sfc_operation_UNK(address, famicom);
}

/// <summary>
/// AAC
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_AAC(uint16_t address, sfc_famicom_t* famicom) {
    sfc_operation_UNK(address, famicom);
}

/// <summary>
/// ANC
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ANC(uint16_t address, sfc_famicom_t* famicom) {
    sfc_operation_UNK(address, famicom);
}

/// <summary>
/// ASR
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ASR(uint16_t address, sfc_famicom_t* famicom) {
    sfc_operation_UNK(address, famicom);
}

/// <summary>
/// ALR
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ALR(uint16_t address, sfc_famicom_t* famicom) {
    sfc_operation_UNK(address, famicom);
}

/// <summary>
/// RTI: Return from I
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_RTI(uint16_t address, sfc_famicom_t* famicom) {
    // P
    SFC_P = SFC_POP();
    SFC_RF_SE;
    SFC_BF_CL;
    // PC
    const uint8_t pcl = SFC_POP();
    const uint8_t pch = SFC_POP();
    SFC_PC
        = (uint16_t)pcl
        | (uint16_t)pch << 8
        ;
}

/// <summary>
/// BRK
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BRK(uint16_t address, sfc_famicom_t* famicom) {
    sfc_operation_UNK(address, famicom);
}

/// <summary>
/// NOP: No Operation
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_NOP(uint16_t address, sfc_famicom_t* famicom) {

}

/// <summary>
/// RTS: Return from Subroutine
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_RTS(uint16_t address, sfc_famicom_t* famicom) {
    const uint8_t pcl = SFC_POP();
    const uint8_t pch = SFC_POP();
    SFC_PC
        = (uint16_t)pcl
        | (uint16_t)pch << 8
        ;
    SFC_PC++;
}

/// <summary>
/// JSR: Jump to Subroutine
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_JSR(uint16_t address, sfc_famicom_t* famicom) {
    const uint16_t pc1 = SFC_PC - 1;
    SFC_PUSH((uint8_t)(pc1 >> 8));
    SFC_PUSH((uint8_t)(pc1));
    SFC_PC = address;
}

/// <summary>
/// BVC: Branch if Overflow Clear
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BVC(uint16_t address, sfc_famicom_t* famicom) {
    if (!SFC_VF) SFC_PC = address;
}

/// <summary>
/// BVC: Branch if Overflow Set
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BVS(uint16_t address, sfc_famicom_t* famicom) {
    if (SFC_VF) SFC_PC = address;
}

/// <summary>
/// BPL: Branch if Plus
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BPL(uint16_t address, sfc_famicom_t* famicom) {
    if (!SFC_SF) SFC_PC = address;
}

/// <summary>
/// BMI: Branch if Minus
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BMI(uint16_t address, sfc_famicom_t* famicom) {
    if (SFC_SF) SFC_PC = address;
}

/// <summary>
/// BCC: Branch if Carry Clear
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BCC(uint16_t address, sfc_famicom_t* famicom) {
    if (!SFC_CF) SFC_PC = address;
}

/// <summary>
/// BCS: Branch if Carry Set
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BCS(uint16_t address, sfc_famicom_t* famicom) {
    if (SFC_CF) SFC_PC = address;
}

/// <summary>
/// BNE: Branch if Not Equal
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BNE(uint16_t address, sfc_famicom_t* famicom) {
    if (!SFC_ZF) SFC_PC = address;
}

/// <summary>
/// BEQ: Branch if Equal
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BEQ(uint16_t address, sfc_famicom_t* famicom) {
    if (SFC_ZF) SFC_PC = address;
}

/// <summary>
/// JMP
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_JMP(uint16_t address, sfc_famicom_t* famicom) {
    SFC_PC = address;
}

/// <summary>
/// PLP: Pull P
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_PLP(uint16_t address, sfc_famicom_t* famicom) {
    SFC_P = SFC_POP();
    SFC_RF_SE;
    SFC_BF_CL;
}

/// <summary>
/// PHP: Push P
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_PHP(uint16_t address, sfc_famicom_t* famicom) {
    SFC_PUSH(SFC_P | (uint8_t)(SFC_FLAG_R | SFC_FLAG_B));
}

/// <summary>
/// PLA: Pull A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_PLA(uint16_t address, sfc_famicom_t* famicom) {
    SFC_A = SFC_POP();
    CHECK_ZSFLAG(SFC_A);
}

/// <summary>
/// PHA: Push A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_PHA(uint16_t address, sfc_famicom_t* famicom) {
    SFC_PUSH(SFC_A);
}

/// <summary>
/// ROR A : Rotate Right for A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_RORA(uint16_t address, sfc_famicom_t* famicom) {
    uint16_t result16 = SFC_A;
    result16 |= ((uint16_t)SFC_CF) << (8 - SFC_INDEX_C);
    SFC_CF_IF(result16 & 1);
    result16 >>= 1;
    SFC_A = (uint8_t)result16;
    CHECK_ZSFLAG(SFC_A);
}

/// <summary>
/// ROR: Rotate Right
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ROR(uint16_t address, sfc_famicom_t* famicom) {
    uint16_t result16 = SFC_READ(address);
    result16 |= ((uint16_t)SFC_CF) << (8 - SFC_INDEX_C);
    SFC_CF_IF(result16 & 1);
    result16 >>= 1;
    const uint8_t result8 = (uint8_t)result16;
    SFC_WRITE(address, result8);
    CHECK_ZSFLAG(result8);
}

/// <summary>
/// ROL: Rotate Left
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ROL(uint16_t address, sfc_famicom_t* famicom) {
    uint16_t result16 = SFC_READ(address);
    result16 <<= 1;
    result16 |= ((uint16_t)SFC_CF) >> (SFC_INDEX_C);
    SFC_CF_IF(result16 & (uint16_t)0x100);
    const uint8_t result8 = (uint8_t)result16;
    SFC_WRITE(address, result8);
    CHECK_ZSFLAG(result8);
}

/// <summary>
/// ROL A : Rotate Left for A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ROLA(uint16_t address, sfc_famicom_t* famicom) {
    uint16_t result16 = SFC_A;
    result16 <<= 1;
    result16 |= ((uint16_t)SFC_CF) >> (SFC_INDEX_C);
    SFC_CF_IF(result16 & (uint16_t)0x100);
    SFC_A = (uint8_t)result16;
    CHECK_ZSFLAG(SFC_A);
}

/// <summary>
/// LSR: Logical Shift Right
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_LSR(uint16_t address, sfc_famicom_t* famicom) {
    uint8_t data = SFC_READ(address);
    SFC_CF_IF(data & 1);
    data >>= 1;
    SFC_WRITE(address, data);
    CHECK_ZSFLAG(data);
}

/// <summary>
/// LSR A : Logical Shift Right for A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_LSRA(uint16_t address, sfc_famicom_t* famicom) {
    SFC_CF_IF(SFC_A & 1);
    SFC_A >>= 1;
    CHECK_ZSFLAG(SFC_A);
}

/// <summary>
/// ASL: Arithmetic Shift Left
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ASL(uint16_t address, sfc_famicom_t* famicom) {
    uint8_t data = SFC_READ(address);
    SFC_CF_IF(data & (uint8_t)0x80);
    data <<= 1;
    SFC_WRITE(address, data);
    CHECK_ZSFLAG(data);
}

/// <summary>
/// ASL A : Arithmetic Shift Left for A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ASLA(uint16_t address, sfc_famicom_t* famicom) {
    SFC_CF_IF(SFC_A & (uint8_t)0x80);
    SFC_A <<= 1;
    CHECK_ZSFLAG(SFC_A);
}

/// <summary>
/// BIT: Bit Test
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BIT(uint16_t address, sfc_famicom_t* famicom) {
    const uint8_t value = SFC_READ(address);
    SFC_VF_IF(value & (uint8_t)(1 << 6));
    SFC_SF_IF(value & (uint8_t)(1 << 7));
    SFC_ZF_IF(!(SFC_A & value))
}

/// <summary>
/// CPY: Compare memory with Y
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_CPY(uint16_t address, sfc_famicom_t* famicom) {
    const uint16_t result16 = (uint16_t)SFC_Y - (uint16_t)SFC_READ(address);
    SFC_CF_IF(!(result16 & (uint16_t)0x8000));
    CHECK_ZSFLAG((uint8_t)result16);
}

/// <summary>
/// CPX
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_CPX(uint16_t address, sfc_famicom_t* famicom) {
    const uint16_t result16 = (uint16_t)SFC_X - (uint16_t)SFC_READ(address);
    SFC_CF_IF(!(result16 & (uint16_t)0x8000));
    CHECK_ZSFLAG((uint8_t)result16);
}

/// <summary>
/// CMP: Compare memory with A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_CMP(uint16_t address, sfc_famicom_t* famicom) {
    const uint16_t result16 = (uint16_t)SFC_A - (uint16_t)SFC_READ(address);
    SFC_CF_IF(!(result16 & (uint16_t)0x8000));
    CHECK_ZSFLAG((uint8_t)result16);
}

/// <summary>
/// SEI: Set I
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SEI(uint16_t address, sfc_famicom_t* famicom) {
    SFC_IF_SE;
}

/// <summary>
/// CLI - Clear I
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_CLI(uint16_t address, sfc_famicom_t* famicom) {
    //SFC_IF_CL;
    sfc_operation_UNK(address, famicom);
}

/// <summary>
/// CLV: Clear V
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_CLV(uint16_t address, sfc_famicom_t* famicom) {
    SFC_VF_CL;
}

/// <summary>
/// SED: Set D
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SED(uint16_t address, sfc_famicom_t* famicom) {
    SFC_DF_SE;
}

/// <summary>
/// CLD: Clear D
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_CLD(uint16_t address, sfc_famicom_t* famicom) {
    SFC_DF_CL;
}

/// <summary>
/// SEC: Set Carry
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SEC(uint16_t address, sfc_famicom_t* famicom) {
    SFC_CF_SE;
}

/// <summary>
/// CLC: Clear Carry
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_CLC(uint16_t address, sfc_famicom_t* famicom) {
    SFC_CF_CL;
}

/// <summary>
/// EOR: "Exclusive-Or" memory with A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_EOR(uint16_t address, sfc_famicom_t* famicom) {
    SFC_A ^= SFC_READ(address);
    CHECK_ZSFLAG(SFC_A);
}

/// <summary>
/// ORA: 'Or' memory with A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ORA(uint16_t address, sfc_famicom_t* famicom) {
    SFC_A |= SFC_READ(address);
    CHECK_ZSFLAG(SFC_A);
}

/// <summary>
/// AND: 'And' memory with A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_AND(uint16_t address, sfc_famicom_t* famicom) {
    SFC_A &= SFC_READ(address);
    CHECK_ZSFLAG(SFC_A);
}

/// <summary>
/// DEY: Decrement Y
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_DEY(uint16_t address, sfc_famicom_t* famicom) {
    SFC_Y--;
    CHECK_ZSFLAG(SFC_Y);
}

/// <summary>
/// INY:  Increment Y
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_INY(uint16_t address, sfc_famicom_t* famicom) {
    SFC_Y++;
    CHECK_ZSFLAG(SFC_Y);
}

/// <summary>
/// DEX: Decrement X
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_DEX(uint16_t address, sfc_famicom_t* famicom) {
    SFC_X--;
    CHECK_ZSFLAG(SFC_X);
}

/// <summary>
/// INX
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_INX(uint16_t address, sfc_famicom_t* famicom) {
    SFC_X++;
    CHECK_ZSFLAG(SFC_X);
}

/// <summary>
/// DEC: Decrement memory
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_DEC(uint16_t address, sfc_famicom_t* famicom) {
    uint8_t data = SFC_READ(address);
    --data;
    SFC_WRITE(address, data);
    CHECK_ZSFLAG(data);
}

/// <summary>
/// INC: Increment memory
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_INC(uint16_t address, sfc_famicom_t* famicom) {
    uint8_t data = SFC_READ(address);
    ++data;
    SFC_WRITE(address, data);
    CHECK_ZSFLAG(data);
}

/// <summary>
/// SBC: Subtract with Carry
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SBC(uint16_t address, sfc_famicom_t* famicom) {
    const uint8_t src = SFC_READ(address);
    const uint16_t result16 = (uint16_t)SFC_A - (uint16_t)src - (SFC_CF ? 0 : 1);
    SFC_CF_IF(!(result16 >> 8));
    const uint8_t result8 = (uint8_t)result16;
    SFC_VF_IF(((SFC_A ^ src) & 0x80) && ((SFC_A ^ result8) & 0x80));
    SFC_A = result8;
    CHECK_ZSFLAG(SFC_A);
}

/// <summary>
/// ADC: Add with Carry
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ADC(uint16_t address, sfc_famicom_t* famicom) {
    const uint8_t src = SFC_READ(address);
    const uint16_t result16 = (uint16_t)SFC_A + (uint16_t)src + (SFC_CF ? 1 : 0);
    SFC_CF_IF(result16 >> 8);
    const uint8_t result8 = (uint8_t)result16;
    SFC_VF_IF(!((SFC_A ^ src) & 0x80) && ((SFC_A ^ result8) & 0x80));
    SFC_A = result8;
    CHECK_ZSFLAG(SFC_A);
}

/// <summary>
/// TXS: Transfer X to SP
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_TXS(uint16_t address, sfc_famicom_t* famicom) {
    SFC_SP = SFC_X;
}

/// <summary>
/// TSX: Transfer SP to X
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_TSX(uint16_t address, sfc_famicom_t* famicom) {
    SFC_X = SFC_SP;
    CHECK_ZSFLAG(SFC_X);
}

/// <summary>
/// TYA: Transfer Y to A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_TYA(uint16_t address, sfc_famicom_t* famicom) {
    SFC_A = SFC_Y;
    CHECK_ZSFLAG(SFC_A);
}

/// <summary>
/// TAY: Transfer A to Y
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_TAY(uint16_t address, sfc_famicom_t* famicom) {
    SFC_Y = SFC_A;
    CHECK_ZSFLAG(SFC_Y);
}

/// <summary>
/// TXA: Transfer X to A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_TXA(uint16_t address, sfc_famicom_t* famicom) {
    SFC_A = SFC_X;
    CHECK_ZSFLAG(SFC_A);
}

/// <summary>
/// TAX: Transfer A to X
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_TAX(uint16_t address, sfc_famicom_t* famicom) {
    SFC_X = SFC_A;
    CHECK_ZSFLAG(SFC_X);
}

/// <summary>
/// STY: Store 'Y'
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_STY(uint16_t address, sfc_famicom_t* famicom) {
    SFC_WRITE(address, SFC_Y);
}

/// <summary>
/// STX: Store X
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_STX(uint16_t address, sfc_famicom_t* famicom) {
    SFC_WRITE(address, SFC_X);
}

/// <summary>
/// STA: Store 'A'
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_STA(uint16_t address, sfc_famicom_t* famicom) {
    SFC_WRITE(address, SFC_A);
}

/// <summary>
/// LDY: Load 'Y'
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_LDY(uint16_t address, sfc_famicom_t* famicom) {
    SFC_Y = SFC_READ(address);
    CHECK_ZSFLAG(SFC_Y);
}

/// <summary>
/// LDX: Load X
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_LDX(uint16_t address, sfc_famicom_t* famicom) {
    SFC_X = SFC_READ(address);
    CHECK_ZSFLAG(SFC_X);
}

/// <summary>
/// LDA: Load A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_LDA(uint16_t address, sfc_famicom_t* famicom) {
    SFC_A = SFC_READ(address);
    CHECK_ZSFLAG(SFC_A);
}


// ---------------------------------- 执行


/// <summary>
/// SFCs the cpu execute one.
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_cpu_execute_one(sfc_famicom_t* famicom) {
    famicom->interfaces.before_execute(famicom->argument, famicom);
    const uint8_t opcode = SFC_READ(SFC_PC++);
    switch (opcode)
    {
        OP(00,IMP, BRK) OP(01,INX, ORA) OP(02,UNK, UNK) OP(03,INX, SLO) OP(04,ZPG, NOP) OP(05,ZPG, ORA) OP(06,ZPG, ASL) OP(07,ZPG, SLO)
        OP(08,IMP, PHP) OP(09,IMM, ORA) OP(0A,IMP,ASLA) OP(0B,IMM, ANC) OP(0C,ABS, NOP) OP(0D,ABS, ORA) OP(0E,ABS, ASL) OP(0F,ABS, SLO)
        OP(10,REL, BPL) OP(11,INY, ORA) OP(12,UNK, UNK) OP(13,INY, SLO) OP(14,ZPX, NOP) OP(15,ZPX, ORA) OP(16,ZPX, ASL) OP(17,ZPX, SLO)
        OP(18,IMP, CLC) OP(19,ABY, ORA) OP(1A,IMP, NOP) OP(1B,ABY, SLO) OP(1C,ABX, NOP) OP(1D,ABX, ORA) OP(1E,ABX, ASL) OP(1F,ABX, SLO)
        OP(20,ABS, JSR) OP(21,INX, AND) OP(22,UNK, UNK) OP(23,INX, RLA) OP(24,ZPG, BIT) OP(25,ZPG, AND) OP(26,ZPG, ROL) OP(27,ZPG, RLA)
        OP(28,IMP, PLP) OP(29,IMM, AND) OP(2A,IMP,ROLA) OP(2B,IMM, ANC) OP(2C,ABS, BIT) OP(2D,ABS, AND) OP(2E,ABS, ROL) OP(2F,ABS, RLA)
        OP(30,REL, BMI) OP(31,INY, AND) OP(32,UNK, UNK) OP(33,INY, RLA) OP(34,ZPX, NOP) OP(35,ZPX, AND) OP(36,ZPX, ROL) OP(37,ZPX, RLA)
        OP(38,IMP, SEC) OP(39,ABY, AND) OP(3A,IMP, NOP) OP(3B,ABY, RLA) OP(3C,ABX, NOP) OP(3D,ABX, AND) OP(3E,ABX, ROL) OP(3F,ABX, RLA)
        OP(40,IMP, RTI) OP(41,INX, EOR) OP(42,UNK, UNK) OP(43,INX, SRE) OP(44,ZPG, NOP) OP(45,ZPG, EOR) OP(46,ZPG, LSR) OP(47,ZPG, SRE)
        OP(48,IMP, PHA) OP(49,IMM, EOR) OP(4A,IMP,LSRA) OP(4B,IMM, ASR) OP(4C,ABS, JMP) OP(4D,ABS, EOR) OP(4E,ABS, LSR) OP(4F,ABS, SRE)
        OP(50,REL, BVC) OP(51,INY, EOR) OP(52,UNK, UNK) OP(53,INY, SRE) OP(54,ZPX, NOP) OP(55,ZPX, EOR) OP(56,ZPX, LSR) OP(57,ZPX, SRE)
        OP(58,IMP, CLI) OP(59,ABY, EOR) OP(5A,IMP, NOP) OP(5B,ABY, SRE) OP(5C,ABX, NOP) OP(5D,ABX, EOR) OP(5E,ABX, LSR) OP(5F,ABX, SRE)
        OP(60,IMP, RTS) OP(61,INX, ADC) OP(62,UNK, UNK) OP(63,INX, RRA) OP(64,ZPG, NOP) OP(65,ZPG, ADC) OP(66,ZPG, ROR) OP(67,ZPG, RRA)
        OP(68,IMP, PLA) OP(69,IMM, ADC) OP(6A,IMP,RORA) OP(6B,IMM, ARR) OP(6C,IND, JMP) OP(6D,ABS, ADC) OP(6E,ABS, ROR) OP(6F,ABS, RRA)
        OP(70,REL, BVS) OP(71,INY, ADC) OP(72,UNK, UNK) OP(73,INY, RRA) OP(74,ZPX, NOP) OP(75,ZPX, ADC) OP(76,ZPX, ROR) OP(77,ZPX, RRA)
        OP(78,IMP, SEI) OP(79,ABY, ADC) OP(7A,IMP, NOP) OP(7B,ABY, RRA) OP(7C,ABX, NOP) OP(7D,ABX, ADC) OP(7E,ABX, ROR) OP(7F,ABX, RRA)
        OP(80,IMM, NOP) OP(81,INX, STA) OP(82,IMM, NOP) OP(83,INX, SAX) OP(84,ZPG, STY) OP(85,ZPG, STA) OP(86,ZPG, STX) OP(87,ZPG, SAX)
        OP(88,IMP, DEY) OP(89,IMM, NOP) OP(8A,IMP, TXA) OP(8B,IMM, XAA) OP(8C,ABS, STY) OP(8D,ABS, STA) OP(8E,ABS, STX) OP(8F,ABS, SAX)
        OP(90,REL, BCC) OP(91,INY, STA) OP(92,UNK, UNK) OP(93,INY, AHX) OP(94,ZPX, STY) OP(95,ZPX, STA) OP(96,ZPY, STX) OP(97,ZPY, SAX)
        OP(98,IMP, TYA) OP(99,ABY, STA) OP(9A,IMP, TXS) OP(9B,ABY, TAS) OP(9C,ABX, SHY) OP(9D,ABX, STA) OP(9E,ABY, SHX) OP(9F,ABY, AHX)
        OP(A0,IMM, LDY) OP(A1,INX, LDA) OP(A2,IMM, LDX) OP(A3,INX, LAX) OP(A4,ZPG, LDY) OP(A5,ZPG, LDA) OP(A6,ZPG, LDX) OP(A7,ZPG, LAX)
        OP(A8,IMP, TAY) OP(A9,IMM, LDA) OP(AA,IMP, TAX) OP(AB,IMM, LAX) OP(AC,ABS, LDY) OP(AD,ABS, LDA) OP(AE,ABS, LDX) OP(AF,ABS, LAX)
        OP(B0,REL, BCS) OP(B1,INY, LDA) OP(B2,UNK, UNK) OP(B3,INY, LAX) OP(B4,ZPX, LDY) OP(B5,ZPX, LDA) OP(B6,ZPY, LDX) OP(B7,ZPY, LAX)
        OP(B8,IMP, CLV) OP(B9,ABY, LDA) OP(BA,IMP, TSX) OP(BB,ABY, LAS) OP(BC,ABX, LDY) OP(BD,ABX, LDA) OP(BE,ABY, LDX) OP(BF,ABY, LAX)
        OP(C0,IMM, CPY) OP(C1,INX, CMP) OP(C2,IMM, NOP) OP(C3,INX, DCP) OP(C4,ZPG, CPY) OP(C5,ZPG, CMP) OP(C6,ZPG, DEC) OP(C7,ZPG, DCP)
        OP(C8,IMP, INY) OP(C9,IMM, CMP) OP(CA,IMP, DEX) OP(CB,IMM, AXS) OP(CC,ABS, CPY) OP(CD,ABS, CMP) OP(CE,ABS, DEC) OP(CF,ABS, DCP)
        OP(D0,REL, BNE) OP(D1,INY, CMP) OP(D2,UNK, UNK) OP(D3,INY, DCP) OP(D4,ZPX, NOP) OP(D5,ZPX, CMP) OP(D6,ZPX, DEC) OP(D7,ZPX, DCP)
        OP(D8,IMP, CLD) OP(D9,ABY, CMP) OP(DA,IMP, NOP) OP(DB,ABY, DCP) OP(DC,ABX, NOP) OP(DD,ABX, CMP) OP(DE,ABX, DEC) OP(DF,ABX, DCP)
        OP(E0,IMM, CPX) OP(E1,INX, SBC) OP(E2,IMM, NOP) OP(E3,INX, ISB) OP(E4,ZPG, CPX) OP(E5,ZPG, SBC) OP(E6,ZPG, INC) OP(E7,ZPG, ISB)
        OP(E8,IMP, INX) OP(E9,IMM, SBC) OP(EA,IMP, NOP) OP(EB,IMM, SBC) OP(EC,ABS, CPX) OP(ED,ABS, SBC) OP(EE,ABS, INC) OP(EF,ABS, ISB)
        OP(F0,REL, BEQ) OP(F1,INY, SBC) OP(F2,UNK, UNK) OP(F3,INY, ISB) OP(F4,ZPX, NOP) OP(F5,ZPX, SBC) OP(F6,ZPX, INC) OP(F7,ZPX, ISB)
        OP(F8,IMP, SED) OP(F9,ABY, SBC) OP(FA,IMP, NOP) OP(FB,ABY, ISB) OP(FC,ABX, NOP) OP(FD,ABX, SBC) OP(FE,ABX, INC) OP(FF,ABX, ISB)
    }
}

/// <summary>
/// 特殊指令: NMI
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline void sfc_operation_NMI(sfc_famicom_t* famicom) {
    const uint8_t pch = (uint8_t)((SFC_PC) >> 8);
    const uint8_t pcl = (uint8_t)SFC_PC;
    SFC_PUSH(pch);
    SFC_PUSH(pcl);
    SFC_PUSH(SFC_P | (uint8_t)(SFC_FLAG_R | SFC_FLAG_B));
    SFC_IF_SE;
    const uint8_t pcl2 = sfc_read_cpu_address(SFC_VERCTOR_RESET + 0, famicom);
    const uint8_t pch2 = sfc_read_cpu_address(SFC_VERCTOR_RESET + 1, famicom);
    famicom->registers.program_counter = (uint16_t)pcl2 | (uint16_t)pch2 << 8;
}

/// <summary>
/// 命令名称
/// </summary>
struct sfc_opname {
    // 3字名称
    char        name[3];
    // 寻址模式
    uint8_t     mode;
};

/// <summary>
/// 反汇编用数据
/// </summary>
static const struct sfc_opname s_opname_data[256] = {
    { 'B', 'R', 'K', SFC_AM_IMP },
    { 'O', 'R', 'A', SFC_AM_INX },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'S', 'L', 'O', SFC_AM_INX },
    { 'N', 'O', 'P', SFC_AM_ZPG },
    { 'O', 'R', 'A', SFC_AM_ZPG },
    { 'A', 'S', 'L', SFC_AM_ZPG },
    { 'S', 'L', 'O', SFC_AM_ZPG },
    { 'P', 'H', 'P', SFC_AM_IMP },
    { 'O', 'R', 'A', SFC_AM_IMM },
    { 'A', 'S', 'L', SFC_AM_ACC },
    { 'A', 'N', 'C', SFC_AM_IMM },
    { 'N', 'O', 'P', SFC_AM_ABS },
    { 'O', 'R', 'A', SFC_AM_ABS },
    { 'A', 'S', 'L', SFC_AM_ABS },
    { 'S', 'L', 'O', SFC_AM_ABS },

    { 'B', 'P', 'L', SFC_AM_REL },
    { 'O', 'R', 'A', SFC_AM_INY },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'S', 'L', 'O', SFC_AM_INY },
    { 'N', 'O', 'P', SFC_AM_ZPX },
    { 'O', 'R', 'A', SFC_AM_ZPX },
    { 'A', 'S', 'L', SFC_AM_ZPX },
    { 'S', 'L', 'O', SFC_AM_ZPX },
    { 'C', 'L', 'C', SFC_AM_IMP },
    { 'O', 'R', 'A', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_IMP },
    { 'S', 'L', 'O', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_ABX },
    { 'O', 'R', 'A', SFC_AM_ABX },
    { 'A', 'S', 'L', SFC_AM_ABX },
    { 'S', 'L', 'O', SFC_AM_ABX },

    { 'J', 'S', 'R', SFC_AM_ABS },
    { 'A', 'N', 'D', SFC_AM_INX },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'R', 'L', 'A', SFC_AM_INX },
    { 'B', 'I', 'T', SFC_AM_ZPG },
    { 'A', 'N', 'D', SFC_AM_ZPG },
    { 'R', 'O', 'L', SFC_AM_ZPG },
    { 'R', 'L', 'A', SFC_AM_ZPG },
    { 'P', 'L', 'P', SFC_AM_IMP },
    { 'A', 'N', 'D', SFC_AM_IMM },
    { 'R', 'O', 'L', SFC_AM_ACC },
    { 'A', 'N', 'C', SFC_AM_IMM },
    { 'B', 'I', 'T', SFC_AM_ABS },
    { 'A', 'N', 'D', SFC_AM_ABS },
    { 'R', 'O', 'L', SFC_AM_ABS },
    { 'R', 'L', 'A', SFC_AM_ABS },

    { 'B', 'M', 'I', SFC_AM_REL },
    { 'A', 'N', 'D', SFC_AM_INY },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'R', 'L', 'A', SFC_AM_INY },
    { 'N', 'O', 'P', SFC_AM_ZPX },
    { 'A', 'N', 'D', SFC_AM_ZPX },
    { 'R', 'O', 'L', SFC_AM_ZPX },
    { 'R', 'L', 'A', SFC_AM_ZPX },
    { 'S', 'E', 'C', SFC_AM_IMP },
    { 'A', 'N', 'D', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_IMP },
    { 'R', 'L', 'A', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_ABX },
    { 'A', 'N', 'D', SFC_AM_ABX },
    { 'R', 'O', 'L', SFC_AM_ABX },
    { 'R', 'L', 'A', SFC_AM_ABX },

    { 'R', 'T', 'I', SFC_AM_IMP },
    { 'E', 'O', 'R', SFC_AM_INX },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'S', 'R', 'E', SFC_AM_INX },
    { 'N', 'O', 'P', SFC_AM_ZPG },
    { 'E', 'O', 'R', SFC_AM_ZPG },
    { 'L', 'S', 'R', SFC_AM_ZPG },
    { 'S', 'R', 'E', SFC_AM_ZPG },
    { 'P', 'H', 'A', SFC_AM_IMP },
    { 'E', 'O', 'R', SFC_AM_IMM },
    { 'L', 'S', 'R', SFC_AM_ACC },
    { 'A', 'S', 'R', SFC_AM_IMM },
    { 'J', 'M', 'P', SFC_AM_ABS },
    { 'E', 'O', 'R', SFC_AM_ABS },
    { 'L', 'S', 'R', SFC_AM_ABS },
    { 'S', 'R', 'E', SFC_AM_ABS },

    { 'B', 'V', 'C', SFC_AM_REL },
    { 'E', 'O', 'R', SFC_AM_INY },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'S', 'R', 'E', SFC_AM_INY },
    { 'N', 'O', 'P', SFC_AM_ZPX },
    { 'E', 'O', 'R', SFC_AM_ZPX },
    { 'L', 'S', 'R', SFC_AM_ZPX },
    { 'S', 'R', 'E', SFC_AM_ZPX },
    { 'C', 'L', 'I', SFC_AM_IMP },
    { 'E', 'O', 'R', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_IMP },
    { 'S', 'R', 'E', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_ABX },
    { 'E', 'O', 'R', SFC_AM_ABX },
    { 'L', 'S', 'R', SFC_AM_ABX },
    { 'S', 'R', 'E', SFC_AM_ABX },

    { 'R', 'T', 'S', SFC_AM_IMP },
    { 'A', 'D', 'C', SFC_AM_INX },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'R', 'R', 'A', SFC_AM_INX },
    { 'N', 'O', 'P', SFC_AM_ZPG },
    { 'A', 'D', 'C', SFC_AM_ZPG },
    { 'R', 'O', 'R', SFC_AM_ZPG },
    { 'R', 'R', 'A', SFC_AM_ZPG },
    { 'P', 'L', 'A', SFC_AM_IMP },
    { 'A', 'D', 'C', SFC_AM_IMM },
    { 'R', 'O', 'R', SFC_AM_ACC },
    { 'A', 'R', 'R', SFC_AM_IMM },
    { 'J', 'M', 'P', SFC_AM_IND },
    { 'A', 'D', 'C', SFC_AM_ABS },
    { 'R', 'O', 'R', SFC_AM_ABS },
    { 'R', 'R', 'A', SFC_AM_ABS },

    { 'B', 'V', 'S', SFC_AM_REL },
    { 'A', 'D', 'C', SFC_AM_INY },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'R', 'R', 'A', SFC_AM_INY },
    { 'N', 'O', 'P', SFC_AM_ZPX },
    { 'A', 'D', 'C', SFC_AM_ZPX },
    { 'R', 'O', 'R', SFC_AM_ZPX },
    { 'R', 'R', 'A', SFC_AM_ZPX },
    { 'S', 'E', 'I', SFC_AM_IMP },
    { 'A', 'D', 'C', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_IMP },
    { 'R', 'R', 'A', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_ABX },
    { 'A', 'D', 'C', SFC_AM_ABX },
    { 'R', 'O', 'R', SFC_AM_ABX },
    { 'R', 'R', 'A', SFC_AM_ABX },

    { 'N', 'O', 'P', SFC_AM_IMM },
    { 'S', 'T', 'A', SFC_AM_INX },
    { 'N', 'O', 'P', SFC_AM_IMM },
    { 'S', 'A', 'X', SFC_AM_INX },
    { 'S', 'T', 'Y', SFC_AM_ZPG },
    { 'S', 'T', 'A', SFC_AM_ZPG },
    { 'S', 'T', 'X', SFC_AM_ZPG },
    { 'S', 'A', 'X', SFC_AM_ZPG },
    { 'D', 'E', 'Y', SFC_AM_IMP },
    { 'N', 'O', 'P', SFC_AM_IMM },
    { 'T', 'A', 'X', SFC_AM_IMP },
    { 'X', 'X', 'A', SFC_AM_IMM },
    { 'S', 'T', 'Y', SFC_AM_ABS },
    { 'S', 'T', 'A', SFC_AM_ABS },
    { 'S', 'T', 'X', SFC_AM_ABS },
    { 'S', 'A', 'X', SFC_AM_ABS },

    { 'B', 'C', 'C', SFC_AM_REL },
    { 'S', 'T', 'A', SFC_AM_INY },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'A', 'H', 'X', SFC_AM_INY },
    { 'S', 'T', 'Y', SFC_AM_ZPX },
    { 'S', 'T', 'A', SFC_AM_ZPX },
    { 'S', 'T', 'X', SFC_AM_ZPY },
    { 'S', 'A', 'X', SFC_AM_ZPY },
    { 'T', 'Y', 'A', SFC_AM_IMP },
    { 'S', 'T', 'A', SFC_AM_ABY },
    { 'T', 'X', 'S', SFC_AM_IMP },
    { 'T', 'A', 'S', SFC_AM_ABY },
    { 'S', 'H', 'Y', SFC_AM_ABX },
    { 'S', 'T', 'A', SFC_AM_ABX },
    { 'S', 'H', 'X', SFC_AM_ABY },
    { 'A', 'H', 'X', SFC_AM_ABY },

    { 'L', 'D', 'Y', SFC_AM_IMM },
    { 'L', 'D', 'A', SFC_AM_INX },
    { 'L', 'D', 'X', SFC_AM_IMM },
    { 'L', 'A', 'X', SFC_AM_INX },
    { 'L', 'D', 'Y', SFC_AM_ZPG },
    { 'L', 'D', 'A', SFC_AM_ZPG },
    { 'L', 'D', 'X', SFC_AM_ZPG },
    { 'L', 'A', 'X', SFC_AM_ZPG },
    { 'T', 'A', 'Y', SFC_AM_IMP },
    { 'L', 'D', 'A', SFC_AM_IMM },
    { 'T', 'A', 'X', SFC_AM_IMP },
    { 'L', 'A', 'X', SFC_AM_IMM },
    { 'L', 'D', 'Y', SFC_AM_ABS },
    { 'L', 'D', 'A', SFC_AM_ABS },
    { 'L', 'D', 'X', SFC_AM_ABS },
    { 'L', 'A', 'X', SFC_AM_ABS },

    { 'B', 'C', 'S', SFC_AM_REL },
    { 'L', 'D', 'A', SFC_AM_INY },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'L', 'A', 'X', SFC_AM_INY },
    { 'L', 'D', 'Y', SFC_AM_ZPX },
    { 'L', 'D', 'A', SFC_AM_ZPX },
    { 'L', 'D', 'X', SFC_AM_ZPY },
    { 'L', 'A', 'X', SFC_AM_ZPY },
    { 'C', 'L', 'V', SFC_AM_IMP },
    { 'L', 'D', 'A', SFC_AM_ABY },
    { 'T', 'S', 'X', SFC_AM_IMP },
    { 'L', 'A', 'S', SFC_AM_ABY },
    { 'L', 'D', 'Y', SFC_AM_ABX },
    { 'L', 'D', 'A', SFC_AM_ABX },
    { 'L', 'D', 'X', SFC_AM_ABY },
    { 'L', 'A', 'X', SFC_AM_ABY },

    { 'C', 'P', 'Y', SFC_AM_IMM },
    { 'C', 'M', 'P', SFC_AM_INX },
    { 'N', 'O', 'P', SFC_AM_IMM },
    { 'D', 'C', 'P', SFC_AM_INX },
    { 'C', 'P', 'Y', SFC_AM_ZPG },
    { 'C', 'M', 'P', SFC_AM_ZPG },
    { 'D', 'E', 'C', SFC_AM_ZPG },
    { 'D', 'C', 'P', SFC_AM_ZPG },
    { 'I', 'N', 'Y', SFC_AM_IMP },
    { 'C', 'M', 'P', SFC_AM_IMM },
    { 'D', 'E', 'X', SFC_AM_IMP },
    { 'A', 'X', 'S', SFC_AM_IMM },
    { 'C', 'P', 'Y', SFC_AM_ABS },
    { 'C', 'M', 'P', SFC_AM_ABS },
    { 'D', 'E', 'C', SFC_AM_ABS },
    { 'D', 'C', 'P', SFC_AM_ABS },

    { 'B', 'N', 'E', SFC_AM_REL },
    { 'C', 'M', 'P', SFC_AM_INY },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'D', 'C', 'P', SFC_AM_INY },
    { 'N', 'O', 'P', SFC_AM_ZPX },
    { 'C', 'M', 'P', SFC_AM_ZPX },
    { 'D', 'E', 'C', SFC_AM_ZPX },
    { 'D', 'C', 'P', SFC_AM_ZPX },
    { 'C', 'L', 'D', SFC_AM_IMP },
    { 'C', 'M', 'P', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_IMP },
    { 'D', 'C', 'P', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_ABX },
    { 'C', 'M', 'P', SFC_AM_ABX },
    { 'D', 'E', 'C', SFC_AM_ABX },
    { 'D', 'C', 'P', SFC_AM_ABX },

    { 'C', 'P', 'X', SFC_AM_IMM },
    { 'S', 'B', 'C', SFC_AM_INX },
    { 'N', 'O', 'P', SFC_AM_IMM },
    { 'I', 'S', 'B', SFC_AM_INX },
    { 'C', 'P', 'X', SFC_AM_ZPG },
    { 'S', 'B', 'C', SFC_AM_ZPG },
    { 'I', 'N', 'C', SFC_AM_ZPG },
    { 'I', 'S', 'B', SFC_AM_ZPG },
    { 'I', 'N', 'X', SFC_AM_IMP },
    { 'S', 'B', 'C', SFC_AM_IMM },
    { 'N', 'O', 'P', SFC_AM_IMP },
    { 'S', 'B', 'C', SFC_AM_IMM },
    { 'C', 'P', 'X', SFC_AM_ABS },
    { 'S', 'B', 'C', SFC_AM_ABS },
    { 'I', 'N', 'C', SFC_AM_ABS },
    { 'I', 'S', 'B', SFC_AM_ABS },

    { 'B', 'E', 'Q', SFC_AM_REL },
    { 'S', 'B', 'C', SFC_AM_INY },
    { 'S', 'T', 'P', SFC_AM_UNK },
    { 'I', 'S', 'B', SFC_AM_INY },
    { 'N', 'O', 'P', SFC_AM_ZPX },
    { 'S', 'B', 'C', SFC_AM_ZPX },
    { 'I', 'N', 'C', SFC_AM_ZPX },
    { 'I', 'S', 'B', SFC_AM_ZPX },
    { 'S', 'E', 'D', SFC_AM_IMP },
    { 'S', 'B', 'C', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_IMP },
    { 'I', 'S', 'B', SFC_AM_ABY },
    { 'N', 'O', 'P', SFC_AM_ABX },
    { 'S', 'B', 'C', SFC_AM_ABX },
    { 'I', 'N', 'C', SFC_AM_ABX },
    { 'I', 'S', 'B', SFC_AM_ABX },
};

/// <summary>
/// 十六进制字符数据
/// </summary>
static const char SFC_HEXDATA[] = "0123456789ABCDEF";

/// <summary>
/// 转换为16进制
/// </summary>
/// <param name="o">The o.</param>
/// <param name="b">The b.</param>
extern inline void sfc_btoh(char o[], uint8_t b) {
    // 高半字节
    o[0] = SFC_HEXDATA[b >> 4];
    // 低半字节
    o[1] = SFC_HEXDATA[b & (uint8_t)0x0F];
}

/// <summary>
/// 转换为有符号10进制
/// </summary>
/// <param name="o">The o.</param>
/// <param name="b">The b.</param>
static inline void sfc_btod(char o[], uint8_t b) {
    const int8_t sb = (int8_t)b;
    if (sb < 0) {
        o[0] = '-';
        b = -b;
    }
    else o[0] = '+';
    o[1] = SFC_HEXDATA[(uint8_t)b / 100];
    o[2] = SFC_HEXDATA[(uint8_t)b / 10 % 10];
    o[3] = SFC_HEXDATA[(uint8_t)b % 10];
}

#define sfc_fallthrough

/// <summary>
/// StepFC: 反汇编
/// </summary>
/// <param name="code">The code.</param>
/// <param name="buf">The buf.</param>
void sfc_6502_disassembly(sfc_6502_code_t code, char buf[SFC_DISASSEMBLY_BUF_LEN]) {
    enum {
        NAME_FIRSH = 0,
        ADDR_FIRSH = NAME_FIRSH + 4,
        LEN = ADDR_FIRSH + 9
    };
    memset(buf, ' ', LEN); buf[LEN] = ';'; buf[LEN + 1] = 0;
    static_assert(LEN + 1 < SFC_DISASSEMBLY_BUF_LEN, "");
    const struct sfc_opname opname = s_opname_data[code.op];
    // 设置操作码
    buf[NAME_FIRSH + 0] = opname.name[0];
    buf[NAME_FIRSH + 1] = opname.name[1];
    buf[NAME_FIRSH + 2] = opname.name[2];
    // 查看寻址模式
    switch (opname.mode)
    {
    case SFC_AM_UNK:
        sfc_fallthrough;
    case SFC_AM_IMP:
        // XXX     ;
        break;
    case SFC_AM_ACC:
        // XXX A   ;
        buf[ADDR_FIRSH + 0] = 'A';
        break;
    case SFC_AM_IMM:
        // XXX #$AB
        buf[ADDR_FIRSH + 0] = '#';
        buf[ADDR_FIRSH + 1] = '$';
        sfc_btoh(buf + ADDR_FIRSH + 2, code.a1);
        break;
    case SFC_AM_ABS:
        // XXX $ABCD
        sfc_fallthrough;
    case SFC_AM_ABX:
        // XXX $ABCD, X
        sfc_fallthrough;
    case SFC_AM_ABY:
        // XXX $ABCD, Y
        // REAL
        buf[ADDR_FIRSH] = '$';
        sfc_btoh(buf + ADDR_FIRSH + 1, code.a2);
        sfc_btoh(buf + ADDR_FIRSH + 3, code.a1);
        if (opname.mode == SFC_AM_ABS) break;
        buf[ADDR_FIRSH + 5] = ',';
        buf[ADDR_FIRSH + 7] = opname.mode == SFC_AM_ABX ? 'X' : 'Y';
        break;
    case SFC_AM_ZPG:
        // XXX $AB
        sfc_fallthrough;
    case SFC_AM_ZPX:
        // XXX $AB, X
        sfc_fallthrough;
    case SFC_AM_ZPY:
        // XXX $AB, Y
        // REAL
        buf[ADDR_FIRSH] = '$';
        sfc_btoh(buf + ADDR_FIRSH + 1, code.a1);
        if (opname.mode == SFC_AM_ZPG) break;
        buf[ADDR_FIRSH + 3] = ',';
        buf[ADDR_FIRSH + 5] = opname.mode == SFC_AM_ABX ? 'X' : 'Y';
        break;
    case SFC_AM_INX:
        // XXX ($AB, X)
        buf[ADDR_FIRSH + 0] = '(';
        buf[ADDR_FIRSH + 1] = '$';
        sfc_btoh(buf + ADDR_FIRSH + 2, code.a1);
        buf[ADDR_FIRSH + 4] = ',';
        buf[ADDR_FIRSH + 6] = 'X';
        buf[ADDR_FIRSH + 7] = ')';
        break;
    case SFC_AM_INY:
        // XXX ($AB), Y
        buf[ADDR_FIRSH + 0] = '(';
        buf[ADDR_FIRSH + 1] = '$';
        sfc_btoh(buf + ADDR_FIRSH + 2, code.a1);
        buf[ADDR_FIRSH + 4] = ')';
        buf[ADDR_FIRSH + 5] = ',';
        buf[ADDR_FIRSH + 7] = 'X';
        break;
    case SFC_AM_IND:
        // XXX ($ABCD)
        buf[ADDR_FIRSH + 0] = '(';
        buf[ADDR_FIRSH + 1] = '$';
        sfc_btoh(buf + ADDR_FIRSH + 2, code.a2);
        sfc_btoh(buf + ADDR_FIRSH + 4, code.a1);
        buf[ADDR_FIRSH + 6] = ')';
        break;
    case SFC_AM_REL:
        // XXX $AB(-085)
        // XXX $ABCD
        buf[ADDR_FIRSH + 0] = '$';
        //const uint16_t target = base + int8_t(data.a1);
        //sfc_btoh(buf + ADDR_FIRSH + 1, uint8_t(target >> 8));
        //sfc_btoh(buf + ADDR_FIRSH + 3, uint8_t(target & 0xFF));
        sfc_btoh(buf + ADDR_FIRSH + 1, code.a1);
        buf[ADDR_FIRSH + 3] = '(';
        sfc_btod(buf + ADDR_FIRSH + 4, code.a1);
        buf[ADDR_FIRSH + 8] = ')';
        break;
    }
}
