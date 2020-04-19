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
//https://aterlux.ru/article/ntcresistor
//https://radioprog.ru/post/185
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include "aut.h"
#include "mb.h"
#include "mbport.h"
#include "mb_regs.h"
#include "ports.h"
#include "config.h"
#include "pin.h"
#include "eeprom.h"
unsigned int    AUT_mb_adr=0;
unsigned int    AUT_time;
char            AUT_ind_time;
unsigned int    AUT_pwdn_time;
char            run=0;
//Дискретные входы
#define INP_ADC0_READY   usRegDiscBuf[0]
#define INP_ADC1_READY   usRegDiscBuf[1]
#define INP_STOP_CHARGE  usRegDiscBuf[1]
#define INP_STOP_DISCHARGE usRegDiscBuf[2]
#define INP_STOP_CHARGE2 usRegDiscBuf[3]
//Аналоговые входы
#define INP_ADCU        usRegInputBuf[0]
#define INP_ADCT        usRegInputBuf[1]
#define INP_U           usRegInputBuf[2]
#define INP_T           usRegInputBuf[3]
#define INP_B           usRegInputBuf[4]
//Дискретные выходы
#define OUT_STOP_BAL    usRegCoilsBuf[0]
#define OUT_STOP_IND    usRegCoilsBuf[1]
#define OUT_EN_PWDN     usRegCoilsBuf[2]
//Аналоговые выходы
#define OUT_PWM         usRegHoldingBuf[0]
#define OUT_IND         usRegHoldingBuf[1]
#define OUT_U_LOW       usRegHoldingBuf[2]
#define OUT_U_MIN_BAL   usRegHoldingBuf[3]
#define OUT_U_MAX_BAL   usRegHoldingBuf[4]
#define OUT_U_MIN_IND   usRegHoldingBuf[5]
#define OUT_U_MAX_IND   usRegHoldingBuf[6]
#define OUT_ADC_REF     usRegHoldingBuf[7]
#define OUT_MB_ADDR     usRegHoldingBuf[8]
//уставки-напряжения в милливольтах

#ifndef SET_U_LOW
#define SET_U_LOW       2550
#endif
//Те 40.80В на батарее
//Минимальное напряжение на элементе 2500 по паспорту
//На батарее 40V

#ifndef SET_U_MIN_BAL
#define SET_U_MIN_BAL   3340
#endif
//те буферное напряжение- соответственно для батареи = 53,44V
//Переход в буферный режим при 3400 на элементе и напряжении батареи = 54,40В

#ifndef SET_U_MAX_BAL
#define SET_U_MAX_BAL   3650
#endif
//Те 58,4 В на батарее
//Максимальное напряжение на элементе 3700 по паспорту
//Те 59,2В на батарее

#ifndef SET_U_MIN_IND
#define SET_U_MIN_IND   3000
#endif
//напряжение 95% разряда при 0.2С

#ifndef SET_U_MAX_IND
#define SET_U_MAX_IND   3300
#endif
//напряжкние 5% разряда при 0.2с

#ifndef SET_ADC_REF
#define SET_ADC_REF     1112UL
#endif

#ifndef SET_MB_ADDR
#define SET_MB_ADDR     32
#endif

void AUT_init(){
  OUT_U_LOW=    SET_U_LOW;
  OUT_U_MIN_BAL=SET_U_MIN_BAL;
  OUT_U_MAX_BAL=SET_U_MAX_BAL;
  OUT_U_MIN_IND=SET_U_MIN_IND;
  OUT_U_MAX_IND=SET_U_MAX_IND;
  OUT_ADC_REF=  SET_ADC_REF;
  OUT_MB_ADDR=  SET_MB_ADDR;
  VAR_eeprom_init();
  //if(OUT_MB_ADDR!=SET_MB_ADDR) eMBInit( MB_RTU, OUT_MB_ADDR, 0, 115200, MB_PAR_NONE );
}

unsigned short norm_out(unsigned int p, unsigned int min, unsigned int max){
    int  delt =max-min;
    long out  =p  -min;
    if(out>0){
      out=(out*255L)/delt;
      if(out>255) out=255;
      return out;
    }
    else return 0;
}

void AUT_poll_ex(){
 if(!PIN_RX) AUT_pwdn_time=0;
}

void AUT_poll(){ 
    static char ind;
    if(OUT_EN_PWDN){
        AUT_pwdn_time++;
        if(!AUT_pwdn_time) PIN_pwdn();
    }

    if(INP_ADCU && INP_ADC0_READY)
        INP_U=(OUT_ADC_REF*1023UL)/INP_ADCU;

    if(INP_ADC1_READY) INP_T=INP_ADCT;

    if(!AUT_ind_time)ind=OUT_IND;
    if(AUT_ind_time>ind) led(0);
    else                 led(1);
    AUT_ind_time++;

    if(INP_U<OUT_U_LOW) INP_STOP_DISCHARGE=1;
    else                INP_STOP_DISCHARGE=0;

    if(INP_U>OUT_U_MAX_BAL) INP_STOP_CHARGE2=1;
    else                    INP_STOP_CHARGE2=0;

    if(INP_U>OUT_U_MIN_BAL) {
        INP_STOP_CHARGE=1;
        AUT_pwdn_time=0;
        INP_B=norm_out(INP_U,OUT_U_MIN_BAL,OUT_U_MAX_BAL);
        if(!OUT_STOP_BAL) OUT_PWM=INP_B;
    }
    else {
        INP_STOP_CHARGE=0;
        if(!OUT_STOP_BAL) OUT_PWM=0;
    }

    if(INP_U>OUT_U_MIN_IND) {
        if(!OUT_STOP_IND) OUT_IND=norm_out(INP_U,OUT_U_MIN_IND,OUT_U_MAX_IND);
    }
    else {
        if(!OUT_STOP_IND) OUT_IND=0;
    }

    if(OUT_MB_ADDR!=AUT_mb_adr) {
        eMBSetSlaveAddr(OUT_MB_ADDR);
        AUT_mb_adr=OUT_MB_ADDR;
    }
}
