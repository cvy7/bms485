#define _setL(port,bit) port&=~(1<<bit)
#define _setH(port,bit) port|=(1<<bit)
#define _set(port,bit,val) _set##val(port,bit)
#define on(x) _set (x)
#define SET _setH
#define SET2(PORT,BIT,VAR) if(VAR) PORT|=(1<<BIT); else  PORT&=~(1<<BIT);

#define _clrL(port,bit) port|=(1<<bit)
#define _clrH(port,bit) port&=~(1<<bit)
#define _clr(port,bit,val) _clr##val(port,bit)
#define off(x) _clr (x)
#define CLR _clrH

#define _bitL(port,bit) (!(port&(1<<bit)))
#define _bitH(port,bit) (port&(1<<bit))
#define _bit(port,bit,val) _bit##val(port,bit)
#define signal(x) _bit (x)
#define BIT _bitH

