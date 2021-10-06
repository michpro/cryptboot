#ifndef TWI_1_H_
#define TWI_1_H_

#include <stdint.h>

#define TWI_BAUD(fcpu, fscl) fcpu/fscl/2 > 5 ? fcpu/fscl/2-5 : 0

void twiInit(uint8_t baud);
void twiEnable(void);
uint8_t twiStart(uint8_t addr);
uint8_t twiRead(uint8_t *data, uint8_t sendACK);
uint8_t twiWrite(uint8_t data);
void twiStop(void);
void twiRelease(void);

#endif // TWI_1_H_