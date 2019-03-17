
#include <stdlib.h>
#include "saric_ds2482.h"
#include <inttypes.h>
#include "driver/i2c.h"
#include "global.h"



#define DS2482_C_DRST	0xF0	/* Device Reset				P_STATUS			None				*/
#define DS2482_C_WCFG	0xD2	/* Write Configuration		P_CONFIG			Config byte		*/
#define DS2482_C_CHSL	0xC3	/* Channel Select				P_CSEL			Channel Code	*/
#define DS2482_C_SRP	0xE1	/* Set Read Pointer			-					Register			*/
#define DS2482_C_OWRS	0xB4	/* 1-wire Reset				P_STATUS			None				*/
#define DS2482_C_OWWB	0xA5	/* 1-wire Write Byte			P_STATUS			Data byte		*/
#define DS2482_C_OWRB	0x96	/* 1-wire Read Byte			P_STATUS			None 				*/
#define DS2482_C_OWSB	0x87	/* 1-wire Single Bit			P_STATUS			Bit byte			*/
#define DS2482_C_OWT	0x78	/* 1-wire Triplet				P_STATUS			Direction Byte	*/

/* Avaiable registers */
#define DS2482_P_STATUS	0xF0	/* Status register			*/
#define DS2482_P_DATA	0xE1	/* Read Data register		*/
#define DS2482_P_CONFIG	0xC3	/* Configuration Register	*/




/* uint8_t ds2482reset(uint8_t address)
 *
 * Reset the DS2482 device on address
 *
 * Can return:
 *  DS2482_ERR_OK
 *  DS2482_ERR_START
 *  DS2482_ERR_ADDRESS
 *  DS2482_ERR_WRITE
 */
uint8_t ds2482reset(uint8_t address)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, address << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, DS2482_C_DRST, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret == ESP_OK)
		return DS2482_ERR_OK;
	else
		return DS2482_ERR_I2C_DEVICE;
}

/* void ds2482init()
 * 
 * Initialize TWI, this routine is designed for 8Mhz operation.
 */
uint8_t ds2482init( uint8_t addr)
{
	esp_err_t ret;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, addr << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret == ESP_OK)
		return DS2482_ERR_OK;
	else
		return DS2482_ERR_I2C_DEVICE;
}



/* uint8_t ds2482setReadPointer(uint8_t address, uint8_t pointer)
 *
 * Set the read pointer of the ds2482.
 * Valid values for pointer is:
 *  DS2482_P_STATUS	
 *  DS2482_P_DATA
 *  DS2482_P_CSEL
 *  DS2482_P_CONFIG	
 *
 * Can return:
 *  DS2482_ERR_OK
 *  DS2482_ERR_START
 *  DS2482_ERR_ADDRESS
 *  DS2482_ERR_WRITE
 *  DS2482_ERR_READ
 */
uint8_t ds2482setReadPointer(uint8_t address, uint8_t pointer)
{
	esp_err_t ret;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, address << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, DS2482_C_SRP, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, pointer, ACK_CHECK_EN);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK)
                return DS2482_ERR_OK;
        else
                return DS2482_ERR_I2C_DEVICE;
}

/* uint8_t ds2482getConfig(uint8_t address, uint8_t *config)
 *
 * Read the configuration from the ds2482, place it in *config.
 *
 * config is to be treated as invalid unless DS2482_ERR_OK was returned.
 *
 * Can return:
 * DS2482_ERR_OK
 * DS2482_ERR_START
 * DS2482_ERR_ADDRESS
 * DS2482_ERR_WRITE
 * DS2482_ERR_READ
 */
uint8_t ds2482getConfig(uint8_t address, uint8_t *config)
{
	/* Move read pointer to config register */
	*config = ds2482setReadPointer(address, DS2482_P_CONFIG);
	if(*config)	/* Error? */
		return *config;

	esp_err_t ret;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, address << 1 | READ_BIT, ACK_CHECK_EN);
	i2c_master_read_byte(cmd, config, NACK_VAL);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK)
                return DS2482_ERR_OK;
        else
                return DS2482_ERR_I2C_DEVICE;

}

