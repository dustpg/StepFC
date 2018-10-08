#include "sfc_famicom.h"
#include <string.h>
#include <assert.h>


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
// 0x5927 大 UTF-16 小端保存
// 0x6CD5 法 UTF-16 小端保存
// 0x597D 好 UTF-16 小端保存
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
    uint8_t         reserved1[8];
    // CPU BANK 高位偏移
    uint32_t        cpu_hi_banks_offset[4];
    // PPU BANK 低位偏移
    uint32_t        ppu_lo_banks_offset[8];
    // PPU BANK 高位偏移
    uint32_t        ppu_hi_banks_offset[8];

} sfc_state_header_basic_t;


/// <summary>
/// 文件尾 用于存放易变数据
/// </summary>
typedef struct {
    // CPU 数据段大小
    uint32_t        cpu_seg_len;
    // PPU 数据段大小
    uint32_t        ppu_seg_len;
    // APU 数据段大小
    uint32_t        apu_seg_len;
    // Mapper 数据段大小
    uint32_t        mapper_seg_len;
    // 输入设备 数据段大小
    uint32_t        input_seg_len;
    // 保留16字节对齐
    uint32_t        reserved1[3];

} sfc_state_tail_len_t;


/// <summary>
/// 输入设备
/// </summary>
typedef struct {
    // 手柄序列状态#1
    uint16_t            button_index_1;
    // 手柄序列状态#2
    uint16_t            button_index_2;
    // 手柄序列状态
    uint16_t            button_index_mask;
    // 手柄按钮状态
    uint8_t             button_states[16];

} sfc_sl_input_data_t;


/// <summary>
/// 文件尾 用于存放易变数据
/// </summary>
typedef struct {
    // CPU 寄存器
    sfc_cpu_register_t          cpu_data;
    // PPU 数据
    sfc_ppu_data_t              ppu_data;
    // APU 数据
    sfc_apu_register_t          apu_data;
    // Mapper 数据
    sfc_mapper_buffer_t         mapper_data;
    // 输入设备数据
    sfc_sl_input_data_t         input_data;
    // 对齐用
    uint8_t                     unused[12];

} sfc_state_tail_data_t;

enum SFC_SL_CONSTANT2 {
    SL_SIZE_OF_TAIL_DATA = sizeof(sfc_state_tail_data_t),
    SL_SIZE_OF_TAIL_DATA_ALIGNED = (SL_SIZE_OF_TAIL_DATA + 15)/16*16
};


/// <summary>
/// SFCs the sl write stream.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="data">The data.</param>
/// <param name="len">The length.</param>
static inline void sfc_sl_write_stream(sfc_famicom_t* famicom, const void* data, uint32_t len) {
    famicom->interfaces.sl_write_stream(
        famicom->argument,
        data,
        len
    );
}

