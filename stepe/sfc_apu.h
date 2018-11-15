#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com

#include <stdint.h>


/// <summary>
/// 
/// </summary>
enum sfc_channel_index {
    // [VRC7] VRC7
    SFC_VRC7_VRC7 = -3,
    // [VRC6] VRC6
    SFC_VRC6_VRC6 = -2,
    // 帧计数器/序列器
    SFC_FrameCounter = -1,
    // 总体
    SFC_Overview = 0,
    // [2A03] 方波#1
    SFC_2A03_Square1,
    // [2A03] 方波#2
    SFC_2A03_Square2,
    // [2A03] 三角波
    SFC_2A03_Triangle,
    // [2A03] 噪声
    SFC_2A03_Noise,
    // [2A03] DMC
    SFC_2A03_MDC,
    // [VRC6] 方波#1
    SFC_VRC6_Square1,
    // [VRC6] 方波#2
    SFC_VRC6_Square2,
    // [VRC6] 锯齿波
    SFC_VRC6_Saw,
    // [VRC7] FM声道#0
    SFC_VRC7_FM0,
    // [VRC7] FM声道#1
    SFC_VRC7_FM1,
    // [VRC7] FM声道#2
    SFC_VRC7_FM2,
    // [VRC7] FM声道#3
    SFC_VRC7_FM3,
    // [VRC7] FM声道#4
    SFC_VRC7_FM4,
    // [VRC7] FM声道#5
    SFC_VRC7_FM5,
    // 总声道数量
    SFC_CHANNEL_COUNT
} ;

/// <summary>
/// 
/// </summary>
enum sfc_4015_write_flag {
    SFC_APU4015_WRITE_EnableSquare1 = 0x01, // 方波1 使能
    SFC_APU4015_WRITE_EnableSquare2 = 0x02, // 方波2 使能
    SFC_APU4015_WRITE_EnableTriangle= 0x04, // 三角波 使能
    SFC_APU4015_WRITE_EnableNoise   = 0x08, // 噪声 使能
    SFC_APU4015_WRITE_EnableDMC     = 0x10, // DMC 使能
};

/// <summary>
/// 
/// </summary>
enum sfc_4015_read_flag {
    SFC_APU4015_READ_Square1Length  = 0x01, // 方波1 长度计数器>0
    SFC_APU4015_READ_Square2Length  = 0x02, // 方波2 长度计数器>0
    SFC_APU4015_READ_TriangleLength = 0x04, // 三角波 长度计数器>0
    SFC_APU4015_READ_NoiseLength    = 0x08, // 三角波 长度计数器>0
    SFC_APU4015_READ_DMCActive      = 0x10, // DMC激活状态

    SFC_APU4015_READ_Frameinterrupt = 0x40, // 帧中断
    SFC_APU4015_READ_DMCInterrupt   = 0x80, // DMC中断
};

/// <summary>
/// 
/// </summary>
enum sfc_4017_flag {
    SFC_APU4017_ModeStep5   = 0x80, // 5步模式
    SFC_APU4017_IRQDisable  = 0x40, // IRQ禁用
};


/// <summary>
/// 
/// </summary>
enum sfc_apu_ctrl6_flag {
    SFC_APUCTRL6_ConstVolume= 0x10,    // 固定音量
    SFC_APUCTRL6_EnvLoop    = 0x20,    // 循环包络
};

/// <summary>
/// 包络
/// </summary>
typedef struct {
    // 时钟分频器
    uint8_t     divider;
    // 计数器
    uint8_t     counter;
    // 开始标记
    uint8_t     start;
    // 控制器低6位
    uint8_t     ctrl6;

} sfc_envelope_t;


/// <summary>
/// 方波寄存器/数据
/// </summary>
struct sfc_square_data_t {
    // 包络
    sfc_envelope_t  envelope;
    // 当前周期
    uint16_t        cur_period;
    // 序列索引
    uint8_t         seq_index;
    // 未使用
    uint8_t         unused;
    // 长度计数器
    uint8_t         length_counter;
    // 控制寄存器
    uint8_t         ctrl;
    // 扫描单元: 重载
    uint8_t         sweep_reload;
    // 扫描单元: 使能
    uint8_t         sweep_enable;
    // 扫描单元: 反相扫描
    uint8_t         sweep_negate;
    // 扫描单元: 时钟分频器周期
    uint8_t         sweep_period;
    // 扫描单元: 时钟分频器
    uint8_t         sweep_divider;
    // 扫描单元: 移位器
    uint8_t         sweep_shift;
};


/// <summary>
/// 三角波寄存器/数据
/// </summary>
struct sfc_triangle_data_t {
    // 当前周期
    uint16_t        cur_period;
    // 长度计数器
    uint8_t         length_counter;
    // 线性计数器
    uint8_t         linear_counter;
    // 线性计数器 重载值
    uint8_t         value_reload;
    // 线性计数器 重载标志
    uint8_t         flag_reload;
    // 长度计数器/线性计数器暂停值
    uint8_t         flag_halt;
};

