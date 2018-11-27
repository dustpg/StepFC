#include "sfc_famicom.h"
#include <assert.h>
#include <string.h>

#define sfc_fallthrough
// 微软编译器目前不支持C11的关键字_Alignas
#ifdef _MSC_VER
#define SFC_ALIGNAS(a) __declspec(align(a))
#else
#define SFC_ALIGNAS(a) _Alignas(a)
#endif

//#define SFC_NO_SSE

#ifndef NDEBUG
static uint32_t s_dbg_framecounter = 0;
#endif

// NMI - 不可屏蔽中断
extern inline void sfc_operation_NMI(sfc_famicom_t* famicom);

// swap byte
static inline void sfc_swap_byte(uint8_t* a, uint8_t* b) {
    const uint8_t temp = *a; *a = *b; *b = temp;
}

#ifdef SFC_NO_SSE
/// <summary>
/// SFCs the pack bool8 into byte.
/// </summary>
/// <param name="array">The array.</param>
/// <returns></returns>
static inline uint8_t sfc_pack_bool8_into_byte(const uint8_t array[8]) {
    uint8_t hittest = 0;
    for (uint16_t i = 0; i != 8; ++i) {
        hittest <<= 1;
        hittest |= array[i] & 1;
    }
    return hittest;
}

/// <summary>
/// SFCs the expand backgorund 8.
/// </summary>
/// <param name="p0">The p0.</param>
/// <param name="p1">The p1.</param>
/// <param name="high">The high.</param>
/// <param name="output">The output.</param>
static inline void sfc_expand_backgorund_8(
    uint8_t p0, 
    uint8_t p1, 
    uint8_t high, 
    uint8_t* output) {
#if 0
FMT = <<-EOF
    // %d - D%d
    {
        const uint8_t low%da = p0 & (uint8_t)0x%02x;
        const uint8_t low%db = p1 & (uint8_t)0x%02x;
        output[%d] = high | low%da >> %d | low%db >> %d | low%da >> %d | low%db >> %d ;
    }
EOF

def op(i)
  print FMT % [
    i, 7 - i,
    i, 1 << (7-i),
    i, 1 << (7-i),
    i, i, 6 - i, i, 5 - i,
    i, 7 - i, i, 7 - i,
  ]
end

8.times { |i| op i }
#endif
    // 0 - D7
    {
        const uint8_t low0a = p0 & (uint8_t)0x80;
        const uint8_t low0b = p1 & (uint8_t)0x80;
        output[0] = high | low0a >> 6 | low0b >> 5 | low0a >> 7 | low0b >> 7;
    }
    // 1 - D6
    {
        const uint8_t low1a = p0 & (uint8_t)0x40;
        const uint8_t low1b = p1 & (uint8_t)0x40;
        output[1] = high | low1a >> 5 | low1b >> 4 | low1a >> 6 | low1b >> 6;
    }
    // 2 - D5
    {
        const uint8_t low2a = p0 & (uint8_t)0x20;
        const uint8_t low2b = p1 & (uint8_t)0x20;
        output[2] = high | low2a >> 4 | low2b >> 3 | low2a >> 5 | low2b >> 5;
    }
    // 3 - D4
    {
        const uint8_t low3a = p0 & (uint8_t)0x10;
        const uint8_t low3b = p1 & (uint8_t)0x10;
        output[3] = high | low3a >> 3 | low3b >> 2 | low3a >> 4 | low3b >> 4;
    }
    // 4 - D3
    {
        const uint8_t low4a = p0 & (uint8_t)0x08;
        const uint8_t low4b = p1 & (uint8_t)0x08;
        output[4] = high | low4a >> 2 | low4b >> 1 | low4a >> 3 | low4b >> 3;
    }
    // 5 - D2
    {
        const uint8_t low5a = p0 & (uint8_t)0x04;
        const uint8_t low5b = p1 & (uint8_t)0x04;
        output[5] = high | low5a >> 1 | low5b >> 0 | low5a >> 2 | low5b >> 2;
    }
    // 6 - D1
    {
        const uint8_t low6a = p0 & (uint8_t)0x02;
        const uint8_t low6b = p1 & (uint8_t)0x02;
        output[6] = high | low6a >> 0 | low6b << 1 | low6a >> 1 | low6b >> 1;
    }
    // 7 - D0
    {
        const uint8_t low7a = p0 & (uint8_t)0x01;
        const uint8_t low7b = p1 & (uint8_t)0x01;
        output[7] = high | low7a << 1 | low7b << 2 | low7a >> 0 | low7b >> 0;
    }
}
#else
#include <tmmintrin.h>
#include <emmintrin.h>
/// <summary>
/// SFCs the pack bool8 into byte.
/// </summary>
/// <param name="array">The array.</param>
/// <returns></returns>
static inline uint8_t sfc_pack_bool8_into_byte(const uint8_t array[8]) {
    // 载入一个允许不对齐的低64(高64位置0)位数据
    __m128i values = _mm_loadl_epi64((__m128i*)array);
    // 创建序列
    __m128i order = _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7);
    // 根据已有序列将数据重组
    values = _mm_shuffle_epi8(values, order);
    // 将每个32(64也行)位整型左移7位(作为每个8位整型的符号位)
    // 然后将8位整型符号位打包成16位整数
    return (uint8_t)_mm_movemask_epi8(_mm_slli_epi32(values, 7));

}

