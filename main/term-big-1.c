
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include <soc/rtc.h>
#include "freertos/timers.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "lwip/err.h"
#include "lwip/apps/sntp.h"

#include <sys/param.h>

#include <esp_http_server.h>

#include "cJSON.h"

//#define TWDT_TIMEOUT_S          3
//#define TASK_RESET_PERIOD_S 2


#define RS_TXD  (GPIO_NUM_17)
#define RS_RXD  (GPIO_NUM_16)
#define RS_RTS  (GPIO_NUM_4)
#define RS_CTS  (UART_PIN_NO_CHANGE)
#define RS_TASK_STACK_SIZE    (2048)
#define RS_TASK_PRIO          (10)
#define PACKET_READ_TICS        (100 / portTICK_RATE_MS)
#define RS_BUF_SIZE        (127)   //// definice maleho lokalniho rs bufferu


#define MAX_TEMP_BUFFER 256
#define UART_RECV_MAX_SIZE 256
#define HW_ONEWIRE_MAXROMS 64
#define HW_ONEWIRE_MAXDEVICES 32

#define I2C_MEMORY_ADDR 0x50
#define DS1307_ADDRESS  0x68

#define eeprom_start_stored_device 100
#define eeprom_start_my_device 10
#define eeprom_start_wifi_setup 3200

#include "SSD1306.h"
#include "Font5x8.h"
#include "driver/uart.h"
#include "driver/i2c.h"
#include "global.h"
#include "ow.h"
#include "term-big-1.h"
#include "html.c"

char tmp3[MAX_TEMP_BUFFER];
char tmp1[MAX_TEMP_BUFFER];
char tmp2[MAX_TEMP_BUFFER];

uint8_t start_at = 0;
uint8_t re_at = 0;
char uart_recv[UART_RECV_MAX_SIZE];
uint8_t at_recv_complete = 0;

i2c_port_t i2c_num;

EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;

esp_mqtt_client_handle_t mqtt_client;

uint8_t wifi_retry_num = 0;
uint8_t wifi_connected = 0;
uint8_t mqtt_connected = 0;

uint8_t reload_network = 1;

struct struct_my_device
{
  uint8_t mac[6];
  uint8_t myIP[4];
  uint8_t myMASK[4];
  uint8_t myDNS[4];
  uint8_t myGW[4];
  char nazev[8];
  uint8_t mqtt_server[4];
  char mqtt_user[20];
  char mqtt_key[20];
  char wifi_essid[20];
  char wifi_pass[20];
  uint8_t from_dhcp;
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
  char nazev[20];
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

uint8_t Global_HWwirenum = 0;
uint8_t Global_ds18s20num = 0;


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


uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }

void callback_30_second(void* arg);

//////////////////////////////////////////////////////////////////////////
/*
void read_at(char *input)
{
  uint8_t c;

  ///zde zjistuji jestli mam nove data
  while (1)
  {
    at_recv_complete = 0;
    /// do c ulozim nove data
    c = 1;
    if ( re_at > 0 && re_at < (UART_RECV_MAX_SIZE - 1) )
    {
      if (c == ';')
      {
        start_at = 255;
        at_recv_complete = 1;
        re_at = 0;
        break;
        goto endloop;
      }
      input[re_at - 1] = c;
      input[re_at] = 0;
      re_at++;
    }
    if (start_at == 2)
      {
      if (c == '+')
        start_at = 3;
      else
        start_at = 0;
      }
    if (start_at == 1)
      {
      if ( c == 't')
        start_at = 2;
      else
        start_at = 0;
      }
    if (start_at == 0)
      if (c == 'a')
        start_at = 1;
    if (start_at == 3)
    {
      re_at = 1;
      start_at = 4;
    }
endloop:;
  }
}
*/

void new_parse_at(char *input, char *out1, char *out2)
{

  uint8_t count = 0;
  uint8_t q = 0;

  while ( (count < strlen(input)) && (input[count] != ',') && (q < MAX_TEMP_BUFFER - 1))
  {
    out1[q] = input[count];
    out1[q + 1] = 0;
    q++;
    count++;
  }

  count++;
  q = 0;
  while ((count < strlen(input)) && (q < MAX_TEMP_BUFFER - 1) )
  {
    out2[q] = input[count];
    out2[q + 1] = 0;
    q++;
    count++;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
void rs_send_at(uint8_t id, char *cmd, char *args)
{
  char tmp1[MAX_TEMP_BUFFER];
  char tmp2[8];

  tmp1[0] = 0;
  tmp2[0] = 0;

  strcpy(tmp1, "at+");
  itoa(id, tmp2, 10);
  strcat(tmp1, tmp2);
  strcat(tmp1, ",");
  strcat(tmp1, cmd);
  if (strlen(args) > 0)
  {
    strcat(tmp1, ",");
    strcat(tmp1, args);
  }
  strcat(tmp1, ";");

  uart_write_bytes(UART_NUM_2, tmp1, strlen(tmp1));
}
//////////////////////////////////////////////////////////////////////////////////////////


esp_err_t RTC_DS1307_adjust(DateTime *dt) 
{
  printf("%d:%d:%d %d %d-%d-%d\n\r", dt->hour, dt->minute, dt->second, dt->day_week, dt->day, dt->month, dt->year);
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (DS1307_ADDRESS << 1) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, 0, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, bin2bcd(dt->second), ACK_CHECK_EN);
  i2c_master_write_byte(cmd, bin2bcd(dt->minute), ACK_CHECK_EN);
  i2c_master_write_byte(cmd, bin2bcd(dt->hour), ACK_CHECK_EN);
  i2c_master_write_byte(cmd, bin2bcd(dt->day_week), ACK_CHECK_EN);
  i2c_master_write_byte(cmd, bin2bcd(dt->day), ACK_CHECK_EN);
  i2c_master_write_byte(cmd, bin2bcd(dt->month), ACK_CHECK_EN);
  i2c_master_write_byte(cmd, bin2bcd(dt->year-2000), ACK_CHECK_EN);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}


esp_err_t RTC_DS1307_now(DateTime *now) 
{
  now->second = 0;
  now->minute = 0;
  now->hour = 0;
  now->day_week = 0;
  now->day = 0;
  now->month = 0;
  now->year = 2000;

  esp_err_t ret;
  DateTime dt;
  i2c_cmd_handle_t cmd;
  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (DS1307_ADDRESS << 1) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, 0, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  if (ret != ESP_OK)
    return ret;
  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd,  (DS1307_ADDRESS << 1) | READ_BIT, ACK_CHECK_EN);
  i2c_master_read_byte(cmd, &dt.second, ACK_VAL);
  i2c_master_read_byte(cmd, &dt.minute, ACK_VAL);
  i2c_master_read_byte(cmd, &dt.hour, ACK_VAL);
  i2c_master_read_byte(cmd, &dt.day_week, ACK_VAL);
  i2c_master_read_byte(cmd, &dt.day, ACK_VAL);
  i2c_master_read_byte(cmd, &dt.month, ACK_VAL);
  i2c_master_read_byte(cmd, (uint8_t *)&dt.year, NACK_VAL);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  if (ret == ESP_OK)
    {
    now->second = bcd2bin(dt.second); 
    now->minute = bcd2bin(dt.minute);
    now->hour = bcd2bin(dt.hour);
    now->day_week = bcd2bin(dt.day_week);
    now->day = bcd2bin(dt.day);
    now->month = bcd2bin(dt.month);
    now->year = bcd2bin(dt.year)+2000; 
    }
  //printf("%d:%d:%d %d %d-%d-%d\n\r", now->hour, now->minute, now->second, now->day_week, now->day, now->month, now->year);
  return ret;
}


