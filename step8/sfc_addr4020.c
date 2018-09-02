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
        data = 0;
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
    const uint16_t offset = ((uint16_t)(data & 0x07) << 8);
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
        return famicom->main_memory + offset;
    case 3:
        // 存档 SRAM区
        return famicom->save_memory + offset;
    case 4: case 5: case 6: case 7:
        // 高一位为1, [$8000, $10000) 程序PRG-ROM区
        return famicom->prg_banks[data >> 5] + offset;
    }
}


// 立刻产生
void sfc_clock_length_counter_and_sweep_unit(sfc_apu_register_t* apu);
void sfc_clock_envelopes_and_linear_counter(sfc_apu_register_t* apu);

/// <summary>
/// StepFC: 写入CPU地址数据4020
/// </summary>
/// <param name="address">The address.</param>
/// <param name="data">The data.</param>
/// <param name="famicom">The famicom.</param>
extern inline void sfc_write_cpu_address4020(uint16_t address, uint8_t data, sfc_famicom_t* famicom) {
    switch (address & (uint16_t)0x1f)
    {
    case 0x00:
        // $4000 DDLC NNNN 
        famicom->apu.square1.ctrl = data;
        famicom->apu.square1.envelope.ctrl6 = data & (uint8_t)0x3F;
        break;
    case 0x01:
        // $4001 EPPP NSSS
        famicom->apu.square1.sweep_enable = data & (uint8_t)0x80;
        famicom->apu.square1.sweep_negate = data & (uint8_t)0x08;
        famicom->apu.square1.sweep_period = (data >> 4) & (uint8_t)0x07;
        famicom->apu.square1.sweep_shift = data & (uint8_t)0x07;
        famicom->apu.square1.sweep_reload = 1;

        break;
    case 0x02:
        // $4002 TTTT TTTT
        famicom->apu.square1.cur_period
            = (famicom->apu.square1.cur_period & (uint16_t)0xff00)
            | (uint16_t)data;
        famicom->apu.square1.use_period = famicom->apu.square1.cur_period;
        break;
    case 0x03:
        // $4003 LLLL LTTT
        famicom->apu.square1.length_counter = LENGTH_COUNTER_TABLE[data >> 3];
        famicom->apu.square1.cur_period
            = (famicom->apu.square1.cur_period & (uint16_t)0x00ff)
            | (((uint16_t)data & (uint16_t)0x07) << 8);
        famicom->apu.square1.use_period = famicom->apu.square1.cur_period;
        famicom->apu.square1.envelope.start = 1;
        break;
    case 0x04:
        // $4004 DDLC NNNN 
        famicom->apu.square2.ctrl = data;
        famicom->apu.square2.envelope.ctrl6 = data & (uint8_t)0x3F;
        break;
    case 0x05:
        // $4005 EPPP NSSS
        famicom->apu.square2.sweep_enable = data & (uint8_t)0x80;
        famicom->apu.square2.sweep_negate = data & (uint8_t)0x08;
        famicom->apu.square2.sweep_period = (data >> 4) & (uint8_t)0x07;
        famicom->apu.square2.sweep_shift = data & (uint8_t)0x07;
        famicom->apu.square2.sweep_reload = 1;
        break;
    case 0x06:
        // $4006 TTTT TTTT
        famicom->apu.square2.cur_period
            = (famicom->apu.square2.cur_period & (uint16_t)0xff00)
            | (uint16_t)data;
        famicom->apu.square2.use_period = famicom->apu.square2.cur_period;
        break;
    case 0x07:
        // $4007 LLLL LTTT
        famicom->apu.square2.length_counter = LENGTH_COUNTER_TABLE[data >> 3];
        famicom->apu.square2.cur_period
            = (famicom->apu.square2.cur_period & (uint16_t)0x00ff)
            | (((uint16_t)data & (uint16_t)0x07) << 8);
        famicom->apu.square2.use_period = famicom->apu.square2.cur_period;
        famicom->apu.square2.envelope.start = 1;
        break;
    case 0x14:
        // 精灵RAM直接储存器访问
        if (famicom->ppu.oamaddr) {
            uint8_t* dst = famicom->ppu.sprites;
            const uint8_t len = famicom->ppu.oamaddr;
            const uint8_t* src = sfc_get_dma_address(data, famicom);
            // 需要换行
            memcpy(dst, src + len, len);
            memcpy(dst + len, src, 256 - len);
        }
        else memcpy(famicom->ppu.sprites, sfc_get_dma_address(data, famicom), 256);
        famicom->cpu_cycle_count += 513;
        famicom->cpu_cycle_count += famicom->cpu_cycle_count & 1;
        break;
    case 0x15:
        // 状态寄存器
        famicom->apu.status_write = data;
        // TODO: 对应通道长度计数器清零
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
        // 帧计数器
        // $4014:  MI-- ----
        famicom->apu.frame_counter_4017 = data;
        // 写入会重置帧计数器
        famicom->apu.frame_step = 4 + (data >> 7) & 1;
        // 5步模式会立刻产生一个时钟信号
        if (data & 0x80) {
            sfc_clock_length_counter_and_sweep_unit(&famicom->apu);
            sfc_clock_envelopes_and_linear_counter(&famicom->apu);
        }
        break;
    }
}


