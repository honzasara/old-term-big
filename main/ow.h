#ifndef __OW_H
#define __OW_H
#include "saric_ds2482.h" 

extern uint8_t owWriteBlock(uint8_t ds2482_address, uint8_t *block, uint8_t length);
extern uint8_t owMatchRom(uint8_t ds2482_address, uint8_t *rom);
extern uint8_t owReadRom(uint8_t ds2482_address, uint8_t *rom);
extern uint8_t owMatchFirst(uint8_t ds2482_address, uint8_t *rom);
extern uint8_t owMatchNext(uint8_t ds2482_address, uint8_t *rom);
extern uint8_t owVerify(uint8_t ds2482_address, uint8_t *rom);

/* Some 1-wire functions/macros */
#define owReset						ds2482owReset
#define owSkipRom(addr)		 		ds2482owWriteByte(addr, OW_SKIP_ROM)
#define owWriteByte					ds2482owWriteByte
#define owReadByte					ds2482owReadByte	

/* OW commands (are they same for all devices??) */
#define OW_SEARCH_ROM			0xF0
#define OW_READ_ROM				0x33
#define OW_MATCH_ROM				0x55
#define OW_SKIP_ROM				0xCC
#define OW_ALARM_SEARCH			0xEC
#define OW_CONVERT_T				0x44
#define OW_WRITE_SCRATCHPAD	0x4E
#define OW_READ_SCRATCHPAD		0xBE
#define OW_COPY_SCRATCHPAD		0x48
#define OW_RECALL_E2				0xB8
#define OW_READ_PSU				0xB4
#endif

