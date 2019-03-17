#include "html.h"
#include "pgmspace.h"


const char start_html[] PROGMEM = "<html>";
const char stop_html[] PROGMEM = "</html>";

const char Header[] PROGMEM = "<title>term-big-1</title>"; 


/* An HTTP GET handler */
esp_err_t index_get_handler(httpd_req_t *req)
{
    const char* resp_str = (const char*) req->user_ctx;
    char resp[1450]; 


    strcpy_P(resp, start_html);

    strcpy_P(resp, Header);
    strcat_P(resp, stop_html);


    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