/// <summary>
/// StepFC: 使用4比特查找对应的32位整型
/// <remarks>
/// 使用8比特的话会使用256 * 8 = 2kb空间, 缓存不友好
/// </remarks>
/// </summary>
SFC_ALIGNAS(32) static const uint32_t sfc_u32_bit_lut[16] = {
    0x00000000, // 0000
    0x01000000, // 0001
    0x00010000, // 0010
    0x01010000, // 0011

    0x00000100, // 0100
    0x01000100, // 0101
    0x00010100, // 0110
    0x01010100, // 0111

    0x00000001, // 1000
    0x01000001, // 1001
    0x00010001, // 1010
    0x01010001, // 1011

    0x00000101, // 1100
    0x01000101, // 1101
    0x00010101, // 1110
    0x01010101, // 1111
};

/// <summary>
/// Creates the 128 mask.
/// </summary>
/// <param name="a">a.</param>
/// <param name="b">The b.</param>
/// <returns></returns>
static inline __m128i sfc_create_128_mask(uint8_t a, uint8_t b) {
    return _mm_set_epi32(
        sfc_u32_bit_lut[a & 0xF],
        sfc_u32_bit_lut[a >> 4],
        sfc_u32_bit_lut[b & 0xF],
        sfc_u32_bit_lut[b >> 4]
        );
}

/// <summary>
/// SFCs the expand backgorund 16.
/// </summary>
/// <param name="p0">The p0.</param>
/// <param name="p1">The p1.</param>
/// <param name="high">The high.</param>
/// <param name="output">The output.</param>
static inline void sfc_expand_backgorund_16(
    uint8_t p0,
    uint8_t p1,
    uint8_t p2,
    uint8_t p3,
    uint8_t high,
    uint8_t* output) {
    const __m128i value1 = sfc_create_128_mask(p2, p0);
    const __m128i value2 = sfc_create_128_mask(p3, p1);

    __m128i value = _mm_or_si128(value1, value2);
    value = _mm_or_si128(value, _mm_slli_epi32(value1, 1));
    value = _mm_or_si128(value, _mm_slli_epi32(value2, 2));
    value = _mm_or_si128(value, _mm_set1_epi8(high));

    (*(__m128i*)output) = value;
}

/// <summary>
/// SFCs the expand backgorund 16.
/// </summary>
/// <param name="p0">The p0.</param>
/// <param name="p1">The p1.</param>
/// <param name="high">The high.</param>
/// <param name="output">The output.</param>
static inline void sfc_expand_backgorund_16_ex(
    uint8_t p0,
    uint8_t p1,
    uint8_t p2,
    uint8_t p3,
    uint8_t high0,
    uint8_t high1,
    uint8_t* output) {
    const __m128i value1 = sfc_create_128_mask(p2, p0);
    const __m128i value2 = sfc_create_128_mask(p3, p1);

    __m128i value = _mm_or_si128(value1, value2);
    value = _mm_or_si128(value, _mm_slli_epi32(value1, 1));
    value = _mm_or_si128(value, _mm_slli_epi32(value2, 2));
    value = _mm_or_si128(value, _mm_set_epi8(
        high1, high1, high1, high1, high1, high1, high1, high1,
        high0, high0, high0, high0, high0, high0, high0, high0
        ));
    (*(__m128i*)output) = value;
}

#endif

/// <summary>
/// The bit reverse table256
/// </summary>
static const unsigned char BitReverseTable256[] = {
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};


/// <summary>
/// StepFC: 简易模式渲染背景 - 以16像素为单位
/// </summary>
/// <param name="high">The high.</param>
/// <param name="plane_left">The plane left.</param>
/// <param name="plane_right">The plane right.</param>
/// <param name="aligned_palette">The aligned palette.</param>
static inline void sfc_render_background_pixel16(
    uint8_t high,
    const uint8_t* plane_left,
    const uint8_t* plane_right,
    uint8_t* aligned_palette) {
    const uint8_t plane0 = plane_left[0];
    const uint8_t plane1 = plane_left[8];
    const uint8_t plane2 = plane_right[0];
    const uint8_t plane3 = plane_right[8];
#ifdef SFC_NO_SSE
    sfc_expand_backgorund_8(plane0, plane1, high, aligned_palette + 0);
    sfc_expand_backgorund_8(plane2, plane3, high, aligned_palette + 8);
#else
    sfc_expand_backgorund_16(plane0, plane1, plane2, plane3, high, aligned_palette);
#endif
}


/// <summary>
/// StepFC: 简易模式渲染背景 - ExGrafix模式 - 以16像素为单位
/// </summary>
/// <param name="high0">The high0.</param>
/// <param name="high1">The high1.</param>
/// <param name="plane_left">The plane left.</param>
/// <param name="plane_right">The plane right.</param>
/// <param name="aligned_palette">The aligned palette.</param>
static inline void sfc_render_background_pixel16_ex(
    uint8_t high0,
    uint8_t high1,
    const uint8_t* plane_left,
    const uint8_t* plane_right,
    uint8_t* aligned_palette) {
    const uint8_t plane0 = plane_left[0];
    const uint8_t plane1 = plane_left[8];
    const uint8_t plane2 = plane_right[0];
    const uint8_t plane3 = plane_right[8];
#ifdef SFC_NO_SSE
    //static uint32_t fc = 0;

    //static uint8_t data = 0;
    //static int count = 0; ++count;
    //if (count == 13) {
    //    ++data;
    //    count = 0;
    //}
    ////high1 = (data & 3) << 3;

    //if (high0 && high1 && high0 != high1) {
    //    if (fc != s_dbg_framecounter) {
    //        fc = s_dbg_framecounter;
    //        printf("[%04d]MMC5-ExGrafix!\n", fc);
    //    }
    //}

    sfc_expand_backgorund_8(plane0, plane1, high0, aligned_palette + 0);
    sfc_expand_backgorund_8(plane2, plane3, high1, aligned_palette + 8);
#else
    //if (high0 && high1 && high0 != high1) {
    //    printf("MMC5-ExGrafix!\n");
    //}
    sfc_expand_backgorund_16_ex(plane0, plane1, plane2, plane3, high0, high1, aligned_palette);
#endif
}

