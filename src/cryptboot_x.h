/**
 * \file    cryptboot_x.h
 * \brief   TWI/I2C Bootloader for tinyAVR 0-, 1- and 2-series, and megaAVR 0-series,
 *          supporting signed and encrypted firmware loaded from external memory.
 *
 * \copyright SPDX-FileCopyrightText: Copyright 2021 by Michal Protasowicki
 *
 * \license SPDX-License-Identifier: MIT
 *
 */

#ifndef CRYPTBOOT_H_
#define CRYPTBOOT_H_

/* Memory configuration
 * BOOTEND_FUSE * 256 must be above Bootloader Program Memory Usage,
 * this is less than 2048 bytes at optimization level -Os, so BOOTEND_FUSE = 0x08
 */
#define BOOTEND_FUSE                0x08
#define BOOT_SIZE                   (BOOTEND_FUSE * 0x100)
#define MAPPED_APPLICATION_START    (MAPPED_PROGMEM_START + BOOT_SIZE)
#define MAPPED_APPLICATION_SIZE     (MAPPED_PROGMEM_SIZE - BOOT_SIZE)
#define APP_START_JUMP              "jmp 2048\n"        

#define F_CPU                       10000000UL
#define F_SCL                       400000UL
#define T_RISE                      300UL
#define TWI_MEM_ADDR                0xA0
#define TWI_MEM_PAGE_SIZE           0x40
#define TWI_FIRMWARE_AT_ADDR        BOOT_SIZE
#define TWI_CONTROL_DATA_AT         TWI_FIRMWARE_AT_ADDR-TWI_MEM_PAGE_SIZE

#include <avr/eeprom.h>
#include <avr/io.h>
#include <stdbool.h>

#include "twi_1.h"
#include "xtea.h"

// Fuse configuration
// BOOTEND sets the size (end) of the boot section in blocks of 256 bytes.
// APPEND = 0x00 defines the section from BOOTEND * 256 to end of Flash as application code.
// Remaining fuses have default configuration.
FUSES = {
    .OSCCFG = FREQSEL_20MHZ_gc,
#if defined(__AVR_ATmega4808__) || defined(__AVR_ATmega4809__) || \
    defined(__AVR_ATmega3208__) || defined(__AVR_ATmega3209__) || \
    defined(__AVR_ATmega1608__) || defined(__AVR_ATmega1609__)
    .SYSCFG0 = CRCSRC_NOCRC_gc,
#else
    .SYSCFG0 = CRCSRC_NOCRC_gc | RSTPINCFG_UPDI_gc,
#endif
    .SYSCFG1 = SUT_8MS_gc,
    .APPEND = 0x00,
    .BOOTEND = BOOTEND_FUSE
};

LOCKBITS = (LB_RWLOCK_gc);

#ifndef BIG_FIRMWARE
typedef uint16_t usize_t;
#else
typedef uint32_t usize_t;
#endif

typedef struct bootCfg
{
    uint8_t                         key[XTEA_KEY_SIZE];
    uint32_t                        timeStamp;
} bootCfg_t;

typedef struct firmwareCfg
{
    uint8_t                         firmwareMac[2 * XTEA_BLOCK_SIZE];
    uint8_t                         version;
    uint8_t                         mode;
    uint8_t                         cipherRounds;
    uint8_t                         macRounds;
    uint32_t                        timeStamp;
    uint32_t                        firmwareSize;
    uint8_t                         cipherIv[2 * XTEA_IV_SIZE];
    uint8_t                         rfu[4];
    uint8_t                         newKey[XTEA_KEY_SIZE];
} firmwareCfg_t;                //  64 bytes length

#endif // CRYPTBOOT_H_