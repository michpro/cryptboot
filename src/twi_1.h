#ifndef TWI_1_H_
#define TWI_1_H_

#include <stdbool.h>
#include <stdint.h>

// #define TWI_BAUD(fcpu, fscl) fcpu/fscl/2 > 10 ? (fcpu/fscl/2)-10 : 0
#define TWI_BAUD(fcpu, fscl, trise) (uint8_t)(((fcpu/fscl) - (((fcpu*trise)/1000)/1000)/1000 - 10)/2)

#ifndef TWI_TIMEOUT
#define TWI_TIMEOUT         200
#endif

#define TWI_ACK             true
#define TWI_NACK            false
#define TWI_TIMEOUTERR      (uint8_t)0xFF

void twiInit(uint8_t baud);
uint8_t twiStart(uint8_t deviceAddr);
uint8_t twiRead(uint8_t *data, bool ackFlag);
uint8_t twiWrite(uint8_t data);
void twiStop(void);
void twiRelease(void);
bool isDeviceOnBus(uint8_t deviceAddr);
uint8_t twiEepromRead(const uint8_t deviceAddr, const uint16_t address, uint8_t *data, uint8_t length);

#endif // TWI_1_H_