#include "sfc_6502.h"
#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include <assert.h>
#include <string.h>

#define sfc__fallthrough

// REF: http://nesdev.com/apu_ref.txt
// 长度计数器映射表
static const LENGTH_COUNTER_TABLE[] = {
    0x0A, 0xFE, 0x14, 0x02,
    0x28, 0x04, 0x50, 0x06,
    0xA0, 0x08, 0x3C, 0x0A,
    0x0E, 0x0C, 0x1A, 0x0E,
    0x0C, 0x10, 0x18, 0x12,
    0x30, 0x14, 0x60, 0x16,
    0xC0, 0x18, 0x48, 0x1A,
    0x10, 0x1C, 0x20, 0x1E,
};


// IRQ - 中断请求 - 确认
extern inline void sfc_operation_IRQ_acknowledge(sfc_famicom_t* famicom);

// 重启DMC
static inline void sfc_restart_dmc(sfc_famicom_t* famicom) {
    famicom->apu.dmc.curaddr = famicom->apu.dmc.orgaddr;
    famicom->apu.dmc.lenleft = famicom->apu.dmc.length;
    //printf("<sfc_restart_dmc>");
}


/// <summary>
/// The SFC DMC period list NTSC
/// </summary>
static const uint16_t SFC_DMC_PERIOD_LIST_NP[] = {
    // NTSC
    428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54,
    // PAL
    398, 354, 316, 298, 276, 236, 210, 198, 176, 148, 132, 118, 98, 78, 66, 50
};

/// <summary>
/// SFCs the read apu status.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline uint8_t sfc_read_apu_status(sfc_famicom_t* famicom) {
    uint8_t state = 0;
    // IF-D NT21 DMC interrupt (I), frame interrupt (F), DMC active (D), length counter > 0 (N/T/2/1)
    if (famicom->apu.square1.length_counter)
        state |= SFC_APU4015_READ_Square1Length;
    if (famicom->apu.square2.length_counter)
        state |= SFC_APU4015_READ_Square2Length;
    if (famicom->apu.triangle.length_counter)
        state |= SFC_APU4015_READ_TriangleLength;
    if (famicom->apu.noise.length_counter)
        state |= SFC_APU4015_READ_NoiseLength;
    if (famicom->apu.dmc.lenleft)
        state |= SFC_APU4015_READ_DMCActive;
    if (famicom->apu.frame_interrupt)
        state |= SFC_APU4015_READ_Frameinterrupt;

    // 清除中断标记
    famicom->apu.frame_interrupt = 0;
    sfc_operation_IRQ_acknowledge(famicom);

    // TODO: DMC
    return state;
}

/// <summary>
/// StepFC: 读取CPU地址数据4020
/// </summary>
/// <param name="address">The address.</param>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline uint8_t sfc_read_cpu_address4020(uint16_t address, sfc_famicom_t* famicom) {
    uint8_t data = 0;
    switch (address & (uint16_t)0x1f)
    {
    case 0x15:
        // 状态寄存器
        //data = famicom->apu.status_read;
        data = sfc_read_apu_status(famicom);
        break;
    case 0x16:
        // 手柄端口#1
        data = (famicom->button_states+0)[famicom->button_index_1 & famicom->button_index_mask];
        ++famicom->button_index_1;
        break;
    case 0x17:
        // 手柄端口#2
        data = (famicom->button_states+8)[famicom->button_index_2 & famicom->button_index_mask];
        ++famicom->button_index_2;
        break;
    }
    return data;
}


/// <summary>
/// StepFC: 获取DMA地址
/// </summary>
/// <param name="data">The data.</param>
/// <returns></returns>
static inline const uint8_t* sfc_get_dma_address(uint8_t data, const sfc_famicom_t* famicom) {
    switch (data >> 5)
    {
    default:
    case 1:
        // PPU寄存器
        assert(!"PPU REG!");
    case 2:
        // 扩展区
        assert(!"TODO");
    case 0:
        // 系统内存
        return famicom->main_memory + ((uint16_t)(data & 0x07) << 8);
    case 3:
        // 存档 SRAM区
        return famicom->save_memory + ((uint16_t)(data & 0x1f) << 8);
    case 4: case 5: case 6: case 7:
        // 高一位为1, [$8000, $10000) 程序PRG-ROM区
        return famicom->prg_banks[data >> 4] + ((uint16_t)(data & 0x0f) << 8);
    }
}


// 立刻产生
void sfc_clock_length_counter_and_sweep_unit(sfc_apu_register_t* apu);
void sfc_clock_envelopes_and_linear_counter(sfc_apu_register_t* apu);


