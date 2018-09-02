#include "sfc_famicom.h"
#include <assert.h>
#include <string.h>

// 加载默认ROM
static sfc_ecode sfc_load_default_rom(void*, sfc_rom_info_t*);
// 释放默认ROM
static sfc_ecode sfc_free_default_rom(void*, sfc_rom_info_t*);
// 释放默认ROM
static void sfc_before_execute(void*, sfc_famicom_t*);
// 加载新的ROM
static sfc_ecode sfc_load_new_rom(sfc_famicom_t* famicom);
// 加载mapper
extern sfc_ecode sfc_load_mapper(sfc_famicom_t* famicom, uint8_t);


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
    famicom->interfaces.load_rom = sfc_load_default_rom;
    famicom->interfaces.free_rom = sfc_free_default_rom;
    famicom->interfaces.before_execute = sfc_before_execute;
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
/// StepFC: 反初始化famicom
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_famicom_uninit(sfc_famicom_t* famicom) {
    // 释放ROM
    famicom->interfaces.free_rom(famicom->argument, &famicom->rom_info);
}


/// <summary>
/// StepFC: 设置名词表用仓库
/// </summary>
/// <param name="famicom">The famicom.</param>
static inline void sfc_setup_nametable_bank(sfc_famicom_t* famicom) {
    // 4屏
    if (famicom->rom_info.four_screen) {
        famicom->ppu.banks[0x8] = famicom->video_memory + 0x400 * 0;
        famicom->ppu.banks[0x9] = famicom->video_memory + 0x400 * 1;
        famicom->ppu.banks[0xa] = famicom->video_memory_ex + 0x400 * 0;
        famicom->ppu.banks[0xb] = famicom->video_memory_ex + 0x400 * 1;
    }
    // 横版
    else if (famicom->rom_info.vmirroring) {
        famicom->ppu.banks[0x8] = famicom->video_memory + 0x400 * 0;
        famicom->ppu.banks[0x9] = famicom->video_memory + 0x400 * 1;
        famicom->ppu.banks[0xa] = famicom->video_memory + 0x400 * 0;
        famicom->ppu.banks[0xb] = famicom->video_memory + 0x400 * 1;
    }
    // 纵版
    else {
        famicom->ppu.banks[0x8] = famicom->video_memory + 0x400 * 0;
        famicom->ppu.banks[0x9] = famicom->video_memory + 0x400 * 0;
        famicom->ppu.banks[0xa] = famicom->video_memory + 0x400 * 1;
        famicom->ppu.banks[0xb] = famicom->video_memory + 0x400 * 1;
    }
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
    famicom->registers.stack_pointer = 0xff;
    famicom->registers.status = 0
        | SFC_FLAG_R    //  一直为1
        ;
    // 调色板
    // 名称表
    sfc_setup_nametable_bank(famicom);
    // 镜像
    famicom->ppu.banks[0xc] = famicom->ppu.banks[0x8];
    famicom->ppu.banks[0xd] = famicom->ppu.banks[0x9];
    famicom->ppu.banks[0xe] = famicom->ppu.banks[0xa];
    famicom->ppu.banks[0xf] = famicom->ppu.banks[0xb];

    return SFC_ERROR_OK;
}


#include <stdio.h>
#include <stdlib.h>

/// <summary>
/// 加载默认测试ROM
/// </summary>
/// <param name="arg">The argument.</param>
/// <param name="info">The information.</param>
/// <returns></returns>
sfc_ecode sfc_load_default_rom(void* arg, sfc_rom_info_t* info) {
    assert(info->data_prgrom == NULL && "FREE FIRST");
    FILE* const file = fopen("square.nes", "rb");
    //FILE* const file = fopen("sweep_sub.nes", "rb");

    // 文本未找到
    if (!file) return SFC_ERROR_FILE_NOT_FOUND;
    sfc_ecode code = SFC_ERROR_ILLEGAL_FILE;
    // 读取文件头
    sfc_nes_header_t nes_header;
    if (fread(&nes_header, sizeof(nes_header), 1, file)) {
        // 开头4字节
        union { uint32_t u32; uint8_t id[4]; } this_union;
        this_union.id[0] = 'N';
        this_union.id[1] = 'E';
        this_union.id[2] = 'S';
        this_union.id[3] = '\x1A';
        // 比较这四字节
        if (this_union.u32 == nes_header.id) {
            const size_t size1 = 16 * 1024 * nes_header.count_prgrom16kb;
            // 允许没有CHR-ROM(使用CHR-RAM代替)
            const size_t size2 = 8 * 1024 * (nes_header.count_chrrom_8kb | 1);
            uint8_t* const ptr = (uint8_t*)malloc(size1 + size2);
            // 内存申请成功
            if (ptr) {
                code = SFC_ERROR_OK;
                // TODO: 实现Trainer
                // 跳过Trainer数据
                if (nes_header.control1 & SFC_NES_TRAINER) fseek(file, 512, SEEK_CUR);
                // 这都错了就不管我的事情了
                fread(ptr, size1 + size2, 1, file);

                // 填写info数据表格
                info->data_prgrom = ptr;
                info->data_chrrom = ptr + size1;
                info->count_prgrom16kb = nes_header.count_prgrom16kb;
                info->count_chrrom_8kb = nes_header.count_chrrom_8kb;
                info->mapper_number 
                    = (nes_header.control1 >> 4) 
                    | (nes_header.control2 & 0xF0)
                    ;
                info->vmirroring    = (nes_header.control1 & SFC_NES_VMIRROR) > 0;
                info->four_screen   = (nes_header.control1 & SFC_NES_4SCREEN) > 0;
                info->save_ram      = (nes_header.control1 & SFC_NES_SAVERAM) > 0;
                assert(!(nes_header.control1 & SFC_NES_TRAINER) && "unsupported");
                assert(!(nes_header.control2 & SFC_NES_VS_UNISYSTEM) && "unsupported");
                assert(!(nes_header.control2 & SFC_NES_Playchoice10) && "unsupported");
            }
            // 内存不足
            else code = SFC_ERROR_OUT_OF_MEMORY;
        }
        // 非法文件
    }
    fclose(file);
    return code;
}

/// <summary>
/// 释放默认测试ROM
/// </summary>
/// <param name="arg">The argument.</param>
/// <param name="info">The information.</param>
/// <returns></returns>
sfc_ecode sfc_free_default_rom(void* arg, sfc_rom_info_t* info) {
    // 释放动态申请的数据
    free(info->data_prgrom);
    info->data_prgrom = NULL;

    return SFC_ERROR_OK;
}


/// <summary>
/// 默认执行前的行为
/// </summary>
/// <param name="">The .</param>
/// <param name="">The .</param>
/// <returns></returns>
void sfc_before_execute(void* arg, sfc_famicom_t* info) {

}


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
    memset(&famicom->rom_info, 0, sizeof(famicom));
    // 载入ROM
    if (code == SFC_ERROR_OK) {
        code = famicom->interfaces.load_rom(
            famicom->argument,
            &famicom->rom_info
        );
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