// MMC5 直接开刀
extern inline uint8_t* sfc_mmc5_exram(sfc_famicom_t*);
extern inline uint8_t sfc_mmc5_get_5130_hi(sfc_famicom_t* famicom);

/// <summary>
/// SFCs the render background scanline.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="line">The line.</param>
/// <param name="spp">The SPP.</param>
/// <param name="buffer">The buffer.</param>
static void sfc_render_background_scanline(
    sfc_famicom_t* famicom,
    uint16_t line,
    const uint8_t sp0[SFC_HEIGHT + (16)],
    uint8_t* buffer) {
    // 取消背景显示
    if (!(famicom->ppu.data.mask & (uint8_t)SFC_PPU2001_Back)) return;

    // 计算当前水平偏移量
    const uint16_t scrollx
        = (uint16_t)((famicom->ppu.data.v & (uint16_t)0x0400) ? 256 : 0)
        + (uint16_t)((famicom->ppu.data.v & (uint16_t)0x1f) << 3)
        + (uint16_t)famicom->ppu.data.x
        ;
    // 计算当前垂直偏移量
    const uint16_t scrolly
        = (uint16_t)((famicom->ppu.data.v & (uint16_t)0x0800) ? 240 : 0)
        + (uint16_t)(((famicom->ppu.data.v >> 5) & (uint16_t)0x1f) << 3)
        + (uint16_t)(famicom->ppu.data.v >> 12)
        ;
    //const uint16_t scrolly = line + 240;
    // 由于Y是240一换, 需要膜计算
    const uint16_t scrolly_index0 = scrolly / (uint16_t)240;
    const uint16_t scrolly_offset = scrolly % (uint16_t)240;
    // 检测垂直偏移量确定使用图案表的前一半[8-9]还是后一半[10-11]
    const uint8_t* table[2];

    const int first_buck = 8 + ((scrolly_index0 & 1) << 1);
    table[0] = famicom->ppu.banks[first_buck];
    table[1] = famicom->ppu.banks[first_buck+1];
    // 以16像素为单位扫描该行
    SFC_ALIGNAS(16) uint8_t aligned_buffer[SFC_WIDTH + 16 + 16];
    // 正常模式
    if (famicom->ppu.data.ppu_mode == SFC_EZPPU_Normal) {
        // 计算背景所使用的图样表(拜1KiB模式所赐)
        const uint8_t** const ppattern = &famicom->ppu.banks[famicom->ppu.data.ctrl & SFC_PPU2000_BgTabl ? 4 : 0];
        const uint8_t realy = (uint8_t)scrolly_offset;
        // 保险起见扫描16+1次
        for (uint16_t i = 0; i != 17; ++i) {
            const uint16_t realx = scrollx + (i << 4);
            const uint8_t* nt = table[(realx >> 8) & 1];
            const uint8_t xunit = (realx & (uint16_t)0xF0) >> 4;
            // 获取32为单位的调色板索引字节
            const uint8_t attr = (nt + 32 * 30)[(realy >> 5 << 3) | (xunit >> 1)];
            // 获取属性表内位偏移
            const uint8_t aoffset = ((uint8_t)(xunit & 1) << 1) | ((realy & 0x10) >> 2);
            // 计算高两位
            const uint8_t high = (attr & (3 << aoffset)) >> aoffset << 3;
            // 计算图样表位置
            const uint8_t* too_young = nt + ((uint16_t)xunit << 1) + (uint16_t)(realy >> 3 << 5);
            const uint8_t too_young0 = too_young[0];
            const uint8_t* const pattern0 = ppattern[too_young0 >> 6];
            const uint8_t too_young1 = too_young[1];
            const uint8_t* const pattern1 = ppattern[too_young1 >> 6];
            // 渲染16个像素
            sfc_render_background_pixel16(
                high,
                pattern0 + (too_young0 & 0x3F) * 16 + (realy & 7),
                pattern1 + (too_young1 & 0x3F) * 16 + (realy & 7),
                aligned_buffer + (i << 4)
            );
        }
    }
    // MMC5-ExGrafix 模式
    else {
        const uint8_t* const exram = sfc_mmc5_exram(famicom);
        const uint8_t realy = (uint8_t)scrolly_offset;
        const uint8_t chrbank_hi = sfc_mmc5_get_5130_hi(famicom);
        const uint8_t* const real_pattern = famicom->rom_info.data_chrrom;
        // 保险起见扫描16+1次
        for (uint16_t i = 0; i != 17; ++i) {
            const uint16_t realx = scrollx + (i << 4);
            //const uint8_t* nt = table[(realx >> 8) & 1];
            const uint8_t xunit = (realx & (uint16_t)0xF0) >> 4;
            // 获取32为单位的调色板索引字节
            //const uint8_t attr = (nt + 32 * 30)[(realy >> 5 << 3) | (xunit >> 1)];
            // 获取属性表内位偏移
            //const uint8_t aoffset = ((uint8_t)(xunit & 1) << 1) | ((realy & 0x10) >> 2);
            // 计算高两位
            //const uint8_t high = (attr & (3 << aoffset)) >> aoffset << 3;

            // 计算图样表位置
            const uintptr_t offset = ((uint16_t)xunit << 1) + (uint16_t)(realy >> 3 << 5);
            const uint8_t* too_young = exram + offset;
            // 原始数据
            const uint8_t* nt = table[(realx >> 8) & 1];
            const uint8_t* raw_data = nt + offset;

            const uint8_t too_young0 = too_young[0];
            const uint8_t* const pattern0 = real_pattern + 4 * 1024 * ((too_young0 & 0x3f) | chrbank_hi);
            const uint8_t too_young1 = too_young[1];
            const uint8_t* const pattern1 = real_pattern + 4 * 1024 * ((too_young1 & 0x3f) | chrbank_hi);

            // 渲染16个像素
            sfc_render_background_pixel16_ex(
                (too_young0 & 0xc0) >> 3,
                (too_young1 & 0xc0) >> 3,
                pattern0 + raw_data[0] * 16 + (realy & 7),
                pattern1 + raw_data[1] * 16 + (realy & 7),
                aligned_buffer + (i << 4)
            );
        }
    }
    // 将数据复制过去
    {
        const uint8_t* const unaligned_buffer = aligned_buffer + (scrollx & 0x0f);
        memcpy(buffer, unaligned_buffer, SFC_WIDTH);
    }
    // 基于行的精灵0命中测试

    // 已经命中了
    if (famicom->ppu.data.status & (uint8_t)SFC_PPU2002_Sp0Hit) return;
    // 没有必要测试
    const uint8_t hittest_data = sp0[line];
    if (!hittest_data) return;
    // 精灵#0的数据
    uint8_t* const unaligned_buffer = aligned_buffer + (scrollx & 0x0f);
    memset(unaligned_buffer + SFC_WIDTH, 0, 16);

    const uint8_t  xxxxx = famicom->ppu.data.sprites[3];
    const uint8_t hittest = sfc_pack_bool8_into_byte(unaligned_buffer + xxxxx);
    if (hittest_data & hittest)
        famicom->ppu.data.status |= (uint8_t)SFC_PPU2002_Sp0Hit;
}

