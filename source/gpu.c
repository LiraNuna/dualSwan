#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "rom.h"
#include "nec.h"
#include "io.h"
#include "gpu.h"

	// RAM is also VRAM
extern u8 *internalRam;

	// HW Sprite Table
SpriteEntry* SPRITES = (SpriteEntry*)0x07000000;

bool gpuIsColor;
bool isFrame = true;
u8 gpuCurScanline = 0;

	// RAM gpu re-locateable locations
u16 bgMap;
u16 fgMap;
u16 sprTbl;

	// Look Up Tables
u32 layeredTileLUT[0x100];
u8  packedTileLUT4bpp[0x100];
const u32 winLUT[0x2] = {0x0039003B, 0x003B0039};

	// The 8 colors pool, for fast access
u16 gpuPalette[0x08];

	// Grey Scale palette (Pool of 16cols)
const u16 palShades[0x10] = {
	RGB15(30, 30, 30),
	RGB15(28, 28, 28),
	RGB15(26, 26, 26),
	RGB15(24, 24, 24),
	RGB15(22, 22, 22),
	RGB15(20, 20, 20),
	RGB15(18, 18, 18),
	RGB15(16, 16, 16),
	RGB15(14, 14, 14),
	RGB15(12, 12, 12),
	RGB15(10, 10, 10),
	RGB15(8, 8, 8),
	RGB15(6, 6, 6),
	RGB15(4, 4, 4),
	RGB15(2, 2, 2),
	RGB15(0, 0, 0),
};

void gpuInit()
{
	u32 i;

		// Generating a LUT mask.
		// Thanks to FluBBa for his help!
	for(i=0; i<256; ++i) {
		u32 bitmask = 0;
		if(i & 0x01) bitmask |= 0x10000000;
		if(i & 0x02) bitmask |= 0x01000000;
		if(i & 0x04) bitmask |= 0x00100000;
		if(i & 0x08) bitmask |= 0x00010000;
		if(i & 0x10) bitmask |= 0x00001000;
		if(i & 0x20) bitmask |= 0x00000100;
		if(i & 0x40) bitmask |= 0x00000010;
		if(i & 0x80) bitmask |= 0x00000001;
		layeredTileLUT[i] = bitmask;

			// Pixel swap
		packedTileLUT4bpp[i] = i << 4 | i >> 4;
	}
}

void gpuReset()
{
		// That function is so slow....
		// Can someone optimize it? ;P
	gpuCurScanline = 0;
}

	// Speed increase could be gained if this was written in ASM
