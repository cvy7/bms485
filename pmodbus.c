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
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include "eeprom.h"
#include "mb.h"
#include "mbport.h"
#include "config.h"

/* ----------------------- Defines ------------------------------------------*/
#define RTU
#define PMODBUS_TIMEOUT 250
// 0.8c

#define REG_INPUT_START         (1)
//(1000+1)
#define REG_INPUT_NREGS         (8)

#define REG_HOLDING_START       (1)
//(2000+1)
#define REG_HOLDING_NREGS       (9)

#define REG_COILS_START         (1)
//(3000+1)
#define REG_COILS_NREGS         (8)
#define REG_COILS_BYTES REG_COILS_NREGS
// /8

#define REG_DISC_START          (1)
//(4000+1)
#define REG_DISC_NREGS          (8)
#define REG_DISC_BYTES REG_DISC_NREGS
// /8

#define SLAVE_ID                (32)

/* ----------------------- Static variables ---------------------------------*/
static USHORT   usRegInputStart = REG_INPUT_START;
/*static*/ USHORT   usRegInputBuf[REG_INPUT_NREGS];//={0x0123,0x4567,0x89AB,0xCDEF};
static USHORT   usRegHoldingStart = REG_HOLDING_START;
/*static*/ USHORT   usRegHoldingBuf[REG_HOLDING_NREGS];//={0x0123,0x4567,0x89AB,0xCDEF,0xDEAD,0xBEEF,0xDEAD,0xBEEF,0xDEAD,0xBEEF};
static USHORT    usRegCoilsStart = REG_COILS_START;
/*static*/ UCHAR     usRegCoilsBuf[REG_COILS_BYTES];//={1,0,0,0,1};
static USHORT    usRegDiscStart = REG_DISC_START;
/*static*/UCHAR      usRegDiscBuf[REG_DISC_BYTES];//={0,1,0,0,0,0,1};

unsigned char PMODBUS_timeout;
unsigned char booting=0;

/* ----------------------- Start implementation -----------------------------*/
//--------------------------------------------------------------------------

eMBErrorCode
eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    if( ( usAddress >= REG_INPUT_START )
        && ( usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - usRegInputStart );
        while( usNRegs > 0 )
        {
            *pucRegBuffer++ = ( unsigned char )( usRegInputBuf[iRegIndex] >> 8 );
            *pucRegBuffer++ = ( unsigned char )( usRegInputBuf[iRegIndex] & 0xFF );
            iRegIndex++;
            usNRegs--;
        }
        PMODBUS_timeout=PMODBUS_TIMEOUT;
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}

eMBErrorCode
eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    if(usAddress==255 && usNRegs==1 && pucRegBuffer[0]==0xde && eMode==MB_REG_WRITE ) {
        eeprom_update_byte(511, 0xde);/*for(;;);*/
        booting=0xde;
        return eStatus;
        } //bootloader trap

    if( ( usAddress >= REG_HOLDING_START ) &&
        ( usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - usRegHoldingStart );
        switch ( eMode )
        {
            /* Pass current register values to the protocol stack. */
        case MB_REG_READ:
            while( usNRegs > 0 )
            {
                *pucRegBuffer++ = ( UCHAR ) ( usRegHoldingBuf[iRegIndex] >> 8 );
                *pucRegBuffer++ = ( UCHAR ) ( usRegHoldingBuf[iRegIndex] & 0xFF );
                iRegIndex++;
                usNRegs--;
            }
            break;

            /* Update current register values with new values from the
             * protocol stack. */
        case MB_REG_WRITE:
            while( usNRegs > 0 )
            {
                usRegHoldingBuf[iRegIndex] = *pucRegBuffer++ << 8;
                usRegHoldingBuf[iRegIndex] |= *pucRegBuffer++;
                iRegIndex++;
                usNRegs--;
                PMODBUS_timeout=PMODBUS_TIMEOUT;
            }
        }
        PMODBUS_timeout=PMODBUS_TIMEOUT;
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}



eMBErrorCode
eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode )
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iRegIndex;

    if ( ( usAddress >= REG_COILS_START )
        && ( usAddress + usNCoils <= REG_COILS_START + REG_COILS_NREGS ) )
    {
        iRegIndex = ( int ) ( usAddress - usRegCoilsStart );
        PMODBUS_timeout=PMODBUS_TIMEOUT;
        switch ( eMode )
        {
            case MB_REG_READ:
            {
                while ( usNCoils > 0 )
                {
                    UCHAR ucResult = usRegCoilsBuf[iRegIndex];//xMBUtilGetBits( usRegCoilsBuf, iRegIndex, 1 );

                    xMBUtilSetBits( pucRegBuffer, iRegIndex - ( usAddress - usRegCoilsStart ), 1, ucResult );

                    iRegIndex++;
                    usNCoils--;
                }

                break;
            }

            case MB_REG_WRITE:
            {
                while ( usNCoils > 0 )
                {
                    UCHAR ucResult = xMBUtilGetBits( pucRegBuffer, iRegIndex - ( usAddress - usRegCoilsStart ), 1 );

                    usRegCoilsBuf[iRegIndex]=ucResult; //xMBUtilSetBits( usRegCoilsBuf, iRegIndex, 1, ucResult );

                    iRegIndex++;
                    usNCoils--;
                    PMODBUS_timeout=PMODBUS_TIMEOUT;
                }

                break;
            }
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}



eMBErrorCode
eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDisc )
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iRegIndex;

    if ( ( usAddress >= REG_DISC_START )
        && ( usAddress + usNDisc <= REG_DISC_START + REG_DISC_NREGS ) )
    {
        iRegIndex = ( int ) ( usAddress - usRegDiscStart );
        PMODBUS_timeout=PMODBUS_TIMEOUT;
                while ( usNDisc > 0 )
                {
                    UCHAR ucResult =usRegDiscBuf[iRegIndex]; //xMBUtilGetBits( usRegDiscBuf, iRegIndex, 1 );

                    xMBUtilSetBits( pucRegBuffer, iRegIndex - ( usAddress - usRegDiscStart ), 1, ucResult );

                    iRegIndex++;
                    usNDisc--;
                }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}





void PMODBUS_init(){
        eMBErrorCode    eStatus;
        eStatus = eMBInit( MB_RTU, SET_MB_ADDR, 0, 115200, MB_PAR_NONE );
        sei(  );
        /* Enable the Modbus Protocol Stack. */
        eStatus = eMBEnable(  );
        //PMODBUS_timeout=PMODBUS_TIMEOUT;
};

void PMODBUS_rx(){


    if(booting==0xde) for(;;);//
}

void PMODBUS_tx(){

}