/// <summary>
/// SFCs the sprite0 hittest.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="spp">The SPP.</param>
/// <param name="buffer">The buffer.</param>
static inline void sfc_sprite0_hittest(
    sfc_famicom_t* famicom, 
    uint8_t buffer[SFC_WIDTH]) {
    // 先清空
    memset(buffer, 0, SFC_WIDTH);
    // 关闭渲染
    enum { BOTH_BS = SFC_PPU2001_Back | SFC_PPU2001_Sprite };
    if ((famicom->ppu.data.mask & (uint8_t)BOTH_BS) != (uint8_t)BOTH_BS) return;
    // 获取数据以填充
    const uint8_t yyyyy = famicom->ppu.data.sprites[0];
    // 0xef以上就算了
    if (yyyyy >= 0xEF) return;
    // 计算使用的图样板子
    const uint8_t iiiii = famicom->ppu.data.sprites[1];
    const uint8_t sp8x16 = famicom->ppu.data.ctrl & SFC_PPU2000_Sp8x16;
    const uint8_t* const spp = famicom->ppu.banks[sp8x16 ?
        // 偶数使用0000 奇数1000
        (iiiii & 1 ? 4 : 0) :
        // 检查SFC_PPU2000_SpTabl
        (famicom->ppu.data.ctrl & SFC_PPU2000_SpTabl ? 4 : 0)
    ];
    // 属性
    const uint8_t aaaaa = famicom->ppu.data.sprites[2];
    // 获取平面数据
    const uint8_t* const nowp0 = spp + iiiii * 16;
    const uint8_t* const nowp1 = nowp0 + 8;
    uint8_t* const buffer_write = buffer + yyyyy + 1;
    // 8x16的情况
    const int count = sp8x16 ? 16 : 8;

    // 水平翻转
    if (aaaaa & (uint8_t)SFC_SPATTR_FlipH) 
        for (int i = 0; i != count; ++i) {
            const uint8_t data = nowp0[i] | nowp1[i];
            buffer_write[i] = BitReverseTable256[data];
        }
    // 普通
    else 
        for (int i = 0; i != count; ++i) {
            const uint8_t data = nowp0[i] | nowp1[i];
            buffer_write[i] = data;
        }
    // 垂直翻转
    if (aaaaa & (uint8_t)SFC_SPATTR_FlipV) {
        // 8x16
        if (sp8x16) {
            sfc_swap_byte(buffer_write + 0, buffer_write + 0xF);
            sfc_swap_byte(buffer_write + 1, buffer_write + 0xE);
            sfc_swap_byte(buffer_write + 2, buffer_write + 0xD);
            sfc_swap_byte(buffer_write + 3, buffer_write + 0xC);
            sfc_swap_byte(buffer_write + 4, buffer_write + 0xB);
            sfc_swap_byte(buffer_write + 5, buffer_write + 0xA);
            sfc_swap_byte(buffer_write + 6, buffer_write + 0x9);
            sfc_swap_byte(buffer_write + 7, buffer_write + 0x8);
        }
        else {
            sfc_swap_byte(buffer_write + 0, buffer_write + 7);
            sfc_swap_byte(buffer_write + 1, buffer_write + 6);
            sfc_swap_byte(buffer_write + 2, buffer_write + 5);
            sfc_swap_byte(buffer_write + 3, buffer_write + 4);
        }
    }
}

