#include "sfc_famicom.h"
#include "sfc_cpu.h"
#include "sfc_play.h"
#define SFC_NO_SUBRENDER
#include "../common/d2d_interface1.h"
#include "../common/xa2_interface1.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

enum {
    NTSC_CPU_RATE = 1789773,
    SAMPLES_PER_SEC = 44100,
    SAMPLES_PER_FRAME = SAMPLES_PER_SEC / 60,
    SAMPLES_ALIGNED = 8,
    SAMPLES_PER_FRAME_ALIGNED = (SAMPLES_PER_FRAME / SAMPLES_ALIGNED + 1) * SAMPLES_ALIGNED,
    BASIC_BUFFER_COUNT = 16,
    BUFFER_MAX = 4
};


sfc_famicom_t* g_famicom = NULL;
static float s_buffer[SAMPLES_PER_FRAME_ALIGNED * (BASIC_BUFFER_COUNT + 1)];

/// <summary>
/// 由于各种外部原因需要额外插入一帧缓存
/// </summary>
void extra_buffer() {
    xa2_submit_buffer(s_buffer + SAMPLES_PER_FRAME_ALIGNED * BASIC_BUFFER_COUNT, SAMPLES_PER_FRAME);
}


/// <summary>
/// 每次重新开始播放, 提交预先缓存
/// </summary>
void buffer_the_sound() {
    xa2_flush_buffer();
    extra_buffer();
    extra_buffer();
}




extern void sfc_render_frame_easy(
    sfc_famicom_t* famicom, 
    uint8_t* buffer
);


struct interface_audio_state {
    uint32_t                        last_cycle;
    float                           square1_cycle;
    float                           square2_cycle;
    float                           triangle_cycle;
    float                           noise_cycle;
    float                           dmc_cycle;
    sfc_square_channel_state_t      square1;
    sfc_square_channel_state_t      square2;
    sfc_triangle_channel_state_t    triangle;
    sfc_noise_channel_state_t       noise;

    struct sfc_dmc_data_t           dmc;

    uint16_t                        lfsr;
    uint8_t                         square1_seq_index;
    uint8_t                         square2_seq_index;
    uint8_t                         triangle_seq_index;
    uint8_t                         dmc_enable;

} g_states;


static const uint8_t sq_seq_mask[] = {
    0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00,
    0x0f, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
};

static const uint8_t tri_seq[] = {
    15, 14, 13, 12, 11, 10,  9,  8,
    7,  6,  5,  4,  3,  2,  1,  0,
    0,  1,  2,  3,  4,  5,  6,  7,
    8,  9, 10, 11, 12, 13, 14, 15
};

static void do_lfsr_count(int count, uint8_t mode) {
    if (mode) {
        for (int i = 0; i != count; ++i) 
            g_states.lfsr = sfc_lfsr_short(g_states.lfsr);
    }
    else {
        for (int i = 0; i != count; ++i) 
            g_states.lfsr = sfc_lfsr_long(g_states.lfsr);
    }
}

// 读取PRG-ROM数据
extern inline uint8_t sfc_read_prgdata(uint16_t, const sfc_famicom_t*);

static void do_dmc() {
    // 禁止DMC
    if (!g_states.dmc_enable) return;
    // 还有DMC样本
    if (g_states.dmc.lenleft && !g_states.dmc.count) {
        g_states.dmc.count = 8;
        g_states.dmc.data = sfc_read_prgdata(g_states.dmc.curaddr, g_famicom);
        g_states.dmc.curaddr = (uint16_t)(g_states.dmc.curaddr + 1) | (uint16_t)0x8000;
        --g_states.dmc.lenleft;
    }
    // 还有比特数
    if (g_states.dmc.count) {
        --g_states.dmc.count;
        if (g_states.dmc.data & 1) {
            if (g_states.dmc.value <= 125)
                g_states.dmc.value += 2;
        }
        else {
            if (g_states.dmc.value >= 2)
                g_states.dmc.value -= 2;
        }
        g_states.dmc.data >>= 1;
        // 循环
        if ((g_states.dmc.irq_loop & 1) && !g_states.dmc.lenleft) {
            g_states.dmc.curaddr = g_states.dmc.orgaddr;
            g_states.dmc.lenleft = g_states.dmc.length;
        }
    }
}

