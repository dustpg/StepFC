#pragma once
// License: MIT  http://opensource.org/licenses/MIT
// Author: dustpg   mailto:dustpg@gmail.com

#include <stdint.h>

/// <summary>
/// ROM 信息
/// </summary>
typedef struct {
    // PRG-ROM 程序只读储存器 数据指针
    uint8_t*    data_prgrom;
    // CHR-ROM 角色只读存储器 数据指针
    uint8_t*    data_chrrom;
    // 16KB为单位 程序只读储存器 数据长度
    uint32_t    count_prgrom16kb;
    //  8KB为单位 角色只读存储器 数据长度
    uint32_t    count_chrrom_8kb;
    // Mapper 编号
    uint8_t     mapper_number;
    // 是否Vertical Mirroring(否即为水平)
    uint8_t     vmirroring;
    // 是否FourScreen
    uint8_t     four_screen;
    // 是否有SRAM(电池供电的)
    uint8_t     save_ram;
    // 保留以对齐
    uint8_t     reserved[4];

} sfc_rom_info_t;

/// <summary>
/// NES 文件头
/// </summary>
typedef struct {
    // NES^Z
    uint32_t    id;
    // 16k 程序只读储存器 数量
    uint8_t     count_prgrom16kb;
    //  8k 角色只读存储器 数量
    uint8_t     count_chrrom_8kb;
    // 控制信息1
    uint8_t     control1;
    // 控制信息2
    uint8_t     control2;
    // MAPPER变种
    uint8_t     mapper_variant;
    // 高位ROM大小
    uint8_t     upper_rom_size;
    // RAM大小
    uint8_t     ram_size;
    // 保留数据
    uint8_t     reserved[5];

} sfc_nes_header_t;

/// <summary>
/// ROM control 字节 #1
/// </summary>
enum {
    SFC_NES_VMIRROR = 0x01, 
    SFC_NES_SAVERAM = 0x02,
    SFC_NES_TRAINER = 0x04,
    SFC_NES_4SCREEN = 0x08
};

// ROM control byte #2
enum { 
    SFC_NES_VS_UNISYSTEM  = 0x01,
    SFC_NES_Playchoice10 = 0x02
};