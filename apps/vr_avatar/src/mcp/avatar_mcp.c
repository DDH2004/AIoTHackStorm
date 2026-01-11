#include "tal_api.h"
#include "emotion_manager.h"
#include "avatar_mcp.h"
#include <string.h>
#include "http_client_interface.h"
#include "netmgr.h"
#include "cJSON.h"

static face_position_t sg_face_pos = {0.5f, 0.5f}; // Default center

face_position_t avatar_get_position(void)
{
    return sg_face_pos;
}

GW_WIFI_STAT_E get_wf_status(void)
{
    netmgr_status_e status = NETMGR_LINK_DOWN;
    netmgr_conn_get(NETCONN_WIFI, NETCONN_CMD_STATUS, &status);

    if (status == NETMGR_LINK_UP || status == NETMGR_LINK_UP_SWITH) {
        return STAT_IP_GOT;
    }

    return STAT_WIFI_UNCONNECTED;
}

// Configuration for your specific server
#define SERVER_HOST "192.168.34.34"
#define SERVER_PORT 5001
#define SERVER_PATH "/listener/emotion"

// Buffer to hold the raw response (Headers + Body)
static uint8_t s_http_buffer[1024];

void update_emotion_from_server(void)
{
    // PR_NOTICE("Checking WiFi status...");
    if (get_wf_status() < 4) {
        // PR_NOTICE("WiFi not ready");
        return;
    }

    http_client_request_t  request  = {0};
    http_client_response_t response = {0};

    request.host       = SERVER_HOST;
    request.port       = SERVER_PORT;
    request.path       = SERVER_PATH;
    request.method     = "GET";
    request.timeout_ms = 1500;

    memset(s_http_buffer, 0, sizeof(s_http_buffer));
    response.buffer        = s_http_buffer;
    response.buffer_length = sizeof(s_http_buffer);

    PR_NOTICE("Sending HTTP GET to %s:%d%s", SERVER_HOST, SERVER_PORT, SERVER_PATH);
    http_client_status_t status = http_client_request(&request, &response);
    PR_NOTICE("HTTP status: %d, Code: %d", status, response.status_code);

    if (status == HTTP_CLIENT_SUCCESS && response.status_code == 200) {
        if (response.body != NULL && response.body_length > 0) {
            // Ensure null termination just in case
            if (response.body_length < sizeof(s_http_buffer)) {
                s_http_buffer[response.body_length] = '\0';
            } else {
                s_http_buffer[sizeof(s_http_buffer) - 1] = '\0';
            }

            char *body_str = (char *)response.body;
            PR_NOTICE("Body: %s", body_str);

            // Parse JSON
            cJSON *root = cJSON_Parse(body_str);
            if (root) {
                cJSON *emotion_item = cJSON_GetObjectItem(root, "emotion");
                cJSON *x_item       = cJSON_GetObjectItem(root, "x");
                cJSON *y_item       = cJSON_GetObjectItem(root, "y");

                if (x_item && y_item) {
                    sg_face_pos.x = (float)x_item->valuedouble;
                    sg_face_pos.y = (float)y_item->valuedouble;
                }

                if (emotion_item && emotion_item->valuestring) {
                    char *emo_str = emotion_item->valuestring;
                    if (strstr(emo_str, "happy"))
                        emotion_set_current(EMOTION_HAPPY);
                    else if (strstr(emo_str, "sad"))
                        emotion_set_current(EMOTION_SAD);
                    else if (strstr(emo_str, "angry"))
                        emotion_set_current(EMOTION_ANGRY);
                    else if (strstr(emo_str, "confused"))
                        emotion_set_current(EMOTION_CONFUSED);
                    else if (strstr(emo_str, "neutral"))
                        emotion_set_current(EMOTION_NEUTRAL);
                    else if (strstr(emo_str, "fear") || strstr(emo_str, "surprise"))
                        emotion_set_current(EMOTION_SURPRISED);
                }
                cJSON_Delete(root);
            }
        }
    }

    http_client_free(&response);
}