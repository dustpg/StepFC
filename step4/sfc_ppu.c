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
        const uint16_t index = address >> 10;
        const uint16_t offset = address & (uint16_t)0x3FF;
        assert(ppu->banks[index]);
        return ppu->banks[index] + offset;
    }
    // µ÷É«°åË÷Òı
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
        // Ö»Ğ´¼Ä´æÆ÷
    case 1:
        // 0x2001: Mask ($2001) > write
        // Ö»Ğ´¼Ä´æÆ÷
        assert(!"write only!");
        break;
    case 2:
        // 0x2002: Status ($2002) < read
        // Ö»¶Á×´Ì¬¼Ä´æÆ÷
        data = ppu->status;
        // ¶ÁÈ¡ºó»áÇå³ıVBlank×´Ì¬
        ppu->status &= (uint8_t)SFC_PPUFLAG_VBlank;
        break;
    case 3:
        // 0x2003: OAM address port ($2003) > write
        // Ö»Ğ´¼Ä´æÆ÷
        assert(!"write only!");
        break;
    case 4:
        // 0x2004: OAM data ($2004) <> read/write
        // ¶ÁĞ´¼Ä´æÆ÷
        data = ppu->sprites[ppu->oamaddr++];
        break;
    case 5:
        // 0x2005: Scroll ($2005) >> write x2
        // Ë«Ğ´¼Ä´æÆ÷
    case 6:
        // 0x2006: Address ($2006) >> write x2
        // Ë«Ğ´¼Ä´æÆ÷
        assert(!"write only!");
        break;
    case 7:
        // 0x2007: Data ($2007) <> read/write
        // PPU VRAM¶ÁĞ´¶Ë¿Ú
        ppu->vramaddr += (uint16_t)((ppu->ctrl & SFC_PPUFLAG_VINC32) ? 32 : 1);
        assert(!"not impl");
        break;
    }
    return data;
}


/// <summary>
/// StepFC: Ê¹ÓÃCPUµØÖ·¿Õ¼äĞ´ÈëPPU¼Ä´æÆ÷
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
        // PPU OAM Êı¾İ¶Ë¿Ú
        ppu->sprites[ppu->oamaddr++] = data;
        break;
    case 5:
        // 0x2005: Scroll ($2005) >> write x2
        // PPU ¹ö¶¯Î»ÖÃ¼Ä´æÆ÷ - Ë«Ğ´
        ppu->scroll[ppu->writex2 & 1] = data;
        ++ppu->writex2;
        break;
    case 6:
        // 0x2006: Address ($2006) >> write x2
        // PPU µØÖ·¼Ä´æÆ÷ - Ë«Ğ´
        // Ğ´Èë¸ß×Ö½Ú
        if (ppu->writex2 & 1) {
            ppu->vramaddr = (ppu->vramaddr & (uint16_t)0xFF00) | (uint16_t)data;
        }
        // Ğ´ÈëµÍ×Ö½Ú
        else {
            ppu->vramaddr = (ppu->vramaddr & (uint16_t)0x00FF) | ((uint16_t)data << 8);
        }
        ++ppu->writex2;
        break;
    case 7:
        // 0x2007: Data ($2007) <> read/write
        // PPU VRAMÊı¾İ¶Ë
        *sfc_get_ppu_address(ppu->vramaddr, ppu) = data;
        ppu->vramaddr += (uint16_t)((ppu->ctrl & SFC_PPUFLAG_VINC32) ? 32 : 1);
        break;
    }
}