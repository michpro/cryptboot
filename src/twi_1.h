/**
 * \file    twi_1.h
 * \brief   Library for handling TWI/I2C bus in Master mode
 *          for tinyAVR 0-, 1- and 2-series, and megaAVR 0-series.
 *
 * \copyright SPDX-FileCopyrightText: Copyright 2021 Michal Protasowicki
 *
 * \license SPDX-License-Identifier: MIT
 *
 */

#ifndef TWI_1_H_
#define TWI_1_H_

#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>

/**
 * \brief Macro calculating value describing clock frequency of the I2C bus
 */
#define TWI_BAUD(fcpu, fscl, trise) (uint8_t)(((fcpu/fscl) - (((fcpu*trise)/1000)/1000)/1000 - 10)/2)

#define TWI_ACK             true
#define TWI_NACK            false

static void twiInit(uint8_t baud);
static uint8_t twiStart(uint8_t deviceAddr);
static uint8_t twiRead(uint8_t *data, bool ackFlag);
static uint8_t twiWrite(uint8_t data);
static void twiStop(void);
static void twiRelease(void);
static bool isDeviceOnBus(uint8_t deviceAddr);
static void twiEepromRead(const uint8_t deviceAddr, const uint16_t address, uint8_t *data, uint8_t length);
static void twiBeginRead(const uint8_t deviceAddr, const uint16_t address);

/**
 * \brief Initialization of the TWI module in the Master mode.
 * 
 * \param[in] baud a value describing clock frequency of the I2C bus
 * 
 * \return nothing
 */
static void twiInit(uint8_t baud)
{
    PORTB_PIN0CTRL |= PORT_PULLUPEN_bm;
    PORTB_PIN1CTRL |= PORT_PULLUPEN_bm;
    TWI0.MBAUD = baud;
    TWI0.MCTRLB |= TWI_FLUSH_bm;
    TWI0.MCTRLA = TWI_TIMEOUT_200US_gc | TWI_SMEN_bm | TWI_ENABLE_bm;
    TWI0.MSTATUS |= (TWI_BUSSTATE_IDLE_gc | TWI_RIF_bm | TWI_WIF_bm);
}

/**
 * \brief Function that sends a "START" condition to the I2C bus.
 * 
 * \param[in] deviceAddr address of the device with which we start communication (in 8-bit format)
 * 
 * \return status after request has been made
 */
static uint8_t twiStart(uint8_t deviceAddr)
{
    if ((TWI0.MSTATUS & TWI_BUSSTATE_gm) != TWI_BUSSTATE_BUSY_gc)
    {
        TWI0.MCTRLB &= ~(TWI_ACKACT_bm);
        TWI0.MADDR = deviceAddr;

        while (!((TWI0.MSTATUS & TWI_WIF_bm) | (TWI0.MSTATUS & TWI_RIF_bm)));
    }

    return TWI0.MSTATUS;
}

/**
 * \brief 
 * 
 * \param[out]  data    a byte of read data from the I2C bus
 * \param[in]   ackFlag ACK or NACK flag to send after data byte read
 * 
 * \return status after request has been made
 */
static uint8_t twiRead(uint8_t *data, bool ackFlag)
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

/**
 * \brief Function writes data byte to device on the I2C bus
 * 
 * \param[in] data data byte for writing to a device on the I2C bus
 * 
 * \return status after request has been made
 */
static uint8_t twiWrite(uint8_t data)
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

/**
 * \brief Function that sends a "STOP" condition to the I2C bus.
 * 
 * \return nothing
 */
static void twiStop(void)
{
    TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
}

/**
 * \brief The function turns off TWI module of the microcontroller.
 * 
 * \return nothing
 */
static void twiRelease(void)
{
    TWI0.MCTRLA &= ~(TWI_ENABLE_bm);
}

/**
 * \brief This function checks whether any device is active on the I2C bus at the given address.
 * 
 * \param[in] deviceAddr address to check (in 8-bit format)
 * 
 * \return true if device is present, false otherwise
 */
static bool isDeviceOnBus(const uint8_t deviceAddr)
{
    bool result = !(twiStart(deviceAddr) & TWI_RXACK_bm);           // checking if slave device replied with an ACK
    twiStop();

    return result;
}

/**
 * \brief Function reading 'n' bytes of data from external I2C EEPROM memory.
 * 
 * \param[in]   deviceAddr  device address (in 8-bit format)
 * \param[in]   address     address of first memory cell to be read
 * \param[out]  data        buffer for read data
 * \param[in]   length      amount of data to be read
 * 
 * \return nothing
 */
static void twiEepromRead(const uint8_t deviceAddr, const uint16_t address, uint8_t *data, uint8_t length)
{
    twiBeginRead(deviceAddr, address);

    length--;
    while(length--)
    {
        twiRead(data, TWI_ACK);
        data++;
    }
    twiRead(data, TWI_NACK);
    twiStop();
}

/**
 * \brief Function starts the sequence of data readings from external I2C EEPROM memory.
 * 
 * \param[in]   deviceAddr  device address (in 8-bit format)
 * \param[in]   address     address of first memory cell to be read
 * 
 * \return nothing
 */
static void twiBeginRead(const uint8_t deviceAddr, const uint16_t address)
{
    twiStart(deviceAddr & 0xFE);
    twiWrite((uint8_t)(address >> 8));
    twiWrite((uint8_t)(address & 0xFF));
    twiStart(deviceAddr | 0x01);
}

#endif // TWI_1_H_