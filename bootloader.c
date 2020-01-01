/*в Makefile
 * patch Makefile boot.patch
*/
/****************************************************************************
 *   Copyright (C) 2010-2020 by cvy7                                        *
 *                                                                          *
 *   This program is free software; you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by   *
 *   the Free Software Foundation; either version 2 of the License.         *
 *                                                                          *
 *   This program is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with this program; if not, write to the                          *
 *   Free Software Foundation, Inc.,                                        *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.              *
 *                                                                          *
 *     AVRUSBBoot - USB bootloader for Atmel AVR controllers                *
 *     Thomas Fischl <tfischl@gmx.de>                                       *
 ****************************************************************************/
//****************************************************************************/

//if 0
//точка входа- первая функция в секции
#include <avr/boot.h>
void BOOTLOADER_SECTION  boot();
void __attribute__ ((section (".bootloader_start"))) __attribute__((noreturn)) boot_entry(){
boot();
}
//******************************************************************************
#include <avr/io.h>
//#include <avr/signal.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/crc16.h>

#define BOOT_ADDR              132

#define BOOT_STATE_IDLE       1
#define BOOT_STATE_WRITE_PAGE 2
#define BOOT_STATE_F          3
#define BOOT_STATE_ADR_MSB    4
#define BOOT_STATE_ADR_LSB    5
#define BOOT_STATE_NREG_MSB   6
#define BOOT_STATE_NREG_LSB   7
#define BOOT_STATE_NBYTES     8
#define BOOT_STATE_DATA_MSB   9
#define BOOT_STATE_DATA_LSB   10
#define BOOT_STATE_CRC_MSB    11
#define BOOT_STATE_CRC_LSB    12

#define BOOT_FUNC_WRITE_PAGE 2
#define BOOT_FUNC_LEAVE_BOOT 1
#define BOOT_FUNC_GET_PAGESIZE 3
/*
Датаграмма модифицированный Modbus RTU
Запрос для основной программы
10   адрес
0x06 функция запись  регистра
00   старший байт адреса
0xff младший байт адреса
0xde старший байт
0xad младший байт
     CRC старший байт
     CRC младший байт
********************************
отвечает основная программа повтором,
или ошибкой
10   адрес
>127 ошибка
код ошибки
CRC
CRC

если нет ошибки переход на бутлоадер
с записью в eeprom по адресу 511 0xde

иначе загрузчик отваливается
******************************************
В 88 меге все получается сделать по стандарту тк длина страницы 64 байта= 32 регистра
Запрос
100  адрес
0x10 функция запись нескольких регистров
XX   старший байт адреса (номер страницы)
XX   младший байт адреса (номер страницы)
0x00 количество реш=гистров старший байт
0x10 кол-во регистров младший байт
00   кол-во байт далее
     старший байт буфера[0]
     младший байт буфера[0]
--------------------------
     старший байт буфера[255]
     младший байт буфера[255]
     CRC старший байт
     CRC младший байт
********************************************
пауза 1,75mc (2mc)
Ответ
100  адрес
0x10 функция запись нескольких регистров
XX   старший байт адреса (номер страницы)
XX   младший байт адреса (номер страницы)
0x01 количество реш=гистров старший байт
0x00 кол-во регистров младший байт
 CRC старший байт
 CRC младший байт
*/
typedef struct {//на место основных переменных
char state;
char port;
unsigned char ucRTUBuf[64];
} tbootvars;

tbootvars * const bootvars=0x100;

void BOOTLOADER_SECTION boot_program_page (uint16_t page, uint8_t *buf);
void BOOTLOADER_SECTION leaveBootloader();
int  BOOTLOADER_SECTION boot_mb_read();
void BOOTLOADER_SECTION boot_eeprom_update_byte (uint8_t *__p, uint8_t __value);
char BOOTLOADER_SECTION boot_uart0_end_of_write();
uint8_t BOOTLOADER_SECTION  boot_eeprom_read_byte (const uint8_t *__p);

