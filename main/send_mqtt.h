#ifndef SENDMQTT_INCLUDED
#define SENDMQTT_INCLUDED


void send_timeplan(void);
void send_programplan(void);
void send_set_output(void);
void send_network_static_config(void);
void send_network_running_config(void);
void send_device_status(void);
void send_mqtt_find_rom(void);
void send_mqtt_tds(void);
void send_rs_device(void);
void send_rs_tds(void);
void send_rs_wire(void);
void send_thermostat_status(void);
void send_network_static_config(void);
void send_thermostat_ring_status(void);
void send_actions(void);

#endif
