#include "sfc_ppu.h"
#include <assert.h>
#include <string.h>

/// <summary>
/// StepFC: Ó³ÉäPPUµØÖ·¿Õ¼äµ½ Ä£ÄâÆ÷×ÔÉí¿Õ¼ä
/// </summary>
/// <param name="address">The address.</param>
/// <param name="data">The data.</param>
/// <param name="ppu">The ppu.</param>
uint8_t* sfc_get_ppu_address(uint16_t address, sfc_ppu_t* ppu) {
    const uint16_t real_address = address & (uint16_t)0x3FFF;
    // Ê¹ÓÃBANK¶ÁÈ¡
    if (real_address < (uint16_t)0x3F00) {
        const uint16_t index = real_address >> 10;
        const uint16_t offset = real_address & (uint16_t)0x3FF;
        assert(ppu->banks[index]);
        return ppu->banks[index] + offset;
    }
    // µ÷É«°åË÷Òý
    else return ppu->spindexes + (real_address & (uint16_t)0x1f);
}


/// <summary>
/// StepFC: Ê¹ÓÃCPUµØÖ·¿Õ¼ä¶ÁÈ¡PPU¼Ä´æÆ÷
/// </summary>
/// <param name="address">The address.</param>
/// <param name="ppu">The ppu.</param>
/// <returns></returns>
uint8_t sfc_read_ppu_register_via_cpu(uint16_t address, sfc_ppu_t* ppu) {
    uint8_t data = 0x00;
    // 8×Ö½Ú¾µÏñ
    switch (address & (uint16_t)0x7)
    {
    case 0:
        // 0x2000: Controller ($2000) > write
        // Ö»Ð´¼Ä´æÆ÷
    case 1:
        // 0x2001: Mask ($2001) > write
        // Ö»Ð´¼Ä´æÆ÷
        assert(!"write only!");
        break;
    case 2:
        // 0x2002: Status ($2002) < read
        // Ö»¶Á×´Ì¬¼Ä´æÆ÷
        data = ppu->status;
        // ¶ÁÈ¡ºó»áÇå³ýVBlank×´Ì¬
        ppu->status &= ~(uint8_t)SFC_PPUFLAG_VBlank;
        break;
    case 3:
        // 0x2003: OAM address port ($2003) > write
        // Ö»Ð´¼Ä´æÆ÷
        assert(!"write only!");
        break;
    case 4:
        // 0x2004: OAM data ($2004) <> read/write
        // ¶ÁÐ´¼Ä´æÆ÷
        data = ppu->sprites[ppu->oamaddr++];
        break;
    case 5:
        // 0x2005: Scroll ($2005) >> write x2
        // Ë«Ð´¼Ä´æÆ÷
    case 6:
        // 0x2006: Address ($2006) >> write x2
        // Ë«Ð´¼Ä´æÆ÷
        assert(!"write only!");
        break;
    case 7:
        // 0x2007: Data ($2007) <> read/write
        // PPU VRAM¶ÁÐ´¶Ë¿Ú
        data = *sfc_get_ppu_address(ppu->vramaddr, ppu);
        ppu->vramaddr += (uint16_t)((ppu->ctrl & SFC_PPUFLAG_VINC32) ? 32 : 1);
        break;
    }
    return data;
}


/// <summary>
/// StepFC: Ê¹ÓÃCPUµØÖ·¿Õ¼äÐ´ÈëPPU¼Ä´æÆ÷
/// </summary>
/// <param name="address">The address.</param>
/// <param name="data">The data.</param>
/// <param name="ppu">The ppu.</param>
/// <returns></returns>
void sfc_write_ppu_register_via_cpu(uint16_t address, uint8_t data, sfc_ppu_t* ppu) {
    switch (address & (uint16_t)0x7)
    {
    case 0:
        // PPU ¿ØÖÆ¼Ä´æÆ÷
        // 0x2000: Controller ($2000) > write
        ppu->ctrl = data;
        break;
    case 1:
        // PPU ÑÚÂë¼Ä´æÆ÷
        // 0x2001: Mask ($2001) > write
        ppu->mask = data;
        break;
    case 2:
        // 0x2002: Status ($2002) < read
        // Ö»¶Á
        assert(!"read only");
        break;
    case 3:
        // 0x2003: OAM address port ($2003) > write
        // PPU OAM µØÖ·¶Ë¿Ú
        ppu->oamaddr = data;
        break;
    case 4:
        // 0x2004: OAM data ($2004) <> read/write
        // PPU OAM Êý¾Ý¶Ë¿Ú
        ppu->sprites[ppu->oamaddr++] = data;
        break;
    case 5:
        // 0x2005: Scroll ($2005) >> write x2
        // PPU ¹ö¶¯Î»ÖÃ¼Ä´æÆ÷ - Ë«Ð´
        ppu->scroll[ppu->writex2 & 1] = data;
        ++ppu->writex2;
        break;
    case 6:
        // 0x2006: Address ($2006) >> write x2
        // PPU µØÖ·¼Ä´æÆ÷ - Ë«Ð´
        // Ð´Èë¸ß×Ö½Ú
        if (ppu->writex2 & 1) {
            ppu->vramaddr = (ppu->vramaddr & (uint16_t)0xFF00) | (uint16_t)data;
        }
        // Ð´ÈëµÍ×Ö½Ú
        else {
            ppu->vramaddr = (ppu->vramaddr & (uint16_t)0x00FF) | ((uint16_t)data << 8);
        }
        ++ppu->writex2;
        break;
    case 7:
        // 0x2007: Data ($2007) <> read/write
        // PPU VRAMÊý¾Ý¶Ë
        *sfc_get_ppu_address(ppu->vramaddr, ppu) = data;
        ppu->vramaddr += (uint16_t)((ppu->ctrl & SFC_PPUFLAG_VINC32) ? 32 : 1);
        break;
    }
}