//****************************************************************************************************
void BOOTLOADER_SECTION  boot(){
    void (*jump_to_app)(void)=0;
    __asm__ volatile (
    "eor	r1, r1"
                );// ? gcc думает что в r1 всегда 0 ?
SPH=0x04;
SPL=0xff;
SREG=0;
wdt_enable(WDTO_1S);
//leaveBootloader();
for(int i=0x100;i<0x4ff;i++) *(char *)i=0;
char cond=boot_eeprom_read_byte (511);
//jump_to_app();
if(cond==0xde)      {
    UCSR0A |= (1<<TXC0);
    UCSR0A=(1<<RXC0)|(1<<TXC0)|(1<<UDRE0);
    UCSR0B=(1<<TXEN0)|(1<<RXEN0);
    UCSR0C=(1<<UCSZ01)|(1<<UCSZ00)|(1<<USBS0);
    UBRR0L=1;//115200 8n1 @3.6864MHz
    DDRD=(1<<2);
    PORTD=(1<<2);
    //UCSR1A|=(1<<U2X1);
    UBRR0H=0;
    UDR0=BOOT_ADDR;//при каждой загрузке в бутлоадер (например по watchdog у) кидается своим адресом
    while (!(UCSR0A & (1<<TXC0)));
    UCSR0A |= (1<<TXC0);
    PORTD&=~(1<<2);//DTX 485
    char c=UDR0;
}

else leaveBootloader();
PORTB=PORTC=DDRB=DDRC=0;

for(;;){
   bootvars->state = BOOT_STATE_IDLE;
   int r=boot_mb_read();

   uint16_t page =0;
            page =bootvars->ucRTUBuf[2];
            page<<=8;
            page+=bootvars->ucRTUBuf[3];
            //page<<=8;// Адрес передается в страницах
            page<<=1;//Адрес соответствует адресу в словах содержимого- что более логично
   if (r>=0) {  boot_program_page (page,  &bootvars->ucRTUBuf[7]);
               if(boot_program_verify (page,  &bootvars->ucRTUBuf[7])<0) r=-1;
             }
   for(int i=0;i<32000;i++);//только для 485 c RC
   boot_mb_write(r);

   if (r==1) leaveBootloader();
   wdt_reset();
   }
}

void BOOTLOADER_SECTION leaveBootloader() {
      void (*jump_to_app)(void)=0;
      cli();
      boot_rww_enable();
      boot_eeprom_update_byte (511, 1);
      jump_to_app();
}

int BOOTLOADER_SECTION boot_program_verify (uint16_t page, uint8_t *buf)
{   uint16_t i;
    uint16_t adr;
    for (i=0; i<SPM_PAGESIZE; i++)
    {
        adr=page +i;
        uint8_t c = pgm_read_byte( adr);
        if (c!=*(buf+i)) {
            boot_uart_write(0xf1);//boot_uart_write(i);
            return -1;
        }
    }
    return 0;
}

void BOOTLOADER_SECTION boot_program_page (uint16_t page, uint8_t *buf)
{
    uint16_t i;
    uint8_t sreg;
    cli();
    eeprom_busy_wait ();
    if(bootvars->state==BOOT_STATE_CRC_LSB) boot_page_erase (page);//<-Это для дополнительной защиты от случайного повреждения флеши
    if(bootvars->state==BOOT_STATE_CRC_LSB) boot_spm_busy_wait (); // Wait until the memory is erased.
    for (i=0; i<SPM_PAGESIZE; i+=2)
    {
        // Set up little-endian word.
        uint16_t w = *buf++;
        w += (*buf++) << 8;
        boot_page_fill (page + i, w);
    }
    if(bootvars->state==BOOT_STATE_CRC_LSB) boot_page_write (page);// Store buffer in flash page.
    if(bootvars->state==BOOT_STATE_CRC_LSB) boot_spm_busy_wait();  // Wait until the memory is written.
    // Reenable RWW-section again. We need this if we want to jump back
    // to the application after bootloading.
    boot_rww_enable ();
}

