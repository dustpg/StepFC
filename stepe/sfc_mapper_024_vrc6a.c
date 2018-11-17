#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include "sfc_mapper_helper.h"
#include <assert.h>
#include <string.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

#define SFC_VRC6_FACTOR (0.00752f)

// ------------------------------- MAPPER 024 - VRC6a

typedef struct {
    // IRQ contrl
    uint8_t         irq_control;
    // IRQ contrl
    uint8_t         irq_reload;
    // IRQ counter
    uint8_t         irq_counter;
    // IRQ enable
    uint8_t         irq_enable;
    // PPU Banking Style ($B003)
    uint8_t         ppu_style;
    // unused
    uint8_t         unused[3];
    // unused
    uint8_t         registers[8];
} sfc_mapper18_t;


enum {
    // B003
    SFC_18_B3_FROM_CHRROM = 1 << 4,
};


// IRQ - 中断请求 - 确认
extern inline void sfc_operation_IRQ_acknowledge(sfc_famicom_t* famicom);
// 尝试触发
extern inline void sfc_operation_IRQ_try(sfc_famicom_t* famicom);

/// <summary>
/// SFCs the mapper.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
static inline sfc_mapper18_t* sfc_mapper(sfc_famicom_t* famicom) {
    return (sfc_mapper18_t*)famicom->mapper_buffer.mapper04;
}

#define MAPPER sfc_mapper18_t* const mapper = sfc_mapper(famicom);

// 更新周期1
static inline void sfc_vrc6_update_squ1(sfc_vrc6_data_t* vrc6) {
    vrc6->square1.period = vrc6->square1.period_raw >> vrc6->freq_ctrl;
}

// 更新周期2
static inline void sfc_vrc6_update_squ2(sfc_vrc6_data_t* vrc6) {
    vrc6->square2.period = vrc6->square2.period_raw >> vrc6->freq_ctrl;
}

// 更新周期3
static inline void sfc_vrc6_update_saw(sfc_vrc6_data_t* vrc6) {
    vrc6->saw.period = vrc6->saw.period_raw >> vrc6->freq_ctrl;
}

/// <summary>
/// SFCs the mapper 04 reset.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
sfc_ecode sfc_mapper_18_reset(sfc_famicom_t* famicom) {
    const uint32_t count_prgrom8kb = famicom->rom_info.size_prgrom >> 13;
    // 支持VRC6
    famicom->rom_info.extra_sound = SFC_NSF_EX_VCR6;
    // 载入最后的BANK
    sfc_load_prgrom_8k(famicom, 3, count_prgrom8kb - 1);

    sfc_load_prgrom_8k(famicom, 0, 0);
    sfc_load_prgrom_8k(famicom, 1, 0);
    sfc_load_prgrom_8k(famicom, 2, 0);

    return SFC_ERROR_OK;
}