/// <summary>
/// µ÷É«°åÊý¾Ý
/// </summary>
const union sfc_palette_data {
    struct { uint8_t r, g, a, x; };
    uint32_t    data;
} sfc_stdalette[64] = {
    { 0x7F, 0x7F, 0x7F, 0xFF }, { 0x20, 0x00, 0xB0, 0xFF }, { 0x28, 0x00, 0xB8, 0xFF }, { 0x60, 0x10, 0xA0, 0xFF },
    { 0x98, 0x20, 0x78, 0xFF }, { 0xB0, 0x10, 0x30, 0xFF }, { 0xA0, 0x30, 0x00, 0xFF }, { 0x78, 0x40, 0x00, 0xFF },
    { 0x48, 0x58, 0x00, 0xFF }, { 0x38, 0x68, 0x00, 0xFF }, { 0x38, 0x6C, 0x00, 0xFF }, { 0x30, 0x60, 0x40, 0xFF },
    { 0x30, 0x50, 0x80, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF },

    { 0xBC, 0xBC, 0xBC, 0xFF }, { 0x40, 0x60, 0xF8, 0xFF }, { 0x40, 0x40, 0xFF, 0xFF }, { 0x90, 0x40, 0xF0, 0xFF },
    { 0xD8, 0x40, 0xC0, 0xFF }, { 0xD8, 0x40, 0x60, 0xFF }, { 0xE0, 0x50, 0x00, 0xFF }, { 0xC0, 0x70, 0x00, 0xFF },
    { 0x88, 0x88, 0x00, 0xFF }, { 0x50, 0xA0, 0x00, 0xFF }, { 0x48, 0xA8, 0x10, 0xFF }, { 0x48, 0xA0, 0x68, 0xFF },
    { 0x40, 0x90, 0xC0, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF },

    { 0xFF, 0xFF, 0xFF, 0xFF }, { 0x60, 0xA0, 0xFF, 0xFF }, { 0x50, 0x80, 0xFF, 0xFF }, { 0xA0, 0x70, 0xFF, 0xFF },
    { 0xF0, 0x60, 0xFF, 0xFF }, { 0xFF, 0x60, 0xB0, 0xFF }, { 0xFF, 0x78, 0x30, 0xFF }, { 0xFF, 0xA0, 0x00, 0xFF },
    { 0xE8, 0xD0, 0x20, 0xFF }, { 0x98, 0xE8, 0x00, 0xFF }, { 0x70, 0xF0, 0x40, 0xFF }, { 0x70, 0xE0, 0x90, 0xFF },
    { 0x60, 0xD0, 0xE0, 0xFF }, { 0x60, 0x60, 0x60, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF },

    { 0xFF, 0xFF, 0xFF, 0xFF }, { 0x90, 0xD0, 0xFF, 0xFF }, { 0xA0, 0xB8, 0xFF, 0xFF }, { 0xC0, 0xB0, 0xFF, 0xFF },
    { 0xE0, 0xB0, 0xFF, 0xFF }, { 0xFF, 0xB8, 0xE8, 0xFF }, { 0xFF, 0xC8, 0xB8, 0xFF }, { 0xFF, 0xD8, 0xA0, 0xFF },
    { 0xFF, 0xF0, 0x90, 0xFF }, { 0xC8, 0xF0, 0x80, 0xFF }, { 0xA0, 0xF0, 0xA0, 0xFF }, { 0xA0, 0xFF, 0xC8, 0xFF },
    { 0xA0, 0xFF, 0xF0, 0xFF }, { 0xA0, 0xA0, 0xA0, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF }
};