esp_err_t RTC_DS1307_isrunning(uint8_t *run) 
{
  esp_err_t ret;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (DS1307_ADDRESS << 1) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, 0, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  if (ret != ESP_OK)
    return ret;
  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd,  (DS1307_ADDRESS << 1) | READ_BIT, ACK_CHECK_EN);
  i2c_master_read_byte(cmd, run, NACK_VAL);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  *run = (*run>>7);
  return ret;
}

uint8_t sync_ntp_time_with_local_rtc(void)
{
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "82.113.53.41");
  sntp_setservername(1, "37.187.104.44");
  sntp_init();
  time_t now = 0;
  DateTime dt;
  struct tm timeinfo = { 0 };
  uint8_t retry = 0;
  uint8_t ret = 0;
  int retry_count = 10;
  char strftime_buf[64];
  while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) 
    {
     //printf("Waiting for system time to be set... (%d/%d)", retry, retry_count);
     vTaskDelay(2000 / portTICK_PERIOD_MS);
     time(&now);
     setenv("TZ", "CET-1", 0);
     tzset();
     localtime_r(&now, &timeinfo);
     strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

     printf("NTP time = %s\n\r", strftime_buf);
     //printf("%d\n\r", timeinfo.tm_min);
     dt.second = timeinfo.tm_sec;
     dt.minute = timeinfo.tm_min;
     dt.hour = timeinfo.tm_hour;
     dt.day_week = timeinfo.tm_wday;
     dt.day = timeinfo.tm_mday;
     dt.month = timeinfo.tm_mon + 1;
     dt.year = timeinfo.tm_year + 1900;
     RTC_DS1307_adjust(&dt);
     ret = 1; 
    }

  if (retry >=10)
  {  
   printf("Error sync with ntp server\n\r");
   ret = 0;
  }

  sntp_stop();

  return ret;
}


esp_err_t twi_init(i2c_port_t t_i2c_num)
{
#define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
  esp_err_t ret;
  
  i2c_num = t_i2c_num;
  
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = I2C_MASTER_SDA_IO;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_io_num = I2C_MASTER_SCL_IO;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = 400000L;
  i2c_param_config(i2c_num, &conf);
  ret = i2c_driver_install(i2c_num, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
  return ret;
}




httpd_uri_t hello = {
    .uri       = "/index.html",
    .method    = HTTP_GET,
    .handler   = index_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "Hello World!"
};


httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    //ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        //ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &hello);
        //httpd_register_uri_handler(server, &echo);
        //httpd_register_uri_handler(server, &ctrl);
        return server;
    }

    //ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}



esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    httpd_handle_t *server = (httpd_handle_t *) ctx;
    //printf("wifi event: %d \n\r", event->event_id);
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
	    wifi_connected = 1;
            //if (*server == NULL) 
	    //{
	    //   *server = start_webserver();
            //}
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
	    wifi_connected = 0;
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);

	    //if (*server) 
	    //{
	//	     stop_webserver(*server);
	//	     *server = NULL;
          //  }
            break;
        default:
            break;
    }
    return ESP_OK;
}


