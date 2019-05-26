#include "driver/i2c.h"
#include "mqtt_client.h"

#ifndef GLOBALH_INCLUDED
#define GLOBALH_INCLUDED


#define I2C_MASTER_SCL_IO GPIO_NUM_22               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO GPIO_NUM_21               /*!< gpio number for I2C master data  */

#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define I2C_MASTER_FREQ_HZ 400000
#define ACK_VAL I2C_MASTER_ACK                             /*!< I2C ack value */
#define NACK_VAL I2C_MASTER_NACK                             /*!< I2C nack value */

#define max_output 20


#define MAX_TEMP_BUFFER 768
#define HW_ONEWIRE_MAXROMS 64
#define HW_ONEWIRE_MAXDEVICES 32

#define I2C_MEMORY_ADDR 0x50
#define DS1307_ADDRESS  0x68

#define eeprom_start_stored_device 100
#define eeprom_start_my_device 10
#define eeprom_start_wifi_setup 3200

#define max_timeplan 20
#define eeprom_start_timeplan 3300
#define eeprom_size_timeplan 21

#define max_programplan 20
#define eeprom_start_program_plan 3720
#define eeprom_size_program_plan 21

#define max_output 20
#define eeprom_output_start 5000
#define eeprom_size_output 180

#define max_actions 20
#define eeprom_actions_start 8600
#define eeprom_size_actions 16

#define ROOM_CONTROL 1

extern esp_mqtt_client_handle_t mqtt_client; 

typedef unsigned char byte;

extern i2c_port_t i2c_num;
extern uint8_t Global_HWwirenum;
extern uint8_t Global_ds18s20num;


typedef struct
{
  uint8_t rom[8];
  char name[8];
} struct_mac;

typedef struct
{
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t day_week;
  uint8_t day;
  uint8_t month;
  uint16_t year;
} DateTime;

struct struct_remote_wire
{
  struct_mac id[10];
  uint32_t stamp;
  uint8_t ready;
  uint8_t cnt;
} remote_wire[32];


typedef struct
{
  float temp;
  int offset;
  uint8_t online;
} struct_tds;


struct struct_remote_tds
{
  struct_tds id[10];
  uint32_t stamp;
  uint8_t ready;
  uint8_t cnt;
} remote_tds[32];


typedef struct at_data
{
  char cmd[8];
  char args[32];
  uint8_t rsid;
} at_data_t;


typedef struct
{
  uint8_t hw_port;
  uint8_t type;
  uint8_t active;
  char mqtt_topic[64];
  char mqtt_payload_up[32];
  char mqtt_payload_down[32];
  at_data_t rs_up;
  at_data_t rs_down;
  char name[8];
} output_t;


uint8_t output_state[max_output];

struct struct_send_at
{
  uint8_t cnt;
  uint8_t idx;
  uint8_t send_idx;
  at_data_t data[16];
} send_at[32];


typedef struct
{
  uint8_t start_min;
  uint8_t start_hour;
  uint8_t stop_min;
  uint8_t stop_hour;
  uint8_t week_day;
  uint8_t active;
  uint8_t free;
  char name[8];
  float threshold;
  uint8_t condition;
} timeplan_t;


typedef struct
{
  char name[8];
  uint8_t timeplan[10];
  uint8_t active;
  uint8_t free;
} programplan_t;


typedef struct
{
  uint8_t type;
  uint8_t version;
  uint32_t stamp;
  uint32_t uptime;
  uint8_t ready;
  uint8_t online;
  char device_name[10];
} rs_device_t;

rs_device_t rs_device[32];

typedef struct
{
  char name[8];
  uint8_t state;
  uint8_t last_state;
  uint8_t outputs[5];
  uint8_t free;
} actions_t;


struct struct_remote_room_thermostat
{
  int light;
  uint8_t term_mode;
  uint8_t ring_associate_tds[3];
  uint8_t active_program[3];
  float ring_threshold[3];
  uint8_t ring_condition[3];
  char ring_name[3][10];
  uint8_t ready;
  uint8_t ring_action[3];
} remote_room_thermostat[32];




struct struct_my_device
{
  uint8_t myIP[4];
  uint8_t myMASK[4];
  uint8_t myDNS[4];
  uint8_t myGW[4];
  char nazev[8];
  char mqtt_uri[60];
  char wifi_essid[20];
  char wifi_pass[20];
  uint8_t ip_static;
} device;



struct struct_DDS18s20
{
  uint8_t volno;
  uint8_t rom[8];
  uint8_t assigned_ds2482;
  uint8_t tempL;
  uint8_t tempH;
  uint8_t CR;
  uint8_t CP;
  uint8_t CRC;
  float temp;
  int offset;
  char nazev[8];
  uint8_t online;
} tds18s20[HW_ONEWIRE_MAXROMS];


struct struct_1w_rom
{
  uint8_t rom[8];
  uint8_t assigned_ds2482;
} w_rom[HW_ONEWIRE_MAXROMS];


uint8_t tmp_rom[8];


struct struct_ds2482
{
  uint8_t i2c_addr;
  uint8_t HWwirenum;
  uint8_t hwwire_cekam;
} ds2482_address[2];


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

uint8_t save_output(uint8_t id, output_t *ot);
uint8_t load_output(uint8_t id, output_t *ot);
uint8_t check_output(uint8_t id);

uint8_t check_actions(uint8_t id);
uint8_t clear_actions(uint8_t id);
uint8_t save_actions(uint8_t id, actions_t *at);
uint8_t load_actions(uint8_t id, actions_t *at);
void clear_actions_in_thermostat(void);

#endif