int BOOTLOADER_SECTION
bootusMBCRC16( char * pucFrame, int usLen )
{
    int crc=0xffff;
    while( usLen-- )
    {
        crc=_crc16_update(crc, *( pucFrame++ ));//inlined
    }
    return crc;
}

void BOOTLOADER_SECTION boot_uart_write(char c){
        while (!(UCSR0A & (1<<TXC0)) && !(UCSR0A & (1<<UDRE0)));
        UCSR0A |= (1<<TXC0);
        UDR0=c;//таймаут ожидания таким образом определяется watchdog ом
        DDRD|=(1<<2);//DTX 485
        PORTD|=(1<<2);//DTX 485
}

char BOOTLOADER_SECTION boot_uart0_end_of_write(){
    while (!(UCSR0A & (1<<TXC0)));
    UCSR0A |= (1<<TXC0);
    PORTD&=~(1<<2);//DTX 485
    char c=UDR0;
        c= UDR0;//flush uart
        c= UDR0;
    return c;
}

char BOOTLOADER_SECTION boot_uart_read(){
    char c;
    for(;;){//таймаут ожидания таким образом определяется watchdog ом
            if(UCSR0A & (1<<RXC0)) {
                c= UDR0;   
                // boot_uart_write(c);
                // UDR0=c;
                return c;
            }
    }
}

int  BOOTLOADER_SECTION boot_mb_read(){
  unsigned char r,*buf;
  for(;;){
   // switch (bootvars->state) {// создает таблицу во flash-> поэтому if-ы
        if(bootvars->state==BOOT_STATE_CRC_LSB){
            r=boot_uart_read();
            *buf=r;
            int crc=bootusMBCRC16( bootvars->ucRTUBuf, (bootvars->ucRTUBuf[6]+7) );

            if(((crc>>8)&0xff)==*(buf) && (crc&0xff)==*(buf-1) /*bootusMBCRC16( bootvars->ucRTUBuf, (256+9) )*/) {
                if(bootvars->ucRTUBuf[6]==64) return 0;//если количество байт в посылке-64 ucRTUBuf[6]=64;- посылку пишем
                else                          return 1;//если какое то другое количество байт - это последний пакет, даже если он 256 байт
            }                                         //опсле записи покидаем бутлоадер - запись закончена
            boot_uart_write(0xf0);//boot_uart_write(crc>>8);boot_uart_write(crc);
            return -1;//err
        }

        if(bootvars->state==BOOT_STATE_CRC_MSB){
            r=boot_uart_read();
            *buf++=r;
            bootvars->state=BOOT_STATE_CRC_LSB;
            continue;
        }

        if(bootvars->state==BOOT_STATE_DATA_LSB){
            r=boot_uart_read();
            *buf++=r;
            if((buf - bootvars->ucRTUBuf) > (bootvars->ucRTUBuf[6]+6)) bootvars->state=BOOT_STATE_CRC_MSB;
            else                                                       bootvars->state=BOOT_STATE_DATA_LSB;
            wdt_reset();
            continue;
        }

        if(bootvars->state==BOOT_STATE_DATA_MSB){
            r=boot_uart_read();
            *buf++=r;
            bootvars->state=BOOT_STATE_DATA_LSB;
            continue;
        }

        if(bootvars->state==BOOT_STATE_NBYTES){
            r=boot_uart_read();
            *buf++=r;
            bootvars->state=BOOT_STATE_DATA_MSB;
            continue;
        }

        if(bootvars->state== BOOT_STATE_NREG_LSB){
            r=boot_uart_read();
            *buf++=r;
            bootvars->state=BOOT_STATE_NBYTES;
            continue;
        }

        if(bootvars->state==BOOT_STATE_NREG_MSB){
            r=boot_uart_read();
            *buf++=r;
            bootvars->state=BOOT_STATE_NREG_LSB;
            continue;
        }

        if(bootvars->state==BOOT_STATE_ADR_LSB){
            r=boot_uart_read();
            *buf++=r;
            bootvars->state=BOOT_STATE_NREG_MSB;
            continue;
        }

        if(bootvars->state==BOOT_STATE_ADR_MSB){
            r=boot_uart_read();
            *buf++=r;
            bootvars->state=BOOT_STATE_ADR_LSB;
            continue;
        }

        if(bootvars->state==BOOT_STATE_F){
            r=boot_uart_read();
            *buf++=r;
            if(r==0x10) bootvars->state=BOOT_STATE_ADR_MSB;//если 0x10 функция-запись нескольких регистров- принимаем их
            else        bootvars->state=BOOT_STATE_IDLE; //это ошибка синхронизации - ждем адреса
            continue;
        }

        //default:// BOOT_STATE_IDLE ждем адреса бутлодера
        buf=bootvars->ucRTUBuf;
        r=boot_uart_read();
        *buf++=r;
        if(r==BOOT_ADDR) bootvars->state=BOOT_STATE_F;
        wdt_reset();
    }
}