/// <summary>
/// SFCs the sprite overflow test.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint16_t sfc_sprite_overflow_test(sfc_famicom_t* famicom) {
    // 完全关闭渲染
    enum { BOTH_BS = SFC_PPU2001_Back | SFC_PPU2001_Sprite };
    if (!(famicom->ppu.data.mask & (uint8_t)BOTH_BS)) return SFC_HEIGHT;
    // 正式处理
    uint8_t buffer[256 + 16];
    memset(buffer, 0, 256);
    // 8 x 16
    const int height = famicom->ppu.data.ctrl & SFC_PPU2000_Sp8x16 ? 16 : 8;
    for (int i = 0; i != SFC_SPRITE_COUNT; ++i) {
        const uint8_t y = famicom->ppu.data.sprites[i * 4];
        for (int i = 0; i != height; ++i) buffer[y + i]++;
    }
    // 搜索第一个超过8的
    uint16_t line ;
    for (line = 0; line != SFC_HEIGHT; ++line) if (buffer[line] > 8) break;
    return line;
}


/// <summary>
/// SFCs the sprite expand 8.
/// </summary>
/// <param name="p0">The p0.</param>
/// <param name="p1">The p1.</param>
/// <param name="high">The high.</param>
/// <param name="output">The output.</param>
static inline void sfc_sprite_expand_8_on(uint8_t p0, uint8_t p1, uint8_t high, uint8_t* output) {
    // 0 - D7
    const uint8_t low0 = ((p0 & (uint8_t)0x80) >> 6) | ((p1 & (uint8_t)0x80) >> 5);
    if (low0) output[0] = high | low0;
    // 1 - D6
    const uint8_t low1 = ((p0 & (uint8_t)0x40) >> 5) | ((p1 & (uint8_t)0x40) >> 4);
    if (low1) output[1] = high | low1;
    // 2 - D5
    const uint8_t low2 = ((p0 & (uint8_t)0x20) >> 4) | ((p1 & (uint8_t)0x20) >> 3);
    if (low2) output[2] = high | low2;
    // 3 - D4
    const uint8_t low3 = ((p0 & (uint8_t)0x10) >> 3) | ((p1 & (uint8_t)0x10) >> 2);
    if (low3) output[3] = high | low3;
    // 4 - D3
    const uint8_t low4 = ((p0 & (uint8_t)0x08) >> 2) | ((p1 & (uint8_t)0x08) >> 1);
    if (low4) output[4] = high | low4;
    // 5 - D2
    const uint8_t low5 = ((p0 & (uint8_t)0x04) >> 1) | ((p1 & (uint8_t)0x04) >> 0);
    if (low5) output[5] = high | low5;
    // 6 - D1
    const uint8_t low6 = ((p0 & (uint8_t)0x02) >> 0) | ((p1 & (uint8_t)0x02) << 1);
    if (low6) output[6] = high | low6;
    // 7 - D0
    const uint8_t low7 = ((p0 & (uint8_t)0x01) << 1) | ((p1 & (uint8_t)0x01) << 2);
    if (low7) output[7] = high | low7;
}


/// <summary>
/// SFCs the sprite expand 8.
/// </summary>
/// <param name="p0">The p0.</param>
/// <param name="p1">The p1.</param>
/// <param name="high">The high.</param>
/// <param name="output">The output.</param>
static inline void sfc_sprite_expand_8_op(uint8_t p0, uint8_t p1, uint8_t high, uint8_t* output) {
    // 0 - D7
    const uint8_t low0 = ((p0 & (uint8_t)0x80) >> 6) | ((p1 & (uint8_t)0x80) >> 5);
    if ((~output[0] & 1) && low0) output[0] = high | low0 | 1;
    // 1 - D6
    const uint8_t low1 = ((p0 & (uint8_t)0x40) >> 5) | ((p1 & (uint8_t)0x40) >> 4);
    if ((~output[1] & 1) && low1) output[1] = high | low1 | 1;
    // 2 - D5
    const uint8_t low2 = ((p0 & (uint8_t)0x20) >> 4) | ((p1 & (uint8_t)0x20) >> 3);
    if ((~output[2] & 1) && low2) output[2] = high | low2 | 1;
    // 3 - D4
    const uint8_t low3 = ((p0 & (uint8_t)0x10) >> 3) | ((p1 & (uint8_t)0x10) >> 2);
    if ((~output[3] & 1) && low3) output[3] = high | low3 | 1;
    // 4 - D3
    const uint8_t low4 = ((p0 & (uint8_t)0x08) >> 2) | ((p1 & (uint8_t)0x08) >> 1);
    if ((~output[4] & 1) && low4) output[4] = high | low4 | 1;
    // 5 - D2
    const uint8_t low5 = ((p0 & (uint8_t)0x04) >> 1) | ((p1 & (uint8_t)0x04) >> 0);
    if ((~output[5] & 1) && low5) output[5] = high | low5 | 1;
    // 6 - D1
    const uint8_t low6 = ((p0 & (uint8_t)0x02) >> 0) | ((p1 & (uint8_t)0x02) << 1);
    if ((~output[6] & 1) && low6) output[6] = high | low6 | 1;
    // 7 - D0
    const uint8_t low7 = ((p0 & (uint8_t)0x01) << 1) | ((p1 & (uint8_t)0x01) << 2);
    if ((~output[7] & 1) && low7) output[7] = high | low7 | 1;
}


