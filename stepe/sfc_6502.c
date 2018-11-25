#include "sfc_6502.h"
#include "sfc_famicom.h"
#include <assert.h>
#include <string.h>


extern inline uint8_t sfc_read_prgdata(uint16_t, const sfc_famicom_t*);

#ifdef _MSC_VER
#ifdef _DEBUG
#define SFC_FORCEINLINE 
#else
#define SFC_FORCEINLINE __forceinline
#endif
#else
#define SFC_FORCEINLINE __attribute__((always_inline))
#endif

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
//#define SFC_READ_PC(a) sfc_read_cpu_address(a, famicom)
#define SFC_READ_PC(a) sfc_read_prgdata(a, famicom)
#define SFC_PUSH(a) (famicom->main_memory + 0x100)[SFC_SP--] = a;
#define SFC_POP() (famicom->main_memory + 0x100)[++SFC_SP];
#define SFC_WRITE(a, v) sfc_write_cpu_address(a, v, famicom)
#define CHECK_ZSFLAG(x) { SFC_SF_IF(x & (uint8_t)0x80); SFC_ZF_IF(x == 0); }
// 指令实现
#define OP(n, a, o) \
case 0x##n:\
{           \
    cycle_add += (uint32_t)SFC_BAISC_CYCLE_##n;\
    const uint16_t address = sfc_addressing_##a(famicom, &cycle_add);\
    sfc_operation_##o(address, famicom, &cycle_add);\
    break;\
}


// IRQ
extern void sfc_operation_IRQ(sfc_famicom_t * famicom);

// ---------------------------------- 寻址

SFC_FORCEINLINE
/// <summary>
/// 寻址方式: 未知
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="cycle">The cycle.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_UNK(sfc_famicom_t* famicom, uint32_t* const cycle) {
    assert(!"UNKNOWN ADDRESSING MODE");
    return 0;
}

SFC_FORCEINLINE
/// <summary>
/// 寻址方式: 累加器
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="cycle">The cycle.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_ACC(sfc_famicom_t* famicom, uint32_t* const cycle) {
    return 0;
}

SFC_FORCEINLINE
/// <summary>
/// 寻址方式: 隐含寻址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="cycle">The cycle.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_IMP(sfc_famicom_t* famicom, uint32_t* const cycle) {
    return 0;
}

SFC_FORCEINLINE
/// <summary>
/// 寻址方式: 立即寻址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="cycle">The cycle.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_IMM(sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint16_t address = SFC_PC; 
    SFC_PC++;
    return address;
}

SFC_FORCEINLINE
/// <summary>
/// 寻址方式: 绝对寻址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="cycle">The cycle.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_ABS(sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint8_t address0 = SFC_READ_PC(SFC_PC++);
    const uint8_t address1 = SFC_READ_PC(SFC_PC++);
    return (uint16_t)address0 | (uint16_t)((uint16_t)address1 << 8);
}

SFC_FORCEINLINE
/// <summary>
/// 寻址方式: 绝对X变址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="cycle">The cycle.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_ABX(sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint16_t base = sfc_addressing_ABS(famicom, cycle);
    const uint16_t rvar = base + SFC_X;
    *cycle += ((base ^ rvar) >> 8) & 1;
    return rvar;
}

SFC_FORCEINLINE
/// <summary>
/// 寻址方式: 绝对X变址 - 没有额外一周期检测
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="cycle">The cycle.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_abx(sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint16_t base = sfc_addressing_ABS(famicom, cycle);
    const uint16_t rvar = base + SFC_X;
    return rvar;
}


SFC_FORCEINLINE
/// <summary>
/// 寻址方式: 绝对Y变址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="cycle">The cycle.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_ABY(sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint16_t base = sfc_addressing_ABS(famicom, cycle);
    const uint16_t rvar = base + SFC_Y;
    *cycle += ((base ^ rvar) >> 8) & 1;
    return rvar;
}

SFC_FORCEINLINE
/// <summary>
/// 寻址方式: 绝对Y变址 - 没有额外一周期检测
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="cycle">The cycle.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_aby(sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint16_t base = sfc_addressing_ABS(famicom, cycle);
    const uint16_t rvar = base + SFC_Y;
    return rvar;
}

SFC_FORCEINLINE
/// <summary>
/// 寻址方式: 零页寻址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="cycle">The cycle.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_ZPG(sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint16_t address = SFC_READ_PC(SFC_PC++);
    return address;
}

SFC_FORCEINLINE
/// <summary>
/// 寻址方式: 零页X变址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="cycle">The cycle.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_ZPX(sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint16_t base = sfc_addressing_ZPG(famicom, cycle);
    const uint16_t index = base + SFC_X;
    return index & (uint16_t)0x00FF;
}

SFC_FORCEINLINE
/// <summary>
/// 寻址方式: 零页Y变址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="cycle">The cycle.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_ZPY(sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint16_t base = sfc_addressing_ZPG(famicom, cycle);
    const uint16_t index = base + SFC_Y;
    return index & (uint16_t)0x00FF;
}

SFC_FORCEINLINE
/// <summary>
/// 寻址方式: 间接X变址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_INX(sfc_famicom_t* famicom, uint32_t* const cycle) {
    uint8_t base = SFC_READ_PC(SFC_PC++) + SFC_X;
    const uint8_t address0 = SFC_READ(base++);
    const uint8_t address1 = SFC_READ(base++);
    return (uint16_t)address0 | (uint16_t)((uint16_t)address1 << 8);
}

SFC_FORCEINLINE
/// <summary>
/// 寻址方式: 间接Y变址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="cycle">The cycle.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_INY(sfc_famicom_t* famicom, uint32_t* const cycle) {
    uint8_t base = SFC_READ_PC(SFC_PC++);
    const uint8_t address0 = SFC_READ(base++);
    const uint8_t address1 = SFC_READ(base++);
    const uint16_t address 
        = (uint16_t)address0 
        | (uint16_t)((uint16_t)address1 << 8)
        ;

    const uint16_t rvar = address + SFC_Y;
    *cycle += ((address ^ rvar) >> 8) & 1;
    return rvar;
}

SFC_FORCEINLINE
/// <summary>
/// 寻址方式: 间接Y变址 - 没有额外一周期检测
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="cycle">The cycle.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_iny(sfc_famicom_t* famicom, uint32_t* const cycle) {
    uint8_t base = SFC_READ_PC(SFC_PC++);
    const uint8_t address0 = SFC_READ(base++);
    const uint8_t address1 = SFC_READ(base++);
    const uint16_t address
        = (uint16_t)address0
        | (uint16_t)((uint16_t)address1 << 8)
        ;

    const uint16_t rvar = address + SFC_Y;
    return rvar;
}

SFC_FORCEINLINE
/// <summary>
/// 寻址方式: 间接寻址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="cycle">The cycle.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_IND(sfc_famicom_t* famicom, uint32_t* const cycle) {
    // 读取地址
    const uint16_t base1 = sfc_addressing_ABS(famicom, cycle);
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

SFC_FORCEINLINE
/// <summary>
/// 寻址方式: 相对寻址
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="cycle">The cycle.</param>
/// <returns></returns>
static inline uint16_t sfc_addressing_REL(sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint16_t oldpc = SFC_PC;
    const uint8_t data = SFC_READ_PC(SFC_PC++);
    const uint16_t address = SFC_PC + (int8_t)data;
    return address;
}


// ---------------------------------- 指令

SFC_FORCEINLINE
/// <summary>
/// UNK: Unknown
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_UNK(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    assert(!"UNKNOWN INS");
}

SFC_FORCEINLINE
/// <summary>
/// HK2: Hack $02 - 用于提示NSF初始化
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_HK2(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    famicom->nsf.play_clock = famicom->rom_info.clock_per_play;
}


SFC_FORCEINLINE
/// <summary>
/// SHY
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SHY(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    sfc_operation_UNK(address, famicom, cycle);
    //const uint8_t result = SFC_Y & (uint8_t)(((uint8_t)address >> 8) + 1);
    //SFC_WRITE(address, result);
}

SFC_FORCEINLINE
/// <summary>
/// SHX: Store (X & (ADDR_HI + 1))
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SHX(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    sfc_operation_UNK(address, famicom, cycle);
    //const uint8_t result = SFC_X & (uint8_t)(((uint8_t)address >> 8) + 1);
    //SFC_WRITE(address, result);
}

SFC_FORCEINLINE
/// <summary>
/// TAS
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_TAS(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    sfc_operation_UNK(address, famicom, cycle);
}

SFC_FORCEINLINE
/// <summary>
/// AHX
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_AHX(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    sfc_operation_UNK(address, famicom, cycle);
}

SFC_FORCEINLINE
/// <summary>
/// XAA
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_XAA(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    sfc_operation_UNK(address, famicom, cycle);
}

SFC_FORCEINLINE
/// <summary>
/// LAS
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_LAS(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    sfc_operation_UNK(address, famicom, cycle);
}

SFC_FORCEINLINE
/// <summary>
/// SRE: Shift Right then "Exclusive-Or" - LSR + EOR
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SRE(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    // LSR
    uint8_t data = SFC_READ(address);
    SFC_CF_IF(data & 1);
    data >>= 1;
    SFC_WRITE(address, data);
    // EOR
    SFC_A ^= data;
    CHECK_ZSFLAG(SFC_A);
}

SFC_FORCEINLINE
/// <summary>
/// SLO - Shift Left then 'Or' - ASL + ORA
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SLO(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    // ASL
    uint8_t data = SFC_READ(address);
    SFC_CF_IF(data & (uint8_t)0x80);
    data <<= 1;
    SFC_WRITE(address, data);
    // ORA
    SFC_A |= data;
    CHECK_ZSFLAG(SFC_A);
}