int  BOOTLOADER_SECTION boot_mb_write(int res){
  unsigned char *buf;
  int crc;
  /*пауза 1,75mc (2mc)
  Ответ
  100  адрес
  0x10 функция запись нескольких регистров
  XX   старший байт адреса (номер страницы)
  XX   младший байт адреса (номер страницы)
  0x01 количество реш=гистров старший байт
  0x00 кол-во регистров младший байт
   CRC старший байт
   CRC младший байт
  */
      if(res<0){ //err
         for(;;);// пока все с начала
      }
      else{
      buf=bootvars->ucRTUBuf;
      //boot_uart_write(0);//3нуля с запасом на переключение репитеров
      //boot_uart_write(0);
      //boot_uart_write(0);
      //boot_uart_write(0);
      //boot_uart_write(0);
      boot_uart_write(*buf++);
      boot_uart_write(*buf++);
      boot_uart_write(*buf++);
      boot_uart_write(*buf++);
      boot_uart_write(*buf++);
      boot_uart_write(*buf++);
      crc=bootusMBCRC16( bootvars->ucRTUBuf,6);
      *buf=crc;
      boot_uart_write(*buf++);
      *buf=crc>>8;
      boot_uart_write(*buf++);
      boot_uart0_end_of_write();
      }
}

void BOOTLOADER_SECTION boot_eeprom_update_byte (uint8_t *__p, uint8_t __value){
    __asm__ volatile (
                "mov	r18, r22" "\n\t"
                "sbic	0x1f, 1" "\n\t"
                "rjmp	.-4" "\n\t"
                "out	0x22, r25" "\n\t"
                "out	0x21, r24" "\n\t"
                "sbi	0x1f, 0" "\n\t"
                "sbiw	r24, 0x01" "\n\t"
                "in	r0, 0x20" "\n\t"
                "cp	r0, r18" "\n\t"
                "breq	.+14" "\n\t"
                "out	0x1f, r1" "\n\t"
                "out	0x20, r18" "\n\t"
                "in	r0, 0x3f" "\n\t"
                "cli" "\n\t"
                "sbi	0x1f, 2" "\n\t"
                "sbi	0x1f, 1" "\n\t"
                "out	0x3f, r0" "\n\t"
                "ret" "\n\t"
 );
}
uint8_t BOOTLOADER_SECTION  boot_eeprom_read_byte (const uint8_t *__p){
    __asm__ volatile (
            "sbic	0x1f, 1" "\n\t"
            "rjmp	.-4" "\n\t"
            "out	0x22, r25" "\n\t"
            "out	0x21, r24" "\n\t"
            "sbi	0x1f, 0" "\n\t"
            "eor	r25, r25" "\n\t"
            "in	r24, 0x20" "\n\t"
            "ret"
            );
}