// 生成样本
static void make_samples(const uint32_t begin, const uint32_t end) {
    if (begin >= end) return;
    /*
    output = square_out + tnd_out
    
                          95.88
    square_out = -----------------------
                        8128
                 ----------------- + 100
                 square1 + square2

                          159.79
    tnd_out = ------------------------------
                          1
              ------------------------ + 100
              triangle   noise    dmc
              -------- + ----- + -----
                8227     12241   22638
    */

    float* const buffer 
        = s_buffer 
        + SAMPLES_PER_FRAME_ALIGNED 
        *  (g_famicom->frame_counter % BASIC_BUFFER_COUNT)
        ;

    const float cpu_cycle_per_sample 
        = (float)NTSC_CPU_RATE 
        / (float)SAMPLES_PER_SEC
        ;
    assert(g_states.square1.period);
    assert(g_states.square2.period);
    assert(g_states.triangle.period);
    assert(g_states.noise.period);
    assert(g_states.dmc.period);

    const float square1p = g_states.square1.period;
    const float square2p = g_states.square2.period;
    const float trianglep = g_states.triangle.period;
    const float noise_p = g_states.noise.period;

    const float dmc_p = g_states.dmc.period;


    for (uint32_t i = begin; i != end; ++i) {
        // 方波#1, APU频率驱动
        {
            g_states.square1_cycle += cpu_cycle_per_sample * 0.5f;
            const int count1 = (int)(g_states.square1_cycle / square1p);
            g_states.square1_cycle -= (float)count1 * square1p;
            g_states.square1_seq_index += (uint8_t)count1;
            g_states.square1_seq_index = g_states.square1_seq_index & 7;
        }
        const float square1
            = g_states.square1.volume 
            & sq_seq_mask[g_states.square1_seq_index | (g_states.square1.duty << 3)]
            ;
        // 方波#2, APU频率驱动
        {
            g_states.square2_cycle += cpu_cycle_per_sample * 0.5f;
            const int count2 = (int)(g_states.square2_cycle / square2p);
            g_states.square2_cycle -= (float)count2 * square2p;
            g_states.square2_seq_index += (uint8_t)count2;
            g_states.square2_seq_index = g_states.square2_seq_index & 7;
        }
        const float square2
            = g_states.square2.volume
            & sq_seq_mask[g_states.square2_seq_index | (g_states.square2.duty << 3)]
            ;
        // 三角波, CPU频率驱动
        {
            g_states.triangle_cycle += cpu_cycle_per_sample;
            const int count3 = (int)(g_states.triangle_cycle / trianglep);
            g_states.triangle_cycle -= (float)count3 * trianglep;
            g_states.triangle_seq_index 
                += g_states.triangle.inc_mask 
                & (uint8_t)count3
                ;
            g_states.triangle_seq_index = g_states.triangle_seq_index & 31;
        }
        const float triangle 
            = tri_seq[g_states.triangle_seq_index]
            & g_states.triangle.play_mask
            //& 0
            ;
        // 噪声, CPU频率驱动?
        {
            g_states.noise_cycle += cpu_cycle_per_sample ;
            const int count4 = (int)(g_states.noise_cycle / noise_p);
            g_states.noise_cycle -= (float)count4 * noise_p;
            do_lfsr_count(count4, g_states.noise.mode);
        }
        uint8_t mask = (uint8_t)(g_states.lfsr & 1); --mask;
        const float noise 
            = g_states.noise.volume
            & mask
            //& 0
            ;
        // DMC, CPU频率驱动
        {
            g_states.dmc_cycle += cpu_cycle_per_sample;
            // 最高是33kHz, 目前采样率是44.1kHz, 能保证每样本最多一次
            // 如果采样率低于33kHz, 则可能需要每次采样多次
            if (g_states.dmc_cycle > dmc_p) {
                g_states.dmc_cycle -= dmc_p;
                do_dmc();
            }
        }
        const float dmc = g_states.dmc.value;

        const float square_out = 95.88f / ((8128.f / (square1 + square2)) + 100.f);
        const float tnd_out 
            = 159.79f 
            / (1.f / (triangle / 8227.f + noise / 12241.f + dmc / 22638.f) + 100.f)
            ;
        const float output 
            = square_out
            //* 0.f
            + tnd_out
            //* 0.f
            ;
        buffer[i] = output 
            //* 2.f
            ;
    }
}