void procces_mqtt_json(char *topic, uint8_t topic_len, char *data,  uint8_t data_len)
{
  char c[5];
  uint8_t len, let;
  uint8_t ret;
  char parse_topic[MAX_TEMP_BUFFER];
  char parse_data[MAX_TEMP_BUFFER]; 
  /// funkce pro nastavovani modulu
  //
  for (uint8_t i = 0; i < topic_len; i++)
    {
    parse_topic[i] = topic[i];
    parse_topic[i+1] = 0;
    }
  for (uint8_t i = 0; i < data_len; i++)
    {
    parse_data[i] = data[i];
    parse_data[i+1] = 0;
    }

  tmp1[0] = 0;
  strcpy(tmp1, "/regulatory/");
  strcat(tmp1, device.nazev);
  strcat(tmp1, "/");
  len = strlen(tmp1);
  let = strlen(parse_topic);
  uint8_t valid_json = 1;

  cJSON *json = cJSON_Parse(parse_data);
  if (json == NULL)
    {
    valid_json = 0;  
    printf("Chyba cJSON: %s\n\r", parse_data);
    }

  else
  {
    printf("%s || %s\n\r", parse_topic, tmp1);
    if (strncmp(parse_topic, tmp1, len) == 0)
    {
      tmp1[0] = 0;
      uint8_t j = 0;
      for (uint8_t p = len; p < let; p++)
      {
       tmp1[j] = parse_topic[p];
       tmp1[j + 1] = 0;
       j++;
      }


      if (strcmp(tmp1, "at") == 0 && valid_json == 1)
        {
        cJSON *cmd = NULL;
	cJSON *args = NULL;
	cJSON *id = NULL;
	cmd = cJSON_GetObjectItemCaseSensitive(json, "cmd");
	args = cJSON_GetObjectItemCaseSensitive(json, "args");
	id = cJSON_GetObjectItemCaseSensitive(json, "id");
	if (cJSON_IsString(cmd) && cJSON_IsString(args) && cJSON_IsNumber(id))
	  {
          rs_send_at(id->valueint, cmd->valuestring, args->valuestring);
          printf("Sending at\n\r");	  
	  }
        }


      if (strcmp(tmp1, "set/network") == 0 && valid_json == 1)
        {
        cJSON *nazev = NULL;
        nazev = cJSON_GetObjectItemCaseSensitive(json, "nazev");
        if (cJSON_IsString(nazev) && (nazev->valuestring != NULL) && strlen(nazev->valuestring) < 8)
          {
	  strcpy(device.nazev, nazev->valuestring);
	  printf("Novy nazev: %s\n\r", device.nazev);
	  reload_network = 1;
	  }	      
        }

        cJSON *ip = NULL;
        ip = cJSON_GetObjectItemCaseSensitive(json, "ip");
        if (cJSON_IsString(ip) && (ip->valuestring != NULL) )
	 {
	 if (inet_aton(ip->valuestring, &device.myIP) != 0)
	  {
	  printf("Nova ip adresa: %d.%d.%d.%d\n\r", device.myIP[0], device.myIP[1], device.myIP[2], device.myIP[3]);
          reload_network = 1;
	  }
         else
	  printf("!! spatny format ip adresy");	
	 }

        cJSON *mask = NULL;
        mask = cJSON_GetObjectItemCaseSensitive(json, "mask");
        if (cJSON_IsString(mask) && (mask->valuestring != NULL) )
         {
         if (inet_aton(mask->valuestring, &device.myMASK) != 0)
           {
           printf("Nova maska site: %d.%d.%d.%d\n\r", device.myMASK[0], device.myMASK[1], device.myMASK[2], device.myMASK[3]);
           reload_network = 1;
           }
         else
           printf("!! spatny format masky site");
         }

        cJSON *gw = NULL;
        gw = cJSON_GetObjectItemCaseSensitive(json, "gw");
        if (cJSON_IsString(gw) && (gw->valuestring != NULL) )
         {
         if (inet_aton(gw->valuestring, &device.myGW) != 0)
           {
           printf("Nova gw ip: %d.%d.%d.%d\n\r", device.myGW[0], device.myGW[1], device.myGW[2], device.myGW[3]);
           reload_network = 1;
           }
         else
           printf("!! spatny format vychozi brany");
         }

        cJSON *dns = NULL;
        dns = cJSON_GetObjectItemCaseSensitive(json, "dns");
        if (cJSON_IsString(dns) && (dns->valuestring != NULL) )
         {
         if (inet_aton(dns->valuestring, &device.myDNS) != 0)
           {
           printf("Nova dns ip: %d.%d.%d.%d\n\r", device.myDNS[0], device.myDNS[1], device.myDNS[2], device.myDNS[3]);
           reload_network = 1;
           }
         else
           printf("!! spatny format dns");
         }

        cJSON *broker = NULL;
        broker = cJSON_GetObjectItemCaseSensitive(json, "mqtt");
        if (cJSON_IsString(broker) && (broker->valuestring != NULL) )
         {
         if (inet_aton(broker->valuestring, &device.mqtt_server) != 0)
           {
           printf("Nova nova adresa mqtt serveru: %d.%d.%d.%d\n\r", device.mqtt_server[0], device.mqtt_server[1], device.mqtt_server[2], device.mqtt_server[3]);
           reload_network = 1;
           }
         else
           printf("!! spatny format adresy mqtt serveru");
         }


    cJSON_Delete(json);
    }

  }
}



//////////////////////////////////////////////////////////////////////////////////////////////////
void mqtt_subscribe(void)
{
  strcpy(tmp1, "/regulatory/");
  strcat(tmp1, device.nazev);
  strcat(tmp1, "/#");
  esp_mqtt_client_subscribe(mqtt_client, tmp1, 0);
}



esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id = 0;
    // your_context_t *context = event->context;
    //
    //printf("mqtt event = %d\n\r", event->event_id);
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
	    mqtt_connected = 1;
            mqtt_subscribe();
            break;
        case MQTT_EVENT_DISCONNECTED:
	    mqtt_connected = 0;
            //ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            //ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            //msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            //ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            //ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            //ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
	    procces_mqtt_json(event->topic, event->topic_len, event->data, event->data_len);
	    callback_30_second("jep");
            break;
        case MQTT_EVENT_ERROR:
            //ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
	    //mqtt_connected = 0;
            break;
	case MQTT_EVENT_BEFORE_CONNECT:
	    //mqtt_connected = 0;
	    break;
        default:
            //ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }

    msg_id=msg_id;
    return ESP_OK;
}



void wifi_init(void *arg)
{
    tcpip_adapter_ip_info_t ipInfo;
    nvs_flash_init();
    tcpip_adapter_init();
    tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
    tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, device.nazev);
    inet_pton(AF_INET, "192.168.1.250", &ipInfo.ip);
    inet_pton(AF_INET, "192.168.1.1", &ipInfo.gw);
    inet_pton(AF_INET, "255.255.255.0", &ipInfo.netmask);

    tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);

    ip_addr_t dnsserver;
    inet_pton(AF_INET, "192.168.1.1", &dnsserver);
    dns_setserver(0, &dnsserver);
  
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, arg));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "saric",
            .password = "saric111",
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    //ESP_LOGI(TAG, "start the WIFI SSID:[%s]", CONFIG_WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_start());
    //ESP_LOGI(TAG, "Waiting for wifi");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, 1000);
}