/// <summary>
/// SFCs the sprite expand 8.
/// </summary>
/// <param name="p0">The p0.</param>
/// <param name="p1">The p1.</param>
/// <param name="high">The high.</param>
/// <param name="output">The output.</param>
static inline void sfc_sprite_expand_8_rn(uint8_t p0, uint8_t p1, uint8_t high, uint8_t* output) {
    // 7 - D7
    const uint8_t low0 = ((p0 & (uint8_t)0x80) >> 6) | ((p1 & (uint8_t)0x80) >> 5);
    if (low0) output[7] = high | low0;
    // 6 - D6
    const uint8_t low1 = ((p0 & (uint8_t)0x40) >> 5) | ((p1 & (uint8_t)0x40) >> 4);
    if (low1) output[6] = high | low1;
    // 5 - D5
    const uint8_t low2 = ((p0 & (uint8_t)0x20) >> 4) | ((p1 & (uint8_t)0x20) >> 3);
    if (low2) output[5] = high | low2;
    // 4 - D4
    const uint8_t low3 = ((p0 & (uint8_t)0x10) >> 3) | ((p1 & (uint8_t)0x10) >> 2);
    if (low3) output[4] = high | low3;
    // 3 - D3
    const uint8_t low4 = ((p0 & (uint8_t)0x08) >> 2) | ((p1 & (uint8_t)0x08) >> 1);
    if (low4) output[3] = high | low4;
    // 2 - D2
    const uint8_t low5 = ((p0 & (uint8_t)0x04) >> 1) | ((p1 & (uint8_t)0x04) >> 0);
    if (low5) output[2] = high | low5;
    // 1 - D1
    const uint8_t low6 = ((p0 & (uint8_t)0x02) >> 0) | ((p1 & (uint8_t)0x02) << 1);
    if (low6) output[1] = high | low6;
    // 0 - D0
    const uint8_t low7 = ((p0 & (uint8_t)0x01) << 1) | ((p1 & (uint8_t)0x01) << 2);
    if (low7) output[0] = high | low7;
}


/// <summary>
/// SFCs the sprite expand 8.
/// </summary>
/// <param name="p0">The p0.</param>
/// <param name="p1">The p1.</param>
/// <param name="high">The high.</param>
/// <param name="output">The output.</param>
static inline void sfc_sprite_expand_8_rp(uint8_t p0, uint8_t p1, uint8_t high, uint8_t* output) {
    // 0 - D7
    const uint8_t low0 = ((p0 & (uint8_t)0x80) >> 6) | ((p1 & (uint8_t)0x80) >> 5);
    if ((~output[7-0] & 1) && low0) output[7-0] = high | low0 | 1;
    // 1 - D6
    const uint8_t low1 = ((p0 & (uint8_t)0x40) >> 5) | ((p1 & (uint8_t)0x40) >> 4);
    if ((~output[7-1] & 1) && low1) output[7-1] = high | low1 | 1;
    // 2 - D5
    const uint8_t low2 = ((p0 & (uint8_t)0x20) >> 4) | ((p1 & (uint8_t)0x20) >> 3);
    if ((~output[7-2] & 1) && low2) output[7-2] = high | low2 | 1;
    // 3 - D4
    const uint8_t low3 = ((p0 & (uint8_t)0x10) >> 3) | ((p1 & (uint8_t)0x10) >> 2);
    if ((~output[7-3] & 1) && low3) output[7-3] = high | low3 | 1;
    // 4 - D3
    const uint8_t low4 = ((p0 & (uint8_t)0x08) >> 2) | ((p1 & (uint8_t)0x08) >> 1);
    if ((~output[7-4] & 1) && low4) output[7-4] = high | low4 | 1;
    // 5 - D2
    const uint8_t low5 = ((p0 & (uint8_t)0x04) >> 1) | ((p1 & (uint8_t)0x04) >> 0);
    if ((~output[7-5] & 1) && low5) output[7-5] = high | low5 | 1;
    // 6 - D1
    const uint8_t low6 = ((p0 & (uint8_t)0x02) >> 0) | ((p1 & (uint8_t)0x02) << 1);
    if ((~output[7-6] & 1) && low6) output[7-6] = high | low6 | 1;
    // 7 - D0
    const uint8_t low7 = ((p0 & (uint8_t)0x01) << 1) | ((p1 & (uint8_t)0x01) << 2);
    if ((~output[7-7] & 1) && low7) output[7-7] = high | low7 | 1;
}


enum {
    SFC_BUFFER_WIDTH = SFC_WIDTH + 8,
};

