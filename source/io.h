#ifndef __IO_H__
#define __IO_H__

extern	u8 ioPort[0x100];

void ioInit(void);
void ioReset(void);
u8 ioReadByte(u8 port);
void ioWriteByte(u32 port, u8 value);

#endif
