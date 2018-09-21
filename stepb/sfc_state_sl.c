#include "sfc_famicom.h"
#include <string.h>


/// <summary>
/// 判断用文件头
/// </summary>
typedef struct {
    // -StepFC-
    char    _stepfc_[8];
    // SL大法好
    char    great_sl[8];
    
} sfc_state_header_determine_t;


// 判断用文件头/数据
// 0x5927 大 UTF-16
// 0x6CD5 法 UTF-16
// 0x597D 好 UTF-16
static const uint8_t sfc_header_determine[] = {
    '-', 'S', 't', 'e', 'p', 'F', 'C', '-',
    'S', 'L', 0x27, 0x59, 0xD5, 0x6C, 0x7D, 0x59
};


// RAM 掩码位
enum SFC_RAM_MASK {
    // 主RAM
    SL_CPU_MAIN_RAM_2KB = 1 << 0,
    // SRAM
    SL_CPU_SRAM_8KB = 1 << 1,
    // 主VRAM
    SL_PPU_VRAM_2KB = 1 << 2,
    // 额外VRAM
    SL_PPU_EXVRAM_2KB = 1 << 3,
};


/// <summary>
/// 
/// </summary>
enum SFC_SL_CONSTANT {
    // 主版本号
    SL_MAJOR_VER = 0x01,
    // 副版本号
    SL_MINOR_VER = 0x00,
};

/// <summary>
/// 状态基本数据用文件头
/// </summary>
typedef struct {
    // 大小端检查用, 可以检测该位转换到其他平台
    uint32_t        endian_01020304;
    // 本结构体大小
    uint32_t        size_of_this;
    // 主 版本号
    uint8_t         major_version;
    // 副 版本号
    uint8_t         minor_version;
    // RAM 用掩码
    uint8_t         ram_mask;
    // 未使用
    uint8_t         unused;
    // 当前帧ID
    uint32_t        frame_counter;
    // CPU 周期数 低4字节
    uint32_t        cpu_cycle_lo;
    // CPU 周期数 高4字节
    uint32_t        cpu_cycle_hi;
    // APU 周期数 低4字节
    uint32_t        apu_cycle_lo;
    // APU 周期数 高4字节
    uint32_t        apu_cycle_hi;
    // PPU 周期数 低4字节
    uint32_t        ppu_cycle_lo;
    // PPU 周期数 高4字节
    uint32_t        ppu_cycle_hi;
    // 16字节对齐/ 保留用
    uint8_t         reserved[8];

} sfc_state_header_basic_t;


/// <summary>
/// 文件尾 用于存放易变数据
/// </summary>
typedef struct {
    // CPU 寄存器
    sfc_cpu_register_t          cpu_reg;*
} sfc_state_tail_t;

/// <summary>
/// SFCs the sl write stream.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="data">The data.</param>
/// <param name="len">The length.</param>
static inline void sfc_sl_write_stream(sfc_famicom_t* famicom, const uint8_t* data, uint32_t len) {
    famicom->interfaces.sl_write_stream(
        famicom->argument,
        data,
        len
    );
}

/// <summary>
/// SFCs the state of the famicom save.
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_famicom_save_state(sfc_famicom_t* famicom) {
    // 判断用数据
    sfc_sl_write_stream(famicom, sfc_header_determine, sizeof(sfc_header_determine));
    // 基础数据
    sfc_state_header_basic_t basic;
    memset(&basic, 0, sizeof(basic));
    basic.endian_01020304 = 0x01020304;
    basic.size_of_this = sizeof(basic);
    basic.major_version = SL_MAJOR_VER;
    basic.minor_version = SL_MINOR_VER;
    // 填写表格
    basic.frame_counter = famicom->frame_counter;
    basic.cpu_cycle_lo = famicom->cpu_cycle_count;
    // RAM
    basic.ram_mask
        // 主RAM是肯定的
        = SL_CPU_MAIN_RAM_2KB
        // 不缺这8KB
        | SL_CPU_SRAM_8KB
        // VRAM是肯定的
        | SL_PPU_VRAM_2KB
        // 不缺这2KB
        | SL_PPU_EXVRAM_2KB
        ;
    // 主RAM
    if (basic.ram_mask & SL_CPU_MAIN_RAM_2KB)
        sfc_sl_write_stream(
            famicom,
            famicom->main_memory,
            sizeof(famicom->main_memory)
        );
    // SRAM
    if (basic.ram_mask & SL_CPU_SRAM_8KB)
        sfc_sl_write_stream(
            famicom,
            famicom->save_memory,
            sizeof(famicom->save_memory)
        );
    // VRAM
    if (basic.ram_mask & SL_PPU_VRAM_2KB)
        sfc_sl_write_stream(
            famicom,
            famicom->video_memory,
            sizeof(famicom->video_memory)
        );
    // EX-VRAM
    if (basic.ram_mask & SL_PPU_VRAM_2KB)
        sfc_sl_write_stream(
            famicom,
            famicom->video_memory_ex,
            sizeof(famicom->video_memory_ex)
        );
}

/// <summary>
/// SFCs the state of the famicom load.
/// </summary>
/// <param name="famicom">The famicom.</param>
sfc_ecode sfc_famicom_load_state(sfc_famicom_t* famicom) {
    return SFC_ERROR_OK;
}
