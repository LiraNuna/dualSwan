#include "types.h"
#include <stdlib.h>
#include <nds.h>

#include "rom.h"
#include "memory.h"
#include "ieeprom.h"
#include "gpu.h"
#include "io.h"

extern u8 initialIoValue[0x100];
extern	u8 *externalEeprom;
extern	u32	eepromAddressMask;
extern	u32	romAddressMask;

u8 ioPort[0x100];
u8 rtcReadCount = 0;

void ioReset()
{
	int i=0;

		// Reset I/O Ports
	for (; i<0x100; ++i)
		ioPort[i] = initialIoValue[i];

		// Activate values
	for (i=0; i<0xC9; ++i)
		ioWriteByte(i, initialIoValue[i]);

	rtcReadCount = 0;
}

void ioInit()
{
		// Reset I/O
	ioReset();
}

u8 ioReadByte(u8 port)
{
	u32 i;

		// Read Keys
	u16 KEYS = ~REG_KEYINPUT;

	switch (port) {
		case 0x4E: case 0x4F:
		case 0x50: case 0x51:
		case 0x80: case 0x81:
		case 0x82: case 0x83:
		case 0x84: case 0x85:
		case 0x86: case 0x87:
		case 0x88: case 0x89:
		case 0x8A: case 0x8B:
		case 0x8C: case 0x8D:
		case 0x8E: case 0x8F:
		case 0x90: case 0x91:
		case 0x92: case 0x93:
		case 0x94:
				// 'Normal' ports
			return ioPort[port];
		case 0xAA:
				// Vblank Counter
			return 0xFF;
		case 0xB3:
				// Communication direction
			if (ioPort[0xB3] < 0x80)
				return 0x00;

			if (ioPort[0xB3] < 0xC0)
				return 0x84;

			return 0xC4;
		case 0xB5:
				// Map function keys
			if(ioPort[0xB5] & 0x40)
				return (ioPort[0xB5] & 0xF0) | (KEYS&KEY_START) >> 2 | (KEYS&KEY_A) << 2 | (KEYS&KEY_B) << 2;

				// Map directional keys - Horisontal
			if(ioPort[0xB5] & 0x20)
				return (ioPort[0xB5] & 0xF0) | (KEYS&KEY_UP) >> 6 | (KEYS&KEY_DOWN) >> 5 | (KEYS&KEY_RIGHT) >> 3 | (KEYS&KEY_LEFT) >> 2;

				// Map directional keys - Vertical
		//	if(ioPort[0xB5] & 0x10) {
		//		return (ioPort[0xB5] & 0xF0) | LEFT << 0 | UP << 1 | RIGHT << 2 | DOWN << 3;
		//	}

			return ioPort[0xB5] & 0xF0;
		case 0xBE:
				// internal EEPROM status/command register

				// EEPROM write
			if(ioPort[0xBE] & 0x20)
				return ioPort[0xBE] | 0x02;

				// EEPROM read
			if(ioPort[0xBE] & 0x10)
				return ioPort[0xBE] | 0x01;

				// else ack both
			return ioPort[0xBE] | 0x03;
		case 0xBA:
				// EEPROM even byte read
			i = ioPort[0xBD] << 9 |  ioPort[0xBC] << 1;
			return internalEeprom[i & 0x3FF];
		case 0xBB:
				// EEPROM odd byte read
			i = ioPort[0xBD] << 9 | ioPort[0xBC] << 1;
			return internalEeprom[(i+1) & 0x3FF];
		case 0xC0 :
				// ROM Bank Base Selector for segments 4-$F
			return (ioPort[0xC0] & 0xF) | 0x20;
		case 0xC4:
				// EEPROM eeprom even byte read
			i = ioPort[0xC7] << 8 | ioPort[0xc6] << 1;
			return externalEeprom[i & eepromAddressMask];
		case 0xC5:
				// EEPROM eeprom odd byte read
			i = ioPort[0xC7] << 8 | ioPort[0xc6] <<1;
			return externalEeprom[(i+1) & eepromAddressMask];
		case 0xC8:
				// EEPROM eeprom status/command register

				// ack EEPROM write
			if(ioPort[0xC8] & 0x20)
				return ioPort[0xc8] | 0x2;

				// ack EEPROM read
			if(ioPort[0xC8] & 0x10)
				return ioPort[0xc8] | 0x1;

				// else ack both
			return ioPort[0xc8] | 0x3;
		case 0xCA:
				// RTC Command and status register
			return ioPort[0xCA] | 0x80;
		case 0xCB:
				// RTC data register - get time command
			if(ioPort[0xCA] == 0x15) {

					// Still need to re-direct reads to DS's RTC
				switch(rtcReadCount) {
					case 0:
							rtcReadCount++;
						return 0; // Year
					case 1:
							rtcReadCount++;
						return 0; // Month
					case 2:
							rtcReadCount++;
						return 0; // Day of the month
					case 3:
							rtcReadCount++;
						return 0; // Day of the week
					case 4:
							rtcReadCount++;
						return 0; // Hours
					case 5:
							rtcReadCount++;
						return 0; // Minutes
					case 6:
							rtcReadCount=0;
						return 0; // Seconds
				}
				return 0;
			} else
				return ioPort[0xCB] | 0x80;
	}

	return gpuReadPort(port);
}

void ioWriteByte(u32 port, u8 value)
{
	int i;

	ioPort[port] = value;

	switch (port) {
		case 0x48:	// DMA
					// bit 7 set to start dma transfer
				if(value & 0x80) {
					u32 dmaStart = ioPort[0x40] | ioPort[0x41] << 8 | ioPort[0x42] << 16;
					u32 dmaEnd   = ioPort[0x44] | ioPort[0x45] << 8 | ioPort[0x43] << 16;
					u32 dmaSize  = ioPort[0x46] | ioPort[0x47] << 8;

					while(dmaSize--)
						cpuWriteByte(dmaEnd++, cpuReadByte(dmaStart++));

					ioPort[0x46] = ioPort[0x47] = ioPort[0x48] = 0;
					ioPort[0x41] = dmaStart >> 8;
					ioPort[0x40] = dmaStart & 0xFF;
					ioPort[0x45] = dmaEnd >> 8;
					ioPort[0x44] = dmaEnd & 0xFF;
				}
			return;
		case 0xBA:
				i = ioPort[0xBD] << 9 | ioPort[0xBC] << 1;
				internalEeprom[i & 0x3FF] = value;
			return;
		case 0xBB:
				i = ioPort[0xBD] << 9 | ioPort[0xBC] << 1;
				internalEeprom[(i+1) & 0x3FF] = value;
			return;
		case 0xC0:
				// ROM Bank Base Selector for segments 4-$F
			return;
		case 0xC2:
				// ROM Bank selector for segment 2
			return;
		case 0xC3:
				// ROM Bank selector for segment 2
			return;
		case 0xC4:
				i = ioPort[0xC7] << 8 | ioPort[0xC6] << 1;
				externalEeprom[i & eepromAddressMask] = value;
			return;
		case 0xC5:
				i = ioPort[0xC7] << 8 | ioPort[0xC6] << 1;
				externalEeprom[(i+1) & eepromAddressMask] = value;
			return;
		case 0xCA:
				if(value == 0x15)
					rtcReadCount = 0;
			return;
	}
		// Re-direct GPU ports
	gpuWritePort(port, value);
}
