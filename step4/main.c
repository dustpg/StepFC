#include "sfc_famicom.h"
#include "sfc_cpu.h"
#include <stdio.h>
#include <math.h>


void sfc_log_exec(void* arg, sfc_famicom_t* famicom) {
    static int line = 0;
    line++;
    char buf[SFC_DISASSEMBLY_BUF_LEN2];
    const uint16_t pc = famicom->registers.program_counter;
    sfc_fc_disassembly(pc, famicom, buf);
    //printf(
    //    "%4d - %s   A:%02X X:%02X Y:%02X P:%02X SP:%02X\n",
    //    line, buf,
    //    (int)famicom->registers.accumulator,
    //    (int)famicom->registers.x_index,
    //    (int)famicom->registers.y_index,
    //    (int)famicom->registers.status,
    //    (int)famicom->registers.stack_pointer
    //);
}


void sfc_display_vram(sfc_famicom_t* famicom) {
    const char HEXDATA[] = "0123456789ABCDEF";
    const int height = 30;
    char buf[(64 + 1) * 30];
    char* write_itr = buf;
    char* read_itr = famicom->video_memory;
    for (int i = 0; i != height; ++i) {
        for (int x = 0; x != 32; ++x) {
            write_itr[0] = HEXDATA[((*read_itr) & 0xf)];
            write_itr[1] = HEXDATA[((*read_itr) >> 4)];
            write_itr += 2;
            ++read_itr;
        }
        *write_itr = '\n';
        write_itr++;
    }
    write_itr[-1] = 0;
    printf(buf);
    putchar('\n');
}

sfc_famicom_t* g_famicom = NULL;
extern uint32_t sfc_stdalette[];

uint32_t palette_data[16];

uint32_t get_pixel(unsigned x, unsigned y, const uint8_t* nt) {
    if (x == 255 && y == 100) {
        int bk = 9;
    }
    // 获取所在名称表
    const unsigned id = (x >> 3) + (y >> 3) * 32;
    const uint32_t name = nt[id];
    // 查找对应图样表
    const uint8_t* nowp0 = g_famicom->ppu.banks[0] + name * 16;
    const uint8_t* nowp1 = nowp0 + 8;
    // Y坐标为平面内偏移
    const int offset = y & 0x7;
    const uint8_t p0 = nowp0[offset];
    const uint8_t p1 = nowp1[offset];
    // X坐标为字节内偏移
    const uint8_t shift = (~x) & 0x7;
    const uint8_t mask = 1 << shift;
    // 计算低二位
    const uint8_t low = ((p0 & mask) >> shift) | ((p1 & mask) >> shift << 1);
    // 计算所在属性表
    const unsigned aid = (x >> 5) * (y >> 5);
    const uint8_t attr = nt[aid + (32*30)];
    // 获取属性表内位偏移
    const uint8_t aoffset = ((x & 0x10) >> 3) | ((y & 0x10) >> 2);
    // 计算高两位
    const uint8_t high = (attr & (3 << aoffset)) >> aoffset << 2;
    // 合并作为颜色
    const uint8_t index = high | low;

    return palette_data[index];
}

extern void main_render(void* bgrx)  {

    for (int i = 0; i != 20000; ++i)
        sfc_cpu_execute_one(g_famicom);

    sfc_vblank_start(g_famicom);
    sfc_do_vblank(g_famicom);

    uint32_t* data = bgrx;
    // 生成调色板颜色
    {
        for (int i = 0; i != 16; ++i) {
            palette_data[i] = sfc_stdalette[g_famicom->ppu.spindexes[i]];
        }
        palette_data[4 * 1] = palette_data[0];
        palette_data[4 * 2] = palette_data[0];
        palette_data[4 * 3] = palette_data[0];
    }

    const uint8_t* now = g_famicom->ppu.banks[8];
    for (unsigned i = 0; i != 256 * 240; ++i) {
        data[i] = get_pixel(i & 0xff, i >> 8, now);
    }
    //sfc_display_vram(g_famicom);
    //putchar('\n');
}

extern int main_cpp();

/// <summary>
/// 应用程序入口
/// </summary>
/// <returns></returns>
int main() {
    sfc_interface_t interfaces = { NULL };
    interfaces.before_execute = sfc_log_exec;

    sfc_famicom_t famicom;
    g_famicom = &famicom;
    sfc_famicom_init(&famicom, NULL, &interfaces);
    printf(
        "ROM: PRG-RPM: %d x 16kb   CHR-ROM %d x 8kb   Mapper: %03d\n", 
        (int)famicom.rom_info.count_prgrom16kb,
        (int)famicom.rom_info.count_chrrom_8kb,
        (int)famicom.rom_info.mapper_number
    );

    main_cpp();

    sfc_famicom_uninit(&famicom);
    return 0;
}