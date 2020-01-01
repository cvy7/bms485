#ifndef PIN_H
#define PIN_H
void PIN_poll();
void PIN_init();
void PIN_pwdn();
extern volatile unsigned int time65;
#define PIN_RX (PIND & 1)
#endif // PIN_H
