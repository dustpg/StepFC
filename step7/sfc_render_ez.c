#include "sfc_famicom.h"
#include <assert.h>
#include <string.h>

// 微软编译器目前不支持C11的关键字_Alignas
#ifdef _MSC_VER
#define SFC_ALIGNAS(a) __declspec(align(a))
#else
#define SFC_ALIGNAS(a) _Alignas(a)
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
        hittest |= array[i];
    }
    return hittest;
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
    __m128i values = _mm_loadl_epi64((__m128i*)array);
    __m128i order = _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7);
    values = _mm_shuffle_epi8(values, order);
    const int result = _mm_movemask_epi8(_mm_slli_epi32(values, 7));
    return (uint8_t)result;
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
/// The palette data
/// </summary>
static uint8_t s_sfc_palette_data[32];

/// <summary>
/// 获取坐标像素
/// </summary>
/// <param name="x">The x.</param>
/// <param name="y">The y.</param>
/// <param name="nt">The nt.</param>
/// <param name="bg">The bg.</param>
/// <returns></returns>
static inline uint8_t sfc_bg_get_pixel(
    uint8_t x, 
    uint8_t y, 
    uint8_t* background,
    const uint8_t* nt, 
    const uint8_t* bg) {
    // TODO: 优化为按图块作业

    // 获取所在名称表
    const unsigned id = (x >> 3) + (y >> 3) * 32;
    const uint32_t name = nt[id];
    // 查找对应图样表
    const uint8_t* nowp0 = bg + name * 16;
    const uint8_t* nowp1 = nowp0 + 8;
    // Y坐标为平面内偏移
    const uint8_t p0 = nowp0[y & 0x7];
    const uint8_t p1 = nowp1[y & 0x7];
    // X坐标为字节内偏移
    const uint8_t shift = (~x) & 0x7;
    const uint8_t mask = 1 << shift;
    // 背景检测
    *background = ((p0 & mask) >> shift) | ((p1 & mask) >> shift);
    // 计算低二位
    const uint8_t low = ((p0 & mask) >> shift) | ((p1 & mask) >> shift << 1);
    // 计算所在属性表
    const unsigned aid = (x >> 5) + (y >> 5) * 8;
    const uint8_t attr = nt[aid + (32 * 30)];
    // 获取属性表内位偏移
    const uint8_t aoffset = ((x & 0x10) >> 3) | ((y & 0x10) >> 2);
    // 计算高两位
    const uint8_t high = (attr & (3 << aoffset)) >> aoffset << 2;
    // 合并作为颜色
    return s_sfc_palette_data[high | low];
}


/// <summary>
/// SFCs the render background scanline.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="line">The line.</param>
/// <param name="spp">The SPP.</param>
/// <param name="buffer">The buffer.</param>
static inline void sfc_render_background_scanline(
    sfc_famicom_t* famicom, 
    uint16_t line,
    const uint8_t sp0[240+(16)],
    uint8_t* buffer) {
    uint8_t backgorund_hittest[256 + 16];

    // 计算当前偏移量
    const uint16_t scrollx = famicom->ppu.scroll[0] + 
        ((famicom->ppu.nametable_select & 1) << 8);
    const uint16_t scrolly = famicom->ppu.scroll[1];
    // TODO: s_sfc_palette_data 静态数据

    // 在扫描期间写入调色板数据是合法的
    for (int i = 0; i != 16; ++i) {
        s_sfc_palette_data[i] = famicom->ppu.spindexes[i];
    }
    s_sfc_palette_data[4 * 1] = s_sfc_palette_data[0];
    s_sfc_palette_data[4 * 2] = s_sfc_palette_data[0];
    s_sfc_palette_data[4 * 3] = s_sfc_palette_data[0];
    // 计算背景所使用的图样表
    const uint8_t* const pattern = famicom->ppu.banks[famicom->ppu.ctrl & SFC_PPU2000_BgTabl ? 4 : 0];
    // 检测垂直偏移量确定使用图案表的前一半[8-9]还是后一半[10-11]

    // 目前假定使用前一半
    const uint8_t* const * const table = famicom->ppu.banks + 8;


    //// 以16像素为单位扫描该行
    //SFC_ALIGNAS(16) uint8_t aligned_buffer[256 + 16];
    //// 因为保险起见扫描17次
    //for (int i = 0; i != 17; ++i) {

    //}

    // 扫描该行像素
    for (uint16_t i = 0; i != (uint16_t)0x100; ++i) {
        const uint16_t realx = scrollx + i;
        const uint16_t realy = scrolly + line;
        const uint8_t* nt = table[(realx >> 8) & 1];
        buffer[i] = sfc_bg_get_pixel(
            (uint8_t)realx, 
            (uint8_t)realy, 
            backgorund_hittest + i,
            nt,
            pattern
        );
    }
    // 基于行的精灵0命中测试

    // 已经命中了
    if (famicom->ppu.status & (uint8_t)SFC_PPU2002_Sp0Hit) return;
    // 没有必要测试
    const uint8_t hittest_data = sp0[line];
    if (!hittest_data) return;
    // 精灵#0的数据
    memset(backgorund_hittest + 256, 0, 16);
    const uint8_t  xxxxx = famicom->ppu.sprites[3];
    const uint8_t hittest = sfc_pack_bool8_into_byte(backgorund_hittest + xxxxx);
    if (hittest_data & hittest)
        famicom->ppu.status |= (uint8_t)SFC_PPU2002_Sp0Hit;
}


