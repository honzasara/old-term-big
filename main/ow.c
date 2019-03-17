#include "saric_ds2482.h"
#include "ow.h"

/* uint8_t owWriteBlock(uint8_t ds2482_address, uint8_t *block, uint8_t length)
 *.
 * Write multiple bytes to the 1-wire bus.
 * *rom should point at the first byte in the block to be written,
 * length should be the number of bytes to write.
 *
 * Can return:
 * DS2482_ERR_OK
 * DS2482_ERR_START
 * DS2482_ERR_ADDRESS
 * DS2482_ERR_WRITE
 */
uint8_t owWriteBlock(uint8_t ds2482_address, uint8_t *block, uint8_t length)
{
	uint8_t r, c;
	
	for(c=0;c<length;c++)
	{
		r = ds2482owWriteByte(ds2482_address, *(block+c));
		if(r)
			return r;
	}

	return DS2482_ERR_OK;
}

/* uint8_t owMatchRom(uint8_t ds2482_address, uint8_t *rom)
 * Write a Match Rom sequence to the 1-wire bus.
 * *rom should point at the first byte in a 8 byte sequence with the ROM to send
 *
 * Can return:
 * DS2482_ERR_OK
 * DS2482_ERR_START
 * DS2482_ERR_ADDRESS
 * DS2482_ERR_WRITE
 */
uint8_t owMatchRom(uint8_t ds2482_address, uint8_t *rom)
{
	uint8_t r;
	r = ds2482owWriteByte(ds2482_address, OW_MATCH_ROM);
	if(r)
		return r;

	r = owWriteBlock(ds2482_address, rom, 8);
	if(r)
		return r;
	
	return DS2482_ERR_OK;
}

/* uint8_t owReadRom(uint8_t ds2482_address, uint8_t *rom)
 *
 * Only usable when there is one single 1-wire device on the bus.
 * Read the ROM from this device, and place it in the memory pointed to 
 * by *rom.
 * *rom should be able to contain at least 8 bytes
 *
 * Can return:
 * DS2482_ERR_OK
 * DS2482_ERR_START
 * DS2482_ERR_ADDRESS
 * DS2482_ERR_WRITE
 */
uint8_t owReadRom(uint8_t ds2482_address, uint8_t *rom)
{
	uint8_t r,a;
	r = owWriteByte(ds2482_address, OW_READ_ROM);
	if(r)
		return r;
	
	for(a=0; a < 8; a++)
	{
		/* Read response byte */
		r = owReadByte(ds2482_address, (rom+a));
		if(r)
			return r;
	}

	return DS2482_ERR_OK;
}

uint8_t lastDiscrepancy, lastDevice;

/* uint8_t owMatchFirst(uint8_t ds2482_address, char *rom)
 *
 * Initiate search for devices.
 * ROM will be where the ROM goes, and it should contain the ROM last found!
 * so just pass the same rom buffer over and over again, without modifying it!
 *
 * Can return anything owMatchLast returns
 */
uint8_t owMatchFirst(uint8_t ds2482_address, uint8_t *rom)
{
	lastDiscrepancy = lastDevice = 0;
	return owMatchNext(ds2482_address, rom);
}

/* uint8_t owMatchNext(uint8_t ds2482_address, char *rom)
 *
 * Main device searcher.
 * *rom will be where the ROM goes, and it should contain the ROM last found!
 * so just pass the same rom buffer over and over again, without modifying it!
 *
 * Can return:
 * DS2482_ERR_OK
 * DS2482_ERR_START
 * DS2482_ERR_ADDRESS
 * DS2482_ERR_WRITE
 * DS2482_ERR_READ
 * DS2482_ERR_NO_DEVICE
 */
uint8_t owMatchNext(uint8_t ds2482_address, uint8_t *rom)
{
	uint8_t r, bit_number, last_zero, direction, bit_test, serial_byte_mask, serial_byte_number;
	bit_number = 1;
	last_zero = 0;
	serial_byte_mask = 1;
	serial_byte_number = 0;

	if(lastDevice)
		return DS2482_ERR_NO_DEVICE;

	r = owReset(ds2482_address);
	if(r)	return r;

	r = owWriteByte(ds2482_address, OW_SEARCH_ROM);
	if(r)	return r;
	
	while(serial_byte_number < 8)
	{
		/* If this discrepancy occurs before the last, use the same bit as we did the last time in this place */
		if(bit_number < lastDiscrepancy)
			direction = ((rom[serial_byte_number] & serial_byte_mask) > 0);
		else
			/* If same bit as last time, pick 1, else pick 0 */
			direction = (bit_number == lastDiscrepancy);

		r=ds2482owWriteTriplet(ds2482_address, &direction);
		if(r) return r;

		/* Convert direction to bit_test value */
		bit_test = (direction&(DS2482_S_SBR|DS2482_S_TSB)) >> 5;
		direction>>=7;
		if(bit_test == 3)	/* Both bits where 1 */
			break;

		if(bit_test == 0)
		{
			/* Multiple matches, the DS2482 handled our branching, we brached to direction */ 
			if(!direction)
				last_zero = bit_number;
		}
		/* else bit_test > 0 which means the direction was decied automaticly by the DS2482 */

		/* Set or clear the bit in the ROM */
		if(direction)
			rom[serial_byte_number] |= serial_byte_mask;
		else
			rom[serial_byte_number] &= ~serial_byte_mask;
			
		bit_number++;
		serial_byte_mask <<= 1;

		if(serial_byte_mask == 0)
		{
			serial_byte_number++;
			serial_byte_mask = 1;
		}
	}	/* while */
	
	if(!(bit_number < 65))
	{
		lastDiscrepancy = last_zero;
		lastDevice = (lastDiscrepancy == 0);
	}else
	{
		return DS2482_ERR_NO_DEVICE;
	}
	return DS2482_ERR_OK;
}

/* uint8_t owVerify(uint8_t ds2482_address, char *rom)
 *
 * Checks if *rom is available on the 1-wire net.
 * Uses same vars as owMatch* functions so using this will break any unfinnished search
 * from the owMatch* functions.
 *
 * Can return:
 * DS2482_ERR_OK
 * DS2482_ERR_START
 * DS2482_ERR_ADDRESS
 * DS2482_ERR_WRITE
 * DS2482_ERR_READ
 * DS2482_ERR_NO_DEVICE
 */
uint8_t owVerify(uint8_t ds2482_address, uint8_t *rom)
{
	uint8_t a,r, rom_copy[8];
	lastDiscrepancy = 64;
	lastDevice = 0;
	for(a= 0; a< 8; a++)
		rom_copy[a] = rom[a];
	
	r = owMatchNext(ds2482_address, rom_copy);

	for(a= 0; a< 8; a++)
	{
		if(rom_copy[a] != rom[a])
			return DS2482_ERR_NO_DEVICE;
	}

	return r;
}