SFC_FORCEINLINE
/// <summary>
/// RRA: Rotate Right then Add with Carry - ROR + ADC
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_RRA(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
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

SFC_FORCEINLINE
/// <summary>
/// RLA: Rotate Left then 'And' - ROL + AND
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_RLA(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
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

SFC_FORCEINLINE
/// <summary>
/// ISB: Increment memory then Subtract with Carry - INC + SBC
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ISB(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
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

SFC_FORCEINLINE
/// <summary>
/// ISC
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ISC(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    sfc_operation_UNK(address, famicom, cycle);
}

SFC_FORCEINLINE
/// <summary>
/// DCP: Decrement memory then Compare with A - DEC + CMP
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_DCP(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    // DEC
    uint8_t data = SFC_READ(address);
    --data;
    SFC_WRITE(address, data);
    // CMP
    const uint16_t result16 = (uint16_t)SFC_A - (uint16_t)data;
    SFC_CF_IF(!(result16 & (uint16_t)0x8000));
    CHECK_ZSFLAG((uint8_t)result16);
}

SFC_FORCEINLINE
/// <summary>
/// SAX: Store A 'And' X - 
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SAX(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_WRITE(address, SFC_A & SFC_X);
}

SFC_FORCEINLINE
/// <summary>
/// LAX: Load 'A' then Transfer X - LDA  + TAX
/// </summary>
/// <remarks>
/// 非法指令
/// </remarks>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_LAX(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_A = SFC_READ(address);
    SFC_X = SFC_A;
    CHECK_ZSFLAG(SFC_X);
}

SFC_FORCEINLINE
/// <summary>
/// SBX
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SBX(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    sfc_operation_UNK(address, famicom, cycle);
}

SFC_FORCEINLINE
/// <summary>
/// AXS
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_AXS(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    // AXS 操作地址是立即数
    const uint16_t tmp = (SFC_A & SFC_X) - SFC_READ_PC(address);
    SFC_X = (uint8_t)tmp;
    CHECK_ZSFLAG(SFC_X);
    SFC_CF_IF((tmp & 0x8000)== 0);
}

SFC_FORCEINLINE
/// <summary>
/// ARR
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ARR(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    // ARR 指令 是立即数
    SFC_A &= SFC_READ_PC(address);
    SFC_CF;
    SFC_A = (SFC_A >> 1) | (SFC_CF << 7);
    CHECK_ZSFLAG(SFC_A);
    SFC_CF_IF((SFC_A >> 6) & 1);
    SFC_VF_IF(((SFC_A >> 5) ^ (SFC_A >> 6)) & 1);
}

SFC_FORCEINLINE
/// <summary>
/// AAC
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_AAC(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    sfc_operation_UNK(address, famicom, cycle);
}

SFC_FORCEINLINE
/// <summary>
/// ANC
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ANC(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    // ANC两个指令都是立即数
    SFC_A &= SFC_READ_PC(address);
    CHECK_ZSFLAG(SFC_A);
    SFC_CF_IF(SFC_SF);
}

SFC_FORCEINLINE
/// <summary>
/// ASR
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ASR(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    // ASR 指令是立即数
    SFC_A &= SFC_READ_PC(address);
    SFC_CF_IF(SFC_A & 1);
    SFC_A >>= 1;
    CHECK_ZSFLAG(SFC_A);
}

SFC_FORCEINLINE
/// <summary>
/// ALR
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ALR(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    sfc_operation_UNK(address, famicom, cycle);
}

SFC_FORCEINLINE
/// <summary>
/// RTI: Return from I
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_RTI(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
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
    // 清除计数
    famicom->registers.irq_counter 
        = famicom->registers.irq_in_process
        & famicom->registers.irq_flag
        & (~(SFC_P) >> SFC_INDEX_I) 
        //& famicom->registers.nmi_in_process
        ;
    famicom->registers.irq_in_process = 0;
    //famicom->registers.nmi_in_process = 1;
}

SFC_FORCEINLINE
/// <summary>
/// BRK
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BRK(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint16_t pcp1 = SFC_PC + 1;
    const uint8_t pch = (uint8_t)((pcp1) >> 8);
    const uint8_t pcl = (uint8_t)pcp1;
    SFC_PUSH(pch);
    SFC_PUSH(pcl);
    SFC_PUSH(SFC_P | (uint8_t)(SFC_FLAG_R) | (uint8_t)(SFC_FLAG_B));
    SFC_IF_SE;
    const uint8_t pcl2 = SFC_READ_PC(SFC_VERCTOR_BRK + 0);
    const uint8_t pch2 = SFC_READ_PC(SFC_VERCTOR_BRK + 1);
    famicom->registers.program_counter = (uint16_t)pcl2 | (uint16_t)pch2 << 8;
}

SFC_FORCEINLINE
/// <summary>
/// NOP: No Operation
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_NOP(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {

}


SFC_FORCEINLINE
/// <summary>
/// RTS: Return from Subroutine
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_RTS(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint8_t pcl = SFC_POP();
    const uint8_t pch = SFC_POP();
    SFC_PC
        = (uint16_t)pcl
        | (uint16_t)pch << 8
        ;
    SFC_PC++;
}

SFC_FORCEINLINE
/// <summary>
/// JSR: Jump to Subroutine
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_JSR(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint16_t pc1 = SFC_PC - 1;
    SFC_PUSH((uint8_t)(pc1 >> 8));
    SFC_PUSH((uint8_t)(pc1));
    SFC_PC = address;
}

SFC_FORCEINLINE
/// <summary>
/// StepFC: 执行分支跳转
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
/// <param name="cycle">The cycle.</param>
static inline void sfc_branch(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint16_t saved = SFC_PC;
    SFC_PC = address;
    ++(*cycle);
    *cycle += (address ^ saved) >> 8 & 1;
}

SFC_FORCEINLINE
/// <summary>
/// BVC: Branch if Overflow Clear
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BVC(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    if (!SFC_VF) sfc_branch(address, famicom, cycle);
}

SFC_FORCEINLINE
/// <summary>
/// BVC: Branch if Overflow Set
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BVS(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    if (SFC_VF) sfc_branch(address, famicom, cycle);
}

SFC_FORCEINLINE
/// <summary>
/// BPL: Branch if Plus
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BPL(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    if (!SFC_SF) sfc_branch(address, famicom, cycle);
}

SFC_FORCEINLINE
/// <summary>
/// BMI: Branch if Minus
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BMI(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    if (SFC_SF) sfc_branch(address, famicom, cycle);
}

SFC_FORCEINLINE
/// <summary>
/// BCC: Branch if Carry Clear
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BCC(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    if (!SFC_CF) sfc_branch(address, famicom, cycle);
}

SFC_FORCEINLINE
/// <summary>
/// BCS: Branch if Carry Set
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BCS(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    if (SFC_CF) sfc_branch(address, famicom, cycle);
}

SFC_FORCEINLINE
/// <summary>
/// BNE: Branch if Not Equal
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BNE(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    if (!SFC_ZF) sfc_branch(address, famicom, cycle);
}

SFC_FORCEINLINE
/// <summary>
/// BEQ: Branch if Equal
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BEQ(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    if (SFC_ZF) sfc_branch(address, famicom, cycle);
}

SFC_FORCEINLINE
/// <summary>
/// JMP
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_JMP(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_PC = address;
}

SFC_FORCEINLINE
/// <summary>
/// PLP: Pull P
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_PLP(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_P = SFC_POP();
    SFC_RF_SE;
    SFC_BF_CL;
    if (!SFC_IF)
        famicom->registers.irq_counter = famicom->registers.irq_flag << 1;
}

SFC_FORCEINLINE
/// <summary>
/// PHP: Push P
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_PHP(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_PUSH(SFC_P | (uint8_t)(SFC_FLAG_R | SFC_FLAG_B));
}

SFC_FORCEINLINE
/// <summary>
/// PLA: Pull A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_PLA(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_A = SFC_POP();
    CHECK_ZSFLAG(SFC_A);
}

SFC_FORCEINLINE
/// <summary>
/// PHA: Push A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_PHA(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_PUSH(SFC_A);
}

SFC_FORCEINLINE
/// <summary>
/// ROR A : Rotate Right for A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_RORA(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    uint16_t result16 = SFC_A;
    result16 |= ((uint16_t)SFC_CF) << (8 - SFC_INDEX_C);
    SFC_CF_IF(result16 & 1);
    result16 >>= 1;
    SFC_A = (uint8_t)result16;
    CHECK_ZSFLAG(SFC_A);
}

SFC_FORCEINLINE
/// <summary>
/// ROR: Rotate Right
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ROR(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    uint16_t result16 = SFC_READ(address);
    result16 |= ((uint16_t)SFC_CF) << (8 - SFC_INDEX_C);
    SFC_CF_IF(result16 & 1);
    result16 >>= 1;
    const uint8_t result8 = (uint8_t)result16;
    SFC_WRITE(address, result8);
    CHECK_ZSFLAG(result8);
}

SFC_FORCEINLINE
/// <summary>
/// ROL: Rotate Left
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ROL(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    uint16_t result16 = SFC_READ(address);
    result16 <<= 1;
    result16 |= ((uint16_t)SFC_CF) >> (SFC_INDEX_C);
    SFC_CF_IF(result16 & (uint16_t)0x100);
    const uint8_t result8 = (uint8_t)result16;
    SFC_WRITE(address, result8);
    CHECK_ZSFLAG(result8);
}