/* uint8_t ds2482setConfig(uint8_t address, uint8_t config)
 *
 * Write the configuration in config to the ds2482
 *
 * Can return:
 * DS2482_ERR_OK
 * DS2482_ERR_START
 * DS2482_ERR_ADDRESS
 * DS2482_ERR_WRITE
 */
uint8_t ds2482setConfig(uint8_t address, uint8_t config)
{
	config&= 0x0F;
	config|= (~config)<<4;

	esp_err_t ret;
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, address << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, DS2482_C_WCFG, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, config, ACK_CHECK_EN);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK)
                return DS2482_ERR_OK;
        else
                return DS2482_ERR_I2C_DEVICE;
}



/* uint8_t ds2482owReset(uint8_t address)
 *
 * Reset the 1-wire bus.
 *
 * Can return:
 * DS2482_ERR_OK
 * DS2482_ERR_START
 * DS2482_ERR_ADDRESS
 * DS2482_ERR_WRITE
 * DS2482_ERR_READ
 * DS2482_ERR_NO_DEVICE
 */
uint8_t ds2482owReset(uint8_t address)
{
	uint8_t byte;
        uint8_t p=0;

	esp_err_t ret;
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, address << 1 | WRITE_BIT, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, DS2482_C_OWRS, ACK_CHECK_EN);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        if (ret != ESP_OK)
        	return DS2482_ERR_WRITE;

	/* This will set the read pointer to the status register.
	 * Keep reading the status register until the 1WB byte is cleared.
	 */
	while(1)
	{
		p++;
                if (p>253)
                  return DS2482_ERR_READ;		
		cmd = i2c_cmd_link_create();
        	i2c_master_start(cmd);
        	i2c_master_write_byte(cmd, address << 1 | READ_BIT, ACK_CHECK_EN);
        	i2c_master_read_byte(cmd, &byte, NACK_VAL);
        	i2c_master_stop(cmd);
        	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
        	i2c_cmd_link_delete(cmd);
        	if (ret != ESP_OK)
                	return DS2482_ERR_I2C_DEVICE;

		if(!(byte & DS2482_S_1WB))
			break;
	}

	/* Check if there is any devices detected */
	if(!(byte & DS2482_S_PPD))
		return DS2482_ERR_NO_DEVICE;

	return DS2482_ERR_OK;
}


/* uint8_t ds2482owWriteByte(uint8_t address, uint8_t byte)
 *
 * Write a byte to the 1-wire bus.
 *
 * Can return:
 * DS2482_ERR_OK
 * DS2482_ERR_START
 * DS2482_ERR_ADDRESS
 * DS2482_ERR_WRITE
 */
uint8_t ds2482owWriteByte(uint8_t address, uint8_t byte)
{
	uint8_t p=0;
	esp_err_t ret;
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, address << 1 | WRITE_BIT, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, DS2482_C_OWWB, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, byte, ACK_CHECK_EN);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        if (ret != ESP_OK)
                return DS2482_ERR_WRITE;

	/* This will set the read pointer to the status register.
	 * Keep reading the status register until the 1WB bit is cleared.
	 */
	while(1)
	{
		p++;
		if (p>253){
			return DS2482_ERR_READ;
			}
		cmd = i2c_cmd_link_create();
                i2c_master_start(cmd);
                i2c_master_write_byte(cmd, address << 1 | READ_BIT, ACK_CHECK_EN);
                i2c_master_read_byte(cmd, &byte, NACK_VAL);
                i2c_master_stop(cmd);
                ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
                i2c_cmd_link_delete(cmd);
                if (ret != ESP_OK)
                        return DS2482_ERR_I2C_DEVICE;

		if(!(byte & DS2482_S_1WB))
			break;
	}
	return DS2482_ERR_OK;
}

