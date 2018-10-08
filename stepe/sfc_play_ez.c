#include "sfc_famicom.h"
#include "sfc_play.h"
#include <assert.h>
#include <string.h>


/// <summary>
/// SFCs the state of the check square1.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
sfc_square_channel_state_t sfc_check_square1_state(const sfc_famicom_t* famicom) {
    sfc_square_channel_state_t state = { 0, 0, 0 };
    // 方波#1
    const uint16_t square1period = famicom->apu.square1.cur_period;
    state.period = square1period + 1;
    // 使能
    if (!(famicom->apu.status_write & SFC_APU4015_WRITE_EnableSquare1)) return state;
    // 长度计数器为0
    if (famicom->apu.square1.length_counter == 0) return state;
    // 输出频率
    if (square1period < 8 || square1period > 0x7ff) return state;

    state.duty = famicom->apu.square1.ctrl >> 6;
    // 固定音量
    if (famicom->apu.square1.envelope.ctrl6 & (SFC_APUCTRL6_ConstVolume))
        state.volume = famicom->apu.square1.envelope.ctrl6 & 0xf;
    // 包络音量
    else
        state.volume = famicom->apu.square1.envelope.counter;
    return state;
}

/// <summary>
/// SFCs the state of the check square2.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
sfc_square_channel_state_t sfc_check_square2_state(const sfc_famicom_t* famicom) {
    sfc_square_channel_state_t state = { 0, 0, 0 };
    // 方波#2
    const uint16_t square2period = famicom->apu.square2.cur_period;
    state.period = square2period + 1;
    // 使能
    if (!(famicom->apu.status_write & SFC_APU4015_WRITE_EnableSquare2)) return state;
    // 长度计数器为0
    if (famicom->apu.square2.length_counter == 0) return state;
    // 输出频率
    if (square2period < 8 || square2period > 0x7ff) return state;

    state.duty = famicom->apu.square2.ctrl >> 6;
    // 固定音量
    if (famicom->apu.square2.envelope.ctrl6 & (SFC_APUCTRL6_ConstVolume))
        state.volume = famicom->apu.square2.envelope.ctrl6 & 0xf;
    // 包络音量
    else
        state.volume = famicom->apu.square2.envelope.counter;
    return state;
}


/// <summary>
/// SFCs the state of the check triangle.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
sfc_triangle_channel_state_t sfc_check_triangle_state(const sfc_famicom_t* famicom) {
    sfc_triangle_channel_state_t state;
    // 周期
    state.period = famicom->apu.triangle.cur_period + 1;
    // 播放掩码: 使能位
    state.play_mask
        = famicom->apu.status_write & SFC_APU4015_WRITE_EnableTriangle
        ? 0x0F
        : 0
        ;
    // 递增掩码: 长度计数器/线性计数器 有效
    state.inc_mask
        = (famicom->apu.triangle.length_counter && famicom->apu.triangle.linear_counter)
        ? 0xFF
        : 0
        ;
    return state;
}

// 查找表
static const uint16_t SFC_NOISE_PERIOD_LIST[] = {
    // NTSC
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,
    // PAL
    4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778
};

/// <summary>
/// SFCs the state of the check noise.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
sfc_noise_channel_state_t sfc_check_noise_state(const sfc_famicom_t* famicom) {
    sfc_noise_channel_state_t state = { 0, 0, 0 };
    // 短模式
    state.mode = famicom->apu.noise.short_mode__period_index >> 7;
    // 周期
    // TODO: PAL的场合
    state.period = SFC_NOISE_PERIOD_LIST[famicom->apu.noise.short_mode__period_index & 0xF];
    // 使能
    if (famicom->apu.status_write & SFC_APU4015_WRITE_EnableNoise) {
        // 长度计数器有效
        if (famicom->apu.noise.length_counter) {
            // 固定音量
            if (famicom->apu.noise.envelope.ctrl6 & (SFC_APUCTRL6_ConstVolume))
                state.volume = famicom->apu.noise.envelope.ctrl6 & 0xf;
            // 包络音量
            else
                state.volume = famicom->apu.noise.envelope.counter;
        }
    }
    return state;
}