/// <summary>
/// SFCs the sweep square.
/// </summary>
/// <param name="square">The square.</param>
/// <param name="one">The one.</param>
void sfc_sweep_square(struct sfc_square_data_t* square, uint16_t one) {
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
        // 向上扫描
        if (!square->sweep_negate) {
            const uint16_t target 
                = square->cur_period
                + (square->cur_period >> square->sweep_shift)
                ;
            square->use_period = target;
            if (target < (uint16_t)0x0800)
                square->cur_period = target;
        }
        // 向下扫描
        else {
            uint16_t target
                = square->cur_period
                - (square->cur_period >> square->sweep_shift);
            target -= one;
            square->use_period = target;
            if (target >= 8)
                square->cur_period = target;
        }
    }
}

/// <summary>
/// SFCs the clock length counter and sweep unit.
/// </summary>
/// <param name="apu">The apu.</param>
void sfc_clock_length_counter_and_sweep_unit(sfc_apu_register_t* apu) {
    // 方波#1 长度计数器
    if (!(apu->square1.ctrl & (uint8_t)0x20) && apu->square1.length_counter)
        --apu->square1.length_counter;
    // 方波#2 长度计数器
    if (!(apu->square2.ctrl & (uint8_t)0x20) && apu->square2.length_counter)
        --apu->square2.length_counter;

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
            // 计数器>0 ?
            if (envelope->counter) --envelope->counter;
            // 循环标志
            else if (envelope->ctrl6 & (uint8_t)SFC_APUCTRL6_EnvLoop)
                envelope->counter = 15;
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
}


/// <summary>
/// SFCs the apu set interrupt.
/// </summary>
/// <param name="apu">The apu.</param>
void sfc_apu_set_interrupt(sfc_apu_register_t* apu) {

}


extern void pin_audio();



/// <summary>
/// SFCs the trigger frame counter.
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_trigger_frame_counter(sfc_apu_register_t* apu) {
    // 5步模式
    if (apu->frame_counter_4017 & SFC_APU4017_ModeStep5) {
        apu->frame_step++;
        apu->frame_step = apu->frame_step % 5;
        // l - l - - ->   ++%5   ->   1 2 3 4 0
        // e e e e - ->   ++%5   ->   1 2 3 4 0
        switch (apu->frame_step)
        {
        case 1:     case 3:
            sfc_clock_length_counter_and_sweep_unit(apu);
            sfc__fallthrough;
        case 2:     case 4:
            sfc_clock_envelopes_and_linear_counter(apu);
            pin_audio();
        }
    }
    // 四步模式
    else {
        apu->frame_step++;
        apu->frame_step = apu->frame_step % 4;
        // - - - f   ->   ++%4   ->   1 2 3 0
        if (!apu->frame_step) sfc_apu_set_interrupt(apu);
        // - l - l   ->   ++%4   ->   1 2 3 0
        if (!(apu->frame_step & 1)) sfc_clock_length_counter_and_sweep_unit(apu);
        // e e e e
        sfc_clock_envelopes_and_linear_counter(apu);

        pin_audio();
    }
}