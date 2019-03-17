#ifndef __DS2482_H
#define __DS2482_H

#include <stdlib.h>
#include <inttypes.h>
#include "driver/i2c.h"
#include "global.h"

#define TRUE    1
#define FALSE   0
#define true    1
#define false   0
#define True    1
#define False   0


/* Bit weights for configuration register */
#define DS2482_G_APU		0x01	/* Active pullup				*/
#define DS2482_G_PPM		0x02	/* Presence Pulse Masking	*/
#define DS2482_G_SPU		0x04	/* Strong Pullup				*/
#define DS2482_G_1WS		0x08	/* 1-wire Speed Overdrive	*/

/* Bit weights for status register */
#define DS2482_S_1WB		0x01	/* 1-Wire Busy 				*/
#define DS2482_S_PPD		0x02	/* Presence Pulse Detected	*/
#define DS2482_S_SD		0x04	/* Short Detected				*/
#define DS2482_S_LL		0x08	/* Logic Level 				*/
#define DS2482_S_RST		0x10	/* Device Reset 				*/
#define DS2482_S_SBR		0x20	/* Single Bit Result 		*/
#define DS2482_S_TSB		0x40	/* Tripet Second Bit			*/
#define DS2482_S_DIR		0x80	/* Branch Direction Taken	*/

#define DS2482_ERR_OK			0
#define DS2482_ERR_START		1
#define DS2482_ERR_ADDRESS		2
#define DS2482_ERR_WRITE		3
#define DS2482_ERR_READ			4
#define DS2482_ERR_NO_DEVICE	5
#define DS2482_ERR_I2C_DEVICE	10

#ifdef __AVR__
typedef enum {
    I2C_ERROR_OK,
    I2C_ERROR_DEV,
    I2C_ERROR_ACK,
    I2C_ERROR_TIMEOUT,
    I2C_ERROR_BUS,
    I2C_ERROR_BUSY
} i2c_err_t;
#endif

  uint8_t ds2482init(uint8_t addr);
  uint8_t ds2482reset(uint8_t address);
  uint8_t ds2482setReadPointer(uint8_t address, uint8_t pointer);
  uint8_t ds2482getConfig(uint8_t address, uint8_t *config);
  uint8_t ds2482setConfig(uint8_t address, uint8_t config);
  uint8_t ds2482owReset(uint8_t address);
  uint8_t ds2482owWriteByte(uint8_t address, uint8_t byte);
  uint8_t ds2482owWriteTriplet(uint8_t address, uint8_t *direction);
  uint8_t ds2482owReadByte(uint8_t address, uint8_t *byte);
#endif