/// <summary>
/// StepFC: 写入方波-寄存器0
/// </summary>
/// <param name="sq">The sq.</param>
/// <param name="value">The value.</param>
/// <returns></returns>
extern inline sfc_square_set0(sfc_square_data_t* sq, uint8_t value) {
    // DDLC NNNN 
    sq->ctrl = value;
    sq->envelope.ctrl6 = value & (uint8_t)0x3F;
}

/// <summary>
/// StepFC: 写入方波-寄存器1
/// </summary>
/// <param name="sq">The sq.</param>
/// <param name="value">The value.</param>
/// <returns></returns>
extern inline sfc_square_set1(sfc_square_data_t* sq, uint8_t value) {
    // EPPP NSSS
    sq->sweep_enable = value & (uint8_t)0x80;
    sq->sweep_negate = value & (uint8_t)0x08;
    sq->sweep_period = (value >> 4) & (uint8_t)0x07;
    sq->sweep_shift  = value & (uint8_t)0x07;
    sq->sweep_reload = 1;
}

/// <summary>
/// StepFC: 写入方波-寄存器2
/// </summary>
/// <param name="sq">The sq.</param>
/// <param name="value">The value.</param>
/// <returns></returns>
extern inline sfc_square_set2(sfc_square_data_t* sq, uint8_t value) {
    // TTTT TTTT
    sq->cur_period = (sq->cur_period & (uint16_t)0xff00) | (uint16_t)value;
}

/// <summary>
/// StepFC: 写入方波-寄存器3
/// </summary>
/// <param name="sq">The sq.</param>
/// <param name="value">The value.</param>
/// <returns></returns>
extern inline sfc_square_set3(sfc_square_data_t* sq, uint8_t value) {
    // LLLL LTTT
    // 禁止状态不会重置长度计数器
    if (sq->x015_flag) sq->length_counter = LENGTH_COUNTER_TABLE[value >> 3];
    // 合并11bit周期
    const uint16_t high_bits = ((uint16_t)(value & 0x07) << 8);
    sq->cur_period = (sq->cur_period & (uint16_t)0x00ff) | high_bits;
    // 相关复位
    sq->envelope.start = 1;
    sq->seq_index = 0;
    //sq->use_period = sq->cur_period;
    //assert(sq->cur_period != 0xffff);
}


/// <summary>
/// SFCs the audio overview.
/// </summary>
/// <param name="famicom">The famicom.</param>
static inline void sfc_audio_overview(sfc_famicom_t* famicom) {
    const uint32_t cycle = famicom->cpu_cycle_count;
    famicom->interfaces.audio_change(famicom->argument, cycle, SFC_Overview);
}

