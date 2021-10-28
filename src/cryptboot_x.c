/**
 * \file    cryptboot_x.c
 * \brief   TWI/I2C Bootloader for tinyAVR 0-, 1- and 2-series, and megaAVR 0-series,
 *          supporting signed and encrypted firmware loaded from external memory.
 *          For the code to be placed in the constructors section it is necessary
 *          to disable standard startup files in Toolchain->AVR/GNU Linker->General.
 *          Bootloader communicates with external memory on standard TWI pins:
 *          TWI0 SCL   PA2
 *          TWI0 SDA   PA1
 *
 * \copyright SPDX-FileCopyrightText: Copyright 2021 by Michal Protasowicki
 *
 * \license SPDX-License-Identifier: MIT
 *
 */

#include "cryptboot_x.h"

static firmwareCfg_t    firmwareConfig;
static bootCfg_t        bootConfig;
static xteaCtx_t        ctx;
static uint8_t          buffer[MAPPED_PROGMEM_PAGE_SIZE];

static bool isBootloaderRequested(void);
static bool isFirmwareSchouldBeProcessed(void);
static bool isFirmwareMacOk(void);
static void processFirmwareData(void);
static void loadBootloaderData(void);

/**
 * \brief   Main boot function.
 *          Put in the constructors section (.ctors) to save Flash.
 *          Naked attribute used since function prologue and epilogue is unused.
 * 
 * \return  function passes execution to the application
 */
__attribute__((naked)) __attribute__((section(".ctors"))) void boot(void)
{
    uint8_t causeOfReset;

    asm volatile("clr r1");                                         // Initialize system for AVR GCC support, expects R1 = 0
    causeOfReset = RSTCTRL.RSTFR;                                   // Get reset cause
    CPU_CCP = CCP_IOREG_gc;                                         // Un-protect protected I/O registers
    CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm;                             // Set main clock prescaler to 2 -> CLK_MAIN = 10 MHz
                                                                    // With this setting, uC will work properly for a power supply in range of 2.7-5.5V 
                                                                    // If WDRF is set OR nothing except BORF is set, that's not bootloader entry condition so jump to app
    if (!(causeOfReset && (causeOfReset & RSTCTRL_WDRF_bm || (!(causeOfReset & (~RSTCTRL_BORF_bm))))))
    {
        twiInit(TWI_BAUD(F_CPU, F_SCL, T_RISE));                    // Initialize I2C interface in Master mode

        if(isBootloaderRequested())                                 // Check if entering application or continuing to bootloader
        {
            processFirmwareData();                                  // Start programming at start for application section
                                                                    // Update timestamp [and encryption key, if present]
            bootConfig.timeStamp = firmwareConfig.timeStamp;        // to prevent firmware from reloading after reboot
            eeprom_update_block((uint8_t *)&bootConfig, (uint8_t *)(MAPPED_EEPROM_SIZE - sizeof(bootConfig)), sizeof(bootConfig));
            eeprom_busy_wait();
            _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);        // Issue system reset
        }

        twiRelease();                                               // Releasing I2C interface before starting application
    }
    RSTCTRL.RSTFR = causeOfReset;                                   // Clear the reset causes before jumping to app
    GPIOR0 = causeOfReset;                                          // but, stash the reset cause in GPIOR0 for use by app
    NVMCTRL.CTRLB = NVMCTRL_BOOTLOCK_bm;                            // Enable Boot Section Lock
    __asm__ __volatile__ (APP_START_JUMP);                          // Go to application, located immediately after boot section
}

/**
 * \brief Boot access request function
 * 
 * \return true if new firmware needs to be loaded, false otherwise 
 */
static bool isBootloaderRequested(void)
{
    register uint8_t result = false;

    if (isDeviceOnBus(TWI_MEM_ADDR))
    {
        loadBootloaderData();
        if (isFirmwareSchouldBeProcessed() && isFirmwareMacOk())
        {
            result = true;
        }
    }

    return result;
}

/**
 * \brief   Auxiliary function that checks the preconditions for further firmware processing
 *          (this allows to bypass time-consuming calculation of the signature by Bootloader if conditions are not correct):
 *          - checking if 'MAC type' is 'CFB-MAC' and MAC size is 8 bytes, and if the encryption algorithm used is XTEA.
 *          - checking if time stamp in the firmware descriptor and time stamp stored in internal EEPROM memory
 *            of the microcontroller are different from each other.
 *          - checking the size of the new firmware.
 * 
 * \return true if further actions can be taken, false if current application needs to be started
 */
static bool isFirmwareSchouldBeProcessed(void)
{
    register uint8_t result = false;

#ifndef DOWNGRADE_ALLOWED
    if (((firmwareConfig.mode & 0xFA) == 0)
        && ((firmwareConfig.timeStamp > bootConfig.timeStamp) || (bootConfig.timeStamp == 0xFFFFFFFF))
        && (firmwareConfig.firmwareSize > 0)
        && (firmwareConfig.firmwareSize <= MAPPED_APPLICATION_SIZE))
#else
    if (((firmwareConfig.mode & 0xFA) == 0)
        && (firmwareConfig.timeStamp != bootConfig.timeStamp)
        && (firmwareConfig.timeStamp != 0xFFFFFFFF)
        && (firmwareConfig.firmwareSize > 0)
        && (firmwareConfig.firmwareSize <= MAPPED_APPLICATION_SIZE))
#endif
    {
         result = true;
    }

    return result;
}