/// <summary>
/// SFCs the sprite0 hittest.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="spp">The SPP.</param>
/// <param name="buffer">The buffer.</param>
static inline void sfc_sprite0_hittest(
    sfc_famicom_t* famicom, 
    const uint8_t* spp,
    uint8_t buffer[256]) {
    // 先清空
    memset(buffer, 0, 256);
    // 关闭渲染
    enum { BOTH_BS = SFC_PPU2001_Back | SFC_PPU2001_Sprite };
    if ((famicom->ppu.mask & (uint8_t)BOTH_BS) != (uint8_t)BOTH_BS) return;
    // 获取数据以填充
    const uint8_t yyyyy = famicom->ppu.sprites[0];
    // 0xef以上就算了
    if (yyyyy >= 0xEF) return;
    // 属性
    const uint8_t aaaaa = famicom->ppu.sprites[2];
    // 获取平面数据
    const uint8_t* nowp0 = spp + famicom->ppu.sprites[1] * 16;
    const uint8_t* nowp1 = nowp0 + 8;
    uint8_t* const buffer_write = buffer + yyyyy + 1;
    // 水平翻转
    if (aaaaa & (uint8_t)SFC_SPATTR_FlipH) 
        for (int i = 0; i != 8; ++i) {
            const uint8_t data = nowp0[i] | nowp1[i];
            buffer_write[i] = BitReverseTable256[data];
        }
    // 普通
    else 
        for (int i = 0; i != 8; ++i) {
            const uint8_t data = nowp0[i] | nowp1[i];
            buffer_write[i] = data;
        }
    // 垂直翻转
    if (aaaaa & (uint8_t)SFC_SPATTR_FlipV) {
        sfc_swap_byte(buffer_write + 0, buffer_write + 7);
        sfc_swap_byte(buffer_write + 1, buffer_write + 6);
        sfc_swap_byte(buffer_write + 2, buffer_write + 5);
        sfc_swap_byte(buffer_write + 3, buffer_write + 4);
    }
    // TODO: 8x16 支持
}


/// <summary>
/// SFCs the sprite overflow test.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint16_t sfc_sprite_overflow_test(sfc_famicom_t* famicom) {
    // 完全关闭渲染
    enum { BOTH_BS = SFC_PPU2001_Back | SFC_PPU2001_Sprite };
    if (!(famicom->ppu.mask & (uint8_t)BOTH_BS)) return 256; 
    // 正式处理
    uint8_t buffer[256 + 16];
    memset(buffer, 0, 256);
    // 8 x 16
    const int height = famicom->ppu.ctrl & SFC_PPU2000_Sp8x16 ? 16 : 8;
    for (int i = 0; i != 64; ++i) {
        const uint8_t y = famicom->ppu.sprites[i * 4];
        for (int i = 0; i != height; ++i) buffer[y + i]++;
    }
    uint16_t line ;
    for (line = 0; line != 240; ++line) if (buffer[line] > 8) break;
    return line;
}