/// <summary>
/// 
/// </summary>
struct sfc_noise_data_t {
    // 包络
    sfc_envelope_t  envelope;
    // 线性反馈移位寄存器(暂时没用到)
    uint16_t        lfsr;
    // 长度计数器
    uint8_t         length_counter;
    // 短模式[D7] 周期索引[D0-D3]
    uint8_t         short_mode__period_index;
};

/// <summary>
/// 
/// </summary>
struct sfc_dmc_data_t {
    // 原始地址
    uint16_t        orgaddr;
    // 当前地址
    uint16_t        curaddr;
    // 原始长度
    uint16_t        length;
    // 剩余长度
    uint16_t        lenleft;
    // 周期
    uint16_t        period;
    // 周期索引
    uint16_t        index;
    // 输出
    uint8_t         value;
    // 中断[D1]/循环[D0]
    uint8_t         irq_loop;
    // 8步计数
    uint8_t         count;
    // 字节数据
    uint8_t         data;
};


/// <summary>
/// VRC6 方波数据
/// </summary>
typedef struct {
    // 周期
    uint16_t        period;
    // 周期-原始
    uint16_t        period_raw;
    // 音量
    uint8_t         volume;
    // 占空比
    uint8_t         duty;
    // 使能位
    uint8_t         enable;
    // 索引
    uint8_t         index;
} sfc_vrc6_square_data_t;

/// <summary>
/// VRC6 锯齿波数据
/// </summary>
typedef struct {
    // 周期
    uint16_t        period;
    // 周期-原始
    uint16_t        period_raw;
    // 累加率
    uint8_t         rate;
    // 内部累加器
    uint8_t         accumulator;
    // 使能位
    uint8_t         enable;
    // 增加次数
    uint8_t         count;
} sfc_vrc6_saw_data_t;

/// <summary>
/// VRC6数据
/// </summary>
typedef struct {
    // 方波 #1
    sfc_vrc6_square_data_t      square1;
    // 方波 #2
    sfc_vrc6_square_data_t      square2;
    // 锯齿波
    sfc_vrc6_saw_data_t         saw;
    // 频率控制
    uint8_t                     freq_ctrl;
    // 暂停
    uint8_t                     halt;
} sfc_vrc6_data_t;


/// <summary>
/// VRC7 声道数据
/// </summary>
typedef struct {
    // 频率 [9bit]
    uint16_t    freq;
    // 八度 [3bit]
    uint8_t     octave;
    // 开关 [1bit]
    uint8_t     trigger;
    // 延音 [1bit]
    uint8_t     sustain;
    // 乐器 [4bit]
    uint8_t     instrument;
    // 音量 [4bit]
    uint8_t     volume;
    // 未使用
    uint8_t     unused[1];
} sfc_vrc7_ch_t;


/// <summary>
/// VRC7数据
/// </summary>
typedef struct {
    // 6个声道
    sfc_vrc7_ch_t   ch[6];
    // 寄存器选择
    uint8_t         selected;
    // 未使用
    uint8_t         unused[3];
} sfc_vrc7_data_t;


/// <summary>
/// APU寄存器数据
/// </summary>
typedef struct {
    // 方波 #1
    struct sfc_square_data_t    square1;
    // 方波 #2
    struct sfc_square_data_t    square2;
    // 三角波
    struct sfc_triangle_data_t  triangle;
    // 噪声
    struct sfc_noise_data_t     noise;
    // DMC
    struct sfc_dmc_data_t       dmc;
    // VRC6
    sfc_vrc6_data_t             vrc6;
    // VRC7
    sfc_vrc7_data_t             vrc7;
    // 状态寄存器(写: 声道使能)
    uint8_t                     status_write;
    // 状态寄存器(读:)
    //uint8_t                     status_read;
    // 帧计数器写入寄存器
    uint8_t                     frame_counter;
    // 帧中断标志
    uint8_t                     frame_interrupt;
    // 步数计数
    uint8_t                     frame_step;

} sfc_apu_register_t;


// typedef
struct sfc_famicom;
typedef struct sfc_famicom sfc_famicom_t;

// 重置后
void sfc_apu_on_reset(sfc_apu_register_t*);

// 触发一次帧计数器
void sfc_trigger_frame_counter(sfc_famicom_t*);


// LFSR 长模式
static inline uint16_t sfc_lfsr_long(uint16_t v) {
    const uint16_t a = v & 1;
    const uint16_t b = (v >> 1) & 1;
    return (uint16_t)(v >> 1) | (uint16_t)((a ^ b) << 14);
}

// LFSR 短模式
static inline uint16_t sfc_lfsr_short(uint16_t v) {
    const uint16_t a = v & 1;
    const uint16_t b = (v >> 6) & 1;
    return (uint16_t)(v >> 1) | (uint16_t)((a ^ b) << 14);
}