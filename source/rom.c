#include "types.h"
#include <stdlib.h>
#include <string.h>
#include "rom.h"

u8* romBuffer;
u32 curBufferOffset = 0;

pRomHeader romGetHeader()
{
	pRomHeader wsromHeader = (pRomHeader)malloc(sizeof(romHeader));

		// Header is located in the last 10bytes
		// I thought headers are on the beginning of the file...
	FAT_fseek(wsRom, romGetSize() - 10, SEEK_SET);
	FAT_fread(wsromHeader, 10, 1, wsRom);

	return wsromHeader;
}

u32 romSramSize()
{
	pRomHeader RomHeader = romGetHeader();
	u32 retVal = 0x0000;

	switch (RomHeader->eepromSize & 0xF0) {
		case WS_SRAM_SIZE_1k:
			retVal = 0x0400;
		case WS_SRAM_SIZE_8k:
			retVal = 0x2000;
		case WS_SRAM_SIZE_16k:
			retVal = 0x4000;
	}
		// Not to waste memory
	free(RomHeader);

	return retVal;
}

u32 romEepromSize()
{
	pRomHeader RomHeader = romGetHeader();
	u32 retVal = 0;

	switch (RomHeader->eepromSize & 0x0F) {
		case WS_EEPROM_SIZE_64k:
			retVal = 0x10000;
		case WS_EEPROM_SIZE_256k:
			retVal = 0x40000;
	}
		// Not to waste memory
	free(RomHeader);

	return retVal;
}

void romFillBuffer(u32 offset)
{
	curBufferOffset = offset;
	FAT_fseek(wsRom, offset, SEEK_SET);
	FAT_fread(romBuffer, ROM_BUFFER_SIZE, 1, wsRom);
}

u8 romGetByte(u32 offset)
{
		// Is the requested byte inside our buffer?
	if(offset > curBufferOffset)
		if(offset < curBufferOffset + ROM_BUFFER_SIZE)
			return romBuffer[offset - curBufferOffset];

		// Let's fill the buffer again
	romFillBuffer(offset);

		// And pass the first byte
	return *romBuffer;
}

u32 romGetSize()
{
		// Easy using chishm's implementation
	return wsRom->length;
}