/// <summary>
/// SFCs the mapper 18 character update.
/// </summary>
/// <param name="famicom">The famicom.</param>
static void sfc_mapper_18_chr_update(sfc_famicom_t* famicom) {
    MAPPER;
    const uint8_t ppu_style = mapper->ppu_style;
#ifndef NDEBUG
    //static uint8_t ppu_style_old = 0;
    //if (ppu_style_old != ppu_style) {
    //    ppu_style_old = ppu_style;
    //    printf("new control: 0x%2x\n", ppu_style);
    //}
#endif
    // 图样表
    switch (ppu_style & 3)
    {
    case 0:
        for (int i = 0; i != 8; ++i)
            sfc_load_chrrom_1k(famicom, i, mapper->registers[i]);
        break;
    case 1:
        for (int i = 0; i != 8; ++i)
            sfc_load_chrrom_1k(famicom, i, mapper->registers[i >> 1]);
        break;
    case 2:
    case 3:
        for (int i = 0; i != 4; ++i)
            sfc_load_chrrom_1k(famicom, i, mapper->registers[i]);
        sfc_load_chrrom_1k(famicom, 4, mapper->registers[4]);
        sfc_load_chrrom_1k(famicom, 5, mapper->registers[4]);
        sfc_load_chrrom_1k(famicom, 6, mapper->registers[5]);
        sfc_load_chrrom_1k(famicom, 7, mapper->registers[5]);
    }
    // 名称表


    // 使用CIRAM 镜像模式
    if (!(ppu_style & SFC_18_B3_FROM_CHRROM)) {
        switch (ppu_style & 0x2F) {
        case 0x20:
        case 0x27:
            sfc_switch_nametable_mirroring(famicom, SFC_NT_MIR_Vertical);
            break;
        case 0x23:
        case 0x24:
            sfc_switch_nametable_mirroring(famicom, SFC_NT_MIR_Horizontal);
            break;
        case 0x28:
        case 0x2F:
            sfc_switch_nametable_mirroring(famicom, SFC_NT_MIR_SingleLow);
            break;
        case 0x2B:
        case 0x2C:
            sfc_switch_nametable_mirroring(famicom, SFC_NT_MIR_SingleHigh);
            break;
        default:
            switch (ppu_style & 0x07) {
            case 0x00:
            case 0x06:
            case 0x07:
                famicom->ppu.banks[0x8] = famicom->video_memory + 0x400 * (mapper->registers[6] & 1);
                famicom->ppu.banks[0x9] = famicom->ppu.banks[0x8];
                famicom->ppu.banks[0xa] = famicom->video_memory + 0x400 * (mapper->registers[7] & 1);
                famicom->ppu.banks[0xb] = famicom->ppu.banks[0xa];
                break;
            case 0x01:
            case 0x05:
                famicom->ppu.banks[0x8] = famicom->video_memory + 0x400 * (mapper->registers[4] & 1);
                famicom->ppu.banks[0x9] = famicom->video_memory + 0x400 * (mapper->registers[5] & 1);
                famicom->ppu.banks[0xa] = famicom->video_memory + 0x400 * (mapper->registers[6] & 1);
                famicom->ppu.banks[0xb] = famicom->video_memory + 0x400 * (mapper->registers[7] & 1);
                break;
            case 0x02:
            case 0x03:
            case 0x04:
                famicom->ppu.banks[0x8] = famicom->video_memory + 0x400 * (mapper->registers[6] & 1);
                famicom->ppu.banks[0x9] = famicom->video_memory + 0x400 * (mapper->registers[7] & 1);
                famicom->ppu.banks[0xa] = famicom->ppu.banks[0x8];
                famicom->ppu.banks[0xb] = famicom->ppu.banks[0x9];
                break;
            }
            // 镜像
            famicom->ppu.banks[0xc] = famicom->ppu.banks[0x8];
            famicom->ppu.banks[0xd] = famicom->ppu.banks[0x9];
            famicom->ppu.banks[0xe] = famicom->ppu.banks[0xa];
            famicom->ppu.banks[0xf] = famicom->ppu.banks[0xb];
            break;
        }
    }
    // 使用CHR-ROM
    else {
        // TODO: 完成
        assert(!"NOT IMPL");
    }
}



