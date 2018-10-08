#include "sfc_famicom.h"
#include <assert.h>
#include <string.h>

// 加载新的ROM
static sfc_ecode sfc_load_new_rom(sfc_famicom_t* famicom);
// 加载mapper
extern sfc_ecode sfc_load_mapper(sfc_famicom_t* famicom, uint8_t);

// 默认加载释放ROM
static sfc_ecode sfc_loadfree_rom(void* arg, sfc_rom_info_t* info) { return SFC_ERROR_FAILED; }

// 默认音频事件
static void sfc_audio_changed(void*a, uint32_t b, int c) {}
// 默认SRAM读写事件
static void sfc_save_load_sram(void*a, const sfc_rom_info_t*b, const uint8_t*c) {}
// 默认状态读写事件
static void sfc_sl_stream(void*a, const uint8_t* b, uint32_t c) {}

// 声明一个随便(SB)的函数指针类型
typedef void(*sfc_funcptr_t)();

/// <summary>
/// StepFC: 初始化famicom
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="argument">The argument.</param>
/// <param name="interfaces">The interfaces.</param>
/// <returns></returns>
sfc_ecode sfc_famicom_init(
    sfc_famicom_t* famicom, 
    void* argument, 
    const sfc_interface_t* interfaces) {
    assert(famicom && "bad famicom");
    // 清空数据
    memset(famicom, 0, sizeof(sfc_famicom_t));
    // 保留参数
    famicom->argument = argument;
    // 载入默认接口
    famicom->interfaces.load_rom = sfc_loadfree_rom;
    famicom->interfaces.free_rom = sfc_loadfree_rom;
    famicom->interfaces.audio_changed = sfc_audio_changed;
    famicom->interfaces.load_sram = sfc_save_load_sram;
    famicom->interfaces.save_sram = sfc_save_load_sram;
    famicom->interfaces.sl_write_stream = sfc_sl_stream;
    famicom->interfaces.sl_read_stream = sfc_sl_stream;
    // 初步BANK
    famicom->prg_banks[0] = famicom->main_memory;
    famicom->prg_banks[3] = famicom->save_memory;
    // 提供了接口
    if (interfaces) {
        const int count = sizeof(*interfaces) / sizeof(interfaces->load_rom);
        // 复制有效的函数指针
        // UB: C标准并不一定保证sizeof(void*)等同sizeof(fp) (非冯体系)
        //     所以这里声明了一个sfc_funcptr_t
        sfc_funcptr_t* const func_src = (sfc_funcptr_t*)interfaces;
        sfc_funcptr_t* const func_des = (sfc_funcptr_t*)&famicom->interfaces;
        for (int i = 0; i != count; ++i) if (func_src[i]) func_des[i] = func_src[i];
    }
    // 默认是NTSC制式
    famicom->config = SFC_CONFIG_NTSC;
    // 一开始载入测试ROM
    return sfc_load_new_rom(famicom);
    return SFC_ERROR_OK;
}

/// <summary>
/// SFCs the check save sram.
/// </summary>
/// <param name="famicom">The famicom.</param>
static inline void sfc_check_save_sram(sfc_famicom_t* famicom) {
    if (famicom->rom_info.save_ram) {
        famicom->interfaces.save_sram(
            famicom->argument,
            &famicom->rom_info,
            famicom->save_memory
        );
    }
}

/// <summary>
/// SFCs the check save sram.
/// </summary>
/// <param name="famicom">The famicom.</param>
static inline void sfc_check_load_sram(sfc_famicom_t* famicom) {
    if (famicom->rom_info.save_ram) {
        famicom->interfaces.load_sram(
            famicom->argument,
            &famicom->rom_info,
            famicom->save_memory
        );
    }
}

/// <summary>
/// StepFC: 反初始化famicom
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_famicom_uninit(sfc_famicom_t* famicom) {
    // 检测SRAM
    sfc_check_save_sram(famicom);
    // 释放ROM
    famicom->interfaces.free_rom(famicom->argument, &famicom->rom_info);
}