/// <summary>
/// 音频事件接口
/// </summary>
/// <param name="arg">The argument.</param>
/// <param name="cycle">The cycle.</param>
/// <param name="type">The type.</param>
static void this_audio_event(void* arg, uint32_t cycle, int type) {

    const uint32_t old_index = g_states.last_cycle * SAMPLES_PER_SEC / NTSC_CPU_RATE;
    const uint32_t now_index = cycle * SAMPLES_PER_SEC / NTSC_CPU_RATE;
    // 将目前的状态覆盖 区间[old_index, now_index)
    make_samples(old_index, now_index);
    g_states.last_cycle = cycle;


    switch (type)
    {
    case 0:
        g_states.dmc = g_famicom->apu.dmc;
        g_states.dmc_enable = g_famicom->apu.status_write & SFC_APU4015_WRITE_EnableDMC;
    case 6:
        g_states.square1.u32 = sfc_check_square1_state(g_famicom).u32;
        g_states.square2.u32 = sfc_check_square2_state(g_famicom).u32;
        g_states.triangle.u32 = sfc_check_triangle_state(g_famicom).u32;
        g_states.noise.u32 = sfc_check_noise_state(g_famicom).u32;
        break;
    case 1:
        g_states.square1.u32 = sfc_check_square1_state(g_famicom).u32;
        // 写入了$4003
        if (!g_famicom->apu.square1.seq_index) {
            g_famicom->apu.square1.seq_index = 1;
            g_states.square1_seq_index = 0;
        }
        break;
    case 2:
        g_states.square2.u32 = sfc_check_square2_state(g_famicom).u32;
        // 写入了$4004
        if (!g_famicom->apu.square2.seq_index) {
            g_famicom->apu.square2.seq_index = 1;
            g_states.square2_seq_index = 0;
        }
        break;
    case 3:
        g_states.triangle.u32 = sfc_check_triangle_state(g_famicom).u32;
        break;
    case 4:
        g_states.noise.u32 = sfc_check_noise_state(g_famicom).u32;
        break;
    case 5:
        g_states.dmc = g_famicom->apu.dmc;
        break;
    }
    // 替换当前的DMC
    g_famicom->apu.dmc = g_states.dmc;
}

// Windows API
extern void _stdcall Sleep(unsigned);

/// <summary>
/// 提交当前缓冲区
/// </summary>
void submit_now_buffer() {
    float* const buffer
        = s_buffer
        + SAMPLES_PER_FRAME_ALIGNED
        * (g_famicom->frame_counter % BASIC_BUFFER_COUNT)
        ;
    d2d_submit_wave(buffer, SAMPLES_PER_FRAME);
    xa2_submit_buffer(buffer, SAMPLES_PER_FRAME);
}

/// <summary>
/// 播放该帧音频
/// </summary>
static void play_audio() {
    const unsigned left = xa2_buffer_left();
    // 太低
    if (!left) {
        // 可以跳过视频帧, 这里直接加入空白音频帧
        extra_buffer();
        printf("<buffer empty>\n");
    }
    // 太高
    else if (left > BUFFER_MAX) {
        printf("<buffer overflow[%d]>\n", left);
        // 可以让图像接口空等一两帧, 这里直接睡过去
        Sleep(30);
        // 这里还能直接return跳过该音频帧
        //return;
    }
    // 收尾
    const uint32_t old_index = g_states.last_cycle * SAMPLES_PER_SEC / NTSC_CPU_RATE;
    make_samples(old_index, SAMPLES_PER_FRAME);
    g_states.last_cycle = 0;
    // 提交当前缓存
    submit_now_buffer();
}


// 标准调色板
extern uint32_t sfc_stdpalette[];

/// <summary>
/// 主渲染
/// </summary>
/// <param name="rgba">The RGBA.</param>
extern void main_render(void* rgba) {
    uint8_t buffer[(256+8) * 256];
    sfc_render_frame_easy(g_famicom, buffer);

    // 生成调色板数据
    uint32_t palette[32];
    
    for (int i = 0; i != 32; ++i)
        palette[i] = sfc_stdpalette[g_famicom->ppu.data.spindexes[i]];
    // 镜像数据
    palette[4 * 1] = palette[0];
    palette[4 * 2] = palette[0];
    palette[4 * 3] = palette[0];
    palette[4 * 4] = palette[0];
    palette[4 * 5] = palette[0];
    palette[4 * 6] = palette[0];
    palette[4 * 7] = palette[0];

    uint32_t* data = rgba;
    for (int y = 0; y != 240; ++y) {
        for (int x = 0; x != 256; ++x) {
            *data = palette[buffer[y*(256+8)+x] >> 1];
            ++data;
        }
    }

    play_audio();
}


// SRAM保存用文件头
const char SFC_SRAM_THIS_HEADER[16] = "-StepFC-SRAMWRAM";

/// <summary>
/// 生成文件
/// </summary>
/// <param name="buffer">The buffer.</param>
static void sfc_make_file_name(
    const sfc_rom_info_t* info, 
    const char* folder, 
    char buffer[256]) {
    snprintf(buffer, 256, "%s/%08X.sfc", folder, info->prgrom_crc32b);
}

/// <summary>
/// Thises the load sram.
/// </summary>
/// <param name="arg">The argument.</param>
/// <param name="info">The information.</param>
/// <param name="ptr">The PTR.</param>
void this_load_sram(void*arg, const sfc_rom_info_t* info, uint8_t* ptr) {
    char buffer[256]; sfc_make_file_name(info, "save", buffer);
    FILE* const file = fopen(buffer, "rb");
    if (!file) return;
    char header[sizeof(SFC_SRAM_THIS_HEADER)];
    // 成功读取
    if (fread(header, sizeof(header), 1, file)) {
        // 检测文件头
        if (!memcmp(header, SFC_SRAM_THIS_HEADER, sizeof(header))) {
            const size_t len = sizeof(((sfc_famicom_t*)0)->save_memory);
            const size_t read_count = fread(ptr, len, 1, file);
            //int bk = 9;
        }
    }
    fclose(file);
}

