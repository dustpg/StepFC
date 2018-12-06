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
/// StepFC: 三角波澜序列
/// </summary>
static const uint8_t sfc_tri_seq[] = {
    15, 14, 13, 12, 11, 10,  9,  8,
    7,  6,  5,  4,  3,  2,  1,  0,
    0,  1,  2,  3,  4,  5,  6,  7,
    8,  9, 10, 11, 12, 13, 14, 15
};


/// <summary>
/// SFCs the state of the check square.
/// </summary>
/// <param name="square">The square.</param>
/// <param name="flag">The flag.</param>
static sfc_square_ch_state_t sfc_check_square_state(
    const sfc_square_data_t* square, uint16_t min) {
    // 初始基础状态
    sfc_square_ch_state_t state;
    state.duty = 0;
    state.volume = 0;
    // 写入方波真正周期
    const uint16_t square1period = square->cur_period;
    state.period_x2 = (square1period + 1) * 2;
    // 使能
    if (!square->x015_flag) return state;
    // 长度计数器为0
    if (square->length_counter == 0) return state;
    // 输出频率
    if (square1period <= min || square1period > 0x7ff) return state;
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
/// SFCs the state of the check triangle.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static sfc_triangle_ch_state_t sfc_check_triangle_state(const sfc_famicom_t* famicom) {
    sfc_triangle_ch_state_t state;
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
static sfc_noise_ch_state_t sfc_check_noise_state(const sfc_famicom_t* famicom) {
    sfc_noise_ch_state_t state = { 0, 0, 0 };
    // 短模式
    state.count = famicom->apu.noise.short_mode__period_index & 0x80 ? 6 : 1;
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
//                              2A03
// ----------------------------------------------------------------------------


// 读取PRG-ROM数据
extern inline uint8_t sfc_read_prgdata(uint16_t, const sfc_famicom_t*);


/// <summary>
/// StepFC: 处理DMC声道
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_do_dmc(sfc_famicom_t* famicom) {
    // 禁止DMC
    if (!(famicom->apu.status_write & SFC_APU4015_WRITE_EnableDMC)) return;
    sfc_dmc_data_t* const dmc = &famicom->apu.dmc;
    // 还有DMC样本
    if (dmc->lenleft && !dmc->count) {
        dmc->count = 8;
        // TODO: 花费时钟
        dmc->data = sfc_read_prgdata(dmc->curaddr, famicom);
        dmc->curaddr = (uint16_t)(dmc->curaddr + 1) | (uint16_t)0x8000;
        --dmc->lenleft;
    }
    // 还有比特数
    if (dmc->count) {
        --dmc->count;
        if (dmc->data & 1) {
            if (dmc->value <= 125) dmc->value += 2;
        }
        else {
            if (dmc->value >= 2) dmc->value -= 2;
        }
        // 位移寄存器
        dmc->data >>= 1;
        // 循环
        if ((dmc->irq_loop & 1) && !dmc->lenleft) {
            dmc->curaddr = dmc->orgaddr;
            dmc->lenleft = dmc->length;
        }
    }
}


/// <summary>
/// SteFC: 2A03 整型采样模式 - 采样
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
/// <param name="chw">The CHW.</param>
/// <param name="cps">The CPS.</param>
void sfc_2a03_smi_sample(sfc_famicom_t* famicom, sfc_2a03_smi_ctx_t* ctx, const float chw[], sfc_fixed_t cps) {
    sfc_apu_register_t* const apu = &famicom->apu;
    // 方波#1 CPU/2频率驱动
    {
        const uint16_t clock = sfc_fixed_add(&ctx->sq1_clock, cps);
        apu->square1_clock += clock;
        const uint16_t count = apu->square1_clock / ctx->sq1_state.period_x2;
        apu->square1_clock -= count * ctx->sq1_state.period_x2;
        apu->square1.seq_index += (uint8_t)count;
        apu->square1.seq_index &= 7;
        // 计算输出
        const uint8_t v4
            = ctx->sq1_state.volume
            & sfc_sq_seq_mask[apu->square1.seq_index | ctx->sq1_state.duty]
            ;
        ctx->sq1_output = (float)v4 * chw[0];
    }
    // 方波#2 CPU/2频率驱动
    {
        const uint16_t clock = sfc_fixed_add(&ctx->sq2_clock, cps);
        apu->square2_clock += clock;
        const uint16_t count = apu->square2_clock / ctx->sq2_state.period_x2;
        apu->square2_clock -= count * ctx->sq2_state.period_x2;
        apu->square2.seq_index += (uint8_t)count;
        apu->square2.seq_index &= 7;
        // 计算输出
        const uint8_t v4
            = ctx->sq2_state.volume
            & sfc_sq_seq_mask[apu->square2.seq_index | ctx->sq2_state.duty]
            ;
        ctx->sq2_output = (float)v4 * chw[1];
    }
    // 三角波 CPU 频率驱动
    {
        const uint16_t clock = sfc_fixed_add(&ctx->tri_clock, cps);
        apu->triangle_clock += clock;
        const uint16_t count = apu->triangle_clock / ctx->tri_state.period;
        apu->triangle_clock -= count * ctx->tri_state.period;
        const uint8_t inc = ctx->tri_state.inc_mask & (uint8_t)count;
        apu->triangle.seq_index += inc;
        apu->triangle.seq_index &= 31;
        const uint8_t data = sfc_tri_seq[apu->triangle.seq_index];
        ctx->tri_output = (float)data * chw[2];
    }
    // 噪音 CPU 频率驱动?
    {
        const uint16_t clock = sfc_fixed_add(&ctx->noi_clock, cps);
        apu->noise_clock += clock;
        uint16_t* const lfsr = &apu->noise.lfsr;
#if 1
        const uint16_t period = ctx->noi_state.period;
        while (apu->noise_clock >= period) {
            apu->noise_clock -= period;
            *lfsr = sfc_lfsr_2a03(*lfsr, ctx->noi_state.count);
        }
#else
        uint16_t count = apu->noise_clock / ctx->noi_state.period;
        apu->noise_clock -= count * ctx->noi_state.period;
        while (count--) *lfsr = sfc_lfsr_2a03(*lfsr, ctx->noi_state.count);
#endif
        // 为0输出
        uint8_t mask = *lfsr & 1; --mask;
        const uint8_t vol = ctx->noi_state.volume & mask;
        ctx->noi_output = (float)vol * chw[3];
    }
    // DMC
    {
        const uint16_t clock = sfc_fixed_add(&ctx->dmc_clock, cps);
        apu->dmc.clock += clock;
        const uint16_t period = famicom->apu.dmc.period;
        while (apu->dmc.clock <= period) {
            apu->dmc.clock -= period;
            sfc_do_dmc(famicom);
        }
        ctx->dmc_output = (float)apu->dmc.value * chw[4];
    }
}

/// <summary>
/// SteFC: 2A03 整型采样模式 - 更新方波#1
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
void sfc_2a03_smi_update_sq1(const sfc_famicom_t* famicom, sfc_2a03_smi_ctx_t* ctx) {
    ctx->sq1_state = sfc_check_square_state(&famicom->apu.square1, 7);
}

/// <summary>
/// SteFC: 2A03 整型采样模式 - 更新方波#2
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
void sfc_2a03_smi_update_sq2(const sfc_famicom_t* famicom, sfc_2a03_smi_ctx_t* ctx) {
    ctx->sq2_state = sfc_check_square_state(&famicom->apu.square2, 7);
}

/// <summary>
/// StepFC: 2A03 整型采样模式 - 更新三角波
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
void sfc_2a03_smi_update_tri(const sfc_famicom_t* famicom, sfc_2a03_smi_ctx_t* ctx) {
    ctx->tri_state = sfc_check_triangle_state(famicom);
}

/// <summary>
/// StepFC: 2A03 整型采样模式 - 更新噪音
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
void sfc_2a03_smi_update_noi(const sfc_famicom_t* famicom, sfc_2a03_smi_ctx_t* ctx) {
    ctx->noi_state = sfc_check_noise_state(famicom);
}



// ----------------------------------------------------------------------------
//                              MMC5
// ----------------------------------------------------------------------------


#ifdef SFC_SMF_DEFINED
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


#endif

/// <summary>
/// SteFC: MMC5 整型采样模式 - 采样
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
/// <param name="chw">The CHW.</param>
/// <param name="cps">The CPS.</param>
void sfc_mmc5_smi_sample(sfc_famicom_t* famicom, sfc_mmc5_smi_ctx_t* ctx, const float chw[], sfc_fixed_t cps) {
    sfc_mmc5_data_t* const mmc5 = &famicom->apu.mmc5;
    // 方波#1 CPU/2频率驱动
    {
        const uint16_t clock = sfc_fixed_add(&ctx->sq1_clock, cps);
        mmc5->square1_clock += clock;
        const uint16_t count = mmc5->square1_clock / ctx->sq1_state.period_x2;
        mmc5->square1_clock -= count * ctx->sq1_state.period_x2;
        mmc5->square1.seq_index += (uint8_t)count;
        mmc5->square1.seq_index &= 7;
        // 计算输出
        const uint8_t v4
            = ctx->sq1_state.volume
            & sfc_sq_seq_mask[mmc5->square1.seq_index | ctx->sq1_state.duty]
            ;
        ctx->sq1_output = (float)v4 * chw[0];
    }
    // 方波#2 CPU/2频率驱动
    {
        const uint16_t clock = sfc_fixed_add(&ctx->sq2_clock, cps);
        mmc5->square2_clock += clock;
        const uint16_t count = mmc5->square2_clock / ctx->sq2_state.period_x2;
        mmc5->square2_clock -= count * ctx->sq2_state.period_x2;
        mmc5->square2.seq_index += (uint8_t)count;
        mmc5->square2.seq_index &= 7;
        // 计算输出
        const uint8_t v4
            = ctx->sq2_state.volume
            & sfc_sq_seq_mask[mmc5->square2.seq_index | ctx->sq2_state.duty]
            ;
        ctx->sq2_output = (float)v4 * chw[1];
    }
    // PCM
    ctx->pcm_output = (float)mmc5->pcm_output * chw[2];
}

/// <summary>
/// SteFC: MMC5 整型采样模式 - 更新方波#1
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
void sfc_mmc5_smi_update_sq1(const sfc_famicom_t* famicom, sfc_mmc5_smi_ctx_t* ctx) {
    ctx->sq1_state = sfc_check_square_state(&famicom->apu.mmc5.square1, 0);
}

/// <summary>
/// SteFC: MMC5 整型采样模式 - 更新方波#2
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="ctx">The CTX.</param>
void sfc_mmc5_smi_update_sq2(const sfc_famicom_t* famicom, sfc_mmc5_smi_ctx_t* ctx) {
    ctx->sq2_state = sfc_check_square_state(&famicom->apu.mmc5.square2, 0);
}