/// <summary>
/// SFCs the switch nametable mirroring.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="mode">The mode.</param>
void sfc_switch_nametable_mirroring(sfc_famicom_t* famicom, sfc_nametable_mirroring_mode mode) {
    switch (mode)
    {
    case SFC_NT_MIR_SingleLow:
        famicom->ppu.banks[0x8] = famicom->video_memory + 0x400 * 0;
        famicom->ppu.banks[0x9] = famicom->video_memory + 0x400 * 0;
        famicom->ppu.banks[0xa] = famicom->video_memory + 0x400 * 0;
        famicom->ppu.banks[0xb] = famicom->video_memory + 0x400 * 0;
        break;
    case SFC_NT_MIR_SingleHigh:
        famicom->ppu.banks[0x8] = famicom->video_memory + 0x400 * 1;
        famicom->ppu.banks[0x9] = famicom->video_memory + 0x400 * 1;
        famicom->ppu.banks[0xa] = famicom->video_memory + 0x400 * 1;
        famicom->ppu.banks[0xb] = famicom->video_memory + 0x400 * 1;
        break;
    case SFC_NT_MIR_Vertical:
        famicom->ppu.banks[0x8] = famicom->video_memory + 0x400 * 0;
        famicom->ppu.banks[0x9] = famicom->video_memory + 0x400 * 1;
        famicom->ppu.banks[0xa] = famicom->video_memory + 0x400 * 0;
        famicom->ppu.banks[0xb] = famicom->video_memory + 0x400 * 1;
        break;
    case SFC_NT_MIR_Horizontal:
        famicom->ppu.banks[0x8] = famicom->video_memory + 0x400 * 0;
        famicom->ppu.banks[0x9] = famicom->video_memory + 0x400 * 0;
        famicom->ppu.banks[0xa] = famicom->video_memory + 0x400 * 1;
        famicom->ppu.banks[0xb] = famicom->video_memory + 0x400 * 1;
        break;
    case SFC_NT_MIR_FourScreen:
        famicom->ppu.banks[0x8] = famicom->video_memory + 0x400 * 0;
        famicom->ppu.banks[0x9] = famicom->video_memory + 0x400 * 1;
        famicom->ppu.banks[0xa] = famicom->video_memory_ex + 0x400 * 0;
        famicom->ppu.banks[0xb] = famicom->video_memory_ex + 0x400 * 1;
        break;
    default:
        assert(!"BAD ACTION");
    }
    // 镜像
    famicom->ppu.banks[0xc] = famicom->ppu.banks[0x8];
    famicom->ppu.banks[0xd] = famicom->ppu.banks[0x9];
    famicom->ppu.banks[0xe] = famicom->ppu.banks[0xa];
    famicom->ppu.banks[0xf] = famicom->ppu.banks[0xb];
}



/// <summary>
/// StepFC: 设置名称表用仓库
/// </summary>
/// <param name="famicom">The famicom.</param>
static inline void sfc_setup_nametable_bank(sfc_famicom_t* famicom) {
    sfc_switch_nametable_mirroring(famicom,
        famicom->rom_info.four_screen ? (SFC_NT_MIR_FourScreen) :
        (famicom->rom_info.vmirroring ? SFC_NT_MIR_Vertical : SFC_NT_MIR_Horizontal)
    );
}

/// <summary>
/// StepFC: 重置famicom
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
sfc_ecode sfc_famicom_reset(sfc_famicom_t* famicom) {
    // 清空PPU数据
    memset(&famicom->ppu, 0, sizeof(famicom->ppu));
    // 重置mapper
    sfc_ecode ecode = famicom->mapper.reset(famicom);
    if (ecode) return ecode;
    // 初始化寄存器
    const uint8_t pcl = sfc_read_cpu_address(SFC_VERCTOR_RESET + 0, famicom);
    const uint8_t pch = sfc_read_cpu_address(SFC_VERCTOR_RESET + 1, famicom);
    famicom->registers.program_counter = (uint16_t)pcl | (uint16_t)pch << 8;
    famicom->registers.accumulator = 0;
    famicom->registers.x_index = 0;
    famicom->registers.y_index = 0;
    famicom->registers.stack_pointer = 0xfd;
    famicom->registers.status = 0x34
        | SFC_FLAG_R    //  一直为1
        ;
    // 调色板
    // 名称表
    sfc_setup_nametable_bank(famicom);
    // 重置APU
    sfc_apu_on_reset(&famicom->apu);
    return SFC_ERROR_OK;
}


// CRC32-b HASH
extern uint32_t sfc_crc32b(uint32_t input, const void *buf, size_t bufLen);

/// <summary>
/// StepFC: 加载ROM
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
sfc_ecode sfc_load_new_rom(sfc_famicom_t* famicom) {
    // 先释放旧的ROM
    sfc_ecode code = famicom->interfaces.free_rom(
        famicom->argument,
        &famicom->rom_info
    );
    // 清空数据
    memset(&famicom->rom_info, 0, sizeof(famicom->rom_info));
    // 载入ROM
    if (code == SFC_ERROR_OK) {
        code = famicom->interfaces.load_rom(
            famicom->argument,
            &famicom->rom_info
        );
    }
    // 载入ROM后处理
    if (code == SFC_ERROR_OK) {
        // 计算HASH
        sfc_rom_info_t* const info = &famicom->rom_info;
        info->prgrom_crc32b = sfc_crc32b(0, info->data_prgrom, info->count_prgrom16kb * 16 * 1024);
        info->chrrom_crc32b = sfc_crc32b(0, info->data_chrrom, info->count_chrrom_8kb * 8 * 1024);
        // 载入SRAM
        sfc_check_load_sram(famicom);
    }
    // 载入新的Mapper
    if (code == SFC_ERROR_OK) {
        code = sfc_load_mapper(
            famicom,
            famicom->rom_info.mapper_number
        );
    }
    // 首次重置
    if (code == SFC_ERROR_OK) {
        code = sfc_famicom_reset(famicom);
    }
    return code;
}