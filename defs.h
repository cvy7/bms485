#define CEVENT_HMI  0x3f
#define CEVENT_HMI_SET1  0x7f
#define CEVENT_HMI_INIT  0xff

#define CEVENT_SET1  6
#define CEVENT_INIT  7

#define CEVENT_SET2  2
#define CEVENT_AUT  1
#define CEVENT_MAIN 0

#define TINT (0)
#define TUINT (1)
#define TLONG (2)
#define TFLOAT (3)
#define TCHAR (4)

#define FRAMING_ERROR (1<<FE0)
#define PARITY_ERROR  (1<<UPE0)
#define DATA_OVERRUN  (1<<DOR0)
#define DATA_REGISTER_EMPTY (1<<UDRE0)

#define SET(PORT,BIT,VAR) if(VAR) PORT|=(1<<BIT); else  PORT&=~(1<<BIT);
#define BIT(PORT,B) ((PORT & (1<<B))>0)

//#ifdef HW1408
// #define USART_TX_vect USART0_TX_vect
// #define USART_RX_vect USART0_RX_vect

//#endif
#define ERR_CONNECT (999)