void mqtt_init(void)
{
  esp_mqtt_client_config_t mqtt_cfg = { };
  char mqtt[64];
  char c[5];
  strcpy(mqtt, "mqtt://");
  for (uint8_t m = 0; m < 4; m++)
    {
    itoa(device.mqtt_server[m], c, 10);
    strcat(mqtt, c);
    if (m != 3) strcat(mqtt, ".");
    }
  printf("mqtt broker uri=%s\n\r", mqtt);
  
  mqtt_cfg.event_handle = mqtt_event_handler; 
  mqtt_cfg.disable_auto_reconnect = false;
  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_set_uri(mqtt_client, mqtt);
  esp_mqtt_client_start(mqtt_client);
}




void setup(void)
{

  uart_config_t uart_config = {
     .baud_rate = 19200,
     .data_bits = UART_DATA_8_BITS,
     .parity    = UART_PARITY_DISABLE,
     .stop_bits = UART_STOP_BITS_1,
     .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
     .rx_flow_ctrl_thresh = 122
  };

  uart_param_config(UART_NUM_2, &uart_config);
  uart_set_pin(UART_NUM_2, RS_TXD, RS_RXD, RS_RTS, RS_CTS);
  uart_driver_install(UART_NUM_2, RS_BUF_SIZE, RS_BUF_SIZE, 0, NULL, 0);
  uart_set_mode(UART_NUM_2, UART_MODE_RS485_HALF_DUPLEX);

  ds2482_address[0].i2c_addr = 0b0011000;
  ds2482_address[0].HWwirenum = 0;
  ds2482_address[0].hwwire_cekam = false;
  ds2482_address[1].i2c_addr = 0b0011011;
  ds2482_address[1].HWwirenum = 0;
  ds2482_address[1].hwwire_cekam = false;
 
  twi_init(I2C_NUM_0);

  GLCD_Setup();
  GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
  GLCD_Clear();
  GLCD_GotoXY(0, 16);
  GLCD_PrintString("booting ...");

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// nacteni z i2c pameti
esp_err_t i2c_eeprom_readByte(uint8_t deviceAddress, uint16_t address, uint8_t *data)
{
  esp_err_t ret;
  i2c_cmd_handle_t cmd;

  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, deviceAddress << 1 | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, (address >> 8), ACK_CHECK_EN);
  i2c_master_write_byte(cmd, (address & 0xFF), ACK_CHECK_EN);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  if (ret != ESP_OK)
    return ret;

  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, deviceAddress << 1 | READ_BIT, ACK_CHECK_EN);
  i2c_master_read_byte(cmd, data, NACK_VAL);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

