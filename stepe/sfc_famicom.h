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
    // 音频事件
    void(*audio_changed)(void*, uint32_t, enum sfc_channel_index);
    // 保存SRAM 8KB
    void(*save_sram)(void*, const sfc_rom_info_t*, const uint8_t*, uint32_t);
    // 读取SRAM 8KB
    void(*load_sram)(void*, const sfc_rom_info_t*, uint8_t*, uint32_t len);
    // 状态保存 写入流
    void(*sl_write_stream)(void*, const uint8_t*, uint32_t len);
    // 状态读取 读取流
    void(*sl_read_stream)(void*, uint8_t*, uint32_t len);
} sfc_interface_t;


#ifdef SFC_BEFORE_EXECUTE
// 执行指令前调用
extern void sfc_before_execute(void*, sfc_famicom_t*);
#endif

/// <summary>
/// StepFC: Mapper接口
/// </summary>
typedef struct {
    // Mapper 重置
    sfc_ecode(*reset)(sfc_famicom_t*);
    // 读取低地址
    uint8_t(*read_low)(sfc_famicom_t*, uint16_t);
    // 写入低地址
    void(*write_low)(sfc_famicom_t*, uint16_t, uint8_t);
    // 写入高地址
    void(*write_high)(sfc_famicom_t*, uint16_t, uint8_t);
    // 水平同步
    void(*hsync)(sfc_famicom_t*, uint16_t);
    // 写入RAM到流
    void(*write_ram_to_stream)(const sfc_famicom_t*);
    // 从流读取RAM
    void(*read_ram_from_stream)(sfc_famicom_t*);
} sfc_mapper_t;



// NSF 播放状态
typedef struct  {
    // PLAY当前时钟周期
    uint32_t        play_clock;
    // 帧序列器时钟周期
    uint32_t        framecounter_clock;
} sfc_nsf_playstate_t;

/// <summary>
/// FC 模拟器主体
/// </summary>
struct sfc_famicom {
    // 参数
    void*               argument;
    // 扩展接口
    sfc_interface_t     interfaces;
    // 配置信息
    sfc_config_t        config;
    // Mapper接口
    sfc_mapper_t        mapper;
    // Mapper缓存
    sfc_mapper_buffer_t mapper_buffer;
    // 寄存器
    sfc_cpu_register_t  registers;
    // (真)帧计数器
    uint32_t            frame_counter;
    // CPU 周期计数
    uint32_t            cpu_cycle_count;
    // NSF状态
    sfc_nsf_playstate_t nsf;
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
    uint8_t*            prg_banks[0x10000 >> 12];
#ifndef NDEBUG
    // 调试用BANK
    void*               debug_banks[16];
#endif
    // 自定义BUS: 00-3F 调制表  40-7F  波形表  80-82 FDS  100-10C NSF , 180-1FF VRC7
    uint8_t             bus_memory[512];
    // 显存
    uint8_t             video_memory[2 * 1024];
    // 4屏用额外显存
    uint8_t             video_memory_ex[2 * 1024];
    // 主内存
    uint8_t             main_memory[2 * 1024];
    // 工作(work)/保存(save)内存
    uint8_t             save_memory[8 * 1024];
    // 额外32KiB
    uint8_t             expansion_ram32[32 * 1024];
};


// VRC7 PATCH用128字节
static inline const uint8_t* sfc_get_vrc7_patchc(const sfc_famicom_t* f) { return f->bus_memory + 0x180; }
static inline uint8_t* sfc_get_vrc7_patch(sfc_famicom_t* f) { return f->bus_memory + 0x180; }
// FDS调制用64字节
static inline  int8_t* sfc_get_fds1_modtbl(sfc_famicom_t* f) { return (int8_t*)f->bus_memory; }
// FDS波形用64字节
static inline uint8_t* sfc_get_fds1_wavtbl(sfc_famicom_t* f) { return f->bus_memory + 64; }

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

// 保存状态
void sfc_famicom_save_state(sfc_famicom_t*);
// 读取状态
sfc_ecode sfc_famicom_load_state(sfc_famicom_t*);

// 初始化NSF
void sfc_famicom_nsf_init(sfc_famicom_t*, uint8_t index, uint8_t pal);
// 播放NSF
void sfc_famicom_nsf_play(sfc_famicom_t*);

// ----------------------- 内部函数
