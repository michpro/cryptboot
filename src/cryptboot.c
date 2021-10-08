/*
 * TWI/I2C Bootloader for tinyAVR 0- and 1-series, and megaAVR 0-series
 * The device TWI is in slave configuration, receiving data from a TWI/I2C master
 * Each byte sent from the Master is echoed by the slave to confirm reception.
 *
 * For the code to be placed in the constructors section it is necessary
 * to disable standard startup files in Toolchain->AVR/GNU Linker->General.
 *
 * The example is written for ATtiny817 with the following pinout:
 * TWI0 SCL   PA2
 * TWI0 SDA   PA1
 * LED0       PB4
 * SW1        PC5 (external pull-up)
 */

#include "cryptboot.h"

// static uint8_t          data = 0;
// static bool             eepromPresent = false;

static firmwareCfg_t    firmwareConfig;
static bootCfg_t        bootConfig;

// Interface function prototypes
static inline bool isBootloaderRequested(void);
static inline bool isFirmwareTimestampMatch(void);
static inline bool isFirmwareMacOk(void);
static inline void writeFirmware(void);
static inline void loadBootloaderData(void);
static void statusLedInit(void);
static void statusLedToggle(void);

void blink(uint8_t ton, uint8_t toff);
void numToBlinks(uint8_t number);

/**
 * Main boot function
 * Put in the constructors section (.ctors) to save Flash.
 * Naked attribute used since function prologue and epilogue is unused
 */
__attribute__((naked)) __attribute__((section(".ctors"))) void boot(void)
{
    asm volatile("clr r1");                                         // Initialize system for AVR GCC support, expects R1 = 0

    statusLedInit();
    // twiInit(TWI_BAUD(F_CPU, F_SCL, T_RISE));                        // Initialize I2C interface in Master mode
    twiInit(17);

    if(!isBootloaderRequested())                                    // Check if entering application or continuing to bootloader
    {
        NVMCTRL.CTRLB = NVMCTRL_BOOTLOCK_bm;                        // Enable Boot Section Lock

        twiRelease();                                               // Releasing I2C interface before starting application
        appStart_t appStart = (appStart_t)(BOOT_SIZE / sizeof(appStart_t));
        appStart();                                                 // Go to application, located immediately after boot section
    }

    writeFirmware();                                                // Start programming at start for application section
    _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);                // Issue system reset

    // while(true)
    // {
    //     if (eepromPresent)
    //     {
    //         // numToBlinks(data);
    //     } else 
    //     {
    //         // blink(5, 45);
    //     }
    // }
}

/**
 * Boot access request function
 */
static inline bool isBootloaderRequested(void)
{
    register uint8_t result = false;

    // eepromPresent = isDeviceOnBus(TWI_MEM_ADDR);

    // if(eepromPresent)
    if (isDeviceOnBus(TWI_MEM_ADDR))
    {
        loadBootloaderData();
        if (!isFirmwareTimestampMatch() && isFirmwareMacOk())
        {
            result = true;
        }

    }

    // return true;
    return result;
}

static inline bool isFirmwareTimestampMatch(void)
{
    register uint8_t result = false;

    if ((bootConfig.timeStamp != 0xFFFFFFFF) 
        && (firmwareConfig.firmwareSize > 0) && (firmwareConfig.firmwareSize <= MAPPED_APPLICATION_SIZE)
        && (firmwareConfig.timeStamp == bootConfig.timeStamp))
    {
        result = true;
    }

    return result;
}

static inline bool isFirmwareMacOk(void)
{
    uint32_t    startPos        = 0;
    uint32_t    remainingBytes  = firmwareConfig.firmwareSize;
    uint8_t     shift           = (firmwareConfig.firmwareSize < MAPPED_PROGMEM_PAGE_SIZE) ? firmwareConfig.firmwareSize : MAPPED_PROGMEM_PAGE_SIZE;
    uint8_t     buffer[MAPPED_PROGMEM_PAGE_SIZE];
    xteaCtx_t   ctx;

    xteaCfbMacInit(&ctx, bootConfig.key, firmwareConfig.macRounds);
    xteaCfbMacUpdate(&ctx, firmwareConfig.cipherIv, (XTEA_IV_SIZE + 8 + 2));

    while (shift)
    {
        twiEepromRead(TWI_MEM_ADDR, (TWI_FIRMWARE_AT_ADDR + startPos), (uint8_t *)(&buffer), shift);
        xteaCfbMacUpdate(&ctx, buffer, shift);

        startPos += shift;
        remainingBytes = firmwareConfig.firmwareSize - startPos;
        if (remainingBytes < MAPPED_PROGMEM_PAGE_SIZE)
        {
            shift = remainingBytes;
        }
    }

    xteaCfbMacFinish(&ctx);

    return xteaCfbMacCmp(&ctx, firmwareConfig.firmwareMac);
}

