#include "send_mqtt.h"
#include "cJSON.h"
#include "term-big-1.h"
#include "mqtt_client.h"
#include "lwip/sockets.h"
#include "esp_wifi.h"
#include "lwip/dns.h"
#include "esp_event_loop.h"



/// funkce odeslani timeplanu pres mqtt
void send_timeplan(void)
{
  char s_tmp1[MAX_TEMP_BUFFER];
  char s_tmp2[MAX_TEMP_BUFFER];
  timeplan_t tp;
  for (uint8_t id = 0; id < max_timeplan; id++)
    {
    load_timeplan(id, &tp);
    if (tp.free == 1)
      {
      cJSON *timepl = cJSON_CreateObject();
      strcpy(s_tmp1, "/regulatory/timeplan/");
      cJSON *c_start_min = cJSON_CreateNumber(tp.start_min);
      cJSON *c_start_hour = cJSON_CreateNumber(tp.start_hour);
      cJSON *c_stop_min = cJSON_CreateNumber(tp.stop_min);
      cJSON *c_stop_hour = cJSON_CreateNumber(tp.stop_hour);
      cJSON *c_name = cJSON_CreateString(tp.name);
      cJSON *c_active = cJSON_CreateNumber(tp.active);
      cJSON *c_free = cJSON_CreateNumber(tp.free);
      cJSON *c_cond = cJSON_CreateNumber(tp.condition);
      cJSON *c_thresh = cJSON_CreateNumber(tp.threshold);
      cJSON *c_id = cJSON_CreateNumber(id);
      cJSON *term_name = cJSON_CreateString(device.nazev);
      cJSON *c_week = cJSON_CreateNumber(tp.week_day);
      cJSON_AddItemToObject(timepl, "term_name", term_name);
      cJSON_AddItemToObject(timepl, "start_min", c_start_min);
      cJSON_AddItemToObject(timepl, "start_hour", c_start_hour);
      cJSON_AddItemToObject(timepl, "stop_min", c_stop_min);
      cJSON_AddItemToObject(timepl, "stop_hour", c_stop_hour);
      cJSON_AddItemToObject(timepl, "week_day", c_week);
      cJSON_AddItemToObject(timepl, "name", c_name);
      cJSON_AddItemToObject(timepl, "condition", c_cond);
      cJSON_AddItemToObject(timepl, "treshold", c_thresh);
      cJSON_AddItemToObject(timepl, "active", c_active);
      cJSON_AddItemToObject(timepl, "free", c_free);
      cJSON_AddItemToObject(timepl, "id", c_id);

      char * string = cJSON_Print(timepl);
      printf("sendtimeplan: %d\n\r", strlen(string));
      for (uint16_t i = 0; i < strlen(string); i++)
        {
        s_tmp2[i] = string[i];
        s_tmp2[i+1] = 0;
        }
      esp_mqtt_client_publish(mqtt_client, s_tmp1, s_tmp2, 0, 1, 0);
      cJSON_Delete(timepl);
      cJSON_free(string);
      }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// funkce odeslani programplanu pres mqtt
void send_programplan(void)
{
  char s_tmp1[MAX_TEMP_BUFFER];
  char s_tmp2[MAX_TEMP_BUFFER];
  programplan_t pp;
  int array[10];
  for (uint8_t id = 0; id < max_programplan; id++)
    {
    if (check_programplan(id) == 1)
      {
      cJSON *prgpl = cJSON_CreateObject();
      load_programplan(id, &pp);
      strcpy(s_tmp1, "/regulatory/programplan/");
      cJSON *c_active = cJSON_CreateNumber(pp.active);
      cJSON *c_free = cJSON_CreateNumber(pp.free);
      cJSON *c_id = cJSON_CreateNumber(id);
      cJSON *c_name = cJSON_CreateString(pp.name);
      for (uint8_t ii = 0; ii < 10; ii++) array[ii] = pp.timeplan[ii];
      cJSON *en = cJSON_CreateIntArray(array, 10);
      cJSON *term_name = cJSON_CreateString(device.nazev);
      cJSON_AddItemToObject(prgpl, "term_name", term_name);
      cJSON_AddItemToObject(prgpl, "name", c_name);
      cJSON_AddItemToObject(prgpl, "active", c_active);
      cJSON_AddItemToObject(prgpl, "free", c_free);
      cJSON_AddItemToObject(prgpl, "id", c_id);
      cJSON_AddItemToObject(prgpl, "timeplans", en);

      char * string = cJSON_Print(prgpl);
      printf("sendprogramplan: %d\n\r", strlen(string));
      for (uint16_t i = 0; i < strlen(string); i++)
        {
        s_tmp2[i] = string[i];
        s_tmp2[i+1] = 0;
        }
      esp_mqtt_client_publish(mqtt_client, s_tmp1, s_tmp2, 0, 1, 0);
      cJSON_Delete(prgpl);
      cJSON_free(string);
      }
    }
}
/////////////////////////////////////////////////////////////////////////
/// funkce odeslani actions pro vystupy
void send_actions(void)
{
  char s_tmp1[MAX_TEMP_BUFFER];
  char s_tmp2[MAX_TEMP_BUFFER];
  actions_t at;
  int array[5];
  for (uint8_t ai = 0; ai < max_actions; ai++)
    {
    if (check_actions(ai) == 1)
      {
      cJSON *outj = cJSON_CreateObject();
      load_actions(ai, &at);
      strcpy(s_tmp1, "/regulatory/actions/");
      cJSON *c_term_name = cJSON_CreateString(device.nazev);
      cJSON *c_id = cJSON_CreateNumber(ai);
      cJSON *c_name = cJSON_CreateString(at.name);
      cJSON *c_state = cJSON_CreateNumber(at.last_state);
      for (uint8_t ii = 0; ii < 5; ii++) array[ii] = at.outputs[ii];
      cJSON *outputs = cJSON_CreateIntArray(array, 5);

      cJSON_AddItemToObject(outj, "state", c_state);
      cJSON_AddItemToObject(outj, "term_name", c_term_name);
      cJSON_AddItemToObject(outj, "id", c_id);
      cJSON_AddItemToObject(outj, "name", c_name);
      cJSON_AddItemToObject(outj, "outputs", outputs);

      char * string = cJSON_Print(outj);
      printf("send actions: %d\n\r", strlen(string));
      for (uint16_t i = 0; i < strlen(string); i++)
        {
        s_tmp2[i] = string[i];
        s_tmp2[i+1] = 0;
        }
      esp_mqtt_client_publish(mqtt_client, s_tmp1, s_tmp2, 0, 1, 0);
      cJSON_Delete(outj);
      cJSON_free(string);

      }
    }
}

//////////////////////////////////////////////////////////////////////////
/// funkce odeslani nastaveni vystupu pres mqtt
void send_set_output(void)
{
  char s_tmp1[MAX_TEMP_BUFFER];
  char s_tmp2[MAX_TEMP_BUFFER];
  output_t ot;
  for (uint8_t oi = 0; oi < max_output; oi++)
    {
    if (check_output(oi) == 1)
      {
      cJSON *outj = cJSON_CreateObject();
      load_output(oi, &ot);
      strcpy(s_tmp1, "/regulatory/output/");
      cJSON *c_active = cJSON_CreateNumber(ot.active);
      cJSON *c_hw = cJSON_CreateNumber(ot.hw_port);
      cJSON *c_type = cJSON_CreateNumber(ot.type);
      cJSON *c_id = cJSON_CreateNumber(oi);
      cJSON *c_state = cJSON_CreateNumber(output_state[oi]);
      cJSON *c_name = cJSON_CreateString(ot.name);
      cJSON *c_term_name = cJSON_CreateString(device.nazev);
      cJSON *c_mqtt_topic = cJSON_CreateString(ot.mqtt_topic);
      cJSON *c_mqtt_payload_up = cJSON_CreateString(ot.mqtt_payload_up);
      cJSON *c_mqtt_payload_down = cJSON_CreateString(ot.mqtt_payload_down);

      cJSON *c_rs_cmd_up = cJSON_CreateString(ot.rs_up.cmd);
      cJSON *c_rs_args_up = cJSON_CreateString(ot.rs_up.args);
      cJSON *c_rs_rsid_up = cJSON_CreateNumber(ot.rs_up.rsid);
      cJSON *rsobj_up = cJSON_CreateObject();
      cJSON_AddItemToObject(rsobj_up, "rsid", c_rs_rsid_up);
      cJSON_AddItemToObject(rsobj_up, "cmd", c_rs_cmd_up);
      cJSON_AddItemToObject(rsobj_up, "args", c_rs_args_up);
      cJSON *c_rs_cmd_down = cJSON_CreateString(ot.rs_down.cmd);
      cJSON *c_rs_args_down = cJSON_CreateString(ot.rs_down.args);
      cJSON *c_rs_rsid_down = cJSON_CreateNumber(ot.rs_down.rsid);
      cJSON *rsobj_down = cJSON_CreateObject();
      cJSON_AddItemToObject(rsobj_down, "rsid", c_rs_rsid_down);
      cJSON_AddItemToObject(rsobj_down, "cmd", c_rs_cmd_down);
      cJSON_AddItemToObject(rsobj_down, "args", c_rs_args_down);

      cJSON_AddItemToObject(outj, "state", c_state);
      cJSON_AddItemToObject(outj, "term_name", c_term_name);
      cJSON_AddItemToObject(outj, "id", c_id);
      cJSON_AddItemToObject(outj, "type", c_type);
      cJSON_AddItemToObject(outj, "active", c_active);
      cJSON_AddItemToObject(outj, "hw_port", c_hw);
      cJSON_AddItemToObject(outj, "name", c_name);
      cJSON_AddItemToObject(outj, "mqtt_payload_up", c_mqtt_payload_up);
      cJSON_AddItemToObject(outj, "mqtt_payload_down", c_mqtt_payload_down);
      cJSON_AddItemToObject(outj, "mqtt_topic", c_mqtt_topic);
      cJSON_AddItemToObject(outj, "rs_up", rsobj_up);
      cJSON_AddItemToObject(outj, "rs_down", rsobj_down);

      char * string = cJSON_Print(outj);
      printf("send output: %d\n\r", strlen(string));
      for (uint16_t i = 0; i < strlen(string); i++)
        {
        s_tmp2[i] = string[i];
        s_tmp2[i+1] = 0;
        }
      esp_mqtt_client_publish(mqtt_client, s_tmp1, s_tmp2, 0, 1, 0);
      cJSON_Delete(outj);
      cJSON_free(string);
      }
    }
}

////////////////////////////////////////////////////////////////////////////////////
/// funkce odeslani nastaveni site
void send_network_static_config(void)
{
  char s_tmp1[MAX_TEMP_BUFFER];
  char s_tmp2[MAX_TEMP_BUFFER];

  sprintf(s_tmp1, "/regulatory/%s/network/static", device.nazev);
  cJSON *status = cJSON_CreateObject();
  char ipstr[16];
  sprintf(ipstr, "%d.%d.%d.%d", device.myIP[0], device.myIP[2], device.myIP[2], device.myIP[3]);
  cJSON *ip = cJSON_CreateString(ipstr);
  char maskstr[16];
  sprintf(maskstr, "%d.%d.%d.%d", device.myMASK[0], device.myMASK[2], device.myMASK[2], device.myMASK[3]);
  cJSON *mask = cJSON_CreateString(maskstr);
  char gwstr[16];
  sprintf(gwstr, "%d.%d.%d.%d", device.myGW[0], device.myGW[2], device.myGW[2], device.myGW[3]);
  cJSON *gw = cJSON_CreateString(gwstr);
  char dnsstr[16];
  sprintf(dnsstr, "%d.%d.%d.%d", device.myDNS[0], device.myDNS[2], device.myDNS[2], device.myDNS[3]);
  cJSON *dns = cJSON_CreateString(dnsstr);
  cJSON *mqtt_uri = cJSON_CreateString(device.mqtt_uri);
  cJSON *wifi_essid = cJSON_CreateString(device.wifi_essid);
  cJSON *wifi_pass = cJSON_CreateString(device.wifi_pass);
  cJSON *ip_static = cJSON_CreateNumber(device.ip_static);
  cJSON_AddItemToObject(status, "ip", ip);
  cJSON_AddItemToObject(status, "mask", mask);
  cJSON_AddItemToObject(status, "gw", gw);
  cJSON_AddItemToObject(status, "dns", dns);
  cJSON_AddItemToObject(status, "mqtt_uri", mqtt_uri);
  cJSON_AddItemToObject(status, "wifi_essid", wifi_essid);
  cJSON_AddItemToObject(status, "wifi_pass", wifi_pass);
  cJSON_AddItemToObject(status, "static", ip_static);
  char * string = cJSON_Print(status);
  printf("send network conf: %d\n\r", strlen(string));
  for (uint16_t i = 0; i < strlen(string); i++)
    {
    s_tmp2[i] = string[i];
    s_tmp2[i+1] = 0;
    }
  esp_mqtt_client_publish(mqtt_client, s_tmp1, s_tmp2, 0, 1, 0);
  cJSON_Delete(status);
  cJSON_free(string);
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// odesle konfigurace, ktera prave bezi
/// todo udelat odeslani ipadresy, masky, gw, dns, ntp....
void send_network_running_config(void)
{
  char s_tmp1[MAX_TEMP_BUFFER];
  char s_tmp2[MAX_TEMP_BUFFER];
  uint8_t mac[6];
  //char ipstr[16];
  //char maskstr[16];
  sprintf(s_tmp1, "/regulatory/%s/network/run", device.nazev);
  cJSON *status = cJSON_CreateObject();
  tcpip_adapter_ip_info_t ipInfo;
  // IP address.
  tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
  printf("ip: %s\n", ip4addr_ntoa(&ipInfo.ip));
  printf("netmask: %s\n", ip4addr_ntoa(&ipInfo.netmask));
  printf("gw: %s\n", ip4addr_ntoa(&ipInfo.gw));
  ip_addr_t dnssrv=dns_getserver(0);
  printf("dns=%s\n\r", inet_ntoa(dnssrv));
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char macstr[16];
  sprintf(macstr, "%x:%x:%x:%x:%x:%x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
  cJSON *wifi_mac = cJSON_CreateString(macstr);
  cJSON_AddItemToObject(status, "wifi_mac", wifi_mac);
  char * string = cJSON_Print(status);
  printf("send network run: %d\n\r", strlen(string));
  for (uint16_t i = 0; i < strlen(string); i++)
    {
    s_tmp2[i] = string[i];
    s_tmp2[i+1] = 0;
    }
  esp_mqtt_client_publish(mqtt_client, s_tmp1, s_tmp2, 0, 1, 0);
  cJSON_Delete(status);
  cJSON_free(string);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// pres mqtt odeslu muj aktualni status, ping + uptime
void send_device_status(void)
{
  char s_tmp1[MAX_TEMP_BUFFER];
  char s_tmp2[MAX_TEMP_BUFFER];
  int64_t time_since_boot = esp_timer_get_time()/1000/1000;
  uint8_t rssi = 0;
  wifi_ap_record_t wifidata;
  sprintf(s_tmp1, "/regulatory/%s/status/generic", device.nazev);
  cJSON *status = cJSON_CreateObject();
  cJSON *uptime = cJSON_CreateNumber(time_since_boot);
  cJSON_AddItemToObject(status, "uptime", uptime);
  cJSON *heapd = cJSON_CreateNumber(esp_get_free_heap_size());
  if (esp_wifi_sta_get_ap_info(&wifidata) == 0)
    {
    rssi =  wifidata.rssi;
    }
  cJSON *wifirssi = cJSON_CreateNumber(rssi);
  cJSON_AddItemToObject(status, "wifi_rssi", wifirssi);
  cJSON_AddItemToObject(status, "heap", heapd);
  char * string = cJSON_Print(status);
  printf("send mqtt status: %d\n\r", strlen(string));
  for (uint8_t i = 0; i < strlen(string); i++)
    {
    s_tmp2[i] = string[i];
    s_tmp2[i+1] = 0;
    }
  esp_mqtt_client_publish(mqtt_client, s_tmp1, s_tmp2, 0, 1, 0);
  cJSON_Delete(status);
  cJSON_free(string);
}



/////////////////////////////////////////////////////////////////////////
/// pres mqtt odeslu nalezene 1wire devices
void send_mqtt_find_rom(void)
{
  char s_tmp1[MAX_TEMP_BUFFER];
  char s_tmp2[MAX_TEMP_BUFFER];
  char c[5];
  for (uint8_t id = 0; id < Global_HWwirenum; id++ )
    {
    sprintf(s_tmp1, "/regulatory/%s/1wire/", device.nazev);
    cJSON *wire = cJSON_CreateObject();
    cJSON *id_driver = cJSON_CreateNumber(ds2482_address[w_rom[id].assigned_ds2482].i2c_addr);
    cJSON *idf = cJSON_CreateNumber(id);
    s_tmp2[0] = 0;
    for (uint8_t a = 0; a < 8; a++ )
      {
      itoa(w_rom[id].rom[a], c, 16);
      strcat(s_tmp2, c);
      if (a < 7)
        {
        c[0] = ':';
        c[1] = 0;
        strcat(s_tmp2, c);
        }
      }
    cJSON *rom = cJSON_CreateString(s_tmp2);
    cJSON_AddItemToObject(wire, "id_driver", id_driver);
    cJSON_AddItemToObject(wire, "id", idf);
    cJSON_AddItemToObject(wire, "rom", rom);
    char * string = cJSON_Print(wire);
    printf("send local 1wire: %d\n\r", strlen(string));
    s_tmp2[0] = 0;
    for (uint16_t i = 0; i < strlen(string); i++)
      {
      s_tmp2[i] = string[i];
      s_tmp2[i+1] = 0;
      }
    esp_mqtt_client_publish(mqtt_client, s_tmp1, s_tmp2, 0, 1, 0);
    cJSON_Delete(wire);
    cJSON_free(string);
    }
}

////////////////////////////////////////////////////////////////
/// funkce odeslani lokalnich tds pres mqtt
void send_mqtt_tds(void)
{
  char s_tmp1[MAX_TEMP_BUFFER];
  char s_tmp2[MAX_TEMP_BUFFER];
  char c[5];
  for (uint8_t i = 0; i < HW_ONEWIRE_MAXROMS; i++)
    {
    if (tds18s20[i].volno == 1)
      {
      strcpy(s_tmp1, "/regulatory/ds18s20/");
      cJSON *tds = cJSON_CreateObject();
      cJSON *slotid = cJSON_CreateNumber(i);
      cJSON *term_name = cJSON_CreateString(device.nazev);
      s_tmp2[0] = 0;
      for (uint8_t a = 0; a < 8; a++ )
        {
        itoa(w_rom[i].rom[a], c, 16);
        strcat(s_tmp2, c);
        if (a < 7)
          {
          c[0] = ':';
          c[1] = 0;
          strcat(s_tmp2, c);
          }
        }
      cJSON *rom = cJSON_CreateString(s_tmp2);
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
      printf("send local tds: %d\n\r", strlen(string));
      s_tmp2[0] = 0;
      for (uint16_t i = 0; i < strlen(string); i++)
        {
        s_tmp2[i] = string[i];
        s_tmp2[i+1] = 0;
        }
      esp_mqtt_client_publish(mqtt_client, s_tmp1, s_tmp2, 0, 1, 0);
      cJSON_Delete(tds);
      cJSON_free(string);
      }
    }
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// funkce odeslani nalezenych rs zarizeni
void send_rs_device(void)
{
  char s_tmp1[MAX_TEMP_BUFFER];
  char s_tmp2[MAX_TEMP_BUFFER];
  for (uint8_t rsid = 0; rsid < 32; rsid++)
    {
    if (rs_device[rsid].ready == 1)
      {
      sprintf(s_tmp1, "/regulatory/%s/rs/%d/info", device.nazev, rsid);
      cJSON *rs_dev = cJSON_CreateObject();
      cJSON *rs_online = cJSON_CreateNumber(rs_device[rsid].online);
      cJSON *rs_type = cJSON_CreateNumber(rs_device[rsid].type);
      cJSON *rs_version = cJSON_CreateNumber(rs_device[rsid].version);
      cJSON *rs_stamp = cJSON_CreateNumber(rs_device[rsid].stamp);
      cJSON *rs_uptime = cJSON_CreateNumber(rs_device[rsid].uptime);
      cJSON *rs_name = cJSON_CreateString(rs_device[rsid].device_name);
      cJSON_AddItemToObject(rs_dev, "type", rs_type);
      cJSON_AddItemToObject(rs_dev, "version", rs_version);
      cJSON_AddItemToObject(rs_dev, "name", rs_name);
      cJSON_AddItemToObject(rs_dev, "stamp", rs_stamp);
      cJSON_AddItemToObject(rs_dev, "uptime", rs_uptime);
      cJSON_AddItemToObject(rs_dev, "online", rs_online);
      char * string = cJSON_Print(rs_dev);
      printf("send rs device: %d\n\r", strlen(string));
      s_tmp2[0] = 0;
      for (uint8_t i = 0; i < strlen(string); i++)
        {
        s_tmp2[i] = string[i];
        s_tmp2[i+1] = 0;
        }
      esp_mqtt_client_publish(mqtt_client, s_tmp1, s_tmp2, 0, 1, 0);
      cJSON_Delete(rs_dev);
      cJSON_free(string);
      }
    }
}


////////////////////////////////////////////////////////////////////
/// funkce odeslani rs tds cidel
void send_rs_tds(void)
{
  char s_tmp1[MAX_TEMP_BUFFER];
  char s_tmp2[MAX_TEMP_BUFFER];
  for (uint8_t rsid = 0; rsid < 32; rsid++)
    if (remote_tds[rsid].ready == 1)
      {
      for (uint8_t id = 0; id < remote_tds[rsid].cnt; id++ )
        {
	sprintf(s_tmp1, "/regulatory/%s/rs/%d/tds/%d", device.nazev, rsid, id);
        cJSON *rem_tds = cJSON_CreateObject();

        cJSON *rs_temp = cJSON_CreateNumber(remote_tds[rsid].id[id].temp);
        cJSON *rs_offset = cJSON_CreateNumber(remote_tds[rsid].id[id].offset);
        cJSON *rs_online = cJSON_CreateNumber(remote_tds[rsid].id[id].online);
        cJSON *rs_stamp = cJSON_CreateNumber(remote_tds[rsid].stamp);
	cJSON_AddItemToObject(rem_tds, "temp", rs_temp);
        cJSON_AddItemToObject(rem_tds, "offset", rs_offset);
        cJSON_AddItemToObject(rem_tds, "online", rs_online);
	cJSON_AddItemToObject(rem_tds, "stamp", rs_stamp);
        char * string = cJSON_Print(rem_tds);
        printf("send rs wire tds: %d\n\r", strlen(string));
        s_tmp2[0] = 0;
        for (uint16_t i = 0; i < strlen(string); i++)
          {
          s_tmp2[i] = string[i];
          s_tmp2[i+1] = 0;
          }
        esp_mqtt_client_publish(mqtt_client, s_tmp1, s_tmp2, 0, 1, 0);
        cJSON_Delete(rem_tds);
        cJSON_free(string);
      }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// funkce pro odeslani rs 1wire zarizeni
void send_rs_wire(void)
{
  char s_tmp1[MAX_TEMP_BUFFER];
  char s_tmp2[MAX_TEMP_BUFFER];
  for (uint8_t rsid = 0; rsid < 32; rsid++)
    {
    if (remote_wire[rsid].ready == 1)
      {
      for (uint8_t id = 0; id < remote_wire[rsid].cnt; id++)
        {
        sprintf(s_tmp1, "/regulatory/%s/rs/%d/wire/%d", device.nazev, rsid, id);
	cJSON *rem_wire = cJSON_CreateObject();
        cJSON *id_name = cJSON_CreateString(remote_wire[rsid].id[id].name);
        char rom[26];
        char tmpm[6];
        rom[0] = 0;
        tmpm[0] = 0;
        for (uint8_t m = 0; m < 8; m++ )
          {
          itoa(remote_wire[rsid].id[id].rom[m], tmpm, 16);
          strcat(rom, tmpm);
          if (m < 7) strcat(rom, ":");
          tmpm[0] = 0;
          }
        cJSON *id_rom = cJSON_CreateString(rom);
        cJSON_AddItemToObject(rem_wire, "name", id_name);
        cJSON_AddItemToObject(rem_wire, "rom", id_rom);
       
        cJSON *rs_stamp = cJSON_CreateNumber(remote_wire[rsid].stamp);
        cJSON_AddItemToObject(rem_wire, "stamp", rs_stamp);
        char * string = cJSON_Print(rem_wire);
        printf("send rs 1wire: %d\n\r", strlen(string));
        s_tmp2[0] = 0;
        for (uint16_t i = 0; i < strlen(string); i++)
          {
          s_tmp2[i] = string[i];
          s_tmp2[i+1] = 0;
          }
        esp_mqtt_client_publish(mqtt_client, s_tmp1, s_tmp2, 0, 1, 0);
        cJSON_Delete(rem_wire);
        cJSON_free(string);
        }
      }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// funkce pro odeslani termostatu a nastaveni ringu
void send_thermostat_ring_status(void)
{
  char s_tmp1[MAX_TEMP_BUFFER];
  char s_tmp2[MAX_TEMP_BUFFER];
  for (uint8_t rsid = 0; rsid < 32; rsid++)
    {
    if (remote_room_thermostat[rsid].ready == 1)
      {
      for (uint8_t id = 0; id < 3; id++)
        {
        sprintf(s_tmp1, "/regulatory/%s/rs/%d/thermostat/ring/%d", device.nazev, rsid, id);
        cJSON *jid = cJSON_CreateObject();
        cJSON *t_name = cJSON_CreateString(remote_room_thermostat[rsid].ring_name[id]);
        cJSON *t_thresh = cJSON_CreateNumber(remote_room_thermostat[rsid].ring_threshold[id]);
        cJSON *t_prog = cJSON_CreateNumber(remote_room_thermostat[rsid].active_program[id]);
        cJSON *t_atds = cJSON_CreateNumber(remote_room_thermostat[rsid].ring_associate_tds[id]);
        cJSON *t_action = cJSON_CreateNumber(remote_room_thermostat[rsid].ring_action[id]);
        
        cJSON_AddItemToObject(jid, "name", t_name);
        cJSON_AddItemToObject(jid, "threshold", t_thresh);
        cJSON_AddItemToObject(jid, "program", t_prog);
        cJSON_AddItemToObject(jid, "associate_tds", t_atds);
        cJSON_AddItemToObject(jid, "action", t_action);
	char * string = cJSON_Print(jid);
        printf("send rs thermostat ring: %d\n\r", strlen(string));
        s_tmp2[0] = 0;
        for (uint16_t i = 0; i < strlen(string); i++)
          {
          s_tmp2[i] = string[i];
          s_tmp2[i+1] = 0;
          }
        esp_mqtt_client_publish(mqtt_client, s_tmp1, s_tmp2, 0, 1, 0);
        cJSON_Delete(jid);
        cJSON_free(string);
        }
      }
    }
}


// funkce odeslani nastaveni rs termostatu
void send_thermostat_status(void)
{
  char s_tmp1[MAX_TEMP_BUFFER];
  char s_tmp2[MAX_TEMP_BUFFER];
  for (uint8_t rsid = 0; rsid < 32; rsid++)
    {
    if (remote_room_thermostat[rsid].ready == 1)
      {
      sprintf(s_tmp1, "/regulatory/%s/rs/%d/thermostat", device.nazev, rsid);
      cJSON *rem_therm = cJSON_CreateObject();
      cJSON *rs_light = cJSON_CreateNumber(remote_room_thermostat[rsid].light);
      cJSON *rs_name = cJSON_CreateString(rs_device[rsid].device_name);
      cJSON *term_mode = cJSON_CreateNumber(remote_room_thermostat[rsid].term_mode);
      cJSON_AddItemToObject(rem_therm, "light", rs_light);
      cJSON_AddItemToObject(rem_therm, "name", rs_name);
      cJSON_AddItemToObject(rem_therm, "term_mode", term_mode);

      char * string = cJSON_Print(rem_therm);
      printf("send rs thermostat: %d\n\r", strlen(string));
      s_tmp2[0] = 0;
      for (uint16_t i = 0; i < strlen(string); i++)
        {
        s_tmp2[i] = string[i];
        s_tmp2[i+1] = 0;
        }
      esp_mqtt_client_publish(mqtt_client, s_tmp1, s_tmp2, 0, 1, 0);
      cJSON_Delete(rem_therm);
      cJSON_free(string);
      }
    }
}

/// {"light":        997,"name": "loznice","term_mode":    3}