void gpuWriteByte(u16 offset, register u8 value)
{
		// That's why we're here for
	internalRam[offset] = value;

		// Now for dynamic recompilation!

		// See if byte was written in BG map
	if (offset > bgMap && offset < bgMap + 0x0800) {
		offset = (offset - bgMap) >> 1;
		u16 curBgTile = ((u16*)(internalRam + bgMap))[offset];
			// Tile index + Pallete + (Flip X/Y + Bank)
		((u16*)BG_MAP_RAM(31))[offset] = (curBgTile&0x01FF) | (curBgTile&0x1E00) << 3 | (curBgTile&0xE000) >> 4;
		return;
	}

		// See if byte was written in FG map
	if (offset > fgMap && offset < fgMap + 0x0800) {
		offset = (offset - fgMap) >> 1;
		u16 curFgTile = ((u16*)(internalRam + fgMap))[offset];
			// Tile index + Pallete + (Flip X/Y + Bank)
		((u16*)BG_MAP_RAM(30))[offset] = (curFgTile&0x01FF) | (curFgTile&0x1E00) << 3 | (curFgTile&0xE000) >> 4;
		return;
	}

		// Color mode
	if(ioPort[0x60] & 0x40) {
			// 4bit Tiles (16col)
		if (offset > 0x3FFF && offset < 0xC000) {
			u16 gfxOffset = offset - 0x4000;
			u16 memLoc16 = gfxOffset >> 1;
			u16 memLoc32 = gfxOffset >> 2;

				// Packed mode
			if(ioPort[0x60] & 0x20)
					// Nope, can't get it more optimized then that
				((vu16*)0x06400000)[memLoc16] = ((vu16*)0x6000000)[memLoc16] = packedTileLUT4bpp[value] << 8 | packedTileLUT4bpp[internalRam[offset-1]];
			else // Layared mode

					// Optimization possible is to use an increasing pointer, similer to *var++
					// But GCC is gay and gives warnings
				((vu32*)0x06400000)[memLoc32] = ((vu32*)0x6000000)[memLoc32] = layeredTileLUT[internalRam[offset & 0xFFFC]] | layeredTileLUT[internalRam[(offset & 0xFFFC)+1]] << 1 | layeredTileLUT[internalRam[(offset & 0xFFFC)+2]] << 2 | layeredTileLUT[internalRam[(offset & 0xFFFC)+3]] << 3;

			return;
		}
	} else {
			// 2bit Tiles (4col)
		if (offset > 0x1FFF && offset < 0x4000) {
				// 8bit -> 16bit
			u16 memLoc = (offset - 0x2000) >> 1;

				// Layerd mode
				// Optimization possible - inc. reg and pointer
			((vu32*)0x06400000)[memLoc] = ((vu32*)0x6000000)[memLoc] = layeredTileLUT[internalRam[offset & 0xFFFE]] | layeredTileLUT[internalRam[(offset & 0xFFFE)+1]] << 1;

			return;
		}
	}

		// Sprite Table
	if(offset >= sprTbl && offset < sprTbl + 0x0200) {

			// byte from the beginning of the SPR table
		offset -= sprTbl;

			// Sprite entry to take care of
			// WS/C has 128 sprites just like the GBA/DS
		pSpriteEntry curSprite = &SPRITES[offset >> 2];

			// And plotting our way here
		switch(offset & 0x03) {
			case 0x00:
						// Tile GFX Index - 8bits
					curSprite->attribute[2] &= 0xFF00;
					curSprite->attribute[2] |= value;
				return;
			case 0x01:
						// Note: Possibility to address both as a 32bit value
						// Gain of 1 write instead of 2 ?
						// Priority + Tile Index (LSB) + Palette
					curSprite->attribute[2] &= 0x00FF;
					curSprite->attribute[2] |= (~value & 0x20) << 6 | (value & 0x01) << 8 | (value & 0x0E) << 11;
						// HFlip + VFlip
					curSprite->attribute[1] &= 0x00FF;
					curSprite->attribute[1] |= (value & 0xC0) << 6;
				return;
			case 0x02:
						// Y offset
					curSprite->attribute[0] &= 0xFF00;
					curSprite->attribute[0] |= (value + 24) & 0xFF;
				return;
			case 0x03:
						// X offset
					curSprite->attribute[1] &= 0xFF00;
					curSprite->attribute[1] |= (value + 16) & 0xFF;
				return;
		}
	}

		// Palettes memory
	if (offset > 0xFE00) {
			// Reading 16bit color value (Color is actually 12bit 444xBGR)
		u16 color = ((u16*)internalRam)[offset >> 1];
			// Color is BGR, not RGB as the GBA/DS is
		color = (color & 0xF00) >> 7 | (color & 0xF0) << 2 | (color & 0x0F) << 11;

			// Calculating address
		u16* writeAdd = (u16*)0x05000000 + ((offset & 0x1FF) >> 1);

			// Writing to Palette
		*writeAdd = color;
			// OBJ shared palette
		if(offset > 0xFEFF)
			*(writeAdd + 128) = color;

		return;
	}
}

	// This part can be re-written better in ASM
