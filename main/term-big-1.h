#ifndef ___DEFINE_HEADER
#define ___DEFINE_HEADER
#include "driver/uart.h"
#include "driver/i2c.h"
#include "global.h"

#define MAX_TEMP_BUFFER 512
#define HW_ONEWIRE_MAXROMS 64
#define HW_ONEWIRE_MAXDEVICES 32

#define I2C_MEMORY_ADDR 0x50
#define DS1307_ADDRESS  0x68

#define eeprom_start_stored_device 100
#define eeprom_start_my_device 10
#define eeprom_start_wifi_setup 3200
#define max_timeplan 20
#define eeprom_start_time_plan 3300

#define max_programplan 20
#define eeprom_start_program_plan 3300+(max_timeplan*15)


esp_err_t twi_init(i2c_port_t t_i2c_num);

void show_1wire_status(char *text);
void show_heap(char *text);
void show_wifi_status(char *text);
void show_mqtt_status(char *text);
void show_rssi(char *text);
void show_time(char *text);
void show_uptime(char *text);

void new_termostat_program(void);
void new_timeplan(void);

uint8_t check_timeplan(uint8_t id);
uint8_t check_programplan(uint8_t id);
uint8_t save_programplan(uint8_t id, programplan_t *pp);
uint8_t load_programplan(uint8_t id, programplan_t *pp);
void save_timeplan(uint8_t id, timeplan_t *tp);
uint8_t load_timeplan(uint8_t id, timeplan_t *tp);	

#define ROOM_CONTROL 1

rs_device_t rs_device[32];

#endif
