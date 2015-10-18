#ifndef __NEC_H__
#define __NEC_H__

#include "types.h"
#include "memory.h"
#include "io.h"
	// DEBUG TEXT
#include "menu.h"

typedef enum { ES, CS, SS, DS } SREGS;
typedef enum { AW, CW, DW, BW, SP, BP, IX, IY } WREGS;

typedef enum { AL,AH,CL,CH,DL,DH,BL,BH,SPL,SPH,BPL,BPH,IXL,IXH,IYL,IYH } BREGS;

enum {
 NEC_IP=1, NEC_AW, NEC_CW, NEC_DW, NEC_BW, NEC_SP, NEC_BP, NEC_IX, NEC_IY,
 NEC_FLAGS, NEC_ES, NEC_CS, NEC_SS, NEC_DS,
 NEC_VECTOR, NEC_PENDING, NEC_NMI_STATE, NEC_IRQ_STATE };

extern int nec_ICount;

void nec_set_reg(int, unsigned);
int nec_execute(int cycles);
unsigned nec_get_reg(int regnum);
void nec_reset (void *param);
void nec_int(unsigned long wektor);

typedef union
{
    unsigned short w[8];
    unsigned char b[16];
} necbasicregs;

typedef struct
{
 necbasicregs regs;
  unsigned short sregs[4];

 unsigned short ip;

 signed long SignVal;
    unsigned long AuxVal, OverVal, ZeroVal, CarryVal, ParityVal;
 unsigned char TF, IF, DF, MF;
 unsigned long int_vector;
 unsigned long pending_irq;
 unsigned long nmi_state;
 unsigned long irq_state;
 int (*irq_callback)(int irqline);
} nec_Regs;

#endif
