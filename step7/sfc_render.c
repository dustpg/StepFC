#include "sfc_famicom.h"
#include <assert.h>

// NMI - 不可屏蔽中断
extern inline void sfc_operation_NMI(sfc_famicom_t* famicom);

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
static inline uint8_t sfc_bg_get_pixel(uint8_t x, uint8_t y, const uint8_t* nt, const uint8_t* bg) {
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
/// <param name="buffer">The buffer.</param>
static void sfc_render_background_scanline(sfc_famicom_t* famicom, uint16_t line, uint8_t* buffer) {
    // 计算当前偏移量
    const uint16_t scrollx = famicom->ppu.scroll[0] + ((famicom->ppu.nametable_select & 1) << 8);
    const uint16_t scrolly = famicom->ppu.scroll[1];
    // 在扫描期间写入调色板数据是合法的
    for (int i = 0; i != 16; ++i) {
        s_sfc_palette_data[i] = famicom->ppu.spindexes[i];
    }
    s_sfc_palette_data[4 * 1] = s_sfc_palette_data[0];
    s_sfc_palette_data[4 * 2] = s_sfc_palette_data[0];
    s_sfc_palette_data[4 * 3] = s_sfc_palette_data[0];
    // 计算背景所使用的图样表
    const uint8_t* const pattern = famicom->ppu.banks[famicom->ppu.ctrl & SFC_PPUFLAG_BgTabl ? 4 : 0];
    // 检测垂直偏移量确定使用图案表的前一半[8-9]还是后一半[10-11]

    // 目前假定使用前一半
    const uint8_t* const * const table = famicom->ppu.banks + 8;
    // 扫描该行像素
    for (uint16_t i = 0; i != (uint16_t)0x100; ++i) {
        const uint16_t realx = scrollx + i;
        const uint16_t realy = scrolly + line;
        const uint8_t* nt = table[(realx >> 8) & 1];
        buffer[i] = sfc_bg_get_pixel((uint8_t)realx, (uint8_t)realy, nt, pattern);
    }
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
/// SFCs the render frame.
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_render_frame(sfc_famicom_t* famicom, uint8_t* buffer) {
    enum { SCAN_LINE_COUNT = 240 };
    uint8_t* const data = buffer;
    //const uint16_t visible_line = famicom->config.visible_scanline;
    const uint16_t vblank_line = famicom->config.vblank_scanline;
    const uint32_t per_scanline = famicom->config.cpu_cycle_per_scanline;
    uint32_t end_cycle_count = 0;


    // 预渲染

    // 渲染
    for (uint16_t i = 0; i != (uint16_t)SCAN_LINE_COUNT; ++i) {
        end_cycle_count += per_scanline;
        const uint32_t end_cycle_count_this_round = end_cycle_count;
        uint32_t* const count = &famicom->cycle_count;
        // 执行CPU
        for (; *count < end_cycle_count_this_round;)
            sfc_cpu_execute_one(famicom);
        // 渲染背景
        sfc_render_background_scanline(famicom, i, buffer);
        buffer += 256;
        // 执行HBlank
        if (i == 31) 
            famicom->ppu.status |= (uint8_t)SFC_PPUFLAG_Sp0Hit;
    }
    // 后渲染


    // 垂直空白期间

    // 开始
    famicom->ppu.status |= (uint8_t)SFC_PPUFLAG_VBlank;
    if (famicom->ppu.ctrl & (uint8_t)SFC_PPUFLAG_NMIGen) {
        sfc_operation_NMI(famicom);
    }
    // 执行
    for (uint16_t i = 0; i != vblank_line; ++i) {
        end_cycle_count += per_scanline;
        const uint32_t end_cycle_count_this_round = end_cycle_count;
        uint32_t* const count = &famicom->cycle_count;
        for (; *count < end_cycle_count_this_round;)
            sfc_cpu_execute_one(famicom);
    }
    // 结束
    famicom->ppu.status = 0;
    
    // 清除计数器
    famicom->cycle_count -= end_cycle_count;
    // 必须是偶数
    assert((end_cycle_count & 1) == 0);



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
    // 精灵
    const uint8_t* spp = famicom->ppu.banks[famicom->ppu.ctrl & SFC_PPUFLAG_SpTabl ? 4 : 0];

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