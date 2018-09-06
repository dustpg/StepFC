#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com

#include "sfc_rom.h"
#include "sfc_code.h"
#include "sfc_apu.h"
#include "sfc_cpu.h"
#include "sfc_ppu.h"
#include "sfc_mapper.h"
#include "sfc_config.h"

// typedef
struct sfc_famicom;
typedef struct sfc_famicom sfc_famicom_t;

/// <summary>
/// The SFC disassembly buf length
/// </summary>
enum { SFC_DISASSEMBLY_BUF_LEN2 = 48 };


/// <summary>
/// The SFC button index
/// </summary>
enum sfc_button_index { 
    SFC_BUTTON_A = 0,
    SFC_BUTTON_B,
    SFC_BUTTON_SELECT,
    SFC_BUTTON_START,
    SFC_BUTTON_UP,
    SFC_BUTTON_DOWN,
    SFC_BUTTON_LEFT,
    SFC_BUTTON_RIGHT,
};

/// <summary>
/// 
/// </summary>
enum sfc_constant {
    SFC_WIDTH = 256,
    SFC_HEIGHT = 240,
    SFC_SPRITE_COUNT = 64,
};


// 指定地方反汇编
void sfc_fc_disassembly(
    uint16_t address,
    const sfc_famicom_t* famicom,
    char buf[SFC_DISASSEMBLY_BUF_LEN2]
);

/// <summary>
/// StepFC: 扩展接口
/// </summary>
typedef struct {
    // ROM 加载器读取信息
    sfc_ecode(*load_rom)(void*, sfc_rom_info_t*);
    // ROM 加载器卸载
    sfc_ecode(*free_rom)(void*, sfc_rom_info_t*);
    // 执行指令前
    void(*before_execute)(void*, sfc_famicom_t*);

} sfc_interface_t;


/// <summary>
/// StepFC: Mapper接口
/// </summary>
typedef struct {
    // Mapper 重置
    sfc_ecode(*reset)(sfc_famicom_t*);
    // 写入高地址
    void (*write_high)(sfc_famicom_t*, uint16_t, uint8_t);
    // 水平同步
    void(*hsync)(sfc_famicom_t*);

} sfc_mapper_t;


/// <summary>
/// FC 模拟器主体
/// </summary>
struct sfc_famicom {
    // 参数
    void*               argument;
    // 配置信息
    sfc_config_t        config;
    // 扩展接口
    sfc_interface_t     interfaces;
    // Mapper接口
    sfc_mapper_t        mapper;
    // Mapper缓存
    sfc_mapper_buffer_t mapper_buffer;
    // 寄存器
    sfc_cpu_register_t  registers;
    // CPU 周期计数
    uint32_t            cpu_cycle_count;
    // APU
    sfc_apu_register_t  apu;
    // PPU
    sfc_ppu_t           ppu;
    // ROM 信息
    sfc_rom_info_t      rom_info;
    // 手柄序列状态#1
    uint16_t            button_index_1;
    // 手柄序列状态#2
    uint16_t            button_index_2;
    // 手柄序列状态
    uint16_t            button_index_mask;
    // 手柄按钮状态
    uint8_t             button_states[16];
    // 程序内存仓库(Bank)/窗口(Window)
    uint8_t*            prg_banks[0x10000 >> 13];
    // 工作(work)/保存(save)内存
    uint8_t             save_memory[8 * 1024];
    // 显存
    uint8_t             video_memory[2 * 1024];
    // 4屏用额外显存
    uint8_t             video_memory_ex[2 * 1024];
    // 主内存
    uint8_t             main_memory[2 * 1024];
};


// 初始化
sfc_ecode sfc_famicom_init(
    sfc_famicom_t* famicom,
    void* argument,
    const sfc_interface_t* interfaces
);

// 重置
sfc_ecode sfc_famicom_reset(sfc_famicom_t* famicom);

// 反初始化
void sfc_famicom_uninit(sfc_famicom_t*);




// ----------------------- 内部函数
