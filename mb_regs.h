#ifndef MB_REGS_H
#define MB_REGS_H
#define REG_INPUT_NREGS         (8)
#define REG_HOLDING_NREGS       (9)
#define REG_DISC_BYTES          (8)
#define REG_COILS_BYTES          (8)
extern unsigned short usRegInputBuf[REG_INPUT_NREGS];
extern unsigned short usRegHoldingBuf[REG_HOLDING_NREGS];
extern unsigned char  usRegDiscBuf[REG_DISC_BYTES];
extern unsigned char  usRegCoilsBuf[REG_COILS_BYTES];

#endif // MB_REGS_H