void gpuWritePort(u16 wsPort, u8 bVal)
{
	u32 i=0;

	switch(wsPort) {
			// Note: Port 0x00 is equavilant to 0x4000000 (almost no need to recompile)
			// Both controls the graphic mode. Only bit order is slightly diffrent
		case 0x00:
					// Those shifts has increasing values.
					// Maybe use an increasing register ?
				*(vu32*)0x04000000 = 0x010000 | (bVal & 0x23) << 8 | (bVal & 0x04) << 10 | isFrame << 11 | (bVal & 0x08) << 12;
					// Setting up window mode
				*(vu32*)0x04000048 = winLUT[(bVal & 0x10) >> 4];
			break;
		case 0x04:
					// Re-locating the Sprite table
					// Only first 5 bits are effective to location
				sprTbl = (bVal & 0x1F) << 9;

					// Re-Converting sprites on memory
				for(; i<128*3; ++i)
					gpuWriteByte(sprTbl+i, internalRam[sprTbl+i]);
			break;
		case 0x05:
		case 0x06:
					// Enabling sprites from 0x05 to 0x06
				for(; i<128; ++i) {
					SPRITES[i].attribute[0] |= 0x0200;
					if(i >= ioPort[0x05]) {
						if(i < ioPort[0x06])
							SPRITES[i].attribute[0] &= ~0x0200;
					}
				}
			break;
		case 0x07:
					// Re-locating BG map and FG map
					// Only 3bits from a nybble are effective
				bgMap = (bVal & 0x07) << 11;
				fgMap = (bVal & 0x70) << 7;

					// Update new map data
				for(; i<32*32*2; ++i) {
					gpuWriteByte(bgMap+i, internalRam[bgMap+i]);
					gpuWriteByte(fgMap+i, internalRam[fgMap+i]);
				}
			break;

		case 0x08: // X0 of FG window
				*(vu8*)0x04000041 = bVal + 16;
			break;
		case 0x09: // Y0 of FG window
				*(vu8*)0x04000045 = bVal + 24;
			break;
		case 0x0A: // X1 of FG window
				*(vu8*)0x04000040 = bVal + 16;
			break;
		case 0x0B: // Y1 of FG window
				*(vu8*)0x04000044 = bVal + 24;
			break;

			// Ports 0x10 - 0x13 fits like a glove on the DS/GBA's HW.
			// 1) OR with 0x04000000
			// 2) Shift left first 4bits once
			// 3) write  232 to address
			// 4) If bit 5 on generated address is set (0x20), add 8
		case 0x10:
				*(vu16*)0x04000010 = 240 + bVal;
			break;
		case 0x11:
				*(vu16*)0x04000012 = 232 + bVal;
			break;
		case 0x12:
				*(vu16*)0x04000014 = 240 + bVal;
			break;
		case 0x13:
				*(vu16*)0x04000016 = 232 + bVal;
			break;
			// Greyscale Pool
		case 0x1C:
		case 0x1D:
		case 0x1E:
		case 0x1F:
					// Not for Color mode
				if(ioPort[0x60] & BIT(6))
					break;

					// Grey palette Pool
				i = (wsPort - 0x1C) << 1;
				gpuPalette[i] = palShades[bVal & 0x0F];
				gpuPalette[i+1] = palShades[bVal >> 4];

					// Update all palettes with the new colors
				for(i=0x20; i<0x40; ++i)
					gpuWritePort(i, ioPort[i]);
			break;
				// Palletes (Ugly (but fast) code!)
		case 0x20: case 0x21: case 0x22: case 0x23:
		case 0x24: case 0x25: case 0x26: case 0x27:
		case 0x28: case 0x29: case 0x2A: case 0x2B:
		case 0x2C: case 0x2D: case 0x2E: case 0x2F:
		case 0x30: case 0x31: case 0x32: case 0x33:
		case 0x34: case 0x35: case 0x36: case 0x37:
		case 0x38: case 0x39: case 0x3A: case 0x3B:
		case 0x3C: case 0x3D: case 0x3E: case 0x3F:

				wsPort -= 0x20;
				u16* writeAddress = (u16*)0x05000000 + ((wsPort & 0xFE) << 3);

					// If odd, add 2
				writeAddress += (wsPort & 0x01) << 1;

					// 1Byte == 2 Pallete entries
				*writeAddress++ = gpuPalette[bVal & 0x07];
				*writeAddress++ = gpuPalette[(bVal >> 4) & 0x07];

					// Shared OBJ Pallete
				if(wsPort > 0x0F) {
					writeAddress += 126; // -2 previous added
					*writeAddress++ = gpuPalette[bVal & 0x07];
					*writeAddress++ = gpuPalette[(bVal >> 4) & 0x07];
				}
			break;
	}
}

u8 gpuReadPort(u8 port)
{
		// Color / Mono detection.
	if(port == 0xA0) {
		if(gpuIsColor)
			return ioPort[0xA0] | 0x02;
		else
			return ioPort[0xA0] & 0xFD;
	}

	return ioPort[port];
}