static inline void writeFirmware(void)
{
    uint32_t        startPos        = 0;
    uint32_t        remainingBytes  = firmwareConfig.firmwareSize;
    uint8_t         shift           = (firmwareConfig.firmwareSize < XTEA_BLOCK_SIZE) ? firmwareConfig.firmwareSize : XTEA_BLOCK_SIZE;
    uint8_t         buffer[XTEA_BLOCK_SIZE];
    xteaCipherCtx_t ctx;

    // xteaInit(&ctx, bootConfig.key, firmwareConfig.cipherIv, firmwareConfig.cipherRounds);
    // xteaSetOperation(&ctx, xteaDecrypt);
    xteaSetKey(&(ctx.base), bootConfig.key);
    xteaSetIv(&ctx, firmwareConfig.cipherIv);
    ctx.base.rounds = firmwareConfig.cipherRounds;
    ctx.base.operation = xteaDecrypt;

    uint8_t *appPtr = (uint8_t *)MAPPED_APPLICATION_START;
    while (shift)
    {
        twiEepromRead(TWI_MEM_ADDR, (TWI_FIRMWARE_AT_ADDR + startPos), (uint8_t *)(&buffer), shift);
        xteaCfbBlock(&ctx, (uint8_t *)(&buffer));
        memcpy(appPtr, &buffer, shift);
        appPtr += shift;

        startPos += shift;
        remainingBytes = firmwareConfig.firmwareSize - startPos;
        if (remainingBytes < XTEA_BLOCK_SIZE)
        {
            shift = remainingBytes;
            
        }

        if (!((uint16_t)appPtr % MAPPED_PROGMEM_PAGE_SIZE))
        {
            // Page boundary reached, Commit page to Flash
            _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
            while (NVMCTRL.STATUS & NVMCTRL_FBUSY_bm);

            // statusLedToggle();
        }
    }

    // Subtract MAPPED_PROGMEM_START in condition to handle overflow on large flash sizes
    // uint8_t *appPtr = (uint8_t *)MAPPED_APPLICATION_START;
    // while(appPtr - MAPPED_PROGMEM_START <= (uint8_t *)PROGMEM_END)
    // {
    //     /* Receive and echo data before loading to memory */
    //     uint8_t rxData;// = i2cReceive();
    //     // i2cSend(rxData);

    //     // Incremental load to page buffer before writing to Flash
    //     *appPtr = rxData;
    //     appPtr++;
    //     if(!((uint16_t)appPtr % MAPPED_PROGMEM_PAGE_SIZE))
    //     {
    //         // Page boundary reached, Commit page to Flash
    //         _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
    //         while(NVMCTRL.STATUS & NVMCTRL_FBUSY_bm);

    //         statusLedToggle();
    //     }
    // }
}

static inline void loadBootloaderData(void)
{
    twiEepromRead(TWI_MEM_ADDR, TWI_CONTROL_DATA_AT, (uint8_t *)(&firmwareConfig), sizeof(firmwareConfig));
    eeprom_read_block((uint8_t *)&bootConfig, (uint8_t *)(MAPPED_EEPROM_END - sizeof(bootConfig)), sizeof(bootConfig));
}

static void statusLedInit(void)
{
    VPORTA.DIR |= PIN5_bm;                                          // Set LED (PA5) as output
    VPORTA.OUT |= PIN5_bm;                                          // LED off
    // VPORTA.OUT &= ~PIN5_bm;                                         // LED on
}

static void statusLedToggle(void)
{
    VPORTA.OUT ^= PIN5_bm;                                          // Toggle LED0 (PA5)
}

void blink(uint8_t ton, uint8_t toff)
{
    statusLedToggle();
    _delay_ms(ton);
    statusLedToggle();
    _delay_ms(toff);
}

void numToBlinks(uint8_t number)
{
    blink(250, 100);
    for (uint8_t i = 0; i < (number >> 0x04); i++)
    {
        blink(60, 60);
    }
    _delay_ms(200);
    blink(250, 100);
    for (uint8_t i = 0; i < (number & 0x0F); i++)
    {
        blink(60, 60);
    }
    _delay_ms(200);
}