SFC_FORCEINLINE
/// <summary>
/// ROL A : Rotate Left for A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ROLA(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    uint16_t result16 = SFC_A;
    result16 <<= 1;
    result16 |= ((uint16_t)SFC_CF) >> (SFC_INDEX_C);
    SFC_CF_IF(result16 & (uint16_t)0x100);
    SFC_A = (uint8_t)result16;
    CHECK_ZSFLAG(SFC_A);
}

SFC_FORCEINLINE
/// <summary>
/// LSR: Logical Shift Right
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_LSR(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    uint8_t data = SFC_READ(address);
    SFC_CF_IF(data & 1);
    data >>= 1;
    SFC_WRITE(address, data);
    CHECK_ZSFLAG(data);
}

SFC_FORCEINLINE
/// <summary>
/// LSR A : Logical Shift Right for A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_LSRA(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_CF_IF(SFC_A & 1);
    SFC_A >>= 1;
    CHECK_ZSFLAG(SFC_A);
}

SFC_FORCEINLINE
/// <summary>
/// ASL: Arithmetic Shift Left
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ASL(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    uint8_t data = SFC_READ(address);
    SFC_CF_IF(data & (uint8_t)0x80);
    data <<= 1;
    SFC_WRITE(address, data);
    CHECK_ZSFLAG(data);
}

SFC_FORCEINLINE
/// <summary>
/// ASL A : Arithmetic Shift Left for A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ASLA(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_CF_IF(SFC_A & (uint8_t)0x80);
    SFC_A <<= 1;
    CHECK_ZSFLAG(SFC_A);
}

SFC_FORCEINLINE
/// <summary>
/// BIT: Bit Test
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_BIT(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint8_t value = SFC_READ(address);
    SFC_VF_IF(value & (uint8_t)(1 << 6));
    SFC_SF_IF(value & (uint8_t)(1 << 7));
    SFC_ZF_IF(!(SFC_A & value))
}

SFC_FORCEINLINE
/// <summary>
/// CPY: Compare memory with Y
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_CPY(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint16_t result16 = (uint16_t)SFC_Y - (uint16_t)SFC_READ(address);
    SFC_CF_IF(!(result16 & (uint16_t)0x8000));
    CHECK_ZSFLAG((uint8_t)result16);
}

SFC_FORCEINLINE
/// <summary>
/// CPX
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_CPX(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint16_t result16 = (uint16_t)SFC_X - (uint16_t)SFC_READ(address);
    SFC_CF_IF(!(result16 & (uint16_t)0x8000));
    CHECK_ZSFLAG((uint8_t)result16);
}

SFC_FORCEINLINE
/// <summary>
/// CMP: Compare memory with A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_CMP(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint16_t result16 = (uint16_t)SFC_A - (uint16_t)SFC_READ(address);
    SFC_CF_IF(!(result16 & (uint16_t)0x8000));
    CHECK_ZSFLAG((uint8_t)result16);
}

SFC_FORCEINLINE
/// <summary>
/// SEI: Set I
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SEI(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_IF_SE;
}

SFC_FORCEINLINE
/// <summary>
/// CLI - Clear I
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_CLI(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_IF_CL;
    famicom->registers.irq_counter = famicom->registers.irq_flag << 1;
}

SFC_FORCEINLINE
/// <summary>
/// CLV: Clear V
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_CLV(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_VF_CL;
}

SFC_FORCEINLINE
/// <summary>
/// SED: Set D
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SED(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_DF_SE;
}

SFC_FORCEINLINE
/// <summary>
/// CLD: Clear D
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_CLD(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_DF_CL;
}

SFC_FORCEINLINE
/// <summary>
/// SEC: Set Carry
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SEC(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_CF_SE;
}

SFC_FORCEINLINE
/// <summary>
/// CLC: Clear Carry
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_CLC(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_CF_CL;
}

SFC_FORCEINLINE
/// <summary>
/// EOR: "Exclusive-Or" memory with A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_EOR(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_A ^= SFC_READ(address);
    CHECK_ZSFLAG(SFC_A);
}

SFC_FORCEINLINE
/// <summary>
/// ORA: 'Or' memory with A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ORA(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_A |= SFC_READ(address);
    CHECK_ZSFLAG(SFC_A);
}

SFC_FORCEINLINE
/// <summary>
/// AND: 'And' memory with A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_AND(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_A &= SFC_READ(address);
    CHECK_ZSFLAG(SFC_A);
}

SFC_FORCEINLINE
/// <summary>
/// DEY: Decrement Y
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_DEY(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_Y--;
    CHECK_ZSFLAG(SFC_Y);
}

SFC_FORCEINLINE
/// <summary>
/// INY:  Increment Y
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_INY(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_Y++;
    CHECK_ZSFLAG(SFC_Y);
}

SFC_FORCEINLINE
/// <summary>
/// DEX: Decrement X
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_DEX(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_X--;
    CHECK_ZSFLAG(SFC_X);
}

SFC_FORCEINLINE
/// <summary>
/// INX
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_INX(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_X++;
    CHECK_ZSFLAG(SFC_X);
}

SFC_FORCEINLINE
/// <summary>
/// DEC: Decrement memory
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_DEC(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    uint8_t data = SFC_READ(address);
    --data;
    SFC_WRITE(address, data);
    CHECK_ZSFLAG(data);
}

SFC_FORCEINLINE
/// <summary>
/// INC: Increment memory
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_INC(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    uint8_t data = SFC_READ(address);
    ++data;
    SFC_WRITE(address, data);
    CHECK_ZSFLAG(data);
}

SFC_FORCEINLINE
/// <summary>
/// SBC: Subtract with Carry
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_SBC(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint8_t src = SFC_READ(address);
    const uint16_t result16 = (uint16_t)SFC_A - (uint16_t)src - (SFC_CF ? 0 : 1);
    SFC_CF_IF(!(result16 >> 8));
    const uint8_t result8 = (uint8_t)result16;
    SFC_VF_IF(((SFC_A ^ src) & 0x80) && ((SFC_A ^ result8) & 0x80));
    SFC_A = result8;
    CHECK_ZSFLAG(SFC_A);
}

SFC_FORCEINLINE
/// <summary>
/// ADC: Add with Carry
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_ADC(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint8_t src = SFC_READ(address);
    const uint16_t result16 = (uint16_t)SFC_A + (uint16_t)src + (SFC_CF ? 1 : 0);
    SFC_CF_IF(result16 >> 8);
    const uint8_t result8 = (uint8_t)result16;
    SFC_VF_IF(!((SFC_A ^ src) & 0x80) && ((SFC_A ^ result8) & 0x80));
    SFC_A = result8;
    CHECK_ZSFLAG(SFC_A);
}

SFC_FORCEINLINE
/// <summary>
/// TXS: Transfer X to SP
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_TXS(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_SP = SFC_X;
}

SFC_FORCEINLINE
/// <summary>
/// TSX: Transfer SP to X
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_TSX(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_X = SFC_SP;
    CHECK_ZSFLAG(SFC_X);
}

SFC_FORCEINLINE
/// <summary>
/// TYA: Transfer Y to A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_TYA(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_A = SFC_Y;
    CHECK_ZSFLAG(SFC_A);
}

SFC_FORCEINLINE
/// <summary>
/// TAY: Transfer A to Y
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_TAY(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_Y = SFC_A;
    CHECK_ZSFLAG(SFC_Y);
}

SFC_FORCEINLINE
/// <summary>
/// TXA: Transfer X to A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_TXA(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_A = SFC_X;
    CHECK_ZSFLAG(SFC_A);
}

SFC_FORCEINLINE
/// <summary>
/// TAX: Transfer A to X
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_TAX(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_X = SFC_A;
    CHECK_ZSFLAG(SFC_X);
}

SFC_FORCEINLINE
/// <summary>
/// STY: Store 'Y'
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_STY(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_WRITE(address, SFC_Y);
}

SFC_FORCEINLINE
/// <summary>
/// STX: Store X
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_STX(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_WRITE(address, SFC_X);
}

SFC_FORCEINLINE
/// <summary>
/// STA: Store 'A'
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_STA(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_WRITE(address, SFC_A);
}

SFC_FORCEINLINE
/// <summary>
/// LDY: Load 'Y'
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_LDY(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_Y = SFC_READ(address);
    CHECK_ZSFLAG(SFC_Y);
}

SFC_FORCEINLINE
/// <summary>
/// LDX: Load X
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_LDX(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_X = SFC_READ(address);
    CHECK_ZSFLAG(SFC_X);
}

SFC_FORCEINLINE
/// <summary>
/// LDA: Load A
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
static inline void sfc_operation_LDA(uint16_t address, sfc_famicom_t* famicom, uint32_t* const cycle) {
    SFC_A = SFC_READ(address);
    CHECK_ZSFLAG(SFC_A);
}


// ---------------------------------- 指令周期数据