#if defined _MSC_VER
#include <direct.h>
#elif defined __GNUC__
#include <sys/types.h>
#include <sys/stat.h>
#endif

/// <summary>
/// SFCs the create dir.
/// </summary>
/// <param name="dir">The dir.</param>
void sfc_create_dir(const char* dir) {
#if defined _MSC_VER
    _mkdir(dir);
#elif defined __GNUC__
    mkdir(dir/*, 0777*/);
#endif
}

/// <summary>
/// Thises the save sram.
/// </summary>
/// <param name="arg">The argument.</param>
/// <param name="info">The information.</param>
/// <param name="ptr">The PTR.</param>
void this_save_sram(void*arg, const sfc_rom_info_t* info, const uint8_t* ptr) {
    sfc_create_dir("save");
    char buffer[256]; sfc_make_file_name(info, "save", buffer);
    FILE* const file = fopen(buffer, "wb");
    if (!file) return;
    const size_t len = sizeof(((sfc_famicom_t*)0)->save_memory);
    fwrite(SFC_SRAM_THIS_HEADER, sizeof(SFC_SRAM_THIS_HEADER), 1, file);
    const size_t count = fwrite(ptr, len, 1, file);
    fclose(file);
}

void sfc_sl_write_stream(void*, const uint8_t*, uint32_t);
void sfc_sl_read_stream(void*, uint8_t*, uint32_t);

/// <summary>
/// 应用程序入口
/// </summary>
/// <returns></returns>
int main() {
    //void make_mario_jump(); make_mario_jump(); return 0;
    memset(&g_states, 0, sizeof(g_states));
    g_states.square1.period = 1;
    g_states.square2.period = 1;
    g_states.triangle.period = 1;
    g_states.noise.period = 4;
    g_states.lfsr = 1;

    g_states.dmc.period = 500;


    memset(s_buffer, 0, sizeof(s_buffer));
    sfc_interface_t interfaces = { NULL };

    interfaces.audio_changed = this_audio_event;
    interfaces.load_sram = this_load_sram;
    interfaces.save_sram = this_save_sram;
    interfaces.sl_write_stream = sfc_sl_write_stream;
    interfaces.sl_read_stream = sfc_sl_read_stream;

    sfc_famicom_t famicom;
    g_famicom = &famicom;
    if (sfc_famicom_init(&famicom, NULL, &interfaces)) return 1;
    //qload();

    printf(
        "ROM: PRG-ROM: %d x 16kb   CHR-ROM %d x 8kb   Mapper: %03d\n",
        (int)famicom.rom_info.count_prgrom16kb,
        (int)famicom.rom_info.count_chrrom_8kb,
        (int)famicom.rom_info.mapper_number
    );
    xa2_init(SAMPLES_PER_SEC);
    buffer_the_sound();
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

/// <summary>
/// 接口: 写入流
/// </summary>
/// <param name="arg">The argument.</param>
/// <param name="data">The data.</param>
/// <param name="len">The length.</param>
void sfc_sl_write_stream(void* arg, const uint8_t* data, uint32_t len) {
    FILE* const file = arg;
    // TODO: 错误处理
    fwrite(data, len, 1, file);
}

/// <summary>
/// 接口: 从流读
/// </summary>
/// <param name="arg">The argument.</param>
/// <param name="data">The data.</param>
/// <param name="len">The length.</param>
void sfc_sl_read_stream(void* arg, uint8_t* data, uint32_t len) {
    FILE* const file = arg;
    // TODO: 错误处理
    fread(data, len, 1, file);
}

/// <summary>
/// 快速存档
/// </summary>
void qsave() {
    sfc_create_dir("qsave");
    const sfc_rom_info_t* const info = &g_famicom->rom_info;
    char buffer[256]; sfc_make_file_name(info, "qsave", buffer);
    FILE* const file = fopen(buffer, "wb");
    if (!file) return;
    void* const old_arg = g_famicom->argument;
    g_famicom->argument = file;
    sfc_famicom_save_state(g_famicom);
    g_famicom->argument = old_arg;
    fclose(file);
}

/// <summary>
/// 快速读档
/// </summary>
void qload() {
    const sfc_rom_info_t* const info = &g_famicom->rom_info;
    char buffer[256]; sfc_make_file_name(info, "qsave", buffer);
    FILE* const file = fopen(buffer, "rb");
    if (!file) return;
    void* const old_arg = g_famicom->argument;
    g_famicom->argument = file;
    sfc_famicom_load_state(g_famicom);
    g_famicom->argument = old_arg;
    fclose(file);
}

