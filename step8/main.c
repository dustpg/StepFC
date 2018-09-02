#include "sfc_famicom.h"
#include "sfc_cpu.h"
#include "sfc_play.h"
#include "../common/d2d_interface.h"
#include "../common/xa2_interface.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>

void sfc_log_exec(void* arg, sfc_famicom_t* famicom);

sfc_famicom_t* g_famicom = NULL;
extern uint32_t sfc_stdpalette[];
uint32_t palette_data[16];

extern void sfc_render_frame_easy(sfc_famicom_t* famicom, uint8_t* buffer);

/// <summary>
/// 获取坐标像素
/// </summary>
/// <param name="x">The x.</param>
/// <param name="y">The y.</param>
/// <param name="nt">The nt.</param>
/// <param name="bg">The bg.</param>
/// <returns></returns>
uint32_t get_pixel(unsigned x, unsigned y, const uint8_t* nt, const uint8_t* bg) {
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
    const uint8_t index = high | low;

    return palette_data[index];
}

//__declspec(noinline)
void expand_line_8(uint8_t p0, uint8_t p1, uint8_t high, uint32_t* output) {
    // 0 - D7
    const uint8_t low0 = ((p0 & (uint8_t)0x80) >> 7) | ((p1 & (uint8_t)0x80) >> 6);
    palette_data[high] = output[0];
    output[0] = palette_data[high | low0];
    // 1 - D6
    const uint8_t low1 = ((p0 & (uint8_t)0x40) >> 6) | ((p1 & (uint8_t)0x40) >> 5);
    palette_data[high] = output[1];
    output[1] = palette_data[high | low1];
    // 2 - D5
    const uint8_t low2 = ((p0 & (uint8_t)0x20) >> 5) | ((p1 & (uint8_t)0x20) >> 4);
    palette_data[high] = output[2];
    output[2] = palette_data[high | low2];
    // 3 - D4
    const uint8_t low3 = ((p0 & (uint8_t)0x10) >> 4) | ((p1 & (uint8_t)0x10) >> 3);
    palette_data[high] = output[3];
    output[3] = palette_data[high | low3];
    // 4 - D3
    const uint8_t low4 = ((p0 & (uint8_t)0x08) >> 3) | ((p1 & (uint8_t)0x08) >> 2);
    palette_data[high] = output[4];
    output[4] = palette_data[high | low4];
    // 5 - D2
    const uint8_t low5 = ((p0 & (uint8_t)0x04) >> 2) | ((p1 & (uint8_t)0x04) >> 1);
    palette_data[high] = output[5];
    output[5] = palette_data[high | low5];
    // 6 - D1
    const uint8_t low6 = ((p0 & (uint8_t)0x02) >> 1) | ((p1 & (uint8_t)0x02) >> 0);
    palette_data[high] = output[6];
    output[6] = palette_data[high | low6];
    // 7 - D0
    const uint8_t low7 = ((p0 & (uint8_t)0x01) >> 0) | ((p1 & (uint8_t)0x01) << 1);
    palette_data[high] = output[7];
    output[7] = palette_data[high | low7];
}


//__declspec(noinline)
void expand_line_8_r(uint8_t p0, uint8_t p1, uint8_t high, uint32_t* output) {
    // 7 - D7
    const uint8_t low0 = ((p0 & (uint8_t)0x80) >> 7) | ((p1 & (uint8_t)0x80) >> 6);
    palette_data[high] = output[7];
    output[7] = palette_data[high | low0];
    // 6 - D6
    const uint8_t low1 = ((p0 & (uint8_t)0x40) >> 6) | ((p1 & (uint8_t)0x40) >> 5);
    palette_data[high] = output[6];
    output[6] = palette_data[high | low1];
    // 5 - D5
    const uint8_t low2 = ((p0 & (uint8_t)0x20) >> 5) | ((p1 & (uint8_t)0x20) >> 4);
    palette_data[high] = output[5];
    output[5] = palette_data[high | low2];
    // 4 - D4
    const uint8_t low3 = ((p0 & (uint8_t)0x10) >> 4) | ((p1 & (uint8_t)0x10) >> 3);
    palette_data[high] = output[4];
    output[4] = palette_data[high | low3];
    // 3 - D3
    const uint8_t low4 = ((p0 & (uint8_t)0x08) >> 3) | ((p1 & (uint8_t)0x08) >> 2);
    palette_data[high] = output[3];
    output[3] = palette_data[high | low4];
    // 2 - D2
    const uint8_t low5 = ((p0 & (uint8_t)0x04) >> 2) | ((p1 & (uint8_t)0x04) >> 1);
    palette_data[high] = output[2];
    output[2] = palette_data[high | low5];
    // 1 - D1
    const uint8_t low6 = ((p0 & (uint8_t)0x02) >> 1) | ((p1 & (uint8_t)0x02) >> 0);
    palette_data[high] = output[1];
    output[1] = palette_data[high | low6];
    // 0 - D0
    const uint8_t low7 = ((p0 & (uint8_t)0x01) >> 0) | ((p1 & (uint8_t)0x01) << 1);
    palette_data[high] = output[0];
    output[0] = palette_data[high | low7];
}