/// <summary>
/// SFCs the sl read stream.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="data">The data.</param>
/// <param name="len">The length.</param>
static inline void sfc_sl_read_stream(sfc_famicom_t* famicom, void* data, uint32_t len) {
    famicom->interfaces.sl_read_stream(
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
    // RAM检测
    const uint8_t ram_mask
        // 主RAM是肯定的
        = SL_CPU_MAIN_RAM_2KB
        // 不缺这8KB
        | SL_CPU_SRAM_8KB
        // VRAM是肯定的
        | SL_PPU_VRAM_2KB
        // 不缺这2KB
        | SL_PPU_EXVRAM_2KB
        ;
    // 判断用数据
    sfc_sl_write_stream(famicom, sfc_header_determine, sizeof(sfc_header_determine));
    // 基础数据
    {
        sfc_state_header_basic_t basic;
        memset(&basic, 0, sizeof(basic));
        basic.endian_01020304 = 0x01020304;
        basic.size_of_this = sizeof(basic);
        basic.major_version = SL_MAJOR_VER;
        basic.minor_version = SL_MINOR_VER;
        // 填写表格
        basic.frame_counter = famicom->frame_counter;
        basic.cpu_cycle_lo = famicom->cpu_cycle_count;
        basic.ram_mask = ram_mask;
        // BANKS 偏移量
        for (int i = 0; i != 4; ++i)
            basic.cpu_hi_banks_offset[i]
            = famicom->prg_banks[4 + i]
            - famicom->rom_info.data_prgrom
            ;
        for (int i = 0; i != 8; ++i)
            basic.ppu_lo_banks_offset[i]
            = famicom->ppu.banks[i]
            - famicom->rom_info.data_chrrom
            ;
        for (int i = 0; i != 8; ++i)
            basic.ppu_hi_banks_offset[i]
            = famicom->ppu.banks[8 + i]
            - famicom->video_memory
            ;
        // 写入
        sfc_sl_write_stream(famicom, &basic, sizeof(basic));
    }
    {
        // 主RAM
        if (ram_mask & SL_CPU_MAIN_RAM_2KB)
            sfc_sl_write_stream(
                famicom,
                famicom->main_memory,
                sizeof(famicom->main_memory)
            );
        // SRAM
        if (ram_mask & SL_CPU_SRAM_8KB)
            sfc_sl_write_stream(
                famicom,
                famicom->save_memory,
                sizeof(famicom->save_memory)
            );
        // VRAM
        if (ram_mask & SL_PPU_VRAM_2KB)
            sfc_sl_write_stream(
                famicom,
                famicom->video_memory,
                sizeof(famicom->video_memory)
            );
        // EX-VRAM
        if (ram_mask & SL_PPU_VRAM_2KB)
            sfc_sl_write_stream(
                famicom,
                famicom->video_memory_ex,
                sizeof(famicom->video_memory_ex)
            );
    }
    {
        // MAPPER PRG/CHR-RAM
        famicom->mapper.write_ram_to_stream(famicom);
    }
    {
        // 文件尾-长度
        sfc_state_tail_len_t tail_len;
        memset(&tail_len, 0, sizeof(tail_len));
        tail_len.cpu_seg_len = sizeof(sfc_cpu_register_t);
        tail_len.ppu_seg_len = sizeof(sfc_ppu_data_t);
        tail_len.apu_seg_len = sizeof(sfc_apu_register_t);
        tail_len.mapper_seg_len = sizeof(sfc_mapper_buffer_t);
        tail_len.input_seg_len = sizeof(sfc_sl_input_data_t);
        sfc_sl_write_stream(
            famicom,
            &tail_len,
            sizeof(tail_len)
        );
    }
    {
        // 文件尾-数据
        sfc_state_tail_data_t tail_data;
        tail_data.cpu_data = famicom->registers;
        tail_data.ppu_data = famicom->ppu.data;
        tail_data.apu_data = famicom->apu;
        tail_data.mapper_data = famicom->mapper_buffer;
        memcpy(
            &tail_data.input_data.button_index_1,
            &famicom->button_index_1,
            sizeof(tail_data.input_data)
        );
        sfc_sl_write_stream(
            famicom,
            &tail_data,
            sizeof(tail_data)
        );
    }
}

/// <summary>
/// SFCs the state of the famicom load.
/// </summary>
/// <param name="famicom">The famicom.</param>
sfc_ecode sfc_famicom_load_state(sfc_famicom_t* famicom) {
    {
        sfc_state_header_determine_t hd;
        sfc_sl_read_stream(famicom, &hd, sizeof(hd));
        // 非法文件
        if (memcmp(&hd, sfc_header_determine, sizeof(hd)))
            return SFC_ERROR_MAPPER_NOT_FOUND;
    }
    uint8_t ram_mask = 0;
    {
        sfc_state_header_basic_t hb;
        sfc_sl_read_stream(famicom, &hb, sizeof(hb));
        // 检测数据准确性
        if (hb.endian_01020304 != 0x01020304)
            return SFC_ERROR_BAD_ENDAIN;
        if (hb.major_version != SL_MAJOR_VER ||
            hb.minor_version != SL_MINOR_VER)
            return SFC_ERROR_VERSION_NOT_MATCHED;
        // 认为数据匹配 开始写入数据
        famicom->frame_counter = hb.frame_counter;
        famicom->cpu_cycle_count = hb.cpu_cycle_lo;
        ram_mask = hb.ram_mask;
        // BANKS 偏移量
        for (int i = 0; i != 4; ++i)
            famicom->prg_banks[4 + i]
            = hb.cpu_hi_banks_offset[i]
            + famicom->rom_info.data_prgrom
            ;
        for (int i = 0; i != 8; ++i)
            famicom->ppu.banks[i]
            = famicom->rom_info.data_chrrom
            + hb.ppu_lo_banks_offset[i]
            ;
        for (int i = 0; i != 8; ++i)
            famicom->ppu.banks[8 + i]
            = famicom->video_memory
            + hb.ppu_hi_banks_offset[i]
            ;
    }
    {
        // 主RAM
        if (ram_mask & SL_CPU_MAIN_RAM_2KB)
            sfc_sl_read_stream(
                famicom,
                famicom->main_memory,
                sizeof(famicom->main_memory)
            );
        // SRAM
        if (ram_mask & SL_CPU_SRAM_8KB)
            sfc_sl_read_stream(
                famicom,
                famicom->save_memory,
                sizeof(famicom->save_memory)
            );
        // VRAM
        if (ram_mask & SL_PPU_VRAM_2KB)
            sfc_sl_read_stream(
                famicom,
                famicom->video_memory,
                sizeof(famicom->video_memory)
            );
        // EX-VRAM
        if (ram_mask & SL_PPU_VRAM_2KB)
            sfc_sl_read_stream(
                famicom,
                famicom->video_memory_ex,
                sizeof(famicom->video_memory_ex)
            );
    }
    {
        // MAPPER PRG/CHR-RAM
        famicom->mapper.read_ram_from_stream(famicom);
    }
    {
        // 文件尾-长度
        sfc_state_tail_len_t tl;
        // 文件尾-数据
        sfc_state_tail_data_t td;
        // 读取长度信息, 用于不同版本处理, 这里无视掉
        sfc_sl_read_stream(famicom, &tl, sizeof(tl));
        // 由于版本匹配, 直接读对应数据
        sfc_sl_read_stream(famicom, &td, sizeof(td));
        // 暴力写入
        famicom->registers = td.cpu_data;
        famicom->ppu.data = td.ppu_data;
        famicom->apu = td.apu_data;
        famicom->mapper_buffer = td.mapper_data;
        memcpy(
            &famicom->button_index_1,
            &td.input_data.button_index_1,
            sizeof(td.input_data)
        );
    }
    return SFC_ERROR_OK;
}
