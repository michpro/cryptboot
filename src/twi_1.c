#include "twi_1.h"
#include <avr/io.h>

void twiInit(uint8_t baud)
{
    TWI0.MBAUD = baud;
    TWI0.MCTRLB |= TWI_FLUSH_bm;
    // TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm);
    // TWI0.MCTRLA = TWI_SMEN_bm | TWI_ENABLE_bm;
    TWI0.MCTRLA = TWI_TIMEOUT_200US_gc | TWI_SMEN_bm | TWI_ENABLE_bm;
    // TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc;
    TWI0.MSTATUS |= (TWI_BUSSTATE_IDLE_gc | TWI_RIF_bm | TWI_WIF_bm);
}

uint8_t twiStart(uint8_t deviceAddr)
{
    if ((TWI0.MSTATUS & TWI_BUSSTATE_gm) != TWI_BUSSTATE_BUSY_gc)
    {
        // TWI0.MCTRLB &= ~(TWI_ACKACT_bm);
        TWI0.MADDR = deviceAddr;

        while (!((TWI0.MSTATUS & TWI_WIF_bm) | (TWI0.MSTATUS & TWI_RIF_bm)));
        // if (deviceAddr & 0x01)
        // {
        //     while (!(TWI0.MSTATUS & TWI_RIF_bm))
        //     {
        //         // TODO timeout
        //     }
        // } else
        // {
        //     while (!(TWI0.MSTATUS & TWI_WIF_bm))
        //     {
        //         // TODO timeout
        //     }
        // }
    }

    return TWI0.MSTATUS;
}

uint8_t twiRead(uint8_t *data, bool ackFlag)
{
    if ((TWI0.MSTATUS & TWI_BUSSTATE_gm) == TWI_BUSSTATE_OWNER_gc)
    {
        while (!(TWI0.MSTATUS & TWI_RIF_bm))
        {
            // TODO timeout
        }

        if (ackFlag)
        {
            TWI0.MCTRLB &= ~(TWI_ACKACT_bm);                        // Send ACK
        } else
        {
            TWI0.MCTRLB |= TWI_ACKACT_bm;                           // Send NACK
        }

        *data = TWI0.MDATA;
    }

    return TWI0.MSTATUS;
}

uint8_t twiWrite(uint8_t data)
{
    if ((TWI0.MSTATUS & TWI_BUSSTATE_gm) == TWI_BUSSTATE_OWNER_gc)
    {
        while (!((TWI0.MSTATUS & TWI_WIF_bm) | (TWI0.MSTATUS & TWI_RXACK_bm)))
        {
            // TODO timeout
        }

        TWI0.MDATA = data;
    }

    return TWI0.MSTATUS;
}

void twiStop(void)
{
    TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
}

void twiRelease(void)
{
    TWI0.MCTRLA &= ~(TWI_ENABLE_bm);

    VPORTB.DIR &= ~(PIN0_bm | PIN1_bm);
    VPORTB.OUT &= ~(PIN0_bm | PIN1_bm);
}

bool isDeviceOnBus(const uint8_t deviceAddr)
{
    // bool result = !(twiStart(deviceAddr & 0xFE) & TWI_RXACK_bm);  // checking if slave device replied with an ACK
    bool result = !(twiStart(deviceAddr) & TWI_RXACK_bm);  // checking if slave device replied with an ACK
    twiStop();

    return result;
}

uint8_t twiEepromRead(const uint8_t deviceAddr, const uint16_t address, uint8_t *data, uint8_t length)
{
    twiStart(deviceAddr & 0xFE);
    twiWrite((uint8_t)(address >> 8));
    twiWrite((uint8_t)(address & 0xFF));
    twiStart(deviceAddr | 0x01);
    length--;
    while(length--)
    {
        twiRead(data++, TWI_ACK);
    }
    twiRead(data, TWI_NACK);
    twiStop();

    return 0;
}