enum sfc_basic_cycle_data {
    SFC_BAISC_CYCLE_00 = 7, SFC_BAISC_CYCLE_01 = 6, SFC_BAISC_CYCLE_02 = 2, SFC_BAISC_CYCLE_03 = 8,
    SFC_BAISC_CYCLE_04 = 3, SFC_BAISC_CYCLE_05 = 3, SFC_BAISC_CYCLE_06 = 5, SFC_BAISC_CYCLE_07 = 5,
    SFC_BAISC_CYCLE_08 = 3, SFC_BAISC_CYCLE_09 = 2, SFC_BAISC_CYCLE_0A = 2, SFC_BAISC_CYCLE_0B = 2,
    SFC_BAISC_CYCLE_0C = 4, SFC_BAISC_CYCLE_0D = 4, SFC_BAISC_CYCLE_0E = 6, SFC_BAISC_CYCLE_0F = 6,
    SFC_BAISC_CYCLE_10 = 2, SFC_BAISC_CYCLE_11 = 5, SFC_BAISC_CYCLE_12 = 2, SFC_BAISC_CYCLE_13 = 8,
    SFC_BAISC_CYCLE_14 = 4, SFC_BAISC_CYCLE_15 = 4, SFC_BAISC_CYCLE_16 = 6, SFC_BAISC_CYCLE_17 = 6,
    SFC_BAISC_CYCLE_18 = 2, SFC_BAISC_CYCLE_19 = 4, SFC_BAISC_CYCLE_1A = 2, SFC_BAISC_CYCLE_1B = 7,
    SFC_BAISC_CYCLE_1C = 4, SFC_BAISC_CYCLE_1D = 4, SFC_BAISC_CYCLE_1E = 7, SFC_BAISC_CYCLE_1F = 7,
    SFC_BAISC_CYCLE_20 = 6, SFC_BAISC_CYCLE_21 = 6, SFC_BAISC_CYCLE_22 = 2, SFC_BAISC_CYCLE_23 = 8,
    SFC_BAISC_CYCLE_24 = 3, SFC_BAISC_CYCLE_25 = 3, SFC_BAISC_CYCLE_26 = 5, SFC_BAISC_CYCLE_27 = 5,
    SFC_BAISC_CYCLE_28 = 4, SFC_BAISC_CYCLE_29 = 2, SFC_BAISC_CYCLE_2A = 2, SFC_BAISC_CYCLE_2B = 2,
    SFC_BAISC_CYCLE_2C = 4, SFC_BAISC_CYCLE_2D = 4, SFC_BAISC_CYCLE_2E = 6, SFC_BAISC_CYCLE_2F = 6,
    SFC_BAISC_CYCLE_30 = 2, SFC_BAISC_CYCLE_31 = 5, SFC_BAISC_CYCLE_32 = 2, SFC_BAISC_CYCLE_33 = 8,
    SFC_BAISC_CYCLE_34 = 4, SFC_BAISC_CYCLE_35 = 4, SFC_BAISC_CYCLE_36 = 6, SFC_BAISC_CYCLE_37 = 6,
    SFC_BAISC_CYCLE_38 = 2, SFC_BAISC_CYCLE_39 = 4, SFC_BAISC_CYCLE_3A = 2, SFC_BAISC_CYCLE_3B = 7,
    SFC_BAISC_CYCLE_3C = 4, SFC_BAISC_CYCLE_3D = 4, SFC_BAISC_CYCLE_3E = 7, SFC_BAISC_CYCLE_3F = 7,
    SFC_BAISC_CYCLE_40 = 6, SFC_BAISC_CYCLE_41 = 6, SFC_BAISC_CYCLE_42 = 2, SFC_BAISC_CYCLE_43 = 8,
    SFC_BAISC_CYCLE_44 = 3, SFC_BAISC_CYCLE_45 = 3, SFC_BAISC_CYCLE_46 = 5, SFC_BAISC_CYCLE_47 = 5,
    SFC_BAISC_CYCLE_48 = 3, SFC_BAISC_CYCLE_49 = 2, SFC_BAISC_CYCLE_4A = 2, SFC_BAISC_CYCLE_4B = 2,
    SFC_BAISC_CYCLE_4C = 3, SFC_BAISC_CYCLE_4D = 4, SFC_BAISC_CYCLE_4E = 6, SFC_BAISC_CYCLE_4F = 6,
    SFC_BAISC_CYCLE_50 = 2, SFC_BAISC_CYCLE_51 = 5, SFC_BAISC_CYCLE_52 = 2, SFC_BAISC_CYCLE_53 = 8,
    SFC_BAISC_CYCLE_54 = 4, SFC_BAISC_CYCLE_55 = 4, SFC_BAISC_CYCLE_56 = 6, SFC_BAISC_CYCLE_57 = 6,
    SFC_BAISC_CYCLE_58 = 2, SFC_BAISC_CYCLE_59 = 4, SFC_BAISC_CYCLE_5A = 2, SFC_BAISC_CYCLE_5B = 7,
    SFC_BAISC_CYCLE_5C = 4, SFC_BAISC_CYCLE_5D = 4, SFC_BAISC_CYCLE_5E = 7, SFC_BAISC_CYCLE_5F = 7,
    SFC_BAISC_CYCLE_60 = 6, SFC_BAISC_CYCLE_61 = 6, SFC_BAISC_CYCLE_62 = 2, SFC_BAISC_CYCLE_63 = 8,
    SFC_BAISC_CYCLE_64 = 3, SFC_BAISC_CYCLE_65 = 3, SFC_BAISC_CYCLE_66 = 5, SFC_BAISC_CYCLE_67 = 5,
    SFC_BAISC_CYCLE_68 = 4, SFC_BAISC_CYCLE_69 = 2, SFC_BAISC_CYCLE_6A = 2, SFC_BAISC_CYCLE_6B = 2,
    SFC_BAISC_CYCLE_6C = 5, SFC_BAISC_CYCLE_6D = 4, SFC_BAISC_CYCLE_6E = 6, SFC_BAISC_CYCLE_6F = 6,
    SFC_BAISC_CYCLE_70 = 2, SFC_BAISC_CYCLE_71 = 5, SFC_BAISC_CYCLE_72 = 2, SFC_BAISC_CYCLE_73 = 8,
    SFC_BAISC_CYCLE_74 = 4, SFC_BAISC_CYCLE_75 = 4, SFC_BAISC_CYCLE_76 = 6, SFC_BAISC_CYCLE_77 = 6,
    SFC_BAISC_CYCLE_78 = 2, SFC_BAISC_CYCLE_79 = 4, SFC_BAISC_CYCLE_7A = 2, SFC_BAISC_CYCLE_7B = 7,
    SFC_BAISC_CYCLE_7C = 4, SFC_BAISC_CYCLE_7D = 4, SFC_BAISC_CYCLE_7E = 7, SFC_BAISC_CYCLE_7F = 7,
    SFC_BAISC_CYCLE_80 = 2, SFC_BAISC_CYCLE_81 = 6, SFC_BAISC_CYCLE_82 = 2, SFC_BAISC_CYCLE_83 = 6,
    SFC_BAISC_CYCLE_84 = 3, SFC_BAISC_CYCLE_85 = 3, SFC_BAISC_CYCLE_86 = 3, SFC_BAISC_CYCLE_87 = 3,
    SFC_BAISC_CYCLE_88 = 2, SFC_BAISC_CYCLE_89 = 2, SFC_BAISC_CYCLE_8A = 2, SFC_BAISC_CYCLE_8B = 2,
    SFC_BAISC_CYCLE_8C = 4, SFC_BAISC_CYCLE_8D = 4, SFC_BAISC_CYCLE_8E = 4, SFC_BAISC_CYCLE_8F = 4,
    SFC_BAISC_CYCLE_90 = 2, SFC_BAISC_CYCLE_91 = 6, SFC_BAISC_CYCLE_92 = 2, SFC_BAISC_CYCLE_93 = 6,
    SFC_BAISC_CYCLE_94 = 4, SFC_BAISC_CYCLE_95 = 4, SFC_BAISC_CYCLE_96 = 4, SFC_BAISC_CYCLE_97 = 4,
    SFC_BAISC_CYCLE_98 = 2, SFC_BAISC_CYCLE_99 = 5, SFC_BAISC_CYCLE_9A = 2, SFC_BAISC_CYCLE_9B = 5,
    SFC_BAISC_CYCLE_9C = 5, SFC_BAISC_CYCLE_9D = 5, SFC_BAISC_CYCLE_9E = 5, SFC_BAISC_CYCLE_9F = 5,
    SFC_BAISC_CYCLE_A0 = 2, SFC_BAISC_CYCLE_A1 = 6, SFC_BAISC_CYCLE_A2 = 2, SFC_BAISC_CYCLE_A3 = 6,
    SFC_BAISC_CYCLE_A4 = 3, SFC_BAISC_CYCLE_A5 = 3, SFC_BAISC_CYCLE_A6 = 3, SFC_BAISC_CYCLE_A7 = 3,
    SFC_BAISC_CYCLE_A8 = 2, SFC_BAISC_CYCLE_A9 = 2, SFC_BAISC_CYCLE_AA = 2, SFC_BAISC_CYCLE_AB = 2,
    SFC_BAISC_CYCLE_AC = 4, SFC_BAISC_CYCLE_AD = 4, SFC_BAISC_CYCLE_AE = 4, SFC_BAISC_CYCLE_AF = 4,
    SFC_BAISC_CYCLE_B0 = 2, SFC_BAISC_CYCLE_B1 = 5, SFC_BAISC_CYCLE_B2 = 2, SFC_BAISC_CYCLE_B3 = 5,
    SFC_BAISC_CYCLE_B4 = 4, SFC_BAISC_CYCLE_B5 = 4, SFC_BAISC_CYCLE_B6 = 4, SFC_BAISC_CYCLE_B7 = 4,
    SFC_BAISC_CYCLE_B8 = 2, SFC_BAISC_CYCLE_B9 = 4, SFC_BAISC_CYCLE_BA = 2, SFC_BAISC_CYCLE_BB = 4,
    SFC_BAISC_CYCLE_BC = 4, SFC_BAISC_CYCLE_BD = 4, SFC_BAISC_CYCLE_BE = 4, SFC_BAISC_CYCLE_BF = 4,
    SFC_BAISC_CYCLE_C0 = 2, SFC_BAISC_CYCLE_C1 = 6, SFC_BAISC_CYCLE_C2 = 2, SFC_BAISC_CYCLE_C3 = 8,
    SFC_BAISC_CYCLE_C4 = 3, SFC_BAISC_CYCLE_C5 = 3, SFC_BAISC_CYCLE_C6 = 5, SFC_BAISC_CYCLE_C7 = 5,
    SFC_BAISC_CYCLE_C8 = 2, SFC_BAISC_CYCLE_C9 = 2, SFC_BAISC_CYCLE_CA = 2, SFC_BAISC_CYCLE_CB = 2,
    SFC_BAISC_CYCLE_CC = 4, SFC_BAISC_CYCLE_CD = 4, SFC_BAISC_CYCLE_CE = 6, SFC_BAISC_CYCLE_CF = 6,
    SFC_BAISC_CYCLE_D0 = 2, SFC_BAISC_CYCLE_D1 = 5, SFC_BAISC_CYCLE_D2 = 2, SFC_BAISC_CYCLE_D3 = 8,
    SFC_BAISC_CYCLE_D4 = 4, SFC_BAISC_CYCLE_D5 = 4, SFC_BAISC_CYCLE_D6 = 6, SFC_BAISC_CYCLE_D7 = 6,
    SFC_BAISC_CYCLE_D8 = 2, SFC_BAISC_CYCLE_D9 = 4, SFC_BAISC_CYCLE_DA = 2, SFC_BAISC_CYCLE_DB = 7,
    SFC_BAISC_CYCLE_DC = 4, SFC_BAISC_CYCLE_DD = 4, SFC_BAISC_CYCLE_DE = 7, SFC_BAISC_CYCLE_DF = 7,
    SFC_BAISC_CYCLE_E0 = 2, SFC_BAISC_CYCLE_E1 = 6, SFC_BAISC_CYCLE_E2 = 2, SFC_BAISC_CYCLE_E3 = 8,
    SFC_BAISC_CYCLE_E4 = 3, SFC_BAISC_CYCLE_E5 = 3, SFC_BAISC_CYCLE_E6 = 5, SFC_BAISC_CYCLE_E7 = 5,
    SFC_BAISC_CYCLE_E8 = 2, SFC_BAISC_CYCLE_E9 = 2, SFC_BAISC_CYCLE_EA = 2, SFC_BAISC_CYCLE_EB = 2,
    SFC_BAISC_CYCLE_EC = 4, SFC_BAISC_CYCLE_ED = 4, SFC_BAISC_CYCLE_EE = 6, SFC_BAISC_CYCLE_EF = 6,
    SFC_BAISC_CYCLE_F0 = 2, SFC_BAISC_CYCLE_F1 = 5, SFC_BAISC_CYCLE_F2 = 2, SFC_BAISC_CYCLE_F3 = 8,
    SFC_BAISC_CYCLE_F4 = 4, SFC_BAISC_CYCLE_F5 = 4, SFC_BAISC_CYCLE_F6 = 6, SFC_BAISC_CYCLE_F7 = 6,
    SFC_BAISC_CYCLE_F8 = 2, SFC_BAISC_CYCLE_F9 = 4, SFC_BAISC_CYCLE_FA = 2, SFC_BAISC_CYCLE_FB = 7,
    SFC_BAISC_CYCLE_FC = 4, SFC_BAISC_CYCLE_FD = 4, SFC_BAISC_CYCLE_FE = 7, SFC_BAISC_CYCLE_FF = 7,
};


