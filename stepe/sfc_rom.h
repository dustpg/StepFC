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
    // PRG-ROM CRC32-b
    uint32_t    prgrom_crc32b;
    // CHR-ROM CRC32-b
    uint32_t    chrrom_crc32b;
    // 程序只读储存器 数据长度
    uint32_t    size_prgrom;
    // 角色只读存储器 数据长度
    uint32_t    size_chrrom;
    // Mapper 编号
    uint8_t     mapper_number;
    // 是否Vertical Mirroring(否即为水平)
    uint8_t     vmirroring;
    // 是否FourScreen
    uint8_t     four_screen;
    // 是否有SRAM(电池供电的)
    uint8_t     save_ram;
    // 保留以对齐
    uint8_t     reserved;
    // PAL/NTSC 位
    uint8_t     pal_ntsc_bits;
    // 扩展音频 位
    uint8_t     extra_sound;
    // 曲子数量(>0表示NSF)
    uint8_t     song_count;
    // 加载地址
    uint16_t    load_addr;
    // 初始化地址
    uint16_t    init_addr;
    // 播放地址
    uint16_t    play_addr;
    // Bankswitch 初始值
    uint8_t     bankswitch_init[8];
    // 歌曲名称
    char        name[32];
    // 作者名称
    char        artist[32];
    // 版权
    char        copyright[32];

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




/// <summary>
/// NSF 文件头
/// </summary>
typedef struct {
    // NESM
    union {
        // NESM
        char        nesm[4];
        // NESM
        uint32_t    nesm_u32;
    };
    // 1A
    uint8_t     u8_1a;
    // 版本号
    uint8_t     ver;
    // 歌曲数量
    uint8_t     count;
    // 起始数
    uint8_t     start;
    // 加载地址[小端]
    uint16_t    load_addr_le;
    // 初始化地址[小端]
    uint16_t    init_addr_le;
    // 播放地址[小端]
    uint16_t    play_addr_le;
    // 歌曲名称
    char        name[32];
    // 作者名称
    char        artist[32];
    // 版权
    char        copyright[32];
    // 播放速度 NTSC[小端]
    uint16_t    play_speed_ntsc_le;
    // Bankswitch 初始值
    uint8_t     bankswitch_init[8];
    // 播放速度 PAL[小端]
    uint16_t    play_speed__pal_le;
    // PAL/NTSC 位
    uint8_t     pal_ntsc_bits;
    // 扩展音频 位
    uint8_t     extra_sound;
    // 扩展位
    uint8_t     expansion[4];

} sfc_nsf_header_t;

// 交换大小端
void sfc_nsf_swap_endian(sfc_nsf_header_t* header);

/// <summary>
/// NSF 文件头用标志位
/// </summary>
enum NSF_HEADER_FLAGS {
    /*
        bit 0: if clear, this is an NTSC tune
        bit 0: if set, this is a PAL tune
        bit 1: if set, this is a dual PAL/NTSC tune
    */
    SFC_NSF_PNB_IsPAL = 1 << 0,
    SFC_NSF_PNB_Dual = 1 << 1,
    /*
        bit 0: if set, this song uses VRC6 audio
        bit 1: if set, this song uses VRC7 audio
        bit 2: if set, this song uses FDS audio
        bit 3: if set, this song uses MMC5 audio
        bit 4: if set, this song uses Namco 163 audio
        bit 5: if set, this song uses Sunsoft 5B audio
    */
    SFC_NSF_EX_VCR6 = 1 << 0,
    SFC_NSF_EX_VCR7 = 1 << 1,
    SFC_NSF_EX_FDS1 = 1 << 2,
    SFC_NSF_EX_MMC5 = 1 << 3,
    SFC_NSF_EX_N163 = 1 << 4,
    SFC_NSF_EX_FME7 = 1 << 5,
};
