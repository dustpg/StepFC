#include "sfc_famicom.h"
#include "sfc_play.h"
#include <assert.h>
#include <string.h>


/// <summary>
/// SFCs the play square1.
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_play_square1(
    const sfc_famicom_t* famicom,
    struct sfc_square_channel_state_t* state) {
    // 使能
    if (!(famicom->apu.status_write & SFC_APU4015_WRITE_EnableSquare1)) return;
    // 长度计数器为0
    if (famicom->apu.square1.length_counter == 0) return;
    // 方波#1
    const uint16_t square1period = famicom->apu.square1.use_period;
    // 输出频率
    if (square1period < 8 || square1period > 0x7ff) return;
    state->frequency = famicom->config.cpu_clock / 16.f / (float)(square1period + 1);
    state->duty = famicom->apu.square1.ctrl >> 6;
    // 固定音量
    if (famicom->apu.square1.envelope.ctrl6 & (SFC_APUCTRL6_ConstVolume))
        state->volume = famicom->apu.square1.envelope.ctrl6 & 0xf;
    // 包络音量
    else
        state->volume = famicom->apu.square1.envelope.counter;
}

/// <summary>
/// SFCs the play square2
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_play_square2(
    const sfc_famicom_t* famicom,
    struct sfc_square_channel_state_t* state) {
    // 使能
    if (!(famicom->apu.status_write & SFC_APU4015_WRITE_EnableSquare2)) return;
    // 长度计数器为0
    if (famicom->apu.square2.length_counter == 0) return;
    // 方波#1
    const uint16_t square2period = famicom->apu.square2.use_period;
    // 输出频率
    if (square2period < 8 || square2period > 0x7ff) return;
    state->frequency = famicom->config.cpu_clock / 16.f / (float)(square2period + 1);
    state->duty = famicom->apu.square2.ctrl >> 6;
    // 固定音量
    if (famicom->apu.square2.envelope.ctrl6 & (SFC_APUCTRL6_ConstVolume))
        state->volume = famicom->apu.square2.envelope.ctrl6 & 0xf;
    // 包络音量
    else
        state->volume = famicom->apu.square2.envelope.counter;
}


/// <summary>
/// SFCs the play triangle.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="state">The state.</param>
static void sfc_play_triangle(
    const sfc_famicom_t* famicom,
    struct sfc_triangle_channel_state_t* state) {
    // 使能
    if (!(famicom->apu.status_write & SFC_APU4015_WRITE_EnableTriangle)) return;
    // 长度计数器为0
    if (famicom->apu.triangle.length_counter == 0) return;
    // 线性计数器为0
    if (famicom->apu.triangle.linear_counter == 0) return;
    // 输出频率
    state->frequency = famicom->config.cpu_clock / 32.f /
        (float)(famicom->apu.triangle.cur_period + 1);
}

/// <summary>
/// SFCs the play noise.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="state">The state.</param>
static void sfc_play_noise(
    const sfc_famicom_t* famicom,
    struct sfc_noise_channel_state_t* state) {
    // 使能
    if (!(famicom->apu.status_write & SFC_APU4015_WRITE_EnableNoise)) return;
    // 长度计数器为0
    if (famicom->apu.noise.length_counter == 0) return;
    // 数据
    state->data = famicom->apu.noise.short_mode__period_index;
    // 固定音量
    if (famicom->apu.noise.envelope.ctrl6 & (SFC_APUCTRL6_ConstVolume))
        state->volume = famicom->apu.noise.envelope.ctrl6 & 0xf;
    // 包络音量
    else
        state->volume = famicom->apu.noise.envelope.counter;
}


/// <summary>
/// SFCs the play audio easy.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="state">The state.</param>
void sfc_play_audio_easy(
    const sfc_famicom_t* famicom,
    sfc_channel_state_t* state) {
    memset(state, 0, sizeof(*state));
    // 方波#1
    sfc_play_square1(famicom, &state->square1);
    // 方波#1
    sfc_play_square2(famicom, &state->square2);
    // 三角波
    sfc_play_triangle(famicom, &state->triangle);
    // 噪声
    sfc_play_noise(famicom, &state->noise);
}