static inline
void expand_line_8(uint8_t p0, uint8_t p1, uint8_t high, uint8_t* output) {
    // 0 - D7
    const uint8_t low0 = ((p0 & (uint8_t)0x80) >> 7) | ((p1 & (uint8_t)0x80) >> 6);
    s_sfc_palette_data[high] = output[0];
    output[0] = s_sfc_palette_data[high | low0];
    // 1 - D6
    const uint8_t low1 = ((p0 & (uint8_t)0x40) >> 6) | ((p1 & (uint8_t)0x40) >> 5);
    s_sfc_palette_data[high] = output[1];
    output[1] = s_sfc_palette_data[high | low1];
    // 2 - D5
    const uint8_t low2 = ((p0 & (uint8_t)0x20) >> 5) | ((p1 & (uint8_t)0x20) >> 4);
    s_sfc_palette_data[high] = output[2];
    output[2] = s_sfc_palette_data[high | low2];
    // 3 - D4
    const uint8_t low3 = ((p0 & (uint8_t)0x10) >> 4) | ((p1 & (uint8_t)0x10) >> 3);
    s_sfc_palette_data[high] = output[3];
    output[3] = s_sfc_palette_data[high | low3];
    // 4 - D3
    const uint8_t low4 = ((p0 & (uint8_t)0x08) >> 3) | ((p1 & (uint8_t)0x08) >> 2);
    s_sfc_palette_data[high] = output[4];
    output[4] = s_sfc_palette_data[high | low4];
    // 5 - D2
    const uint8_t low5 = ((p0 & (uint8_t)0x04) >> 2) | ((p1 & (uint8_t)0x04) >> 1);
    s_sfc_palette_data[high] = output[5];
    output[5] = s_sfc_palette_data[high | low5];
    // 6 - D1
    const uint8_t low6 = ((p0 & (uint8_t)0x02) >> 1) | ((p1 & (uint8_t)0x02) >> 0);
    s_sfc_palette_data[high] = output[6];
    output[6] = s_sfc_palette_data[high | low6];
    // 7 - D0
    const uint8_t low7 = ((p0 & (uint8_t)0x01) >> 0) | ((p1 & (uint8_t)0x01) << 1);
    s_sfc_palette_data[high] = output[7];
    output[7] = s_sfc_palette_data[high | low7];
}


static inline
void expand_line_8_r(uint8_t p0, uint8_t p1, uint8_t high, uint8_t* output) {
    // 7 - D7
    const uint8_t low0 = ((p0 & (uint8_t)0x80) >> 7) | ((p1 & (uint8_t)0x80) >> 6);
    s_sfc_palette_data[high] = output[7];
    output[7] = s_sfc_palette_data[high | low0];
    // 6 - D6
    const uint8_t low1 = ((p0 & (uint8_t)0x40) >> 6) | ((p1 & (uint8_t)0x40) >> 5);
    s_sfc_palette_data[high] = output[6];
    output[6] = s_sfc_palette_data[high | low1];
    // 5 - D5
    const uint8_t low2 = ((p0 & (uint8_t)0x20) >> 5) | ((p1 & (uint8_t)0x20) >> 4);
    s_sfc_palette_data[high] = output[5];
    output[5] = s_sfc_palette_data[high | low2];
    // 4 - D4
    const uint8_t low3 = ((p0 & (uint8_t)0x10) >> 4) | ((p1 & (uint8_t)0x10) >> 3);
    s_sfc_palette_data[high] = output[4];
    output[4] = s_sfc_palette_data[high | low3];
    // 3 - D3
    const uint8_t low4 = ((p0 & (uint8_t)0x08) >> 3) | ((p1 & (uint8_t)0x08) >> 2);
    s_sfc_palette_data[high] = output[3];
    output[3] = s_sfc_palette_data[high | low4];
    // 2 - D2
    const uint8_t low5 = ((p0 & (uint8_t)0x04) >> 2) | ((p1 & (uint8_t)0x04) >> 1);
    s_sfc_palette_data[high] = output[2];
    output[2] = s_sfc_palette_data[high | low5];
    // 1 - D1
    const uint8_t low6 = ((p0 & (uint8_t)0x02) >> 1) | ((p1 & (uint8_t)0x02) >> 0);
    s_sfc_palette_data[high] = output[1];
    output[1] = s_sfc_palette_data[high | low6];
    // 0 - D0
    const uint8_t low7 = ((p0 & (uint8_t)0x01) >> 0) | ((p1 & (uint8_t)0x01) << 1);
    s_sfc_palette_data[high] = output[0];
    output[0] = s_sfc_palette_data[high | low7];
}

