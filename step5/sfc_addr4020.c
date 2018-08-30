#include "sfc_6502.h"
#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include <assert.h>
#include <string.h>

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
/// StepFC: 写入CPU地址数据4020
/// </summary>
/// <param name="address">The address.</param>
/// <param name="data">The data.</param>
/// <param name="famicom">The famicom.</param>
extern inline void sfc_write_cpu_address4020(uint16_t address, uint8_t data, sfc_famicom_t* famicom) {
    switch (address & (uint16_t)0x1f)
    {
    case 0x16:
        // 手柄端口
        famicom->button_index_mask = (data & 1) ? 0x0 : 0x7;
        if (data & 1) {
            famicom->button_index_1 = 0;
            famicom->button_index_2 = 0;
        }
        break;
    }
}