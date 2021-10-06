#include "twi_1.h"
#include <avr/io.h>

void twiInit(uint8_t baud)
{
    TWI0.MBAUD = baud;
    TWI0.MCTRLB |= TWI_FLUSH_bm;
    TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm);
}

void twiEnable(void)
{
    TWI0.MCTRLA = (1 << TWI_SMEN_bp) | (1 << TWI_ENABLE_bp);
    TWI0_MSTATUS |= TWI_BUSSTATE_IDLE_gc;
}

uint8_t twiStart(uint8_t addr)
{
    if ((TWI0.MSTATUS & TWI_BUSSTATE_gm) != TWI_BUSSTATE_BUSY_gc)
    {
        TWI0.MCTRLB &= ~(1 << TWI_ACKACT_bp);
        TWI0.MADDR = addr;

        if (addr & 0x01)
        {
            while (!(TWI0_MSTATUS & TWI_RIF_bm));
        }
        else
        {
            while (!(TWI0_MSTATUS & TWI_WIF_bm));
        }
    }

    return TWI0.MSTATUS;
}

uint8_t twiRead(uint8_t *data, uint8_t sendACK)
{
    if ((TWI0.MSTATUS & TWI_BUSSTATE_gm) == TWI_BUSSTATE_OWNER_gc)
    {
        while (!(TWI0.MSTATUS & TWI_RIF_bm));
        
        if (sendACK)
        {
            TWI0.MCTRLB &= ~(1 << TWI_ACKACT_bp);                   // Send ACK
        } else
        {
            TWI0.MCTRLB |= 1 << TWI_ACKACT_bp;                      // Send NACK
        }

        *data = TWI0.MDATA;
    }

    return TWI0.MSTATUS;
}

uint8_t twiWrite(uint8_t data)
{
    if ((TWI0.MSTATUS & TWI_BUSSTATE_gm) == TWI_BUSSTATE_OWNER_gc)
    {
        while (!((TWI0.MSTATUS & TWI_WIF_bm) | (TWI0_MSTATUS & TWI_RXACK_bm)));
        
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
}
