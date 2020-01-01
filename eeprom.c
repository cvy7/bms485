#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include <string.h>
#include "mb_regs.h"
#include "pin.h"

#define EEP_MAGIC (0xdeadbeaf)
#define REG_HOLDING_REG_EEP 2
#define LEN_BYTES       16

uint16_t VAR_time;
uint8_t eep_buff[LEN_BYTES];
uint16_t wr_cntr=0;
uint8_t  dis_wr=0;

#if 0
unsigned char EEPROM_read(unsigned int uiAddress)
{
/* Wait for completion of previous write */
while(EECR & (1<<EEPE));
/* Set up address register */
cli();
EEAR = uiAddress;
/* Start eeprom read by writing EERE */
EECR |= (1<<EERE);
sei();
/* Return data from Data Register */
return EEDR;
}

void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{
/* Wait for completion of previous write */
while(EECR & (1<<EEPE));
/* Set up address and Data Registers */
cli();
EEAR = uiAddress;
EEDR = ucData;
/* Write logical one to EEMPE */
EECR |= (1<<EEMPE);
/* Start eeprom write by setting EEPE */
EECR |= (1<<EEPE);
sei();
}

uint8_t eeprom_read_byte2(uint8_t *p){
  return EEPROM_read((unsigned int)p);
}

uint32_t eeprom_read_dword2(uint8_t *p){
   uint32_t res;
   res=eeprom_read_byte2 (p+3);
   res<<=8;
   res|=eeprom_read_byte2(p+2);
   res<<=8;
   res|=eeprom_read_byte2(p+1);
   res<<=8;
   res|=eeprom_read_byte2(p);
   return  res;
}

void eeprom_update_byte2(uint8_t *p, uint8_t byte2){
  uint8_t byte=EEPROM_read((unsigned int)p);
  if(byte!=byte2) EEPROM_write((unsigned int)p,byte2);
}

void eeprom_update_dword2(uint8_t *p,uint32_t dword){
eeprom_update_byte2(p,dword);
dword>>=8;
eeprom_update_byte2(p+1,dword);
dword>>=8;
eeprom_update_byte2(p+2,dword);
dword>>=8;
eeprom_update_byte2(p+3,dword);
}
#endif

uint8_t VAR_ma23(uint8_t n1,uint8_t n2,uint8_t n3){
    if     (n1 == n2) return n1;
    else if(n1 == n3) return n1;
    else if(n3 == n2) return n3;
    else {
        eeprom_update_dword((uint32_t *) (4),0);
        for(;;);
        return 0;
    }
}

void VAR_eeprom_poll(){
    uint8_t  *p;
    p=&usRegHoldingBuf[REG_HOLDING_REG_EEP];

if(dis_wr) return;

for(int i=0; i<LEN_BYTES; i++){
    if(*p != eep_buff[i]){
        eep_buff[i]=*p;
        eeprom_update_byte((uint8_t *) (8+i),eep_buff[i]);
        eeprom_update_byte((uint8_t *) (8+i+LEN_BYTES),eep_buff[i]);
        eeprom_update_byte((uint8_t *) (8+i+2*LEN_BYTES),eep_buff[i]);
        wr_cntr++;    
    }
    p++;
}
if (wr_cntr> 10000) dis_wr=1;
}

void VAR_eeprom_init(){
    uint32_t eep_magic=eeprom_read_dword( (uint32_t *) (4) );
    uint8_t  *p;
    p=&usRegHoldingBuf[REG_HOLDING_REG_EEP];
    if (eep_magic == EEP_MAGIC) {
        for(int i=0; i<LEN_BYTES; i++)
            //eep_buff[i]=*p++=eeprom_read_byte((uint8_t *) (8+i) );
            eep_buff[i]=*p++=VAR_ma23(eeprom_read_byte((uint8_t *) (8+i)),
                                      eeprom_read_byte((uint8_t *) (8+i+LEN_BYTES)),
                                      eeprom_read_byte((uint8_t *) (8+i+2*LEN_BYTES)));
    }
    else {
        for(int i=0; i<LEN_BYTES; i++) eep_buff[i]=(*p++)+1;
        VAR_eeprom_poll();
        eeprom_update_dword((uint32_t *) (4),EEP_MAGIC);
    }
}

