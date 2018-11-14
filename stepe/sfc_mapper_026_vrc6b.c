#include "sfc_cpu.h"
#include "sfc_famicom.h"
#include "sfc_mapper_helper.h"
#include <assert.h>
#include <string.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

// Mapper-026 VRC6b

// VRC6a
extern inline sfc_ecode sfc_load_mapper_18(sfc_famicom_t* famicom);
extern void sfc_mapper_18_write_high(sfc_famicom_t* famicom, uint16_t address, uint8_t value);

/// <summary>
/// SFCs the mapper 1 a write high.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="address">The address.</param>
/// <param name="value">The value.</param>
static void sfc_mapper_1A_write_high(sfc_famicom_t* famicom, uint16_t address, uint8_t value) {
    uint16_t new_addr = address & 0xfffc;
    new_addr |= (address & 1) << 1;
    new_addr |= (address & 2) >> 1;
    //sfc_mapper_18_write_high(famicom, address, value);
    sfc_mapper_18_write_high(famicom, new_addr, value);
}


/// <summary>
/// SFCs the load mapper 1A.
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <returns></returns>
extern inline sfc_ecode sfc_load_mapper_1A(sfc_famicom_t* famicom) {
    sfc_load_mapper_18(famicom);
    famicom->mapper.write_high = sfc_mapper_1A_write_high;
    return SFC_ERROR_OK;
}