// ---------------------------------- 执行


/// <summary>
/// SFCs the cpu execute one.
/// </summary>
/// <param name="famicom">The famicom.</param>
uint32_t sfc_cpu_execute_one(sfc_famicom_t* famicom) {
#ifdef SFC_BEFORE_EXECUTE
    // 执行指令前调用
    sfc_before_execute(famicom->argument, famicom);
#endif
    // IRQ 处理
    if (famicom->registers.irq_counter) {
        --famicom->registers.irq_counter;
        if (famicom->registers.irq_counter == 0) {
            famicom->registers.irq_in_process = 1;
            sfc_operation_IRQ(famicom);
            return 7;
        }
    }
    // 正常处理
    const uint8_t opcode = SFC_READ_PC(SFC_PC++);
    uint32_t cycle_add = 0;
    switch (opcode)
    {
        OP(00,IMP, BRK) OP(01,INX, ORA) OP(02,IMP, HK2) OP(03,INX, SLO) OP(04,ZPG, NOP) OP(05,ZPG, ORA) OP(06,ZPG, ASL) OP(07,ZPG, SLO)
        OP(08,IMP, PHP) OP(09,IMM, ORA) OP(0A,IMP,ASLA) OP(0B,IMM, ANC) OP(0C,ABS, NOP) OP(0D,ABS, ORA) OP(0E,ABS, ASL) OP(0F,ABS, SLO)
        OP(10,REL, BPL) OP(11,INY, ORA) OP(12,UNK, UNK) OP(13,iny, SLO) OP(14,ZPX, NOP) OP(15,ZPX, ORA) OP(16,ZPX, ASL) OP(17,ZPX, SLO)
        OP(18,IMP, CLC) OP(19,ABY, ORA) OP(1A,IMP, NOP) OP(1B,aby, SLO) OP(1C,ABX, NOP) OP(1D,ABX, ORA) OP(1E,abx, ASL) OP(1F,abx, SLO)
        OP(20,ABS, JSR) OP(21,INX, AND) OP(22,UNK, UNK) OP(23,INX, RLA) OP(24,ZPG, BIT) OP(25,ZPG, AND) OP(26,ZPG, ROL) OP(27,ZPG, RLA)
        OP(28,IMP, PLP) OP(29,IMM, AND) OP(2A,IMP,ROLA) OP(2B,IMM, ANC) OP(2C,ABS, BIT) OP(2D,ABS, AND) OP(2E,ABS, ROL) OP(2F,ABS, RLA)
        OP(30,REL, BMI) OP(31,INY, AND) OP(32,UNK, UNK) OP(33,iny, RLA) OP(34,ZPX, NOP) OP(35,ZPX, AND) OP(36,ZPX, ROL) OP(37,ZPX, RLA)
        OP(38,IMP, SEC) OP(39,ABY, AND) OP(3A,IMP, NOP) OP(3B,aby, RLA) OP(3C,ABX, NOP) OP(3D,ABX, AND) OP(3E,abx, ROL) OP(3F,abx, RLA)
        OP(40,IMP, RTI) OP(41,INX, EOR) OP(42,UNK, UNK) OP(43,INX, SRE) OP(44,ZPG, NOP) OP(45,ZPG, EOR) OP(46,ZPG, LSR) OP(47,ZPG, SRE)
        OP(48,IMP, PHA) OP(49,IMM, EOR) OP(4A,IMP,LSRA) OP(4B,IMM, ASR) OP(4C,ABS, JMP) OP(4D,ABS, EOR) OP(4E,ABS, LSR) OP(4F,ABS, SRE)
        OP(50,REL, BVC) OP(51,INY, EOR) OP(52,UNK, UNK) OP(53,iny, SRE) OP(54,ZPX, NOP) OP(55,ZPX, EOR) OP(56,ZPX, LSR) OP(57,ZPX, SRE)
        OP(58,IMP, CLI) OP(59,ABY, EOR) OP(5A,IMP, NOP) OP(5B,aby, SRE) OP(5C,ABX, NOP) OP(5D,ABX, EOR) OP(5E,abx, LSR) OP(5F,abx, SRE)
        OP(60,IMP, RTS) OP(61,INX, ADC) OP(62,UNK, UNK) OP(63,INX, RRA) OP(64,ZPG, NOP) OP(65,ZPG, ADC) OP(66,ZPG, ROR) OP(67,ZPG, RRA)
        OP(68,IMP, PLA) OP(69,IMM, ADC) OP(6A,IMP,RORA) OP(6B,IMM, ARR) OP(6C,IND, JMP) OP(6D,ABS, ADC) OP(6E,ABS, ROR) OP(6F,ABS, RRA)
        OP(70,REL, BVS) OP(71,INY, ADC) OP(72,UNK, UNK) OP(73,iny, RRA) OP(74,ZPX, NOP) OP(75,ZPX, ADC) OP(76,ZPX, ROR) OP(77,ZPX, RRA)
        OP(78,IMP, SEI) OP(79,ABY, ADC) OP(7A,IMP, NOP) OP(7B,aby, RRA) OP(7C,ABX, NOP) OP(7D,ABX, ADC) OP(7E,abx, ROR) OP(7F,abx, RRA)
        OP(80,IMM, NOP) OP(81,INX, STA) OP(82,IMM, NOP) OP(83,INX, SAX) OP(84,ZPG, STY) OP(85,ZPG, STA) OP(86,ZPG, STX) OP(87,ZPG, SAX)
        OP(88,IMP, DEY) OP(89,IMM, NOP) OP(8A,IMP, TXA) OP(8B,IMM, XAA) OP(8C,ABS, STY) OP(8D,ABS, STA) OP(8E,ABS, STX) OP(8F,ABS, SAX)
        OP(90,REL, BCC) OP(91,iny, STA) OP(92,UNK, UNK) OP(93,INY, AHX) OP(94,ZPX, STY) OP(95,ZPX, STA) OP(96,ZPY, STX) OP(97,ZPY, SAX)
        OP(98,IMP, TYA) OP(99,aby, STA) OP(9A,IMP, TXS) OP(9B,ABY, TAS) OP(9C,ABX, SHY) OP(9D,abx, STA) OP(9E,ABY, SHX) OP(9F,ABY, AHX)
        OP(A0,IMM, LDY) OP(A1,INX, LDA) OP(A2,IMM, LDX) OP(A3,INX, LAX) OP(A4,ZPG, LDY) OP(A5,ZPG, LDA) OP(A6,ZPG, LDX) OP(A7,ZPG, LAX)
        OP(A8,IMP, TAY) OP(A9,IMM, LDA) OP(AA,IMP, TAX) OP(AB,IMM, LAX) OP(AC,ABS, LDY) OP(AD,ABS, LDA) OP(AE,ABS, LDX) OP(AF,ABS, LAX)
        OP(B0,REL, BCS) OP(B1,INY, LDA) OP(B2,UNK, UNK) OP(B3,INY, LAX) OP(B4,ZPX, LDY) OP(B5,ZPX, LDA) OP(B6,ZPY, LDX) OP(B7,ZPY, LAX)
        OP(B8,IMP, CLV) OP(B9,ABY, LDA) OP(BA,IMP, TSX) OP(BB,ABY, LAS) OP(BC,ABX, LDY) OP(BD,ABX, LDA) OP(BE,ABY, LDX) OP(BF,ABY, LAX)
        OP(C0,IMM, CPY) OP(C1,INX, CMP) OP(C2,IMM, NOP) OP(C3,INX, DCP) OP(C4,ZPG, CPY) OP(C5,ZPG, CMP) OP(C6,ZPG, DEC) OP(C7,ZPG, DCP)
        OP(C8,IMP, INY) OP(C9,IMM, CMP) OP(CA,IMP, DEX) OP(CB,IMM, AXS) OP(CC,ABS, CPY) OP(CD,ABS, CMP) OP(CE,ABS, DEC) OP(CF,ABS, DCP)
        OP(D0,REL, BNE) OP(D1,INY, CMP) OP(D2,UNK, UNK) OP(D3,iny, DCP) OP(D4,ZPX, NOP) OP(D5,ZPX, CMP) OP(D6,ZPX, DEC) OP(D7,ZPX, DCP)
        OP(D8,IMP, CLD) OP(D9,ABY, CMP) OP(DA,IMP, NOP) OP(DB,aby, DCP) OP(DC,ABX, NOP) OP(DD,ABX, CMP) OP(DE,abx, DEC) OP(DF,abx, DCP)
        OP(E0,IMM, CPX) OP(E1,INX, SBC) OP(E2,IMM, NOP) OP(E3,INX, ISB) OP(E4,ZPG, CPX) OP(E5,ZPG, SBC) OP(E6,ZPG, INC) OP(E7,ZPG, ISB)
        OP(E8,IMP, INX) OP(E9,IMM, SBC) OP(EA,IMP, NOP) OP(EB,IMM, SBC) OP(EC,ABS, CPX) OP(ED,ABS, SBC) OP(EE,ABS, INC) OP(EF,ABS, ISB)
        OP(F0,REL, BEQ) OP(F1,INY, SBC) OP(F2,UNK, UNK) OP(F3,iny, ISB) OP(F4,ZPX, NOP) OP(F5,ZPX, SBC) OP(F6,ZPX, INC) OP(F7,ZPX, ISB)
        OP(F8,IMP, SED) OP(F9,ABY, SBC) OP(FA,IMP, NOP) OP(FB,aby, ISB) OP(FC,ABX, NOP) OP(FD,ABX, SBC) OP(FE,abx, INC) OP(FF,abx, ISB)
    }
    famicom->cpu_cycle_count += cycle_add;
    return cycle_add;
}