/// <summary>
/// StepFC: 写入CPU地址数据4020
/// </summary>
/// <param name="address">The address.</param>
/// <param name="data">The data.</param>
/// <param name="famicom">The famicom.</param>
extern inline void sfc_write_cpu_address4020(uint16_t address, uint8_t data, sfc_famicom_t* famicom) {
    // 0-3  -> SQ1
    // 4-7  -> SQ2
    // 8-B  -> TRI
    // C-F  -> NOI
    // ...  -> DMC
    // 前面16字节
    if ((address & 0x1f) < 16) {
        // 检查当前CPU周期
        const uint32_t cycle = famicom->cpu_cycle_count;
        // 触发事件
        enum sfc_channel_index type = SFC_2A03_Square1 + ((address & 0xf) >> 2);
        famicom->interfaces.audio_change(famicom->argument, cycle, type);
    }
    // 讨论
    switch (address & (uint16_t)0x1f)
    {
    case 0x00:
        // $4000 DDLC NNNN 
        sfc_square_set0(&famicom->apu.square1, data);
        break;
    case 0x01:
        // $4001 EPPP NSSS
        sfc_square_set1(&famicom->apu.square1, data);
        //if (famicom->apu.square1.sweep_enable) {
        //    int printf();
        //    printf("%d\n", data);
        //}
        break;
    case 0x02:
        // $4002 TTTT TTTT
        sfc_square_set2(&famicom->apu.square1, data);
        //famicom->apu.square1.use_period = famicom->apu.square1.cur_period;
        //assert(famicom->apu.square1.cur_period != 0xffff);
        break;
    case 0x03:
        // $4003 LLLL LTTT
        sfc_square_set3(&famicom->apu.square1, data);

        //if (famicom->button_states[0]) {
        //    printf("<4003>\n");
        //}
        break;
    case 0x04:
        // $4004 DDLC NNNN 
        sfc_square_set0(&famicom->apu.square2, data);
        break;
    case 0x05:
        // $4005 EPPP NSSS
        sfc_square_set1(&famicom->apu.square2, data);
        break;
    case 0x06:
        // $4006 TTTT TTTT
        sfc_square_set2(&famicom->apu.square2, data);
        break;
    case 0x07:
        // $4007 LLLL LTTT
        sfc_square_set3(&famicom->apu.square2, data);
        break;
    case 0x08:
        // $4008 CRRR RRRR
        famicom->apu.triangle.value_reload = data & (uint8_t)0x7F;
        famicom->apu.triangle.flag_halt = data >> 7;
        break;
    case 0x0A:
        // $400A TTTT TTTT
        famicom->apu.triangle.cur_period
            = (famicom->apu.triangle.cur_period & (uint16_t)0xff00)
            | (uint16_t)data;
        break;
    case 0x0B:
        // $400B LLLL LTTT
        // 禁止状态不会重置
        if (famicom->apu.status_write & (uint8_t)SFC_APU4015_WRITE_EnableTriangle)
            famicom->apu.triangle.length_counter = LENGTH_COUNTER_TABLE[data >> 3];
        famicom->apu.triangle.cur_period
            = (famicom->apu.triangle.cur_period & (uint16_t)0x00ff)
            | (((uint16_t)data & (uint16_t)0x07) << 8);
        famicom->apu.triangle.flag_reload = 1;
        break;
    case 0x0C:
        // $400C --LC NNNN 
        famicom->apu.noise.envelope.ctrl6 = data & (uint8_t)0x3F;
        break;
    case 0x0E:
        // $400E S--- PPPP
        famicom->apu.noise.short_mode__period_index = data;
        break;
    case 0x0F:
        // $400E LLLL L---
        // 禁止状态不会重置
        if (famicom->apu.status_write & (uint8_t)SFC_APU4015_WRITE_EnableNoise)
            famicom->apu.noise.length_counter = LENGTH_COUNTER_TABLE[data >> 3];

        famicom->apu.noise.envelope.start = 1;
        break;
    case 0x10:
        // $4010 IL-- RRRR
        // TODO: PAL的场合
        famicom->apu.dmc.irq_loop = data >> 6;
        famicom->apu.dmc.period = SFC_DMC_PERIOD_LIST_NP[data & 0xF];
        break;
    case 0x11:
        // $4011 -DDD DDDD
        famicom->interfaces.audio_change(
            famicom->argument, 
            famicom->cpu_cycle_count,
            SFC_2A03_DMC
        );
        famicom->apu.dmc.value = data & (uint8_t)0x7F;
        break;
    case 0x12:
        famicom->interfaces.audio_change(
            famicom->argument,
            famicom->cpu_cycle_count,
            SFC_2A03_DMC_ADDRLEN
        );
        // $4012 AAAA AAAA
        // 11AAAAAA.AA000000
        famicom->apu.dmc.orgaddr = (uint16_t)0xC000 | ((uint16_t)data << 6);
        break;
    case 0x13:
        famicom->interfaces.audio_change(
            famicom->argument,
            famicom->cpu_cycle_count,
            SFC_2A03_DMC_ADDRLEN
        );
        // $4013 LLLL LLLL
        // 0000LLLL.LLLL0001
        famicom->apu.dmc.length = ((uint16_t)data << 4) | 1;
        break;
    case 0x14:
        // 精灵RAM直接储存器访问
        if (famicom->ppu.data.oamaddr) {
            uint8_t* dst = famicom->ppu.data.sprites;
            const uint8_t len = famicom->ppu.data.oamaddr;
            const uint8_t* src = sfc_get_dma_address(data, famicom);
            // 需要换行
            memcpy(dst, src + len, len);
            memcpy(dst + len, src, 256 - len);
        }
        else memcpy(famicom->ppu.data.sprites, sfc_get_dma_address(data, famicom), 256);
        famicom->cpu_cycle_count += 513;
        famicom->cpu_cycle_count += famicom->cpu_cycle_count & 1;
        break;
    case 0x15:
        sfc_audio_overview(famicom);
        // 状态寄存器
        famicom->apu.status_write = data;
        // 方波使能位
        famicom->apu.square1.x015_flag = data & (uint8_t)SFC_APU4015_WRITE_EnableSquare1;
        famicom->apu.square2.x015_flag = data & (uint8_t)SFC_APU4015_WRITE_EnableSquare2;
        // 对应通道长度计数器清零
        if (!(data & (uint8_t)SFC_APU4015_WRITE_EnableSquare1))
            famicom->apu.square1.length_counter = 0;
        if (!(data & (uint8_t)SFC_APU4015_WRITE_EnableSquare2))
            famicom->apu.square2.length_counter = 0;
        if (!(data & (uint8_t)SFC_APU4015_WRITE_EnableTriangle))
            famicom->apu.triangle.length_counter = 0;
        if (!(data & (uint8_t)SFC_APU4015_WRITE_EnableNoise))
            famicom->apu.noise.length_counter = 0;
        // DMC
        if (!(data & (uint8_t)SFC_APU4015_WRITE_EnableDMC))
            famicom->apu.dmc.lenleft = 0;
        else if (!famicom->apu.dmc.lenleft)
            sfc_restart_dmc(famicom);
        break;
    case 0x16:
        // 手柄端口
        famicom->button_index_mask = (data & 1) ? 0x0 : 0x7;
        if (data & 1) {
            famicom->button_index_1 = 0;
            famicom->button_index_2 = 0;
        }
        break;
    case 0x17:
        sfc_audio_overview(famicom);
        // 帧计数器
        // $4014:  MI-- ----
        famicom->apu.frame_counter = data;
        if (data & (uint8_t)SFC_APU4017_IRQDisable) {
            famicom->registers.irq_flag = 0;
            famicom->apu.frame_interrupt = 0;
        }
        // 5步模式会立刻产生一个1/2和1/4时钟信号
        if (data & (uint8_t)SFC_APU4017_ModeStep5) {
            sfc_clock_length_counter_and_sweep_unit(&famicom->apu);
            sfc_clock_envelopes_and_linear_counter(&famicom->apu);
            break;
        }
    }
}