/// <summary>
/// StepFC: 使用简易模式渲染一帧, 效率较高
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_render_frame_easy(sfc_famicom_t* famicom, uint8_t* buffer) {
    enum { SCAN_LINE_COUNT = 240 };
    uint8_t* const data = buffer;
    const uint16_t vblank_line = famicom->config.vblank_scanline;
    const uint32_t per_scanline = famicom->config.master_cycle_per_scanline;
    uint32_t end_cycle_count = 0;

    // 精灵使用的图样板
    const uint8_t* spp = famicom->ppu.banks[famicom->ppu.ctrl & SFC_PPU2000_SpTabl ? 4 : 0];
    // 精灵0用命中测试缓存
    uint8_t sp0_hittest_buffer[256];
    sfc_sprite0_hittest(famicom, spp, sp0_hittest_buffer);
    // 精灵溢出测试
    const uint16_t sp_overflow_line = sfc_sprite_overflow_test(famicom);
    // 预渲染

    // 渲染
    for (uint16_t i = 0; i != (uint16_t)SCAN_LINE_COUNT; ++i) {
        end_cycle_count += per_scanline;
        const uint32_t end_cycle_count_this_round = end_cycle_count / MASTER_CYCLE_PER_CPU;
        uint32_t* const count = &famicom->cpu_cycle_count;
        // 渲染背景
        sfc_render_background_scanline(famicom, i, sp0_hittest_buffer, buffer);
        // 溢出测试
        if (i == sp_overflow_line)
            famicom->ppu.status |= (uint8_t)SFC_PPU2002_SpOver;
        // 执行CPU
        for (; *count < end_cycle_count_this_round;)
            sfc_cpu_execute_one(famicom);

        buffer += 256;
        // 执行HBlank
    }
    // 后渲染


    // 垂直空白期间

    // 开始
    famicom->ppu.status |= (uint8_t)SFC_PPU2002_VBlank;
    if (famicom->ppu.ctrl & (uint8_t)SFC_PPU2000_NMIGen) {
        sfc_operation_NMI(famicom);
    }
    // 执行
    for (uint16_t i = 0; i != vblank_line; ++i) {
        end_cycle_count += per_scanline;
        const uint32_t end_cycle_count_this_round = end_cycle_count / MASTER_CYCLE_PER_CPU;
        uint32_t* const count = &famicom->cpu_cycle_count;
        for (; *count < end_cycle_count_this_round;)
            sfc_cpu_execute_one(famicom);
    }
    // 结束
    famicom->ppu.status = 0;

    // 预渲染
    end_cycle_count += per_scanline * 2;
    // 最后一次保证是偶数(DMA使用)
    const uint32_t end_cycle_count_last_round =
        (uint32_t)(end_cycle_count / MASTER_CYCLE_PER_CPU) & ~(uint32_t)1;
    {
        uint32_t* const count = &famicom->cpu_cycle_count;
        for (; *count < end_cycle_count_last_round;)
            sfc_cpu_execute_one(famicom);
    }

    // 清除计数器
    famicom->cpu_cycle_count -= end_cycle_count_last_round;

    //return;

    // 生成调色板颜色
    //memset(data, 0, 256 * 240 * 4);
    {
        for (int i = 0; i != 16; ++i) {
            s_sfc_palette_data[i] = famicom->ppu.spindexes[i + 16];
        }
        s_sfc_palette_data[4 * 1] = s_sfc_palette_data[0];
        s_sfc_palette_data[4 * 2] = s_sfc_palette_data[0];
        s_sfc_palette_data[4 * 3] = s_sfc_palette_data[0];
    }

    //LARGE_INTEGER t0, t1;
    //QueryPerformanceCounter(&t0);

    for (int i = 63; i != -1; --i) {
        const uint8_t* ptr = famicom->ppu.sprites + i * 4;
        const uint8_t yy = ptr[0];
        const uint8_t ii = ptr[1];
        const uint8_t aa = ptr[2];
        const uint8_t xx = ptr[3];
        if (yy >= 0xef) continue;
        // 查找对应图样表
        const uint8_t* nowp0 = spp + ii * 16;
        const uint8_t* nowp1 = nowp0 + 8;
        const uint8_t high = (aa & 3) << 2 /*| ((aa & 0x20) >> 1)*/;
        // 水平翻转
        if (aa & 0x40) for (uint8_t i = 0; i != 8; ++i) {
            expand_line_8_r(nowp0[i], nowp1[i], high, data + xx + (yy + i + 1) * 256);
        }
        else for (uint8_t i = 0; i != 8; ++i) {
            expand_line_8(nowp0[i], nowp1[i], high, data + xx + (yy + i + 1) * 256);
        }
    }
}