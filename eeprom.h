#ifndef EEPROM_H
#define EEPROM_H

#define VAR_TIME 563
//5 sec
extern uint16_t VAR_time;
void eeprom_update_byte2(uint8_t *p, uint8_t byte2);
void VAR_eeprom_poll(void);
void VAR_eeprom_init(void);

#endif // EEPROM_H