///// zapis do i2c pameti
esp_err_t i2c_eeprom_writeByte(uint8_t deviceAddress, uint16_t address, uint8_t data)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, deviceAddress << 1 | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, (address >> 8), ACK_CHECK_EN);
  i2c_master_write_byte(cmd, (address & 0xFF), ACK_CHECK_EN);
  i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void load_setup_network(void)
{
  uint8_t low;
  for (uint8_t m = 0; m < 6; m++) 
    {
    i2c_eeprom_readByte(I2C_MEMORY_ADDR, eeprom_start_my_device + m, &low);
    device.mac[m] = low;
    }
  for (uint8_t m = 0; m < 4; m++) 
    { 
    i2c_eeprom_readByte(I2C_MEMORY_ADDR, eeprom_start_my_device + 6 + m, &low);
    device.myIP[m] = low;
    }
  for (uint8_t m = 0; m < 4; m++)
    {
    i2c_eeprom_readByte(I2C_MEMORY_ADDR, eeprom_start_my_device + 10 + m, &low);
    device.myMASK[m] = low;
    }
  for (uint8_t m = 0; m < 4; m++)
    {
    i2c_eeprom_readByte(I2C_MEMORY_ADDR, eeprom_start_my_device + 14 + m, &low);
    device.myGW[m] = low;
    }
  for (uint8_t m = 0; m < 4; m++)
    {
    i2c_eeprom_readByte(I2C_MEMORY_ADDR, eeprom_start_my_device + 18 + m, &low);
    device.myDNS[m] = low;
    }
  for (uint8_t m = 0; m < 9; m++)
    {
    i2c_eeprom_readByte(I2C_MEMORY_ADDR, eeprom_start_my_device + 22 + m, &low);
    device.nazev[m] = low;
    }
  for (uint8_t m = 0; m < 4; m++)
    {
    i2c_eeprom_readByte(I2C_MEMORY_ADDR, eeprom_start_my_device + 35 + m, &low);
    device.mqtt_server[m] = low;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ulozi nastaveni site
void save_setup_network(void)
{
  for (uint8_t m = 0; m < 6; m++) i2c_eeprom_writeByte(I2C_MEMORY_ADDR, eeprom_start_my_device + m, device.mac[m]);
  for (uint8_t m = 0; m < 4; m++) i2c_eeprom_writeByte(I2C_MEMORY_ADDR, eeprom_start_my_device + 6 + m, device.myIP[m]);
  for (uint8_t m = 0; m < 4; m++) i2c_eeprom_writeByte(I2C_MEMORY_ADDR, eeprom_start_my_device + 10 + m, device.myMASK[m]);
  for (uint8_t m = 0; m < 4; m++) i2c_eeprom_writeByte(I2C_MEMORY_ADDR, eeprom_start_my_device + 14 + m, device.myGW[m]);
  for (uint8_t m = 0; m < 4; m++) i2c_eeprom_writeByte(I2C_MEMORY_ADDR, eeprom_start_my_device + 18 + m, device.myDNS[m]);
  for (uint8_t m = 0; m < 9; m++) i2c_eeprom_writeByte(I2C_MEMORY_ADDR, eeprom_start_my_device + 22 + m, device.nazev[m]);
  for (uint8_t m = 0; m < 4; m++) i2c_eeprom_writeByte(I2C_MEMORY_ADDR, eeprom_start_my_device + 35 + m, device.mqtt_server[m]);

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
void load_tds18s20_from_eeprom(void)
{
  uint8_t low,high;
  for (uint8_t slot = 0; slot < HW_ONEWIRE_MAXROMS; slot++)
  {
    /// nacteni priznaku volno
    i2c_eeprom_readByte(I2C_MEMORY_ADDR, eeprom_start_stored_device + (slot * 50), &low);
    tds18s20[slot].volno = low;
    /// nacteni rom adresy
    for (uint8_t a = 0; a < 8; a++ ) 
      {
      i2c_eeprom_readByte(I2C_MEMORY_ADDR, eeprom_start_stored_device + (slot * 50) + 1 + a, &low);
      tds18s20[slot].rom[a] = low;
      }
    /// nacteni prizareni ke sbernici
    i2c_eeprom_readByte(I2C_MEMORY_ADDR, eeprom_start_stored_device + (slot * 50) + 9, &low);
    tds18s20[slot].assigned_ds2482 = low;
    /// nacteni nazvu cidla
    for (uint8_t a = 0; a < 20; a++ ) 
    {
      i2c_eeprom_readByte(I2C_MEMORY_ADDR, eeprom_start_stored_device + (slot * 50) + 10 + a, &low);
      tds18s20[slot].nazev[a] = low;
    }
    /// nacteni offestu
    i2c_eeprom_readByte(I2C_MEMORY_ADDR, eeprom_start_stored_device + (slot * 50) + 41, &high); 
    i2c_eeprom_readByte(I2C_MEMORY_ADDR, eeprom_start_stored_device + (slot * 50) + 42, &low);
    tds18s20[slot].offset = (high << 8) + low;
  }
}


void store_tds18s20_to_eeprom(uint8_t slot)
{
  /// ulozeni priznaku volno
  i2c_eeprom_writeByte(I2C_MEMORY_ADDR, eeprom_start_stored_device + (slot * 50), tds18s20[slot].volno);
  /// ulozeni rom adresy
  for (uint8_t a = 0; a < 8; a++ ) i2c_eeprom_writeByte(I2C_MEMORY_ADDR, eeprom_start_stored_device + (slot * 50) + 1 + a, tds18s20[slot].rom[a]);
  /// ulozeni prizareni ke sbernici
  i2c_eeprom_writeByte(I2C_MEMORY_ADDR, eeprom_start_stored_device + (slot * 50) + 9, tds18s20[slot].assigned_ds2482);
  /// ulozeni nazvu cidla
  for (uint8_t a = 0; a < 20; a++ ) i2c_eeprom_writeByte(I2C_MEMORY_ADDR, eeprom_start_stored_device + (slot * 50) + 10 + a, tds18s20[slot].nazev[a]);
  /// ulozeni offsetu
  i2c_eeprom_writeByte(I2C_MEMORY_ADDR, eeprom_start_stored_device + (slot * 50) + 41, (tds18s20[slot].offset >> 8) & 255);
  i2c_eeprom_writeByte(I2C_MEMORY_ADDR, eeprom_start_stored_device + (slot * 50) + 42, tds18s20[slot].offset & 255);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// merime na ds18s20 
uint8_t mereni_hwwire_18s20(uint8_t maxidx)
{
  uint8_t status = 0;
  uint8_t t, e;
  for (uint8_t idx = 0; idx < maxidx; idx++)
  {
    /// pro celou sbernici pustim zacatek mereni teploty
    if (ds2482_address[idx].hwwire_cekam == false)
    {
      status = owReset(ds2482_address[idx].i2c_addr);
      status = owSkipRom(ds2482_address[idx].i2c_addr);
      status = owWriteByte(ds2482_address[idx].i2c_addr, OW_CONVERT_T);
      ds2482_address[idx].hwwire_cekam = true;
    }
    t = 0;
    status = owReadByte(ds2482_address[idx].i2c_addr, &t);
    if (t != 0) ds2482_address[idx].hwwire_cekam = false;

    if (ds2482_address[idx].hwwire_cekam == false)
      for (uint8_t w = 0; w < HW_ONEWIRE_MAXROMS; w++)
      {
        ///proverit toto se mi nezda
        if ((tds18s20[w].volno == 1) && (tds18s20[w].assigned_ds2482 == ds2482_address[idx].i2c_addr))
        {
          status = 0;
          status = status + owReset(ds2482_address[idx].i2c_addr);
          status = status + owMatchRom(ds2482_address[idx].i2c_addr, tds18s20[w].rom );
          status = status + owWriteByte(ds2482_address[idx].i2c_addr, OW_READ_SCRATCHPAD);
          status = status + owReadByte(ds2482_address[idx].i2c_addr, &e);     //0byte
          tds18s20[w].tempL = e;
          status = status + owReadByte(ds2482_address[idx].i2c_addr, &e);     //1byte
          tds18s20[w].tempH = e;
          status = status + owReadByte(ds2482_address[idx].i2c_addr, &e); //2byte
          status = status + owReadByte(ds2482_address[idx].i2c_addr, &e); //3byte
          status = status + owReadByte(ds2482_address[idx].i2c_addr, &e); //4byte
          status = status + owReadByte(ds2482_address[idx].i2c_addr, &e); //5byte
          status = status + owReadByte(ds2482_address[idx].i2c_addr, &e); //6byte
          tds18s20[w].CR = e; //count remain
          status = status + owReadByte(ds2482_address[idx].i2c_addr, &e); //7byte
          tds18s20[w].CP = e; // count per
          status = status + owReadByte(ds2482_address[idx].i2c_addr, &e); //8byte
          tds18s20[w].CRC = e; // crc soucet
          if (status == 0)
          {
            uint16_t temp = (uint16_t) tds18s20[w].tempH << 11 | (uint16_t) tds18s20[w].tempL << 3;
            tds18s20[w].temp = ((temp & 0xfff0) << 3) -  16 + (  (  (tds18s20[w].CP - tds18s20[t].CR) << 7 ) / tds18s20[w].CP ) + tds18s20[w].offset;
            tds18s20[w].online = True;
          }
          else
          {
            tds18s20[w].online = False;
          }
        }
      }
  }
  return status;
}
////////////////////////////////////////////////////////////////////////////////////
/////vyhledani zarizeni na hw 1wire sbernici////////
uint8_t one_hw_search_device(uint8_t idx)
{
  uint8_t r;
  ds2482_address[idx].HWwirenum = 0;
  ds2482init(ds2482_address[idx].i2c_addr);
  ds2482reset(ds2482_address[idx].i2c_addr);
  ds2482owReset(ds2482_address[idx].i2c_addr);
  r = owMatchFirst(ds2482_address[idx].i2c_addr, tmp_rom);
  if (r == DS2482_ERR_NO_DEVICE) {
    /*chyba zadne zarizeni na sbernici*/
  }
  if (r) {
    /*jina chyba*/
  }

  if (r == DS2482_ERR_OK)
    while (1) {
      if (ds2482_address[idx].HWwirenum > HW_ONEWIRE_MAXDEVICES - 1) break;
      for (uint8_t a = 0; a < 8; a++)  w_rom[Global_HWwirenum].rom[a] = tmp_rom[a];
      w_rom[Global_HWwirenum].assigned_ds2482 = idx;
      r = owMatchNext(ds2482_address[idx].i2c_addr, tmp_rom);
      /// celkovy pocet detekovanych roms
      ds2482_address[idx].HWwirenum++;
      Global_HWwirenum++;
      if (r == DS2482_ERR_NO_DEVICE)
      { //hledani dokonceno
        break;
      }
    }
  return r;
}

///////////////////////////////////////////////////////////////////////////////////
void check_function(void)
{
    uint8_t start_count, itmp;

    for (uint8_t init = 0; init < 12; init++)
    {
      GLCD_Clear();
      GLCD_GotoXY(0, 0);
      itoa(init, tmp1, 10);
      strcpy(tmp2, "init: ");
      strcat(tmp2, tmp1);
      strcat(tmp2, "/12");
      GLCD_PrintString(tmp2);
      /// test display
      if (init == 0)
      {
        GLCD_GotoXY(0, 16);
        GLCD_PrintString("display - OK");
      }
      /// test eeprom
      if (init == 1)
      {
        i2c_eeprom_readByte(I2C_MEMORY_ADDR, 0, &start_count);
	start_count++;
	i2c_eeprom_writeByte(I2C_MEMORY_ADDR, 0, start_count);
	i2c_eeprom_readByte(I2C_MEMORY_ADDR, 0, &itmp);
	if (start_count == itmp)
          {
          GLCD_GotoXY(0, 16);
	  GLCD_PrintString("eeprom - OK");
	  }
        else
          {
          GLCD_GotoXY(0, 16);
	  GLCD_PrintString("eeprom - ERR");
	  }
      }
      /// test ds2482
      if (init == 2)
      {
        for (uint8_t f = 0; f < 2; f++)
        {
        GLCD_GotoXY(0, 9 + (9 * f));
	itoa(ds2482_address[f].i2c_addr, tmp1, 10);
	if (ds2482reset(ds2482_address[f].i2c_addr) == DS2482_ERR_OK)
	  {
	  strcpy(tmp2, "ds2482: ");
	  strcat(tmp2, tmp1);
	  strcat(tmp2, " = OK");
	  GLCD_PrintString(tmp2);
	  }
	else
	  {
	  strcpy(tmp2, "ds2482: ");
	  strcat(tmp2, tmp1);
	  strcat(tmp2, " = ERR");
	  GLCD_PrintString(tmp2);
	  }
        }
      }
      /// pocet dostupnych 1wire
      if (init == 3)
      {
        GLCD_GotoXY(0, 16);
	Global_HWwirenum = 0;
	for (uint8_t i = 0; i < 2; i++ )
	  one_hw_search_device(i);
	strcpy(tmp2, "found 1wire: ");
	itoa(Global_HWwirenum, tmp1, 10);
	strcat(tmp2, tmp1);
	GLCD_PrintString(tmp2);
      }
      /// nacteni informaci z eepromky
      if (init == 4)
        {
        GLCD_GotoXY(0, 16);
        GLCD_PrintString("load from eeprom");
        load_tds18s20_from_eeprom();
        load_setup_network();
        }
      /// inicializace wifi
      if (init == 5)
	{
	GLCD_GotoXY(0, 16);
	GLCD_PrintString("start wifi");
        httpd_handle_t server = NULL;
        wifi_init(&server);
	}
      /// inicializace mqqt protokolu
      if (init == 6)
        {
	GLCD_GotoXY(0, 16);
	GLCD_PrintString("init mqtt");
        mqtt_init();
        }
      /// overeni funkce RTC
      if (init == 7)
      {
	RTC_DS1307_isrunning(&itmp);
	if (!itmp)
	{
	  GLCD_GotoXY(0, 16);
	  GLCD_PrintString("RTC = OK");
	}
	else
	{
	  GLCD_GotoXY(0, 12);
	  GLCD_PrintString("RTC = ERR");
	  GLCD_GotoXY(0, 18);
          /// kdyz nebezi RTC pokusim se nacist cas z NTP
	  if (sync_ntp_time_with_local_rtc() == 1)
	    GLCD_PrintString("NTP = OK");
	  else
	    GLCD_PrintString("NTP = ERR");
	}
      }
      /// overeni stavu wifi
      if (init == 8)
      {
        GLCD_GotoXY(0, 16);
        if (wifi_connected == 1)
        {
          strcpy(tmp2, "WIFI: UP ");
	  GLCD_PrintString(tmp2);
        }
	else
	{
	  strcpy(tmp2, "WIFI: DOWN ");
	  GLCD_PrintString(tmp2);
	}
      }

      /*
      /// overeni stavu spojeni na mqtt
      if (init == 9)
      {
      GLCD_GotoXY(0, 16);
      if (mqtt_connected == 1)
        {
        GLCD_PrintString("mqtt: up");
        }
      if (mqtt_connected == 0)
        {
        GLCD_PrintString("mqtt: down");
        }
      }
      */
      /// ukazi na displayi
      GLCD_Render();
      vTaskDelay(50);
    }

}

void printJsonObject(cJSON *item)
{
    char *json_string = cJSON_Print(item);
    if (json_string)
    {
        printf("%s\n", json_string);
        cJSON_free(json_string);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// pres mqtt odeslu muj aktualni status, ping + uptime
void send_mqtt_status(void)
{
  int64_t time_since_boot = esp_timer_get_time()/1000/1000;
  uint8_t rssi = 0;
  wifi_ap_record_t wifidata;
  strcpy(tmp1, "/regulatory/status/");
  strcat(tmp1, device.nazev);
  cJSON *status = cJSON_CreateObject();
  cJSON *live = cJSON_CreateString("live");
  cJSON *uptime = cJSON_CreateNumber(time_since_boot);
  cJSON_AddItemToObject(status, "ping", live);
  cJSON_AddItemToObject(status, "uptime", uptime);
  cJSON *heapd = cJSON_CreateNumber(esp_get_free_heap_size());
  if (esp_wifi_sta_get_ap_info(&wifidata)==0)
    {
    rssi =  wifidata.rssi;
    }
  cJSON *wifirssi = cJSON_CreateNumber(rssi);
  cJSON_AddItemToObject(status, "wifi_rssi", wifirssi);
  cJSON_AddItemToObject(status, "heap", heapd);
  char * string = cJSON_Print(status);
  for (uint8_t i = 0; i < strlen(string); i++)
    {
    tmp2[i] = string[i];
    tmp2[i+1] = 0;
    }

  esp_mqtt_client_publish(mqtt_client, tmp1, tmp2, 0, 1, 0);
  
  cJSON_Delete(status);
  cJSON_free(string);
}
////////////////////////////////////////////////////////////////////////////////////
/// pres mqtt odeslu nalezene 1wire devices
void send_mqtt_find_rom(void)
{
  char c[5];
  for (uint8_t id = 0; id < Global_HWwirenum; id++ )
    {
    strcpy(tmp1, "/regulatory/1wire/");
    cJSON *wire = cJSON_CreateObject();
    cJSON *id_driver = cJSON_CreateNumber(ds2482_address[w_rom[id].assigned_ds2482].i2c_addr);
    cJSON *idf = cJSON_CreateNumber(id);
    cJSON *name = cJSON_CreateString(device.nazev);
    tmp2[0] = 0;
    for (uint8_t a = 0; a < 8; a++ )
      {
      itoa(w_rom[id].rom[a], c, 16);
      strcat(tmp2, c);
      if (a < 7)
        {
        c[0] = ':';
        c[1] = 0;
        strcat(tmp2, c);
        }
      }
    cJSON *rom = cJSON_CreateString(tmp2);
    cJSON_AddItemToObject(wire, "id_driver", id_driver);
    cJSON_AddItemToObject(wire, "id", idf);
    cJSON_AddItemToObject(wire, "term_name", name);
    cJSON_AddItemToObject(wire, "rom", rom);
    char * string = cJSON_Print(wire);
    tmp3[0] = 0;
    for (uint8_t i = 0; i < strlen(string); i++)
      {
      tmp3[i] = string[i];
      tmp3[i+1] = 0;
      }

    esp_mqtt_client_publish(mqtt_client, tmp1, tmp3, 0, 1, 0);
    
    cJSON_Delete(wire);
    cJSON_free(string);
    
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
void send_mqtt_tds(void)
{
  char c[5];
  for (uint8_t i = 0; i < HW_ONEWIRE_MAXROMS; i++)
    {
    if (tds18s20[i].volno == 1)
      {
      strcpy(tmp1, "/regulatory/ds18s20/");
      cJSON *tds = cJSON_CreateObject();
      cJSON *slotid = cJSON_CreateNumber(i);
      cJSON *term_name = cJSON_CreateString(device.nazev);
      tmp2[0] = 0;
      for (uint8_t a = 0; a < 8; a++ )
        {
        itoa(w_rom[i].rom[a], c, 16);
        strcat(tmp2, c);
        if (a < 7)
          {
          c[0] = ':';
          c[1] = 0;
          strcat(tmp2, c);
          }
        }
      cJSON *rom = cJSON_CreateString(tmp2);
      cJSON *name = cJSON_CreateString(tds18s20[i].nazev);
      cJSON *temp = cJSON_CreateNumber(tds18s20[i].temp);
      cJSON *offset = cJSON_CreateNumber(tds18s20[i].offset);
      cJSON *online = cJSON_CreateNumber(tds18s20[i].online);
      cJSON_AddItemToObject(tds, "slot_id", slotid);
      cJSON_AddItemToObject(tds, "term_name", term_name);
      cJSON_AddItemToObject(tds, "rom", rom);
      cJSON_AddItemToObject(tds, "name", name);
      cJSON_AddItemToObject(tds, "temp", temp);
      cJSON_AddItemToObject(tds, "offset", offset);
      cJSON_AddItemToObject(tds, "online", online);
      char * string = cJSON_Print(tds);
      tmp3[0] = 0;
      for (uint8_t i = 0; i < strlen(string); i++)
        {
        tmp3[i] = string[i];
        tmp3[i+1] = 0;
        }

      esp_mqtt_client_publish(mqtt_client, tmp1, tmp3, 0, 1, 0);
      
      cJSON_Delete(tds);
      cJSON_free(string);
      
      }
    }
}


void show_1wire_status(char *text)
{
  strcpy(text, "1wire: ");
  itoa(Global_HWwirenum, tmp1, 10);
  strcat(text, tmp1);
}


void show_heap(char *text)
{
  strcpy(text, "mem:");
  itoa(esp_get_free_heap_size()/1024, tmp1, 10);
  strcat(text, tmp1);
  strcat(text, "kb");
}


void show_wifi_status(char *text)
{
  if (wifi_connected == 1)
     strcpy(text, "wifi - u"); 
  else
     strcpy(text, "wifi - d");
}

void show_mqtt_status(char *text)
{
  if (mqtt_connected == 1)
	  strcpy(text, "mqtt - u");
  if (mqtt_connected == 0)
	  strcpy(text, "mqtt - d");
  if (mqtt_connected == 2)
	  strcpy(text, "mqtt - ?");
}

void show_rssi(char *text)
{
  wifi_ap_record_t wifidata;
  if (esp_wifi_sta_get_ap_info(&wifidata)==0)
    {
    sprintf(text, "rssi:%d", wifidata.rssi);
    }
  else
    {
    strcpy(text, "rssi:---");
    }
}

void show_time(char *text)
{
  DateTime now;
  RTC_DS1307_now(&now);
  sprintf(text, "%.2d:%.2d:%.2d", now.hour, now.minute, now.second);
}

void show_uptime(char *text)
{
  int32_t time_since_boot = esp_timer_get_time()/1000/1000;
  sprintf(text, "rt: %ds", time_since_boot);

  if (time_since_boot > 59)
    {
    sprintf(text, "rt:%dm", time_since_boot/60);
    if (time_since_boot % 2 == 0)
      strcat(text, "*");
    }
  if (time_since_boot > (60*60)-1)
    {
    sprintf(text, "rt:%dh", time_since_boot/60/60);
    if (time_since_boot % 2 == 0)
      strcat(text, "*");
    }
  if (time_since_boot > (60*60*24)-1)
    {
    sprintf(text, "rt:%dd", time_since_boot/60/60*24);
    if (time_since_boot % 2 == 0)
      strcat(text, "*");
    }
}

void show_screen(void)
{
  char str1[20];
  //char str2[32];
  GLCD_Clear();
  str1[0]=0;

  GLCD_GotoXY(0, 0);
  show_time(str1);
  GLCD_PrintString(str1);

  GLCD_GotoXY(0,9);
  show_rssi(str1);
  GLCD_PrintString(str1);

  GLCD_GotoXY(64,0);
  show_mqtt_status(str1);
  GLCD_PrintString(str1);

  GLCD_GotoXY(64,9);
  show_wifi_status(str1);
  GLCD_PrintString(str1);

  GLCD_GotoXY(0,22);
  show_heap(str1);
  GLCD_PrintString(str1);

  GLCD_GotoXY(64,22);
  show_uptime(str1);
  GLCD_PrintString(str1);

  GLCD_Render();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
void callback_one_second(void* arg)
{
  mereni_hwwire_18s20(2);

  tcpip_adapter_ip_info_t ipInfo;
  char str[256];
  /*
  // IP address.
  tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
  printf("ip: %s\n", ip4addr_ntoa(&ipInfo.ip));
  printf("netmask: %s\n", ip4addr_ntoa(&ipInfo.netmask));
  printf("gw: %s\n", ip4addr_ntoa(&ipInfo.gw));
  ip_addr_t dnssrv=dns_getserver(0);
  printf("dns=%s\n\r", inet_ntoa(dnssrv));
  */
 
  show_screen(); 
  printf("free: %d\n\r", esp_get_free_heap_size());
  
  int32_t time_since_boot = esp_timer_get_time()/1000/1000;
  printf("uptime: %d\n\n", time_since_boot);
  show_time(str);
  printf("aktual cas: %s\n\r", str);

  printf("----\n\r");


  if (reload_network == 1)
  {
  
  
  }
}

void callback_30_second(void* arg)
{
  int64_t time_since_boot = esp_timer_get_time()/1000/1000;
  /// hledam nove 1wire zarizeni
  Global_HWwirenum = 0;
  for (uint8_t i = 0; i < 2; i++ )
     one_hw_search_device(i);


  if (mqtt_connected == 1)
    {
    send_mqtt_status();
    send_mqtt_find_rom();
    send_mqtt_tds();
    }


}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void rs_task(void* args)
{
  uint8_t* data = (uint8_t*) malloc(RS_BUF_SIZE);
  while(1)
    {
    int len = uart_read_bytes(UART_NUM_2, data, RS_BUF_SIZE, PACKET_READ_TICS);
    if (len)
	    printf("prijaty at: %s", data);
    }

 free(data);

}


/////////////////////////////////////////
////////////////////////////////////////
void app_main()
{
  setup();
  check_function();

  const esp_timer_create_args_t periodic_timer_args_one_sec = {.callback = &callback_one_second, .name = "one_second" };
  const esp_timer_create_args_t periodic_timer_args_30_sec = {.callback = &callback_30_second, .name = "30_second" };

  xTaskCreate(rs_task, "rs_task", RS_TASK_STACK_SIZE, NULL, RS_TASK_PRIO, NULL);

  esp_timer_handle_t periodic_timer_one_sec;
  esp_timer_create(&periodic_timer_args_one_sec, &periodic_timer_one_sec);
  esp_timer_start_periodic(periodic_timer_one_sec, 1000000);

  esp_timer_handle_t periodic_timer_30_sec;
  esp_timer_create(&periodic_timer_args_30_sec, &periodic_timer_30_sec);
  esp_timer_start_periodic(periodic_timer_30_sec, 30000000);
}



























































































    /*
    //// Print chip information /
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
    */


/*
    char string[200];
    strcpy(string, cJSON_Print(json));
    printf("%s", string);

    cJSON *name = NULL;
    cJSON *monitor = cJSON_CreateObject();
    name = cJSON_CreateString("Awesome 4K");
    cJSON_AddItemToObject(monitor, "name", name);

    name = cJSON_CreateString("66:66:44:33:22");
    cJSON_AddItemToObject(monitor, "mac", name);

    strcpy(string, cJSON_Print(monitor));
    cJSON_Delete(monitor);
    printf("%s", string);
*/
