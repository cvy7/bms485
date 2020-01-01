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

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/twi.h>
#include <stdio.h>
#include <string.h>
//**********************************************************************************************************/
#include "mb.h"
#include "mbport.h"
#include "aut.h"
#include "pin.h"
#include "eeprom.h"
//**********************************************************************************************************
void  main(void){
PIN_init();
PMODBUS_init();
AUT_init();
for(;;){
sei();
    if(time65-AUT_time){//112,5 Гц 8888.8 мкс
        AUT_time=time65;
        PMODBUS_rx();
        AUT_poll();
        PIN_poll();
    }
    if((time65-VAR_time)>=VAR_TIME){//112,5 Гц 8888.8 мкс
        VAR_time=time65;
        VAR_eeprom_poll();
    }
    ( void )eMBPoll(  );
    AUT_poll_ex();
    //led(time65 & 1);
    wdt_reset();
  }
}