/**
 * \brief   A function that verifies correctness of the signature
 *          of the software contained in the firmware descriptor.
 * 
 * \return true if computed software signature matches that in the firmware descriptor, false otherwise.
 */
static bool isFirmwareMacOk(void)
{
    usize_t startPos        = 0;
    usize_t remainingBytes  = (usize_t)firmwareConfig.firmwareSize;
    uint8_t shift           = (firmwareConfig.firmwareSize < MAPPED_PROGMEM_PAGE_SIZE) ? (uint8_t)firmwareConfig.firmwareSize : MAPPED_PROGMEM_PAGE_SIZE;
    uint8_t result          = false;

    xteaCfbMacInit(&ctx, (uint8_t *)&bootConfig.key, firmwareConfig.macRounds);
    xteaCfbMacUpdate(&ctx, (uint8_t *)&firmwareConfig.version, sizeof(firmwareConfig) - sizeof(firmwareConfig.firmwareMac));

    while (shift)
    {
        twiEepromRead(TWI_MEM_ADDR, (TWI_FIRMWARE_AT_ADDR + startPos), (uint8_t *)&buffer, shift);
        xteaCfbMacUpdate(&ctx, (uint8_t *)&buffer, shift);

        startPos += shift;
        remainingBytes = firmwareConfig.firmwareSize - startPos;
        if (remainingBytes < MAPPED_PROGMEM_PAGE_SIZE)
        {
            shift = remainingBytes;
        }
    }

    xteaCfbMacFinish(&ctx);
    result = xteaCfbMacCmp(&ctx, (uint8_t *)&firmwareConfig.firmwareMac);

    if (!result)                                                    // calculated MAC code does not match code contained in the firmware,
    {                                                               // so stored timestamp is updated to prevent re-attempting to load this faulty firmware
        eeprom_update_dword((uint32_t *)(MAPPED_EEPROM_SIZE - sizeof(uint32_t)), firmwareConfig.timeStamp);
        eeprom_busy_wait();
    }

    return result;
}

/**
 * \brief   A function that reads new firmware from external memory
 *          and, if necessary, decrypts it before writing it to
 *          internal FLASH memory of microcontroller.
 * 
 * \return nothing
 */
static void processFirmwareData(void)
{
    usize_t     remainingBytes  = (usize_t)firmwareConfig.firmwareSize;
    uint8_t   * appPtr          = (uint8_t *)MAPPED_APPLICATION_START;
    uint8_t   * dPtr            = (uint8_t *)&firmwareConfig.newKey;

    xteaSetKey(&(ctx.cipher.base), bootConfig.key);
    xteaSetIv(&(ctx.cipher), firmwareConfig.cipherIv);
    ctx.cipher.base.rounds = firmwareConfig.cipherRounds;
    ctx.cipher.base.operation = xteaDecrypt;
    ctx.dataLength = 0;

    twiBeginRead(TWI_MEM_ADDR, TWI_FIRMWARE_AT_ADDR);

    if ((firmwareConfig.mode & 0x0C) == 0x04)                       // if newKey is present in the firmware then decrypt new encryption key
    {
        xteaCfbBlock(&ctx.cipher, dPtr);
        xteaCfbBlock(&ctx.cipher, (dPtr + XTEA_BLOCK_SIZE));
        memcpy(&bootConfig.key, dPtr, XTEA_KEY_SIZE);
    }

    dPtr = (uint8_t *)&buffer;
    while (remainingBytes--)
    {
        twiRead((dPtr + ctx.dataLength), TWI_ACK);
        ctx.dataLength++;
        if ((ctx.dataLength == XTEA_BLOCK_SIZE) || !remainingBytes)
        {
            if ((firmwareConfig.mode & 0x03) == 0x01)
            {
                xteaCfbBlock(&ctx.cipher, dPtr);
            }
            memcpy(appPtr, dPtr, ctx.dataLength);
            appPtr += ctx.dataLength;
            ctx.dataLength = 0;
        }

        if (!((usize_t)appPtr % MAPPED_PROGMEM_PAGE_SIZE) || !remainingBytes)
        {   // Page boundary reached or no more data to write, Commit page to Flash
            while (NVMCTRL.STATUS & NVMCTRL_FBUSY_bm);
            _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
        }
    }

    twiStop();
}

/**
 * \brief   A function that initializes local variables with the data describing firmware
 *          contained in external memory and the key and timestamp data
 *          of current firmware from internal EEPROM of the microcontroller.
 * 
 * \return nothing
 */
static void loadBootloaderData(void)
{
    twiEepromRead(TWI_MEM_ADDR, TWI_CONTROL_DATA_AT, (uint8_t *)&firmwareConfig, sizeof(firmwareConfig));
    eeprom_read_block((uint8_t *)&bootConfig, (void *)(MAPPED_EEPROM_SIZE - sizeof(bootConfig)), sizeof(bootConfig));
}