/// <summary>
/// 特殊指令: NMI
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline void sfc_operation_NMI(sfc_famicom_t* famicom, uint32_t* const cycle) {
    const uint8_t pch = (uint8_t)((SFC_PC) >> 8);
    const uint8_t pcl = (uint8_t)SFC_PC;
    SFC_PUSH(pch);
    SFC_PUSH(pcl);
    SFC_PUSH(SFC_P | (uint8_t)(SFC_FLAG_R));
    SFC_IF_SE;
    const uint8_t pcl2 = SFC_READ_PC(SFC_VERCTOR_NMI + 0);
    const uint8_t pch2 = SFC_READ_PC(SFC_VERCTOR_NMI + 1);
    famicom->registers.program_counter = (uint16_t)pcl2 | (uint16_t)pch2 << 8;

    famicom->cpu_cycle_count += 7;
    //famicom->registers.nmi_in_process = 0;
}

/// <summary>
/// SFCs the operation irq try.
/// </summary>
/// <param name="famicom">The famicom.</param>
extern void sfc_operation_IRQ(sfc_famicom_t * famicom) {
    const uint8_t pch = (uint8_t)((SFC_PC) >> 8);
    const uint8_t pcl = (uint8_t)SFC_PC;
    SFC_PUSH(pch);
    SFC_PUSH(pcl);
    SFC_PUSH(SFC_P | (uint8_t)(SFC_FLAG_R));
    SFC_IF_SE;
    const uint8_t pcl2 = SFC_READ_PC(SFC_VERCTOR_IRQ + 0);
    const uint8_t pch2 = SFC_READ_PC(SFC_VERCTOR_IRQ + 1);
    famicom->registers.program_counter = (uint16_t)pcl2 | (uint16_t)pch2 << 8;

    famicom->cpu_cycle_count += 7;
}


/// <summary>
/// SFCs the operation irq acknowledge.
/// </summary>
/// <param name="famicom">The famicom.</param>
extern inline void sfc_operation_IRQ_acknowledge(sfc_famicom_t* famicom) {
    famicom->registers.irq_flag = 0;
    famicom->registers.irq_counter = 0;
}

/// <summary>
/// SFCs the operation irq try.
/// </summary>
/// <param name="famicom">The famicom.</param>
extern inline void sfc_operation_IRQ_try(sfc_famicom_t* famicom) {
    // 禁用中断
    if (SFC_IF)
        famicom->registers.irq_flag = 1;
    // 直接中断
    else
        famicom->registers.irq_counter = 1;

    // 暂时借用APU的帧中断
    //if (SFC_IF)
    //    famicom->registers.apu_frame_interrupt = 1;
    //else 
    //    famicom->registers.apu_frame_interrupt_counter = 1;
}

/// <summary>
/// 命令名称
/// </summary>
struct sfc_opname {
    // 3字名称
    char        name[3];
    // 寻址模式[低4位] | 指令长度[高4位]
    uint8_t     mode_inslen;
};

