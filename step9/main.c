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

extern void sfc_render_frame_easy(
    sfc_famicom_t* famicom, 
    uint8_t* buffer
);

extern int sub_render(void* bgrx) {
    return 0;
}

void play_audio() {
    sfc_channel_state_t state ;
    //int changed = 0;
    //for (int i = 0; i != 3; ++i)
    //if (memcmp(states + i, states + i + 1, sizeof(states[0])))
    //    changed++;
    //assert(changed < 2);

    sfc_play_audio_easy(g_famicom, &state);
#if 1
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
    // 三角波
    xa2_play_triangle(
        state.triangle.frequency
    );
#endif
    // 噪音
    xa2_play_noise(
        state.noise.data,
        state.noise.volume
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

    // 0->1 =>
    // 1->0

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


void qsave() {
    FILE* const file = fopen("save.sfc", "wb");
    if (!file) return;
    sfc_famicom_t buf;
    buf = *g_famicom;
    // BANK保存为偏移量
    /*for (int i = 0; i != 4; ++i) {
        buf.prg_banks[i]
            = buf.prg_banks[i]
            - buf.rom_info.data_prgrom
            ;
    }*/
    // BANK保存为偏移量
    for (int i = 4; i != 8; ++i) {
        buf.prg_banks[i] 
            = (size_t)(buf.prg_banks[i] 
            - buf.rom_info.data_prgrom)
            ;
    }
    // BANK保存为偏移量
    for (int i = 0; i != 8; ++i) {
        buf.ppu.banks[i]
            = (size_t)(buf.ppu.banks[i]
                - buf.rom_info.data_chrrom)
            ;
    }
    // BANK保存为偏移量
    for (int i = 8; i != 16; ++i) {
        buf.ppu.banks[i]
            = (size_t)(buf.ppu.banks[i]
                - g_famicom->video_memory)
            ;
    }

    fwrite(&buf, 1, sizeof(buf), file);
    fclose(file);
}

void qload() {
    FILE* const file = fopen("save.sfc", "rb");
    if (!file) return;
    sfc_famicom_t buf;
    fread(&buf, 1, sizeof(buf), file);
    g_famicom->registers = buf.registers;
    g_famicom->cpu_cycle_count = buf.cpu_cycle_count;
    g_famicom->apu = buf.apu;
    g_famicom->mapper_buffer = buf.mapper_buffer;

    g_famicom->ppu = buf.ppu;


    memcpy(g_famicom->main_memory, buf.main_memory, sizeof(buf.main_memory));
    memcpy(g_famicom->video_memory, buf.video_memory, sizeof(buf.main_memory));
    //memcpy(g_famicom->video_memory_ex, buf.video_memory_ex, sizeof(buf.main_memory));
    // TODO: 保存CHR-RAM

    // BANK保存为偏移量
    for (int i = 4; i != 8; ++i) {
        g_famicom->prg_banks[i]
            = (size_t)buf.prg_banks[i]
            + g_famicom->rom_info.data_prgrom
            ;
    }
    // BANK保存为偏移量
    for (int i = 0; i != 8; ++i) {
        g_famicom->ppu.banks[i]
            = (size_t)g_famicom->ppu.banks[i]
            + g_famicom->rom_info.data_chrrom
            ;
    }
    // BANK保存为偏移量
    for (int i = 8; i != 16; ++i) {
        g_famicom->ppu.banks[i]
            = (size_t)g_famicom->ppu.banks[i]
            + g_famicom->video_memory
            ;
    }

    fclose(file);
}

uint8_t g_seach[0x800] = { 0 };

void sfc_log_exec(void* arg, sfc_famicom_t* famicom) {
    const uint32_t cycle = famicom->cpu_cycle_count;
    const uint16_t pc = famicom->registers.program_counter;
    static int line = 0;  line++;
    //if (pc == 0xa826) {
    //}
    //else if (pc == 0xc82f) {
    //}
    //else 
    return;
    uint8_t search = 0;
    if (search) {

        for (int i = 0; i != 0x800; ++i) {
            if (g_famicom->main_memory[i] == search)
                g_seach[i]++;
        }
    }
    //if (line < 230297) return;

    char buf[SFC_DISASSEMBLY_BUF_LEN2];
    sfc_fc_disassembly(pc, famicom, buf);
    printf(
        "%4d[%5d] - %s   A:%02X X:%02X Y:%02X P:%02X SP:%02X\n",
        line, famicom->cpu_cycle_count ,buf,
        (int)famicom->registers.accumulator,
        (int)famicom->registers.x_index,
        (int)famicom->registers.y_index,
        (int)famicom->registers.status,
        (int)famicom->registers.stack_pointer
    );
}