/// <summary>
/// SFCs the render sprites.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="buffer">The buffer.</param>
static inline void sfc_render_sprites(sfc_famicom_t* famicom, uint8_t* buffer) {
    // 8 x 16
    const uint8_t sp8x16 = (famicom->ppu.data.ctrl & (uint8_t)SFC_PPU2000_Sp8x16) >> 2;
    //assert(sp8x16 == 0);
    // 精灵用图样
    const size_t offset1 = (sp8x16) ? 0 : (famicom->ppu.data.ctrl & SFC_PPU2000_SpTabl ? 4 : 0);
    const size_t offset2 = (sp8x16) ? 4 : offset1;
    const size_t offset3 = (sp8x16) ? 0 : 16;

    const uint8_t* banks[8];
    banks[0] = famicom->ppu.banks[offset1 + 0];
    banks[1] = famicom->ppu.banks[offset1 + 1];
    banks[2] = famicom->ppu.banks[offset1 + 2];
    banks[3] = famicom->ppu.banks[offset1 + 3];
    banks[4] = famicom->ppu.banks[offset2 + 0] + offset3;
    banks[5] = famicom->ppu.banks[offset2 + 1] + offset3;
    banks[6] = famicom->ppu.banks[offset2 + 2] + offset3;
    banks[7] = famicom->ppu.banks[offset2 + 3] + offset3;

    //const uint8_t* sppbuffer[2];
    //sppbuffer[0] = famicom->ppu.banks[
    //    (sp8x16) ? 0 : (famicom->ppu.data.ctrl & SFC_PPU2000_SpTabl ? 4 : 0)];
    //sppbuffer[1] = sp8x16 ? famicom->ppu.banks[4] : sppbuffer[0] + 16;

    // 遍历所有精灵
    for (int index = 0; index != SFC_SPRITE_COUNT; ++index) {
        const uint8_t* const base = famicom->ppu.data.sprites +
            (SFC_SPRITE_COUNT - 1 - index) * 4;
        const uint8_t yyyy = base[0];
        // 显示不下
        if (yyyy >= (uint8_t)0xEF) continue;
        const uint8_t iiii = base[1];
        const uint8_t aaaa = base[2];
        const uint8_t xxxx = base[3];
        const uint8_t high = ((aaaa & 3) | 4) << 3;
        // 计算BANK
        const uint8_t* const bank = banks[(iiii >> 6) | (iiii & 1) << 2];
        const uint8_t* const nowp0 = bank + ((int)(iiii & (uint8_t)0x3e)) * 16;
        //const uint8_t* const nowp0 = sppbuffer[iiii & 1] + (iiii & (uint8_t)0xFE) * 16;
        const uint8_t* const nowp1 = nowp0 + 8;

        uint8_t* const write = buffer + xxxx + (yyyy + 1) * SFC_BUFFER_WIDTH;
        // hVHP
        switch (((uint8_t)(aaaa >> 5) | sp8x16) & (uint8_t)0x0f)
        {
        case 0x8:
            // 1000: 8x16 前
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_on(nowp0[j + 16], nowp1[j + 16], high, write + SFC_BUFFER_WIDTH * (j + 8));
            sfc_fallthrough;
        case 0x0:
            // 0000: 前
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_on(nowp0[j], nowp1[j], high, write + SFC_BUFFER_WIDTH * j);
            break;
        case 0x9:
            // 1001: 8x16 后
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_op(nowp0[j + 16], nowp1[j + 16], high, write + SFC_BUFFER_WIDTH * (j + 8));
            sfc_fallthrough;
        case 0x1:
            // 0001: 后
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_op(nowp0[j], nowp1[j], high, write + SFC_BUFFER_WIDTH * j);
            break;
        case 0xA:
            // 1010: 8x16 水平翻转 前 
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_rn(nowp0[j + 16], nowp1[j + 16], high, write + SFC_BUFFER_WIDTH * (j + 8));
            sfc_fallthrough;
        case 0x2:
            // 0010: 水平翻转 前 
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_rn(nowp0[j], nowp1[j], high, write + SFC_BUFFER_WIDTH * j);
            break;
        case 0xB:
            // 1011: 8x16 水平翻转 后
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_rp(nowp0[j + 16], nowp1[j + 16], high, write + SFC_BUFFER_WIDTH * (j + 8));
            sfc_fallthrough;
        case 0x3:
            // 0011: 水平翻转 后
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_rp(nowp0[j], nowp1[j], high, write + SFC_BUFFER_WIDTH * j);
            break;
        case 0xC:
            // 1100: 8x16 垂直翻转 前
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_on(nowp0[j + 16], nowp1[j + 16], high, write + SFC_BUFFER_WIDTH * (7 - j));
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_on(nowp0[j], nowp1[j], high, write + SFC_BUFFER_WIDTH * (15 - j));
            break;
        case 0x4:
            // 0100: 垂直翻转 前 
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_on(nowp0[j], nowp1[j], high, write + SFC_BUFFER_WIDTH * (7 - j));
            break;
        case 0xD:
            // 1101: 8x16 垂直翻转 后
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_op(nowp0[j + 16], nowp1[j + 16], high, write + SFC_BUFFER_WIDTH * (7 - j));
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_op(nowp0[j], nowp1[j], high, write + SFC_BUFFER_WIDTH * (15 - j));
            break;
        case 0x5:
            // 0101: 垂直翻转 后
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_op(nowp0[j], nowp1[j], high, write + SFC_BUFFER_WIDTH * (7 - j));
            break;
        case 0xE: 
            // 1110: 8x16 垂直翻转 水平翻转 前 
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_rn(nowp0[j + 16], nowp1[j + 16], high, write + SFC_BUFFER_WIDTH * (7 - j));
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_rn(nowp0[j], nowp1[j], high, write + SFC_BUFFER_WIDTH * (15 - j));
            break;
        case 0x6:
            // 0110: 8x16 垂直翻转 水平翻转 前 
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_rn(nowp0[j], nowp1[j], high, write + SFC_BUFFER_WIDTH * (7 - j));
            break;
        case 0xF:
            // 1111: 8x16 垂直翻转 水平翻转 后
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_rp(nowp0[j + 16], nowp1[j + 16], high, write + SFC_BUFFER_WIDTH * (7 - j));
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_rp(nowp0[j], nowp1[j], high, write + SFC_BUFFER_WIDTH * (15 - j));
            break;
        case 0x7:
            // 0111: 垂直翻转 水平翻转 后
            for (int j = 0; j != 8; ++j)
                sfc_sprite_expand_8_rp(nowp0[j], nowp1[j], high, write + SFC_BUFFER_WIDTH * (7 - j));
            break;
        }
    }
}
 

