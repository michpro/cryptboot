#ifndef CRYPTBOOT_H_
#define CRYPTBOOT_H_

/* Memory configuration
 * BOOTEND_FUSE * 256 must be above Bootloader Program Memory Usage,
 * this is xxx bytes at optimization level -Os, so BOOTEND_FUSE = 0x08
 */
#define BOOTEND_FUSE                (0x08)
#define BOOT_SIZE                   (BOOTEND_FUSE * 0x100)
#define MAPPED_APPLICATION_START    (MAPPED_PROGMEM_START + BOOT_SIZE)
#define MAPPED_APPLICATION_SIZE     (MAPPED_PROGMEM_SIZE - BOOT_SIZE)

#define F_CPU_RESET                 (20E6/6)
#define F_CPU                       20000000UL
#define F_SCL                       400000UL
#define T_RISE                      300UL
#define TWI_MEM_ADDR                0xA0
#define TWI_MEM_ADDR_RD             (TWI_MEM_ADDR | 0x01)
#define TWI_MEM_ADDR_WR             (TWI_MEM_ADDR & 0xFE)
#define TWI_MEM_SIZE                0x4000
#define TWI_MEM_PAGE_SIZE           0x40
#define TWI_FIRMWARE_AT_ADDR        BOOT_SIZE
#define TWI_CONTROL_DATA_AT         (TWI_FIRMWARE_AT_ADDR - TWI_MEM_PAGE_SIZE)
#define XXX ((F_CPU/F_SCL) - (((F_CPU*300)/1000)/1000)/1000 - 10)/2
#define __DELAY_BACKWARD_COMPATIBLE__
#include <util/delay.h>

#include <avr/eeprom.h>
#include <avr/io.h>
#include <stdbool.h>
// #include "USERSIG.h"
#include "twi_1.h"
#include "xtea.h"
uint8_t a = XXX;
/* Fuse configuration
 * BOOTEND sets the size (end) of the boot section in blocks of 256 bytes.
 * APPEND = 0x00 defines the section from BOOTEND*256 to end of Flash as application code.
 * Remaining fuses have default configuration.
 */
FUSES = {
    .OSCCFG = FREQSEL_20MHZ_gc,
    .SYSCFG0 = CRCSRC_NOCRC_gc | RSTPINCFG_UPDI_gc,
    .SYSCFG1 = SUT_8MS_gc,
    .APPEND = 0x00,
    .BOOTEND = BOOTEND_FUSE
};

// Define application pointer type
typedef void (*const appStart_t)(void);

typedef struct bootCfg
{
    uint8_t                         key[XTEA_KEY_SIZE];
    uint32_t                        timeStamp;
} bootCfg_t;

typedef struct firmwareCfg
{
    uint8_t                         firmwareMac[XTEA_BLOCK_SIZE];
    uint8_t                         cipherIv[XTEA_IV_SIZE];
    uint32_t                        timeStamp;
    uint32_t                        firmwareSize;
    uint8_t                         cipherRounds;
    uint8_t                         macRounds;
    uint8_t                         newKey[XTEA_KEY_SIZE];
} firmwareCfg_t;

#endif // CRYPTBOOT_H_