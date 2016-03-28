#ifndef __GPU_H__
#define __GPU_H__

extern	u8 gpuCurScanline;
extern	bool gpuIsColor;

void gpuInit();
void gpuReset();
void gpuReset();
void gpuWriteByte(u16 offset, u8 value);
void gpuWritePort(u16 port, u8 value);
u8 gpuReadPort(u8 port);

#endif