/// <summary>
/// SFCs the sweep square.
/// </summary>
/// <param name="square">The square.</param>
/// <param name="one">The one.</param>
void sfc_sweep_square(sfc_square_data_t* square, uint16_t one) {
    // 重载扫描
    if (square->sweep_reload) {
        square->sweep_reload = 0;
        const uint8_t old_divider = square->sweep_divider;
        square->sweep_divider = square->sweep_period;
        if (square->sweep_enable && !old_divider)
            goto sweep_it;
        return;
    }
    // 检测是否可用
    if (!square->sweep_enable) return;
    // 等待分频器输出时钟
    if (square->sweep_divider) --square->sweep_divider;
    else {
        square->sweep_divider = square->sweep_period;
    sweep_it:
        // 有效位移
        if (square->sweep_shift) {
            // 向上扫描
            if (!square->sweep_negate) {
                if (square->cur_period < (uint16_t)0x0800) {
                    const uint16_t target
                        = square->cur_period
                        + (square->cur_period >> square->sweep_shift)
                        ;
                    square->cur_period = target;
                }
            }
            // 向下扫描
            else {
                if (square->cur_period >= 8) {
                    uint16_t target
                        = square->cur_period
                        - (square->cur_period >> square->sweep_shift)
                        ;
                    target -= one;
                    square->cur_period = target;
                }
            }
        }
    }
}

/// <summary>
/// 触发长度计数器
/// </summary>
/// <param name="sq">The sq.</param>
static inline void sfc_clock_sqlc(sfc_square_data_t* sq) {
    if ((~sq->ctrl & (uint8_t)0x20) && sq->length_counter)
        --sq->length_counter;
}

/// <summary>
/// SFCs the clock length counter and sweep unit.
/// </summary>
/// <param name="apu">The apu.</param>
void sfc_clock_length_counter_and_sweep_unit(sfc_apu_register_t* apu) {
    // 方波#1 长度计数器
    sfc_clock_sqlc(&apu->square1);
    // 方波#2 长度计数器
    sfc_clock_sqlc(&apu->square2);
    // 三角波 长度计数器
    if (!(apu->triangle.flag_halt) && apu->triangle.length_counter)
        --apu->triangle.length_counter;
    // 噪音-- 长度计数器
    if (!(apu->noise.envelope.ctrl6 & (uint8_t)0x20) && apu->noise.length_counter)
        --apu->noise.length_counter;
    // 扫描
    sfc_sweep_square(&apu->square1, 1);
    sfc_sweep_square(&apu->square2, 0);
}