/* uint8_t ds2482owWriteTriplet(uint8_t address, uint8_t *direction)
 *
 * Write a 1-wire triplet packet to the bus. From 1-wire Triplet in DS2482 datasheet:
 *
 * Generates three times slots, two read-time slots and one-write time slot, at 
 * the selected 1-Wire IO channel. The type of write-time slot depends on the 
 * result of the read-time slots and the direction byte.  
 * The direction byte determines the type of write-time slot if both read-time 
 * slots are 0 (a typical case). In this case the DS2482 will generate a write-1 
 * time slot if V = 1 and a write-0 time slot if V = 0.  
 * If the read-time slots are 0 and 1, there will follow a write 0 time slot.  
 * If the read-time slots are 1 and 0, there will follow a write 1 time slot.  
 * If the read-time slots are both 1 (error case), the subsequent write time 
 * slot will be a write 1.  
 *
 * The direction var should contain the desired way to take if both bits are 0, 
 * and the status register will be placed in *direction when finnished, to allow
 * SBR, TSB and DIR to be readed.
 * 
 * Can return:
 * DS2482_ERR_OK
 * DS2482_ERR_START
 * DS2482_ERR_ADDRESS
 * DS2482_ERR_WRITE
 * DS2482_ERR_READ
 */
uint8_t ds2482owWriteTriplet(uint8_t address, uint8_t *direction)
{
	uint8_t p=0;


        esp_err_t ret;
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, address << 1 | WRITE_BIT, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, DS2482_C_OWT, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, (*direction)?0x80:0x00, ACK_CHECK_EN);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        if (ret != ESP_OK)
                return DS2482_ERR_WRITE;


	/* This will set the read pointer to the status register.
	 * Keep reading the status register until the 1WB bit is cleared.
	 */
	while(1)
	{
		p++;
		if (p>253)
			return DS2482_ERR_READ;

		cmd = i2c_cmd_link_create();
                i2c_master_start(cmd);
                i2c_master_write_byte(cmd, address << 1 | READ_BIT, ACK_CHECK_EN);
                i2c_master_read_byte(cmd, direction, NACK_VAL);
                i2c_master_stop(cmd);
                ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
                i2c_cmd_link_delete(cmd);
                if (ret != ESP_OK)
                        return DS2482_ERR_I2C_DEVICE;

		if(!((*direction) & DS2482_S_1WB))
			break;
	}
	return DS2482_ERR_OK;
}

/* uint8_t ds2482owReadByte(uint8_t address, uint8_t *byte)
 *
 * Read a byte from the 1-wire bus.
 * byte is to be treated as invalid unless DS2482_ERR_OK was returned.
 *
 * Can return:
 * DS2482_ERR_OK
 * DS2482_ERR_START
 * DS2482_ERR_ADDRESS
 * DS2482_ERR_WRITE
 */
uint8_t ds2482owReadByte(uint8_t address, uint8_t *byte)
{
	uint8_t t;
	uint8_t p=0;

        esp_err_t ret;
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, address << 1 | WRITE_BIT, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, DS2482_C_OWRB, ACK_CHECK_EN);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        if (ret != ESP_OK)
                return DS2482_ERR_WRITE;

	/* This will set the read pointer to the status register.
	 * Keep reading the status register until the 1WB bit is cleared.
	 */
	while(1)
	{
		p++;
		if (p>253)
			return DS2482_ERR_READ;

                cmd = i2c_cmd_link_create();
                i2c_master_start(cmd);
                i2c_master_write_byte(cmd, address << 1 | READ_BIT, ACK_CHECK_EN);
                i2c_master_read_byte(cmd, &t, NACK_VAL);
                i2c_master_stop(cmd);
                ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
                i2c_cmd_link_delete(cmd);
                if (ret != ESP_OK)
                        return DS2482_ERR_I2C_DEVICE;

		if(!(t & DS2482_S_1WB))
			break;
	}

	/* Now set read pointer to data register */
	t = ds2482setReadPointer(address, DS2482_P_DATA);
	if(t)	/* Error? */
		return t;
	
	cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, address << 1 | READ_BIT, ACK_CHECK_EN);
        i2c_master_read_byte(cmd, byte, NACK_VAL);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        if (ret != ESP_OK)
        	return DS2482_ERR_I2C_DEVICE;

	return DS2482_ERR_OK;
}

