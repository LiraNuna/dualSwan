#ifndef __ROM_H__
#define __ROM_H__

#include <stdio.h>
#include "types.h"

#define WS_SYSTEM_MONO			0x00
#define WS_SYSTEM_COLOR			0x01

#define WS_ROM_SIZE_2MBIT		0x01
#define WS_ROM_SIZE_4MBIT		0x02
#define WS_ROM_SIZE_8MBIT		0x03
#define WS_ROM_SIZE_16MBIT		0x04
#define WS_ROM_SIZE_24MBIT		0x05
#define WS_ROM_SIZE_32MBIT		0x06
#define WS_ROM_SIZE_48MBIT		0x07
#define WS_ROM_SIZE_64MBIT		0x08
#define WS_ROM_SIZE_128MBIT		0x09

#define WS_EEPROM_SIZE_NONE		0x00
#define WS_SRAM_SIZE_NONE		0x00
#define WS_EEPROM_SIZE_64k		0x01
#define WS_EEPROM_SIZE_256k		0x02
#define WS_SRAM_SIZE_1k			0x0A
#define WS_SRAM_SIZE_16k		0x14
#define WS_SRAM_SIZE_8k			0x32

// Pushing it to the limits...
// I'de appriciate help on how to load bigger games...
#define ROM_BUFFER_SIZE (2 * 1024 * 1024)

typedef struct ws_romHeaderStruct
{
	u8 developperId;
	u8 minimumSupportSystem;
	u8 cartId;
	u8 unknown;
	u8 romSize;
	u8 eepromSize;
	u8 additionnalCapabilities;
	u8 RTC;
	u16 checkSum;

} PACKED romHeader, *pRomHeader;

extern FILE* wsRom;
extern u32 wsRomSize;
extern u8* romBuffer;

pRomHeader romGetHeader();
void romFillBuffer(u32 offset);
u32 romSramSize();
u32 romEepromSize();
u8 romGetByte(u32 offset);

#endif