/// <summary>
/// SFCs the clock envelope.
/// </summary>
/// <param name="envelope">The envelope.</param>
void sfc_clock_envelope(sfc_envelope_t* envelope) {
    // 写入了第四个寄存器(START 标记了)
    if (envelope->start) {
        // 重置
        envelope->start = 0;
        envelope->counter = 15;
        envelope->divider = envelope->ctrl6 & (uint8_t)0x0F;
    }
    // 对时钟分频器发送一个时钟信号
    else {
        if (envelope->divider) envelope->divider--;
        // 时钟分频器输出一个信号
        else {
            envelope->divider = envelope->ctrl6 & (uint8_t)0x0F;
            // 处理计数器-有效/循环情况
            const uint8_t loop = envelope->ctrl6 & (uint8_t)SFC_APUCTRL6_EnvLoop;
            if (envelope->counter | loop) {
                --envelope->counter;
                envelope->counter &= 0xf;
            }
            //// 计数器>0 ?
            //if (envelope->counter) --envelope->counter;
            //// 循环标志
            //else if (envelope->ctrl6 & (uint8_t)SFC_APUCTRL6_EnvLoop)
            //    envelope->counter = 15;
        }
    }
}

/// <summary>
/// SFCs the clock envelopes and linear counter.
/// </summary>
/// <param name="apu">The apu.</param>
void sfc_clock_envelopes_and_linear_counter(sfc_apu_register_t* apu) {
    // 方波#1
    sfc_clock_envelope(&apu->square1.envelope);
    // 方波#2
    sfc_clock_envelope(&apu->square2.envelope);
    // 噪音
    sfc_clock_envelope(&apu->noise.envelope);


    // 三角波 - 线性计数器

    // 标记了重载
    if (apu->triangle.flag_reload) {
        apu->triangle.linear_counter = apu->triangle.value_reload;
    }
    // 没有就减1
    else if (apu->triangle.linear_counter) {
        --apu->triangle.linear_counter;
    }
    // 控制C位为0 清除重载标志
    if (!apu->triangle.flag_halt)
        apu->triangle.flag_reload = 0;
}


// IRQ - 中断请求 - 尝试
extern inline void sfc_operation_IRQ_try(sfc_famicom_t* famicom);

/// <summary>
/// SFCs the apu set interrupt.
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_apu_set_interrupt(sfc_famicom_t* famicom) {
    if (famicom->apu.frame_counter & (uint8_t)SFC_APU4017_IRQDisable) return;
    famicom->apu.frame_interrupt = 1;
    sfc_operation_IRQ_try(famicom);
}


/// <summary>
/// SFCs the trigger frame counter.
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_trigger_frame_counter(sfc_famicom_t* famicom) {
    sfc_apu_register_t* const apu = &famicom->apu;
    // 触发一次 
    // TODO: 优化触发
    const uint32_t cycle = famicom->cpu_cycle_count;
    famicom->interfaces.audio_change(famicom->argument, cycle, SFC_FrameCounter);
    // 5步模式
    if ((apu->frame_counter & (uint8_t)SFC_APU4017_ModeStep5)) {
        apu->frame_step++;
        apu->frame_step = apu->frame_step % 5;
        // l - l - - ->   ++%5   ->   1 2 3 4 0
        // e e e e - ->   ++%5   ->   1 2 3 4 0
        switch (apu->frame_step)
        {
        case 0: return;
        case 1:     case 3:
            sfc_clock_length_counter_and_sweep_unit(apu);
            sfc__fallthrough;
        case 2:     case 4:
            sfc_clock_envelopes_and_linear_counter(apu);
        }
    }
    // 四步模式
    else {
        apu->frame_step++;
        apu->frame_step = apu->frame_step % 4;
        // - - - f   ->   ++%4   ->   1 2 3 0
        if (!apu->frame_step) sfc_apu_set_interrupt(famicom);
        // - l - l   ->   ++%4   ->   1 2 3 0
        if (!(apu->frame_step & 1)) sfc_clock_length_counter_and_sweep_unit(apu);
        // e e e e
        sfc_clock_envelopes_and_linear_counter(apu);
    }
    // MMC5 伪帧序列器
    if (famicom->rom_info.extra_sound & SFC_NSF_EX_MMC5) {
        // 方波#1 长度计数器
        sfc_clock_sqlc(&apu->mmc5.square1);
        // 方波#2 长度计数器
        sfc_clock_sqlc(&apu->mmc5.square2);
        // 方波#1 包络发生器
        sfc_clock_envelope(&apu->mmc5.square1.envelope);
        // 方波#2 包络发生器
        sfc_clock_envelope(&apu->mmc5.square2.envelope);
    }
}



/// <summary>
/// SFCs the apu on reset.
/// </summary>
/// <param name="apu">The apu.</param>
void sfc_apu_on_reset(sfc_apu_register_t* apu) {
    // 噪音的LFSR 初始化为1
    apu->noise.lfsr = 1;
    apu->dmc.period = SFC_DMC_PERIOD_LIST_NP[0];
    // FME-7 噪音的LFSR 初始化为1
    apu->fme7.lfsr = 1;
}