/// <summary>
/// 反汇编用数据
/// </summary>
static const struct sfc_opname s_opname_data[256] = {
    // 0
    { 'B', 'R', 'K', SFC_AM_IMP | 1 << 4 },
    { 'O', 'R', 'A', SFC_AM_INX | 2 << 4 },
    { 'S', 'T', 'P', SFC_AM_UNK | 1 << 4 },
    { 'S', 'L', 'O', SFC_AM_INX | 2 << 4 },
    { 'N', 'O', 'P', SFC_AM_ZPG | 2 << 4 },
    { 'O', 'R', 'A', SFC_AM_ZPG | 2 << 4 },
    { 'A', 'S', 'L', SFC_AM_ZPG | 2 << 4 },
    { 'S', 'L', 'O', SFC_AM_ZPG | 2 << 4 },
    { 'P', 'H', 'P', SFC_AM_IMP | 1 << 4 },
    { 'O', 'R', 'A', SFC_AM_IMM | 2 << 4 },
    { 'A', 'S', 'L', SFC_AM_ACC | 1 << 4 },
    { 'A', 'N', 'C', SFC_AM_IMM | 2 << 4 },
    { 'N', 'O', 'P', SFC_AM_ABS | 3 << 4 },
    { 'O', 'R', 'A', SFC_AM_ABS | 3 << 4 },
    { 'A', 'S', 'L', SFC_AM_ABS | 3 << 4 },
    { 'S', 'L', 'O', SFC_AM_ABS | 3 << 4 },
    // 1
    { 'B', 'P', 'L', SFC_AM_REL | 2 << 4 },
    { 'O', 'R', 'A', SFC_AM_INY | 2 << 4 },
    { 'S', 'T', 'P', SFC_AM_UNK | 1 << 4 },
    { 'S', 'L', 'O', SFC_AM_INY | 2 << 4 },
    { 'N', 'O', 'P', SFC_AM_ZPX | 2 << 4 },
    { 'O', 'R', 'A', SFC_AM_ZPX | 2 << 4 },
    { 'A', 'S', 'L', SFC_AM_ZPX | 2 << 4 },
    { 'S', 'L', 'O', SFC_AM_ZPX | 2 << 4 },
    { 'C', 'L', 'C', SFC_AM_IMP | 1 << 4 },
    { 'O', 'R', 'A', SFC_AM_ABY | 3 << 4 },
    { 'N', 'O', 'P', SFC_AM_IMP | 1 << 4 },
    { 'S', 'L', 'O', SFC_AM_ABY | 3 << 4 },
    { 'N', 'O', 'P', SFC_AM_ABX | 3 << 4 },
    { 'O', 'R', 'A', SFC_AM_ABX | 3 << 4 },
    { 'A', 'S', 'L', SFC_AM_ABX | 3 << 4 },
    { 'S', 'L', 'O', SFC_AM_ABX | 3 << 4 },
    // 2
    { 'J', 'S', 'R', SFC_AM_ABS | 3 << 4 },
    { 'A', 'N', 'D', SFC_AM_INX | 2 << 4 },
    { 'S', 'T', 'P', SFC_AM_UNK | 1 << 4 },
    { 'R', 'L', 'A', SFC_AM_INX | 2 << 4 },
    { 'B', 'I', 'T', SFC_AM_ZPG | 2 << 4 },
    { 'A', 'N', 'D', SFC_AM_ZPG | 2 << 4 },
    { 'R', 'O', 'L', SFC_AM_ZPG | 2 << 4 },
    { 'R', 'L', 'A', SFC_AM_ZPG | 2 << 4 },
    { 'P', 'L', 'P', SFC_AM_IMP | 1 << 4 },
    { 'A', 'N', 'D', SFC_AM_IMM | 2 << 4 },
    { 'R', 'O', 'L', SFC_AM_ACC | 1 << 4 },
    { 'A', 'N', 'C', SFC_AM_IMM | 2 << 4 },
    { 'B', 'I', 'T', SFC_AM_ABS | 3 << 4 },
    { 'A', 'N', 'D', SFC_AM_ABS | 3 << 4 },
    { 'R', 'O', 'L', SFC_AM_ABS | 3 << 4 },
    { 'R', 'L', 'A', SFC_AM_ABS | 3 << 4 },
    // 3
    { 'B', 'M', 'I', SFC_AM_REL | 2 << 4 },
    { 'A', 'N', 'D', SFC_AM_INY | 2 << 4 },
    { 'S', 'T', 'P', SFC_AM_UNK | 1 << 4 },
    { 'R', 'L', 'A', SFC_AM_INY | 2 << 4 },
    { 'N', 'O', 'P', SFC_AM_ZPX | 2 << 4 },
    { 'A', 'N', 'D', SFC_AM_ZPX | 2 << 4 },
    { 'R', 'O', 'L', SFC_AM_ZPX | 2 << 4 },
    { 'R', 'L', 'A', SFC_AM_ZPX | 2 << 4 },
    { 'S', 'E', 'C', SFC_AM_IMP | 1 << 4 },
    { 'A', 'N', 'D', SFC_AM_ABY | 3 << 4 },
    { 'N', 'O', 'P', SFC_AM_IMP | 1 << 4 },
    { 'R', 'L', 'A', SFC_AM_ABY | 3 << 4 },
    { 'N', 'O', 'P', SFC_AM_ABX | 3 << 4 },
    { 'A', 'N', 'D', SFC_AM_ABX | 3 << 4 },
    { 'R', 'O', 'L', SFC_AM_ABX | 3 << 4 },
    { 'R', 'L', 'A', SFC_AM_ABX | 3 << 4 },
    // 4
    { 'R', 'T', 'I', SFC_AM_IMP | 1 << 4 },
    { 'E', 'O', 'R', SFC_AM_INX | 2 << 4 },
    { 'S', 'T', 'P', SFC_AM_UNK | 1 << 4 },
    { 'S', 'R', 'E', SFC_AM_INX | 2 << 4 },
    { 'N', 'O', 'P', SFC_AM_ZPG | 2 << 4 },
    { 'E', 'O', 'R', SFC_AM_ZPG | 2 << 4 },
    { 'L', 'S', 'R', SFC_AM_ZPG | 2 << 4 },
    { 'S', 'R', 'E', SFC_AM_ZPG | 2 << 4 },
    { 'P', 'H', 'A', SFC_AM_IMP | 1 << 4 },
    { 'E', 'O', 'R', SFC_AM_IMM | 2 << 4 },
    { 'L', 'S', 'R', SFC_AM_ACC | 1 << 4 },
    { 'A', 'S', 'R', SFC_AM_IMM | 2 << 4 },
    { 'J', 'M', 'P', SFC_AM_ABS | 3 << 4 },
    { 'E', 'O', 'R', SFC_AM_ABS | 3 << 4 },
    { 'L', 'S', 'R', SFC_AM_ABS | 3 << 4 },
    { 'S', 'R', 'E', SFC_AM_ABS | 3 << 4 },
    // 5
    { 'B', 'V', 'C', SFC_AM_REL | 2 << 4 },
    { 'E', 'O', 'R', SFC_AM_INY | 2 << 4 },
    { 'S', 'T', 'P', SFC_AM_UNK | 1 << 4 },
    { 'S', 'R', 'E', SFC_AM_INY | 2 << 4 },
    { 'N', 'O', 'P', SFC_AM_ZPX | 2 << 4 },
    { 'E', 'O', 'R', SFC_AM_ZPX | 2 << 4 },
    { 'L', 'S', 'R', SFC_AM_ZPX | 2 << 4 },
    { 'S', 'R', 'E', SFC_AM_ZPX | 2 << 4 },
    { 'C', 'L', 'I', SFC_AM_IMP | 1 << 4 },
    { 'E', 'O', 'R', SFC_AM_ABY | 3 << 4 },
    { 'N', 'O', 'P', SFC_AM_IMP | 1 << 4 },
    { 'S', 'R', 'E', SFC_AM_ABY | 3 << 4 },
    { 'N', 'O', 'P', SFC_AM_ABX | 3 << 4 },
    { 'E', 'O', 'R', SFC_AM_ABX | 3 << 4 },
    { 'L', 'S', 'R', SFC_AM_ABX | 3 << 4 },
    { 'S', 'R', 'E', SFC_AM_ABX | 3 << 4 },
    // 6
    { 'R', 'T', 'S', SFC_AM_IMP | 1 << 4 },
    { 'A', 'D', 'C', SFC_AM_INX | 2 << 4 },
    { 'S', 'T', 'P', SFC_AM_UNK | 1 << 4 },
    { 'R', 'R', 'A', SFC_AM_INX | 2 << 4 },
    { 'N', 'O', 'P', SFC_AM_ZPG | 2 << 4 },
    { 'A', 'D', 'C', SFC_AM_ZPG | 2 << 4 },
    { 'R', 'O', 'R', SFC_AM_ZPG | 2 << 4 },
    { 'R', 'R', 'A', SFC_AM_ZPG | 2 << 4 },
    { 'P', 'L', 'A', SFC_AM_IMP | 1 << 4 },
    { 'A', 'D', 'C', SFC_AM_IMM | 2 << 4 },
    { 'R', 'O', 'R', SFC_AM_ACC | 1 << 4 },
    { 'A', 'R', 'R', SFC_AM_IMM | 2 << 4 },
    { 'J', 'M', 'P', SFC_AM_IND | 3 << 4 },
    { 'A', 'D', 'C', SFC_AM_ABS | 3 << 4 },
    { 'R', 'O', 'R', SFC_AM_ABS | 3 << 4 },
    { 'R', 'R', 'A', SFC_AM_ABS | 3 << 4 },
    // 7
    { 'B', 'V', 'S', SFC_AM_REL | 2 << 4 },
    { 'A', 'D', 'C', SFC_AM_INY | 2 << 4 },
    { 'S', 'T', 'P', SFC_AM_UNK | 1 << 4 },
    { 'R', 'R', 'A', SFC_AM_INY | 2 << 4 },
    { 'N', 'O', 'P', SFC_AM_ZPX | 2 << 4 },
    { 'A', 'D', 'C', SFC_AM_ZPX | 2 << 4 },
    { 'R', 'O', 'R', SFC_AM_ZPX | 2 << 4 },
    { 'R', 'R', 'A', SFC_AM_ZPX | 2 << 4 },
    { 'S', 'E', 'I', SFC_AM_IMP | 1 << 4 },
    { 'A', 'D', 'C', SFC_AM_ABY | 3 << 4 },
    { 'N', 'O', 'P', SFC_AM_IMP | 1 << 4 },
    { 'R', 'R', 'A', SFC_AM_ABY | 3 << 4 },
    { 'N', 'O', 'P', SFC_AM_ABX | 3 << 4 },
    { 'A', 'D', 'C', SFC_AM_ABX | 3 << 4 },
    { 'R', 'O', 'R', SFC_AM_ABX | 3 << 4 },
    { 'R', 'R', 'A', SFC_AM_ABX | 3 << 4 },
    // 8
    { 'N', 'O', 'P', SFC_AM_IMM | 2 << 4 },
    { 'S', 'T', 'A', SFC_AM_INX | 2 << 4 },
    { 'N', 'O', 'P', SFC_AM_IMM | 2 << 4 },
    { 'S', 'A', 'X', SFC_AM_INX | 2 << 4 },
    { 'S', 'T', 'Y', SFC_AM_ZPG | 2 << 4 },
    { 'S', 'T', 'A', SFC_AM_ZPG | 2 << 4 },
    { 'S', 'T', 'X', SFC_AM_ZPG | 2 << 4 },
    { 'S', 'A', 'X', SFC_AM_ZPG | 2 << 4 },
    { 'D', 'E', 'Y', SFC_AM_IMP | 1 << 4 },
    { 'N', 'O', 'P', SFC_AM_IMM | 2 << 4 },
    { 'T', 'A', 'X', SFC_AM_IMP | 1 << 4 },
    { 'X', 'X', 'A', SFC_AM_IMM | 2 << 4 },
    { 'S', 'T', 'Y', SFC_AM_ABS | 3 << 4 },
    { 'S', 'T', 'A', SFC_AM_ABS | 3 << 4 },
    { 'S', 'T', 'X', SFC_AM_ABS | 3 << 4 },
    { 'S', 'A', 'X', SFC_AM_ABS | 3 << 4 },
    // 9
    { 'B', 'C', 'C', SFC_AM_REL | 2 << 4 },
    { 'S', 'T', 'A', SFC_AM_INY | 2 << 4 },
    { 'S', 'T', 'P', SFC_AM_UNK | 1 << 4 },
    { 'A', 'H', 'X', SFC_AM_INY | 2 << 4 },
    { 'S', 'T', 'Y', SFC_AM_ZPX | 2 << 4 },
    { 'S', 'T', 'A', SFC_AM_ZPX | 2 << 4 },
    { 'S', 'T', 'X', SFC_AM_ZPY | 2 << 4 },
    { 'S', 'A', 'X', SFC_AM_ZPY | 2 << 4 },
    { 'T', 'Y', 'A', SFC_AM_IMP | 1 << 4 },
    { 'S', 'T', 'A', SFC_AM_ABY | 3 << 4 },
    { 'T', 'X', 'S', SFC_AM_IMP | 1 << 4 },
    { 'T', 'A', 'S', SFC_AM_ABY | 3 << 4 },
    { 'S', 'H', 'Y', SFC_AM_ABX | 3 << 4 },
    { 'S', 'T', 'A', SFC_AM_ABX | 3 << 4 },
    { 'S', 'H', 'X', SFC_AM_ABY | 3 << 4 },
    { 'A', 'H', 'X', SFC_AM_ABY | 3 << 4 },
    // A
    { 'L', 'D', 'Y', SFC_AM_IMM | 2 << 4 },
    { 'L', 'D', 'A', SFC_AM_INX | 2 << 4 },
    { 'L', 'D', 'X', SFC_AM_IMM | 2 << 4 },
    { 'L', 'A', 'X', SFC_AM_INX | 2 << 4 },
    { 'L', 'D', 'Y', SFC_AM_ZPG | 2 << 4 },
    { 'L', 'D', 'A', SFC_AM_ZPG | 2 << 4 },
    { 'L', 'D', 'X', SFC_AM_ZPG | 2 << 4 },
    { 'L', 'A', 'X', SFC_AM_ZPG | 2 << 4 },
    { 'T', 'A', 'Y', SFC_AM_IMP | 1 << 4 },
    { 'L', 'D', 'A', SFC_AM_IMM | 2 << 4 },
    { 'T', 'A', 'X', SFC_AM_IMP | 1 << 4 },
    { 'L', 'A', 'X', SFC_AM_IMM | 2 << 4 },
    { 'L', 'D', 'Y', SFC_AM_ABS | 3 << 4 },
    { 'L', 'D', 'A', SFC_AM_ABS | 3 << 4 },
    { 'L', 'D', 'X', SFC_AM_ABS | 3 << 4 },
    { 'L', 'A', 'X', SFC_AM_ABS | 3 << 4 },
    // B
    { 'B', 'C', 'S', SFC_AM_REL | 2 << 4 },
    { 'L', 'D', 'A', SFC_AM_INY | 2 << 4 },
    { 'S', 'T', 'P', SFC_AM_UNK | 1 << 4 },
    { 'L', 'A', 'X', SFC_AM_INY | 2 << 4 },
    { 'L', 'D', 'Y', SFC_AM_ZPX | 2 << 4 },
    { 'L', 'D', 'A', SFC_AM_ZPX | 2 << 4 },
    { 'L', 'D', 'X', SFC_AM_ZPY | 2 << 4 },
    { 'L', 'A', 'X', SFC_AM_ZPY | 2 << 4 },
    { 'C', 'L', 'V', SFC_AM_IMP | 1 << 4 },
    { 'L', 'D', 'A', SFC_AM_ABY | 3 << 4 },
    { 'T', 'S', 'X', SFC_AM_IMP | 1 << 4 },
    { 'L', 'A', 'S', SFC_AM_ABY | 3 << 4 },
    { 'L', 'D', 'Y', SFC_AM_ABX | 3 << 4 },
    { 'L', 'D', 'A', SFC_AM_ABX | 3 << 4 },
    { 'L', 'D', 'X', SFC_AM_ABY | 3 << 4 },
    { 'L', 'A', 'X', SFC_AM_ABY | 3 << 4 },
    // C
    { 'C', 'P', 'Y', SFC_AM_IMM | 2 << 4 },
    { 'C', 'M', 'P', SFC_AM_INX | 2 << 4 },
    { 'N', 'O', 'P', SFC_AM_IMM | 2 << 4 },
    { 'D', 'C', 'P', SFC_AM_INX | 2 << 4 },
    { 'C', 'P', 'Y', SFC_AM_ZPG | 2 << 4 },
    { 'C', 'M', 'P', SFC_AM_ZPG | 2 << 4 },
    { 'D', 'E', 'C', SFC_AM_ZPG | 2 << 4 },
    { 'D', 'C', 'P', SFC_AM_ZPG | 2 << 4 },
    { 'I', 'N', 'Y', SFC_AM_IMP | 1 << 4 },
    { 'C', 'M', 'P', SFC_AM_IMM | 2 << 4 },
    { 'D', 'E', 'X', SFC_AM_IMP | 1 << 4 },
    { 'A', 'X', 'S', SFC_AM_IMM | 2 << 4 },
    { 'C', 'P', 'Y', SFC_AM_ABS | 3 << 4 },
    { 'C', 'M', 'P', SFC_AM_ABS | 3 << 4 },
    { 'D', 'E', 'C', SFC_AM_ABS | 3 << 4 },
    { 'D', 'C', 'P', SFC_AM_ABS | 3 << 4 },
    // D
    { 'B', 'N', 'E', SFC_AM_REL | 2 << 4 },
    { 'C', 'M', 'P', SFC_AM_INY | 2 << 4 },
    { 'S', 'T', 'P', SFC_AM_UNK | 1 << 4 },
    { 'D', 'C', 'P', SFC_AM_INY | 2 << 4 },
    { 'N', 'O', 'P', SFC_AM_ZPX | 2 << 4 },
    { 'C', 'M', 'P', SFC_AM_ZPX | 2 << 4 },
    { 'D', 'E', 'C', SFC_AM_ZPX | 2 << 4 },
    { 'D', 'C', 'P', SFC_AM_ZPX | 2 << 4 },
    { 'C', 'L', 'D', SFC_AM_IMP | 1 << 4 },
    { 'C', 'M', 'P', SFC_AM_ABY | 3 << 4 },
    { 'N', 'O', 'P', SFC_AM_IMP | 1 << 4 },
    { 'D', 'C', 'P', SFC_AM_ABY | 3 << 4 },
    { 'N', 'O', 'P', SFC_AM_ABX | 3 << 4 },
    { 'C', 'M', 'P', SFC_AM_ABX | 3 << 4 },
    { 'D', 'E', 'C', SFC_AM_ABX | 3 << 4 },
    { 'D', 'C', 'P', SFC_AM_ABX | 3 << 4 },
    // E
    { 'C', 'P', 'X', SFC_AM_IMM | 2 << 4 },
    { 'S', 'B', 'C', SFC_AM_INX | 2 << 4 },
    { 'N', 'O', 'P', SFC_AM_IMM | 2 << 4 },
    { 'I', 'S', 'B', SFC_AM_INX | 2 << 4 },
    { 'C', 'P', 'X', SFC_AM_ZPG | 2 << 4 },
    { 'S', 'B', 'C', SFC_AM_ZPG | 2 << 4 },
    { 'I', 'N', 'C', SFC_AM_ZPG | 2 << 4 },
    { 'I', 'S', 'B', SFC_AM_ZPG | 2 << 4 },
    { 'I', 'N', 'X', SFC_AM_IMP | 1 << 4 },
    { 'S', 'B', 'C', SFC_AM_IMM | 2 << 4 },
    { 'N', 'O', 'P', SFC_AM_IMP | 1 << 4 },
    { 'S', 'B', 'C', SFC_AM_IMM | 2 << 4 },
    { 'C', 'P', 'X', SFC_AM_ABS | 3 << 4 },
    { 'S', 'B', 'C', SFC_AM_ABS | 3 << 4 },
    { 'I', 'N', 'C', SFC_AM_ABS | 3 << 4 },
    { 'I', 'S', 'B', SFC_AM_ABS | 3 << 4 },
    // F
    { 'B', 'E', 'Q', SFC_AM_REL | 2 << 4 },
    { 'S', 'B', 'C', SFC_AM_INY | 2 << 4 },
    { 'S', 'T', 'P', SFC_AM_UNK | 1 << 4 },
    { 'I', 'S', 'B', SFC_AM_INY | 2 << 4 },
    { 'N', 'O', 'P', SFC_AM_ZPX | 2 << 4 },
    { 'S', 'B', 'C', SFC_AM_ZPX | 2 << 4 },
    { 'I', 'N', 'C', SFC_AM_ZPX | 2 << 4 },
    { 'I', 'S', 'B', SFC_AM_ZPX | 2 << 4 },
    { 'S', 'E', 'D', SFC_AM_IMP | 1 << 4 },
    { 'S', 'B', 'C', SFC_AM_ABY | 3 << 4 },
    { 'N', 'O', 'P', SFC_AM_IMP | 1 << 4 },
    { 'I', 'S', 'B', SFC_AM_ABY | 3 << 4 },
    { 'N', 'O', 'P', SFC_AM_ABX | 3 << 4 },
    { 'S', 'B', 'C', SFC_AM_ABX | 3 << 4 },
    { 'I', 'N', 'C', SFC_AM_ABX | 3 << 4 },
    { 'I', 'S', 'B', SFC_AM_ABX | 3 << 4 },
};


/// <summary>
/// StepFC: 获取指令长度
/// </summary>
/// <param name="opcode">The opcode.</param>
/// <returns></returns>
extern inline uint8_t sfc_get_inslen(uint8_t opcode) {
    return s_opname_data[opcode].mode_inslen >> 4;
}


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
    const enum sfc_6502_addressing_mode addrmode = opname.mode_inslen & 0x0f;
    switch (addrmode)
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
        if (addrmode == SFC_AM_ABS) break;
        buf[ADDR_FIRSH + 5] = ',';
        buf[ADDR_FIRSH + 7] = addrmode == SFC_AM_ABX ? 'X' : 'Y';
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
        if (addrmode == SFC_AM_ZPG) break;
        buf[ADDR_FIRSH + 3] = ',';
        buf[ADDR_FIRSH + 5] = addrmode == SFC_AM_ABX ? 'X' : 'Y';
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
        buf[ADDR_FIRSH + 7] = 'Y';
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
