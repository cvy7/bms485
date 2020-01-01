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
 ****************************************************************************/
//****************************************************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include "defs.h"
#include "mb.h"
#include "mb_regs.h"
#include "pin.h"



unsigned int i_adc0,i_adc1;
char adc_state;
volatile unsigned int time65;

void PIN_init(){

    wdt_enable(WDTO_2S);
    wdt_reset();

    //USART0 modbus 485
    UCSR0B=(1<<TXEN0)|(1<<RXEN0);
    UCSR0C=(1<<USBS0)|(1<<UCSZ00)|(1<<UCSZ01);//UPM11 -Even Parity
    UBRR0H=0;
    UBRR0L=1;//115200 3,6864 МГц
    // 0 таймер - шим и системное время одновременно
    OCR0A=0;
    DDRD|=(1<<6);
    TCCR0A=(1<<COM0A1)|(1<<WGM00);
    TCCR0B=3;
    TIMSK0=(1<<TOV0);

    //ацп + бит для включения термодатчика- он снимается при преходе в спячку тк потребляет
    ADMUX=(1<<REFS0)|14;//Измерение опорного относительно VCC
    ADCSRA=(1<<ADEN)|(1<<ADSC)|(1<<ADATE)|(1<<ADIF)|(1<<ADIE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
    PORTC|=(1<<4);
    DDRC|=(1<<4);
}

void PIN_poll(){
OCR0A=usRegHoldingBuf[0];
}

SIGNAL(TIMER0_OVF_vect) //системный таймер
{
          time65++;
}

SIGNAL(ADC_vect)
{
    int adc=ADC,adc0,adc1;
    adc_state++;
    if(adc_state&0x7c){
        if(adc_state&0x80){//измеряем температуру
            adc1=i_adc1>>5;
            i_adc1+=adc;
            i_adc1-=adc1;
            usRegInputBuf[1]=adc1;
            if(adc_state==(128+64)) usRegDiscBuf[1]=1;//ADC1_READY
        }
        else {//измеряем напряжение
            adc0=i_adc0>>5;
            i_adc0+=adc;
            i_adc0-=adc0;
            usRegInputBuf[0]=adc0;
            if(adc_state==64) usRegDiscBuf[0]=1;//ADC0_READY
        }
    }
    else if(adc_state==128) ADMUX=(1<<REFS0)|1;
    else if(adc_state==0  ) ADMUX=(1<<REFS0)|14;
}

void PIN_pwdn(){

}

void led(char L)
{
if(L){
    PORTC&=~1;
    DDRC|=1;
}
else {
    DDRC&=~1;
}
}