/// <summary>
/// SFCs the mapper 18 write high.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="address">The address.</param>
/// <param name="value">The value.</param>
void sfc_mapper_18_write_high(sfc_famicom_t* famicom, uint16_t address, uint8_t value) {
    switch ((address & (uint16_t)0x7fff)>>12)
    {
        sfc_mapper18_t* mapper;
        uint32_t count_prgrom8kb;
        uint32_t index;
    case 0:
        // 16k PRG Select ($8000-$8003)
        count_prgrom8kb = famicom->rom_info.size_prgrom >> 13;
        index = (((uint32_t)value & 0xf) * 2) % count_prgrom8kb;
        sfc_load_prgrom_8k(famicom, 0, index + 0);
        sfc_load_prgrom_8k(famicom, 1, index + 1);
        break;
    case 1:
        // 触发事件
        switch (address & 0x03)
        {
        case 0:
            // Pulse 1 duty and volume
            famicom->apu.vrc6.square1.duty = value >> 4;
            famicom->apu.vrc6.square1.volume = value & 0x0f;
            break;
        case 1:
            // Pulse 1 period low
            famicom->apu.vrc6.square1.period_raw
                = (famicom->apu.vrc6.square1.period_raw & 0xff00)
                | (uint16_t)value;
                ;
            sfc_vrc6_update_squ1(&famicom->apu.vrc6);
            break;
        case 2:
            // Pulse 1 period high
            famicom->apu.vrc6.square1.period_raw
                = (famicom->apu.vrc6.square1.period_raw & 0x00ff)
                | (((uint16_t)value & 0x0f) << 8)
                ;
            // 状态0->1 重置INDEX
            if (!famicom->apu.vrc6.square1.enable && (value >> 7)) {
                famicom->apu.vrc6.square1.index = 0;
            }
            famicom->apu.vrc6.square1.enable = value >> 7;
            sfc_vrc6_update_squ1(&famicom->apu.vrc6);
            break;
        case 3:
            // Frequency Control ($9003)
            famicom->apu.vrc6.halt = value & 1;
            famicom->apu.vrc6.freq_ctrl
                = (value & 6) 
                ? ((value & 4) ? 8 : 4)
                : 0
                ;
            sfc_vrc6_update_squ1(&famicom->apu.vrc6);
            sfc_vrc6_update_squ2(&famicom->apu.vrc6);
            sfc_vrc6_update_saw(&famicom->apu.vrc6);
            famicom->interfaces.audio_changed(famicom->argument, famicom->cpu_cycle_count, SFC_VRC6_VRC6);
            return;
        }
        famicom->interfaces.audio_changed(famicom->argument, famicom->cpu_cycle_count, SFC_VRC6_Square1);
        break;
    case 2:
        switch (address & 0x03)
        {
        case 0:
            // Pulse 2 duty and volume
            famicom->apu.vrc6.square2.duty 
                = (value & 0x80)
                ? 0x0f
                : value >> 4
                ;
            famicom->apu.vrc6.square2.volume = value & 0x0f;
            break;
        case 1:
            // Pulse 2 period low
            famicom->apu.vrc6.square2.period_raw
                = (famicom->apu.vrc6.square2.period_raw & 0x0f00)
                | (uint16_t)value;
            ;
            sfc_vrc6_update_squ2(&famicom->apu.vrc6);
            break;
        case 2:
            // Pulse 2 period high
            famicom->apu.vrc6.square2.period_raw
                = (famicom->apu.vrc6.square2.period_raw & 0x00ff)
                | (((uint16_t)value & 0x0f) << 8)
                ;
            // 状态0->1 重置INDEX
            if (!famicom->apu.vrc6.square2.enable && (value >> 7)) {
                famicom->apu.vrc6.square2.index = 0;
            }
            famicom->apu.vrc6.square2.enable = value >> 7;
            sfc_vrc6_update_squ2(&famicom->apu.vrc6);
            break;
        }
        famicom->interfaces.audio_changed(famicom->argument, famicom->cpu_cycle_count, SFC_VRC6_Square2);
        break;
    case 3:
        // $B00x
        switch (address & 0x03)
        {
        case 0:
            // Saw Accum Rate ($B000)
            famicom->apu.vrc6.saw.rate = value & 0x3f;
            break;
        case 1:
            // Pulse 2 period low
            famicom->apu.vrc6.saw.period_raw
                = (famicom->apu.vrc6.saw.period_raw & 0x0f00)
                | (uint16_t)value;
            ;
            sfc_vrc6_update_saw(&famicom->apu.vrc6);
            break;
        case 2:
            // Pulse 2 period high
            famicom->apu.vrc6.saw.period_raw
                = (famicom->apu.vrc6.saw.period_raw & 0x00ff)
                | (((uint16_t)value & 0x0f) << 8)
                ;
            // 状态0->1 重置INDEX
            if (!famicom->apu.vrc6.saw.enable && (value >> 7)) {
                famicom->apu.vrc6.saw.accumulator = 0;
            }
            famicom->apu.vrc6.saw.enable = value >> 7;
            sfc_vrc6_update_saw(&famicom->apu.vrc6);
            break;
        case 3:
            // PPU Banking Style ($B003)
            sfc_mapper(famicom)->ppu_style = value;
            assert((value & SFC_18_B3_FROM_CHRROM) == 0 && "unsupported");
            sfc_mapper_18_chr_update(famicom);
            return;
        }
        famicom->interfaces.audio_changed(famicom->argument, famicom->cpu_cycle_count, SFC_VRC6_Saw);
        break;
    case 4:
        // 8k PRG Select ($C000-$C003)
        count_prgrom8kb = famicom->rom_info.size_prgrom >> 13;
        index = ((uint32_t)value & 0x1f) % count_prgrom8kb;
        sfc_load_prgrom_8k(famicom, 2, index);
        break;
    case 5:
        // $Dxxx r0-r3
        mapper = sfc_mapper(famicom);
        mapper->registers[(address & 0x0003) + 0] = value;
        sfc_mapper_18_chr_update(famicom);
        break;
    case 6:
        // $Exxx r4-r7
        mapper = sfc_mapper(famicom);
        mapper->registers[(address & 0x0003) + 4] = value;
        sfc_mapper_18_chr_update(famicom);
        break;
    case 7:
        // $Fxxx
        mapper = sfc_mapper(famicom);
        switch (address & 0x03)
        {
        case 0:
            // IRQ Latch
            mapper->irq_reload = value;
            break;
        case 1:
            // IRQ Control
            mapper->irq_control = value;
            mapper->irq_enable = value & 2;
            if (mapper->irq_enable)
                mapper->irq_counter = mapper->irq_reload;
            break;
        case 2:
            // IRQ Acknowledge
            mapper->irq_enable = mapper->irq_control & 1;
            //mapper->irq_control 
            //    = (mapper->irq_control & 5)
            //    | ((mapper->irq_control & 1) << 1)
            //    ;
            sfc_operation_IRQ_acknowledge(famicom);
            break;
        }
    }
}