extern int sub_render(void* bgrx) {
    return 0;
}

static sfc_channel_state_t states[4];
static int index;

void pin_audio() {
    sfc_play_audio_easy(g_famicom, states + index);
    ++index;
    index = index & 3;
}

int index240 = 0;
void play_240() {
    sfc_channel_state_t state = states[index240];
    // 方波#1
    xa2_play_square1(
        state.square1.frequency,
        state.square1.duty,
        state.square1.volume
    );
    // 方波#2
    xa2_play_square2(
        state.square2.frequency,
        state.square2.duty,
        state.square2.volume
    );

    ++index240;
    index240 = index240 & 3;
}


void play_audio() {
    index240 = 0;

    return;
    sfc_channel_state_t state = states[1];
    //assert(memcmp(states + 0, states + 1, sizeof(states[0])) == 0);
    //assert(memcmp(states + 1, states + 2, sizeof(states[0])) == 0);
    //assert(memcmp(states + 2, states + 3, sizeof(states[0])) == 0);
    //sfc_play_audio_easy(g_famicom, &state);
    // 方波#1
    xa2_play_square1(
        state.square1.frequency,
        state.square1.duty,
        state.square1.volume
    );
    // 方波#2
    xa2_play_square2(
        state.square2.frequency,
        state.square2.duty,
        state.square2.volume
    );

}


/// <summary>
/// 主渲染
/// </summary>
/// <param name="bgrx">The BGRX.</param>
extern void main_render(void* bgrx) {
    uint32_t* const data = bgrx;

    uint8_t buffer[256 * 256];

    //printf("\nFRAME: ");
    sfc_render_frame_easy(g_famicom, buffer);

    //sfc_render_frame(g_famicom, buffer);

    // 生成调色板数据
    uint32_t palette[32];
    
    for (int i = 0; i != 32; ++i)
        palette[i] = sfc_stdpalette[g_famicom->ppu.spindexes[i]];
    // 镜像数据
    palette[4 * 1] = palette[0];
    palette[4 * 2] = palette[0];
    palette[4 * 3] = palette[0];
    palette[4 * 4] = palette[0];
    palette[4 * 5] = palette[0];
    palette[4 * 6] = palette[0];
    palette[4 * 7] = palette[0];

    for (int i = 0; i != 256 * 240; ++i) {
        data[i] = palette[buffer[i]>>1];
    }


    play_audio();
#if 0


    // 生成调色板颜色
    {
        for (int i = 0; i != 16; ++i) {
            palette_data[i] = sfc_stdpalette[g_famicom->ppu.spindexes[i]];
        }
        palette_data[4 * 1] = palette_data[0];
        palette_data[4 * 2] = palette_data[0];
        palette_data[4 * 3] = palette_data[0];
    }
    // 背景
    const uint8_t* now = g_famicom->ppu.banks[8];
    const uint8_t* bgp = g_famicom->ppu.banks[
        g_famicom->ppu.ctrl & SFC_PPU2000_BgTabl ? 4 : 0];
    for (unsigned i = 0; i != 256 * 240; ++i) {
        data[i] = get_pixel(i & 0xff, i >> 8, now, bgp);
    }
#endif
}

/// <summary>
/// 应用程序入口
/// </summary>
/// <returns></returns>
int main() {
    sfc_interface_t interfaces = { NULL };
    interfaces.before_execute = sfc_log_exec;

    sfc_famicom_t famicom;
    g_famicom = &famicom;
    if (sfc_famicom_init(&famicom, NULL, &interfaces)) return 1;

    printf(
        "ROM: PRG-RPM: %d x 16kb   CHR-ROM %d x 8kb   Mapper: %03d\n",
        (int)famicom.rom_info.count_prgrom16kb,
        (int)famicom.rom_info.count_chrrom_8kb,
        (int)famicom.rom_info.mapper_number
    );
    xa2_init();
    main_cpp();
    xa2_clean();

    sfc_famicom_uninit(&famicom);
    return 0;
}


/// <summary>
/// Users the input.
/// </summary>
/// <param name="index">The index.</param>
/// <param name="data">The data.</param>
void user_input(int index, unsigned char data) {
    assert(index >= 0 && index < 16);
    g_famicom->button_states[index] = data;
}



void sfc_log_exec(void* arg, sfc_famicom_t* famicom) {
    const uint32_t cycle = famicom->cpu_cycle_count;
    const uint16_t pc = famicom->registers.program_counter;
    static int line = 0;  line++;
    if (pc == 0xe05a) {
        int bk = 8;
    }
    if (pc == 0xe02d) {
        int bk = 8;
    }
    if (pc == 0xe02a) {
        int bk = 8;
    }
    return;
    //if (line < 230297) return;

    char buf[SFC_DISASSEMBLY_BUF_LEN2];
    sfc_fc_disassembly(pc, famicom, buf);
    printf(
        "%4d - %s   A:%02X X:%02X Y:%02X P:%02X SP:%02X\n",
        line, buf,
        (int)famicom->registers.accumulator,
        (int)famicom->registers.x_index,
        (int)famicom->registers.y_index,
        (int)famicom->registers.status,
        (int)famicom->registers.stack_pointer
    );
}
