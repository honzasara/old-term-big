#include "html.h"
#include "pgmspace.h"
#include "term-big-1.h"
#include "global.h"
#include <esp_system.h>
#include <sys/param.h>

#include <esp_http_server.h>

const char start_html[] PROGMEM = "<html>";
const char stop_html[] PROGMEM = "</html>";

const char Header[] PROGMEM = "<title>term-big-1</title>"; 



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t parse(char *text, uint8_t *id, char *name, char *fce)
{
  char tmp[8];
  uint8_t start = 0;
  uint8_t i = 0;
  uint8_t it = 0;
  uint8_t ie = 0;
  uint8_t ret = 0;
  *id = 255;

  name[0] = 0;
  fce[0] = 0;
  while (i < strlen(text))
    {
    if (text[i] == '_' ) {start = 1; goto endloop;}
    if (start == 0)
      {
      fce[ie] = text[i];
      fce[ie + 1] = 0;
      ie++;
      }
    if (start == 1)
      {
      if ((text[i] == '=') && (it < 3)) {start = 2; it = 0;  *id = atoi(tmp); ret = 1; goto endloop;}
      tmp[it] = text[i];
      tmp[it + 1] = 0;
      it++;
      } 
    if ((start == 2) && (it < 8))
      {
      name[it] = text[i];
      name[it + 1] = 0;
      it++;
      }
    endloop:
    i++;
    }
  return ret;
}
//////////////////////////////////
uint8_t parse_ampr(char *input, char *parse, char *last)
{
  uint8_t ret = 0;
  uint8_t i = 0;
  uint8_t l = 0;
  parse[0] = 0;
  while (i < strlen(input))
    {
    if (input[i] == '&') {i++; ret = 2; break;}
    parse[i] = input[i];
    parse[i + 1] = 0;
    i++;
    ret = 1;
    }
  last[0] = 0;
  while (i < strlen(input))
    {
    last[l] = input[i];
    last[l + 1] = 0;
    i++; 
    l++;
    }

  return ret;  
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
esp_err_t set_rsid_name_handler(httpd_req_t *req)
{
  char content[100];
  size_t recv_size = MIN(req->content_len, sizeof(content));
  uint8_t rsid;
  char name[10];
  char fce[10];

  int ret = httpd_req_recv(req, content, recv_size);
  if (ret <= 0)
    {
    if (ret == HTTPD_SOCK_ERR_TIMEOUT) httpd_resp_send_408(req);
    return ESP_FAIL;
    }
  printf("%s\n\r", content);
  parse(content, &rsid, name, fce);
  printf("nastavuji:  fce:%s = %d -> %s\n\r", fce, rsid, name);
  httpd_resp_send(req, NULL, 0);
  return ESP_OK;
}
//////////////////////////////////////////////////////////////////////////
esp_err_t set_program_handler(httpd_req_t *req)
{
  char content[100];
  size_t recv_size = MIN(req->content_len, sizeof(content));
  uint8_t rsid;
  char name[10];
  char fce[10];
  char tmp_con[100];
  char str1[16];
  uint8_t id;


  int ret = httpd_req_recv(req, content, recv_size);
  if (ret <= 0)
    {
    if (ret == HTTPD_SOCK_ERR_TIMEOUT) httpd_resp_send_408(req);
    return ESP_FAIL;
    }
  strncpy(tmp_con, content, recv_size);
  strcpy(content, tmp_con);
  //printf("recv_size:%d, content_size:%d, %s\n\r", recv_size, strlen(content), content);
  while (parse_ampr(content, str1, tmp_con) != 0 )
    {
    //printf("funkce %s\n\r", str1 );
    strcpy(content, tmp_con);
    if (parse(str1, &id, name, fce) == 1)
      {
      printf("fce: %s, name: %s, id: %d \n\r", fce, name, id);
      }
    }

  httpd_resp_send(req, NULL, 0);
  return ESP_OK;
}

////////////////////////////////////////////////////////////////////
esp_err_t new_program_handler(httpd_req_t *req)
{
  char*  buf;
  size_t buf_len;
  const char* resp_str = (const char*) req->user_ctx;
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) 
    {
    buf = malloc(buf_len);
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
      {
      char param[32];
      if (httpd_query_key_value(buf, "new", param, sizeof(param)) == ESP_OK)
        {
	printf("%s\n\r", param);
	if (strcmp(param, "program") == 0)
	  {
          // novy program
	  new_termostat_program();
	  }
	if (strcmp(param, "timeplan") == 0)
	  {
	  // novy timeplan
	  new_timeplan();
	  }
	}      
      }
    }

  httpd_resp_send(req, NULL, 0);
  return ESP_OK;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
esp_err_t index_get_handler(httpd_req_t *req)
{
  const char* resp_str = (const char*) req->user_ctx;
  char resp[1450]; 
  strcpy_P(resp, start_html);
  strcat_P(resp, Header);
  strcat(resp, "<table style='margin-left: auto; margin-right: auto;' border='1'>");
  strcat(resp, "<tbody><tr>");
  strcat(resp, "<th><strong>Hlavni</strong></th>");
  strcat(resp, "<th><a href='rsid.html'><strong>RS485 zarizeni</strong></td>");
  strcat(resp, "<th><a href='thermostat.html'><strong>Pokojove termostaty</strong></th>");
  strcat(resp, "<th><a href='sensors.html'><strong>Parovane cidla</strong></th>");
  strcat(resp, "<th><a href='rsid.html'><strong>Parovane vystupy</strong></th>");
  strcat(resp, "<th><a href='setup.html'><strong>Vlastni nastaveni</strong></th>");
  strcat(resp, "</tr></tbody>");
  strcat_P(resp, stop_html);
  httpd_resp_send(req, resp, strlen(resp));
  printf("size %d generated html size\n\r", strlen(resp));
  return ESP_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
esp_err_t rsid_get_handler(httpd_req_t *req)
{
  const char* resp_str = (const char*) req->user_ctx;
  char resp[2048];
  char str1[1024];
  strcpy_P(resp, start_html);
  strcat_P(resp, Header);
  strcat(resp, "<iframe name='hiddenFrame' width='0' height='0' border='0' style='display: none;'></iframe>");
  strcat(resp, "<table style='margin-left: auto; margin-right: auto;' border='1'>");
  strcat(resp, "<tbody>");
  strcat(resp, "<tr>");
  strcat(resp, "<th><a href='index.html'><strong>Hlavni</strong></th>");
  strcat(resp, "<th><strong>RS485 zarizeni</strong></th>");
  strcat(resp, "<th><a href='thermostat.html'><strong>Pokojove termostaty</strong></th>");
  strcat(resp, "<th><a href='sensors.html'><strong>Parovane cidla</strong></th>");
  strcat(resp, "<th><a href='output.html'><strong>Parovane vystupy</strong></th>");
  strcat(resp, "<th><a href='setup.html'><strong>Vlastni nastaveni</strong></th>");
  strcat(resp, "</tr>");
  strcat(resp, "<tr>");
  strcat(resp, "<td colspan='6'>");
  
  strcat(resp, "<table border='1' style='vertical-align: middle; text-align: center;'>");
  strcat(resp, "<tr><td>id</td><td>nazev</td><td>online</td><td>typ</td><td>version</td><td>stamp</td></tr>");
  
  //rs_device[6].ready = 1;
  //strcpy(rs_device[6].name, "lozn");
  
  for (uint8_t rsid = 0; rsid < 32; rsid++)
    {
    if (rs_device[rsid].ready == 1)
      {
      sprintf(str1, "<tr><td>%d</td><td>%d</td><td><form  action=/set_rsid_name.html method='post' target='hiddenFrame'> <input type=text name='rsid_%d' value='%s'> - <input type=submit value='Nastavit'></form></td><td>%d</td><td>%d</td><td>%d</td></tr>", rsid, rs_device[rsid].online, rsid, rs_device[rsid].name, rs_device[rsid].type, rs_device[rsid].version, rs_device[rsid].stamp);
      strcat(resp, str1);
      }
    }
  strcat(resp, "</table>");
  strcat(resp, "</td>");
  strcat(resp, "</tr>");
  strcat(resp, "</tbody>");
  strcat_P(resp, stop_html);
  httpd_resp_send(req, resp, strlen(resp));
  printf("size %d generated html size\n\r", strlen(resp));
  return ESP_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
esp_err_t timeprogram_get_handler(httpd_req_t *req)
{
  programplan_t pp;
  timeplan_t tp[max_timeplan];
  const char* resp_str = (const char*) req->user_ctx;
  char resp[4196];
  char str1[2048];
  char str2[10];
  char str3[128];
  char str_timeplan[1500];
  strcpy_P(resp, start_html);
  strcat_P(resp, Header);
  strcat(resp, "<iframe name='hiddenFrame' width='0' height='0' border='0' style='display: none;'></iframe>");
  strcat(resp, "<table style='margin-left: auto; margin-right: auto; vertical-align: middle; text-align: center;' border='1'>");
  strcat(resp, "<tbody>");
  strcat(resp, "<tr>");
  strcat(resp, "<th><a href='index.html'><strong>Hlavni</strong></th>");
  strcat(resp, "<th><a href='rsid.html'><strong>RS485 zarizeni</strong></th>");
  strcat(resp, "<th><a href='thermostat.html'><strong>Pokojove termostaty</strong></th>");
  strcat(resp, "<th><a href='sensors.html'><strong>Parovane cidla</strong></th>");
  strcat(resp, "<th><a href='output.html'><strong>Parovane vystupy</strong></th>");
  strcat(resp, "<th><a href='setup.html'><strong>Vlastni nastaveni</strong></th>");
  strcat(resp, "</tr>");
  strcat(resp, "<tr><td><a href='/newp.html?new=program'>Novy</td><td>Nazev</td><td>Aktivni</td><td>Casove plany | <a href='/newp.html?new=timeplan'>Novy casovy plan</td><td>Akce</td></tr>");
  printf("delka hlavicky: %d\n\r", strlen(resp));
  char checked[9];
  checked[0] = 0;

  for (uint8_t ipp = 0; ipp < max_timeplan; ipp++)
    {
    load_timeplan(ipp, &tp[ipp]);
    }

  for (uint8_t id = 0; id < max_programplan; id ++)
    {
    if (check_programplan(id) == 1)
      {
      load_programplan(id, &pp);
      

      strncpy(str2, pp.name, 8);
      if (pp.active == 1)
         strcpy(checked, "checked");
      else
         strcpy(checked, " ");

      sprintf(str1, "<tr><td> %d </td><td><form action=/set_timeprogram.html method='post' target='hiddenFrame'><input type=text name='ppn_%d' value='%s'></td><td><input name='ppa_%d' type='checkbox' %s></td><td>%s</td><td><input type=submit value='Ulozit' name='set_%d'><input type=submit value='Vymazat' name='set_%d'></form></td></tr>",id ,id, str2, id, checked, str_timeplan, id, id );
      strcat(resp, str1);
      printf("str1 %d, celkova delka %d\n\r", strlen(str1), strlen(resp));
      }
    }
  strcat(resp, "</tbody>");
  strcat_P(resp, stop_html);
  httpd_resp_send(req, resp, strlen(resp));
  printf("size %d generated html size\n\r", strlen(resp));


  return ESP_OK;
}

esp_err_t thermostat_get_handler(httpd_req_t *req)
{
   const char* resp_str = (const char*) req->user_ctx;
   char resp[2048];
   char str1[1024];
   char str2[1024];
   char str3[1024];
   char str4[1024];
   strcpy_P(resp, start_html);
   strcat_P(resp, Header);
   strcat(resp, "<table style='margin-left: auto; margin-right: auto;' border='1'>");
   strcat(resp, "<tbody>"); 
   strcat(resp, "<tr>");
   strcat(resp, "<th><a href='index.html'><strong>Hlavni</strong></th>");
   strcat(resp, "<th><a href='rsid.html'><strong>RS485 zarizeni</strong></th>");
   strcat(resp, "<th><strong>Pokojove termostaty</strong></th>");
   strcat(resp, "<th><a href='sensors.html'><strong>Parovane cidla</strong></th>");
   strcat(resp, "<th><a href='output.html'><strong>Parovane vystupy</strong></th>");
   strcat(resp, "<th><a href='setup.html'><strong>Vlastni nastaveni</strong></th>");
   strcat(resp, "</tr>");
   strcat(resp, "<tr>");
   strcat(resp, "<td colspan='6'>");
   strcat(resp, "<table border='1' style='vertical-align: middle; text-align: center;'>");

   strcat(resp, "<tr><td>RSID</td><td>Nazev termostatu</td><td>Mod termostatu</td><td><a href='timeprogram.html'>Casovy program</a></td><td>Nazev okruhu</td><td>Hodnota pro okruh</td><td>Podminka</td></tr>");
   str1[0] = 0;
   str2[0] = 0;
   str3[0] = 0;
   str4[0] = 0;
   for (uint8_t rsid = 0; rsid < 32; rsid++)
     {
     if ((remote_room_thermostat[rsid].ready == 1) && (rs_device[rsid].ready == 1))
       {
       sprintf(str2, "<form action=/set_thermostat.html method='post' target='hiddenFrame'>");
       strcpy(str3, str2);
       for (uint8_t okr = 0; okr < 3; okr++)
         {
         sprintf(str2, "<p><input type=text name='rsid_%d-ring_name_%d' value='%s'></p>", rsid, okr, remote_room_thermostat[rsid].term_name[okr]);
	 strcat(str3, str2);
         }
       strcat(str3, "</form>");

       sprintf(str2, "<form action=/set_thermostat.html method='post' target='hiddenFrame'>");
       strcpy(str4, str2);
       for (uint8_t okr = 0; okr < 3; okr++)
         {
         sprintf(str2, "<p><input type=text name='rsid_%d-ring_value_%d' value='%f'></p>", rsid, okr, remote_room_thermostat[rsid].term_threshold[okr]);
	 strcat(str4, str2);
         }
       strcat(str4, "</form>");

       sprintf(str1, "<tr><td>%d</td><td>%s</td><td>%d</td><td>PROGRAM</td><td>%s</td><td>%s</td><td>akce</td></tr>", rsid, rs_device[rsid].name, remote_room_thermostat[rsid].term_mode, str3, str4);
       strcat(resp, str1);
       }
     }


   strcat(resp, "</table>");
   strcat(resp, "</td>");
   strcat(resp, "</tr>");
   strcat(resp, "</tbody>");
   strcat_P(resp, stop_html);
   httpd_resp_send(req, resp, strlen(resp));
   printf("size %d generated html size\n\r", strlen(resp));
   return ESP_OK;
}

/*
esp_err_t timeplan_get_handler(httpd_req_t *req)
{
    const char* resp_str = (const char*) req->user_ctx;
    char resp[1450];
    strcpy_P(resp, start_html);
    strcat_P(resp, Header);
    strcat(resp, "<table style='margin-left: auto; margin-right: auto;' border='1'>");
    strcat(resp, "<tbody><tr>");
    strcat(resp, "<th><a href='index.html'><strong>Hlavni</strong></th>");
    strcat(resp, "<th><strong>RS485 zarizeni</strong></th>");
    strcat(resp, "<th><strong>Pokojove termostaty</strong></th>");
    strcat(resp, "<th><strong>Parovane cidla</strong></th>");
    strcat(resp, "<th><strong>Parovane vystupy</strong></th>");
    strcat(resp, "<th><a href='setup.html'><strong>Vlastni nastaveni</strong></th>");
    strcat(resp, "</tr></tbody>");

    strcat_P(resp, stop_html);
    httpd_resp_send(req, resp, strlen(resp));
    printf("size %d generated html size\n\r", strlen(resp));
    return ESP_OK;
}
*/
