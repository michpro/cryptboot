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
#define F_CPU_RESET                 (20E6/6)

#include <avr/io.h>
#include <stdbool.h>
// #include "i2c_slave.h"

/* Memory configuration
 * BOOTEND_FUSE * 256 must be above Bootloader Program Memory Usage,
 * this is 490 bytes at optimization level -O3, so BOOTEND_FUSE = 0x02
 */
#define BOOTEND_FUSE                (0x04)
#define BOOT_SIZE                   (BOOTEND_FUSE * 0x100)
#define MAPPED_APPLICATION_START    (MAPPED_PROGMEM_START + BOOT_SIZE)
#define MAPPED_APPLICATION_SIZE     (MAPPED_PROGMEM_SIZE - BOOT_SIZE)

/* Fuse configuration
 * BOOTEND sets the size (end) of the boot section in blocks of 256 bytes.
 * APPEND = 0x00 defines the section from BOOTEND*256 to end of Flash as application code.
 * Remaining fuses have default configuration.
 */
FUSES = {
	.OSCCFG = FREQSEL_20MHZ_gc,
	.SYSCFG0 = CRCSRC_NOCRC_gc | RSTPINCFG_UPDI_gc,
	.SYSCFG1 = SUT_64MS_gc,
	.APPEND = 0x00,
	.BOOTEND = BOOTEND_FUSE
};

// Define application pointer type
typedef void (*const appStart_t)(void);

// Interface function prototypes
static bool isBootloaderRequested(void);
static void twiInit(void);
static uint8_t twiReceive(void);
static void twiSend(uint8_t byte);
static void statusLedInit(void);
static void statusLedToggle(void);

/**
 * Main boot function
 * Put in the constructors section (.ctors) to save Flash.
 * Naked attribute used since function prologue and epilogue is unused
 */
__attribute__((naked)) __attribute__((section(".ctors"))) void boot(void)
{
	asm volatile("clr r1");											// Initialize system for AVR GCC support, expects R1 = 0

	if(!isBootloaderRequested())									// Check if entering application or continuing to bootloader
	{
		NVMCTRL.CTRLB = NVMCTRL_BOOTLOCK_bm;						// Enable Boot Section Lock

		appStart_t appStart = (appStart_t)(BOOT_SIZE / sizeof(appStart_t));				// Go to application, located immediately after boot section
		appStart();
	}

	// Initialize communication interface
	twiInit();
	statusLedInit();

	// Start programming at start for application section
	// Subtract MAPPED_PROGMEM_START in condition to handle overflow on large flash sizes
	uint8_t *appPtr = (uint8_t *)MAPPED_APPLICATION_START;
	while(appPtr - MAPPED_PROGMEM_START <= (uint8_t *)PROGMEM_END)
	{
		/* Receive and echo data before loading to memory */
		uint8_t rxData;// = twiReceive();
		// twiSend(rxData);

		// Incremental load to page buffer before writing to Flash
		*appPtr = rxData;
		appPtr++;
		if(!((uint16_t)appPtr % MAPPED_PROGMEM_PAGE_SIZE))
		{
			// Page boundary reached, Commit page to Flash
			_PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
			while(NVMCTRL.STATUS & NVMCTRL_FBUSY_bm);

			statusLedToggle();
		}
	}

	_PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);				// Issue system reset
}

/**
 * Boot access request function
 */
static bool isBootloaderRequested(void)
{
	if(VPORTC.IN & PIN5_bm)											// Check if SW1 (PC5) is low
	{
		return false;
	}
	return true;
}

/**
 * Communication interface functions
 */
static void twiInit(void)
{
	PORTMUX.CTRLB |= PORTMUX_TWI0_ALTERNATE_gc;						// Switch pins to alternate TWI pin location

	// i2c_slave_init();												// Enable TWI driver
}

static uint8_t twiReceive(void)
{
	// i2c_driver.result = I2C_RESULT_WAITING;							// Poll for transmission complete
	// while(i2c_driver.result != I2C_RESULT_OK)
	// {
	// 	// i2c_slave_isr();
	// }

	// return i2c_driver.received_data[0];
	return 0x00;
}

static void twiSend(uint8_t byte)
{
	// i2c_driver.send_data[0] = byte;

	// i2c_driver.result = I2C_RESULT_WAITING;							// Poll for transmission complete
	// while(i2c_driver.result != I2C_RESULT_OK)
	// {
	// 	i2c_slave_isr();
	// }
}

static void statusLedInit(void)
{
	VPORTB.DIR |= PIN4_bm;											// Set LED0 (PB4) as output
}

static void statusLedToggle(void)
{
	VPORTB.OUT ^= PIN4_bm;											// Toggle LED0 (PB4)
}