// 触发帧计数器
extern void sfc_trigger_frame_counter(sfc_famicom_t*);


/// <summary>
/// StepFC: 使用简易模式渲染一帧, 效率较高
/// </summary>
/// <remarks>
/// buffer: 
/// D0 - 0 为全局背景色 1为非全局背景色, 是背景D1和D2'与'操作的结果
/// D1-D5 - 调色板索引
/// </remarks>
/// <param name="famicom">The famicom.</param>
/// <param name="buffer">The buffer.</param>
void sfc_render_frame_easy(sfc_famicom_t* famicom, uint8_t* buffer) {
    enum { SCAN_LINE_COUNT = SFC_HEIGHT };
    uint8_t* const data = buffer;
    const uint16_t vblank_line = famicom->config.vblank_scanline;
    const uint32_t per_scanline = famicom->config.master_cycle_per_scanline;
    uint32_t end_cycle_count = 0;
    // 帧+1
    ++famicom->frame_counter;
#ifndef NDEBUG
    s_dbg_framecounter = famicom->frame_counter;
#endif
    // 精灵0用命中测试缓存
    uint8_t sp0_hittest_buffer[SFC_WIDTH];
    sfc_sprite0_hittest(famicom, sp0_hittest_buffer);
    // 精灵溢出测试
    const uint16_t sp_overflow_line = sfc_sprite_overflow_test(famicom);
    // 关闭渲染则输出背景色?
    if (!(famicom->ppu.data.mask & (uint8_t)SFC_PPU2001_Back))
        memset(buffer, 0, SFC_BUFFER_WIDTH * SFC_HEIGHT);
    // 渲染
    for (uint16_t i = 0; i != (uint16_t)SCAN_LINE_COUNT; ++i) {
        end_cycle_count += per_scanline;
        const uint32_t end_cycle_count_this_round = end_cycle_count / MASTER_CYCLE_PER_CPU;
        uint32_t* const count = &famicom->cpu_cycle_count;
        // 渲染背景
        sfc_render_background_scanline(famicom, i, sp0_hittest_buffer, buffer);
        // 溢出测试
        if (i == sp_overflow_line)
            famicom->ppu.data.status |= (uint8_t)SFC_PPU2002_SpOver;
        // 执行CPU
        for (; *count < end_cycle_count_this_round;)
            sfc_cpu_execute_one(famicom);
        // 执行换行

        // 取消背景显示
        if (famicom->ppu.data.mask & (uint8_t)SFC_PPU2001_Back) {
            sfc_ppu_do_under_cycle256(&famicom->ppu);
            sfc_ppu_do_under_cycle257(&famicom->ppu);
        }
        // 水平同步
        famicom->mapper.hsync(famicom, i);
        // 执行HBlank

        buffer += SFC_BUFFER_WIDTH;

        // 每65.5(这里就66)行进行一次帧计数
        if (i % 66 == 65)
            sfc_trigger_frame_counter(famicom);
    }
    
    // 后渲染
    {
        end_cycle_count += per_scanline;
        const uint32_t end_cycle_count_this_round = end_cycle_count / MASTER_CYCLE_PER_CPU;
        uint32_t* const count = &famicom->cpu_cycle_count;
        for (; *count < end_cycle_count_this_round;)
            sfc_cpu_execute_one(famicom);
        // 水平同步
        famicom->mapper.hsync(famicom, SCAN_LINE_COUNT);
    }
    // 垂直空白期间

    // 开始
    famicom->ppu.data.status |= (uint8_t)SFC_PPU2002_VBlank;
    if (famicom->ppu.data.ctrl & (uint8_t)SFC_PPU2000_NMIGen) {
        sfc_operation_NMI(famicom);
    }
    // 执行
    for (uint16_t i = 0; i != vblank_line; ++i) {
        end_cycle_count += per_scanline;
        const uint32_t end_cycle_count_this_round = end_cycle_count / MASTER_CYCLE_PER_CPU;
        uint32_t* const count = &famicom->cpu_cycle_count;
        for (; *count < end_cycle_count_this_round;) 
            sfc_cpu_execute_one(famicom);
        // 水平同步
        famicom->mapper.hsync(famicom, SCAN_LINE_COUNT + i + 1);
    }
    // 结束
    famicom->ppu.data.status = 0;
    // 垂直空白结束
    if (famicom->ppu.data.mask & (uint8_t)SFC_PPU2001_Back) {
        sfc_ppu_do_end_of_vblank(&famicom->ppu);
    }
    // 第4次触发
    sfc_trigger_frame_counter(famicom);

    // 预渲染
    end_cycle_count += per_scanline * 2;
    // 最后一次保证是偶数(DMA使用)
    const uint32_t end_cycle_count_last_round =
        famicom->config.cpu_cycle_per_frame;

        //(uint32_t)(end_cycle_count / MASTER_CYCLE_PER_CPU) & ~(uint32_t)1;


    {
        uint32_t* const count = &famicom->cpu_cycle_count;
        for (; *count < end_cycle_count_last_round;)
            sfc_cpu_execute_one(famicom);

        // 水平同步
        famicom->mapper.hsync(famicom, vblank_line + 1 + SCAN_LINE_COUNT);
    }

    // 重置计数器(32位整数太短了)
    famicom->cpu_cycle_count -= end_cycle_count_last_round;

    // 最后渲染精灵
    if (famicom->ppu.data.mask & (uint8_t)SFC_PPU2001_Sprite)
        sfc_render_sprites(famicom, data);
}