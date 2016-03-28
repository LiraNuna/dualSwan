#ifndef __MEMORY_H__
#define __MEMORY_H__

extern u8 *staticRam;
extern u8 *internalRam;
extern u8 *externalEeprom;

void memInit(const char* filename);
void cpuWriteByte(u32 addr, u8 value);
u8 cpuReadByte(u32 addr);

#endif
