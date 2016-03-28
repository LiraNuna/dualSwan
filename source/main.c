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
void handleVBlank() {
	dsFrm += 1;

		// ~60Hz
	if(dsFrm > 59) {
		FPS = wsFrm;
		wsFrm = dsFrm = 0;
	}
}

void videoInit(void) {
	// Power up
	powerOn(POWER_ALL_2D);

	// Bottom screen for text
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);

	// Set up VRAM banks
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	vramSetBankB(VRAM_B_MAIN_SPRITE);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);

	// Clearing VRAM for M3/SC/G6 - size of two tile bases (32Kbyte)
	memset(BG_BMP_RAM(0), 0, 32 << 10);

	// Set up BGs with permenent option
	REG_BG0CNT = BG_MAP_BASE(31) | BG_TILE_BASE(0) | BG_PRIORITY_3; // BG
	REG_BG1CNT = BG_MAP_BASE(30) | BG_TILE_BASE(0) | BG_PRIORITY_1; // FB

	// First time offsets
	REG_BG0HOFS = 240;
	REG_BG0VOFS = 232;
	REG_BG1HOFS = 240;
	REG_BG1VOFS = 232;

	// Setting up border
	REG_BG3CNT = BG_MAP_BASE(29) | BG_TILE_BASE(2) | BG_PRIORITY_0;

	// Filling a tile
	memset(BG_TILE_RAM(2), ~0, 0x20);

	for(int i=2; i<30; ++i) {
		for(int j=3; j<21; ++j) {
			BG_MAP_RAM(29)[i + (j << 5)] = 1;
		}
	}

	// Apply blending at full black on BG3 (Protects palettes changes)
	REG_BLDCNT = BLEND_FADE_BLACK | BLEND_SRC_BG3 | BLEND_DST_BACKDROP;
	REG_BLDY = 0xFFFF;

	// Sub screen menu text
	REG_BG0CNT_SUB = BG_MAP_BASE(31) | BG_TILE_BASE(0) | BG_PRIORITY_0;
	dmaCopyAsynch(font_bin, (u16*)BG_TILE_RAM_SUB(0), font_bin_size);
	BG_PALETTE_SUB[1]  = RGB15(0x1F, 0x1F, 0x1F);
	BG_PALETTE_SUB[17] = RGB15(0x00, 0x1F, 0x00);

	// Init IRQ
	irqInit();
	irqEnable(IRQ_VBLANK);
	irqSet(IRQ_VBLANK, handleVBlank);
}

int main() {
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
