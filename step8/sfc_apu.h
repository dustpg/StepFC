#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com

#include <stdint.h>

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
    SFC_APU4015_READ_DMCActive      = 0x08, // DMC激活状态

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
    // 理论周期
    uint16_t        use_period;
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
    // 状态寄存器(写: 声道使能)
    uint8_t                     status_write;
    // 状态寄存器(读:)
    //uint8_t                     status_read;
    // 帧计数器写入寄存器
    uint8_t                     frame_counter_4017;
    // 步数计数
    uint8_t                     frame_step;

} sfc_apu_register_t;


// typedef
//struct sfc_famicom;
//typedef struct sfc_famicom sfc_famicom_t;

// 重置后
void sfc_apu_on_reset(sfc_apu_register_t*);

// 触发一次帧计数器
void sfc_trigger_frame_counter(sfc_apu_register_t*);