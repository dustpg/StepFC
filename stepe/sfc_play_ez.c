#include "sfc_famicom.h"
#include "sfc_play.h"
#include <assert.h>
#include <string.h>

/// <summary>
/// StepFC: 方波音量掩码
/// </summary>
static const uint8_t sfc_sq_seq_mask[] = {
    0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00,
    0x0f, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
};

/// <summary>
/// SFCs the state of the check square1.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
sfc_square_channel_state_t sfc_check_square1_state(const sfc_famicom_t* famicom) {
    sfc_square_channel_state_t state = { 0, 0, 0 };
    // 方波#1
    const uint16_t square1period = famicom->apu.square1.cur_period;
    state.period = square1period + 1;//  == 0xffff ? 0xffff : square1period + 1;
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
    state.period = square2period + 1;// == 0xffff ? 0xffff : square2period + 1;
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

// ----------------------------------------------------------------------------
//                              MMC5
// ----------------------------------------------------------------------------


/// <summary>
/// SFCs the state of the check square.
/// </summary>
/// <param name="square">The square.</param>
/// <param name="flag">The flag.</param>
static sfc_square_ch_state_t sfc_check_square_state_mmc5(
    sfc_square_data_t* square) {
    // 初始基础状态
    sfc_square_ch_state_t state;
    state.duty = 0;
    state.volume = 0;
    // 写入方波真正周期
    const uint16_t square1period = square->cur_period;
    state.period = square1period + 1;//  == 0xffff ? 0xffff : square1period + 1;
    // 使能
    if (!square->unused__mmc5_5015) return state;
    // 长度计数器为0
    if (square->length_counter == 0) return state;
    // 输出频率
    if (square1period == 0 || square1period > 0x7ff) return state;
    // 计算占空比并移动到D4D3位
    state.duty = (square->ctrl & 0xc0) >> 3;
    // 固定音量
    if (square->envelope.ctrl6 & (SFC_APUCTRL6_ConstVolume))
        state.volume = square->envelope.ctrl6 & 0xf;
    // 包络音量
    else
        state.volume = square->envelope.counter;
    // 返回最终状态
    return state;
}


/// <summary>
/// SFCs the MMC5 per sample.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
/// <param name="cps">The CPS.</param>
void sfc_mmc5_per_sample(sfc_famicom_t* famicom, sfc_mmc5_ctx_t* ctx, float cps) {
    sfc_mmc5_data_t* const mmc5 = &famicom->apu.mmc5;
    {
        // #1 方波 APU频率驱动
        ctx->squ1.clock += cps * 0.5f;
        // 推进次数
        const int count = (int)(ctx->squ1.clock / ctx->squ1.period);
        ctx->squ1.clock -= (float)count *  ctx->squ1.period;
        mmc5->square1.seq_index += (uint8_t)count;
        mmc5->square1.seq_index &= 7;
        // 计算输出
        const uint8_t v4
            = ctx->squ1.state.volume
            & sfc_sq_seq_mask[mmc5->square1.seq_index | ctx->squ1.state.duty]
            ;
        ctx->squ1.output = (float)v4;
    }
    {

        //static float dbg_squp = 0;
        //if (dbg_squp != ctx->squ2.period) {
        //    dbg_squp = ctx->squ2.period;
        //    printf("[%4d]P: %f\n", famicom->frame_counter, dbg_squp);
        //}

        //static uint8_t dbg_squv = 0;
        //if (dbg_squv != ctx->squ2.state.volume) {
        //    dbg_squv = ctx->squ2.state.volume;
        //    printf("[%4d]V = %d\n", famicom->frame_counter, dbg_squv);
        //}


        // #2 方波 APU频率驱动
        ctx->squ2.clock += cps * 0.5f;
        // 推进次数
        const int count = (int)(ctx->squ2.clock / ctx->squ2.period);
        ctx->squ2.clock -= (float)count *  ctx->squ2.period;
        mmc5->square2.seq_index += (uint8_t)count;
        mmc5->square2.seq_index &= 7;
        // 计算输出
        const uint8_t v4
            = ctx->squ2.state.volume
            & sfc_sq_seq_mask[mmc5->square2.seq_index | ctx->squ2.state.duty]
            ;
        ctx->squ2.output = (float)v4;
    }
}

/// <summary>
/// SFCs the MMC5 samplemode begin.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
void sfc_mmc5_samplemode_begin(sfc_famicom_t* famicom, sfc_mmc5_ctx_t* ctx) {
    sfc_mmc5_data_t* const mmc5 = &famicom->apu.mmc5;

    ctx->squ1.clock = (float)mmc5->square1_clock;
    ctx->squ1.state = sfc_check_square_state_mmc5(&mmc5->square1);
    assert(ctx->squ1.state.period);
    ctx->squ1.period = (float)ctx->squ1.state.period;

    ctx->squ2.clock = (float)mmc5->square2_clock;
    ctx->squ2.state = sfc_check_square_state_mmc5(&mmc5->square2);
    assert(ctx->squ2.state.period);
    ctx->squ2.period = (float)ctx->squ2.state.period;

}


/// <summary>
/// SFCs the MMC5 samplemode end.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
void sfc_mmc5_samplemode_end(sfc_famicom_t* famicom, sfc_mmc5_ctx_t* ctx) {
    famicom->apu.mmc5.square1_clock = (uint16_t)ctx->squ1.clock;
    famicom->apu.mmc5.square2_clock = (uint16_t)ctx->squ2.clock;
}


