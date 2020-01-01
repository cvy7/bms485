#ifndef AUT_H
#define AUT_H

#define AUT_TIME        (1)
extern unsigned int AUT_time;
void  AUT_init(void);
void  AUT_poll(void);
void AUT_poll_ex(void);

#endif // AUT_H
