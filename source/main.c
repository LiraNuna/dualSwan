#include "types.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include <nds.h>

#include "resetmem.h"
#include "rom.h"
#include "memory.h"
#include "gpu.h"
#include "io.h"
#include "ws.h"
#include "menu.h"

volatile u8 wsFrm = 0;
volatile u8 dsFrm = 0;
volatile u8 FPS = 0;

	// VBlank IRQ to count FPS
void handleVBlank()
{
	++dsFrm;

		// ~60Hz
	if(dsFrm > 59) {
		FPS = wsFrm;
		wsFrm = dsFrm = 0;
	}
}

void videoInit(void)
{
	int i=0, j;

		// Power up
	powerOn(POWER_ALL_2D);

		// Bottom screen for text
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);

		// Set up VRAM banks
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	vramSetBankB(VRAM_B_MAIN_SPRITE);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);

		// Clearing VRAM for M3/SC/G6 - size of two tile bases (32Kbyte)
	memset((u16*)0x06000000, 0, 32 << 10);

		// Set up BGs with permenent option
	*(vu16*)0x04000008 = 31 << 8 | 0x03; // BG
	*(vu16*)0x0400000A = 30 << 8 | 0x01; // FB

		// First time offsets
	*(vu16*)0x04000010 = 240;
	*(vu16*)0x04000012 = 232;
	*(vu16*)0x04000014 = 240;
	*(vu16*)0x04000016 = 232;

		// Setting up border
	*(vu16*)0x0400000E = 2 << 2 | 29 << 8;

		// Filling a tile
	for(; i<0x10; ++i) *((u16*)0x06008000 + i) = ~0;
	for(i=2; i<30; ++i) for(j=3; j<21; ++j)  ((u16*)0x0600E800)[i + (j << 5)] = 1;

		// Apply blending at full black on BG3 (Protects palettes changes)
	*(vu16*)0x04000050 = 3 << 6 | 1 << 3 | 1 << 13;
	*(vu16*)0x04000054 = ~0;

		// sub screen, text.
	REG_BG0CNT = BG_MAP_BASE(31);
	dmaCopy(font_bin, (u16*)BG_TILE_RAM_SUB(0), font_bin_size);
	BG_PALETTE_SUB[1] = ~0;
	BG_PALETTE_SUB[17] = 0x03E0;

		// Init IRQ
	irqInit();
	irqEnable(IRQ_VBLANK);
	irqSet(IRQ_VBLANK, handleVBlank);
}

int main()
{
		// Init DS Video mode
	videoInit();

		// Init FAT drivers
	FAT_InitFiles();

		// File menu
	handleFileMenu();

	while(1) {
			// Execute a frame
		while( !wsExecuteLine() );

		printText(0, 1, 1, "FPS: %3d", FPS);

		handleInGameMenu();

		++wsFrm;
	}

	return 0;
}