/// <summary>
/// SFCs the mapper 18 hsyc.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="line">The line.</param>
void sfc_mapper_18_hsyc(sfc_famicom_t* famicom, uint16_t line) {
    MAPPER;
    //if (!(mapper->irq_control & SFC_18_F1_IRQ_ENABLE)) return;
    if (!mapper->irq_enable) return;
    // 扫描线模式
    assert((mapper->irq_control & (1 << 2)) == 0 && "UNSUPPORTED");
    // 触发
    if (mapper->irq_counter == (uint8_t)0xff) {
        mapper->irq_counter = mapper->irq_reload;
        sfc_operation_IRQ_try(famicom);
    }
    // +1
    else ++mapper->irq_counter;
}

/// <summary>
/// SFCs the load mapper 18.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline sfc_ecode sfc_load_mapper_18(sfc_famicom_t* famicom) {
    enum { MAPPER_18_SIZE_IMPL = sizeof(sfc_mapper18_t) };
    static_assert(SFC_MAPPER_18_SIZE == MAPPER_18_SIZE_IMPL, "SAME");
    famicom->mapper.reset = sfc_mapper_18_reset;
    famicom->mapper.hsync = sfc_mapper_18_hsyc;
    famicom->mapper.write_high = sfc_mapper_18_write_high;
    MAPPER;
    memset(mapper, 0, MAPPER_18_SIZE_IMPL);
    return SFC_ERROR_OK;
}