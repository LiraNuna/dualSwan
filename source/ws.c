#include "types.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string.h>

#include "rom.h"
#include "nec.h"
#include "memory.h"
#include "gpu.h"
#include "io.h"
#include "ws.h"

void wsInit(const char* filename)
{
		// Init memory emulation
	memInit(filename);
		// Init GPU
	gpuInit();
		// Init I/O emulation
	ioInit();
		// And reset everything
	wsReset();
}

void wsReset()
{
		// Reset I/O
	ioReset();
		// Reset GPU
	gpuReset();
		// Reset CPU emulator
	nec_reset(0);
}

	// Executes one scanline (256 CPU cycles)
bool wsExecuteLine()
{
	bool frameFin = false;
		
		// Update scanline register
	ioPort[0x02] = gpuCurScanline;
		
		// Execute NEC CPU core
		// Hblank : 256 CPU cycles
	nec_execute(256);
		
		// Update BG color to background color
	*(u16*)0x05000000 = *((u16*)0x05000000 + ioPort[0x01]);
		
		// Increase scanline and check for Vertical Blank
	if(++gpuCurScanline == 144)
		frameFin = true;
	   
	   // Vblank : 159 Hblank = 159*256/3072000 = 75.47 Hz
	if(gpuCurScanline > 158) {
		gpuCurScanline = 0;
			
			// VBlank End IRQ
		if((ioPort[0xB2] & BIT(5))) {
				
				// ?!
			if(ioPort[0xA7] != 0x35) {
					// Clear IRQ Flag
				ioPort[0xB6] &= ~BIT(5);
					// Exec IRQ
				nec_int((ioPort[0xB0] + 5) << 2);
			}
		}
	}
		
	ioPort[0x02] = gpuCurScanline;
		
		// VBlank IRQ
	if(frameFin) {
		if(ioPort[0xB2] & BIT(6)) {
				// Clear IRQ Flag
			ioPort[0xB6] &= ~BIT(6);
				// Exec IRQ
			nec_int((ioPort[0xB0] + 6) << 2);
		}
	}
		
		// HBlank IRQ
	if(ioPort[0xA4] && (ioPort[0xb2] & BIT(7))) {
		if(!ioPort[0xA5])
			ioPort[0xA5] = ioPort[0xA4];
			
		if(ioPort[0xA5])
			--ioPort[0xA5];
			
			// HBlank IRQ Active?
		if(!ioPort[0xA5] && (ioPort[0xB2] & BIT(7))) {
				// Clear IRQ Flag
			ioPort[0xB6] &= ~BIT(7);
				// Exec IRQ
			nec_int((ioPort[0xB0] + 7) << 2);
		}
	}
		
		// Hblank Timer IRQ
	if((ioPort[0x02] == ioPort[0x03]) && (ioPort[0xb2] & BIT(4))) {
			// Clear IRQ Flag
		ioPort[0xb6] &= ~BIT(4);
			// Exec IRQ
		nec_int((ioPort[0xB0] + 4) << 2);
	}
		
		// Is frame finished?
	return frameFin;
}
