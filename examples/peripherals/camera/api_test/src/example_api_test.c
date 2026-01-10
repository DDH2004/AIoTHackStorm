/**
 * @file example_api_test.c
 * @brief API Test example for Tuya Dev Board.
 *
 * This example connects to WiFi and sends a POST request to the specified API endpoints
 * with a dummy face image.
 */

#include "tal_api.h"
#include "tkl_output.h"
#include "netmgr.h"
#include "mix_method.h"
#include "atop_base.h"
#include "dummy_face_data.h"

#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
#include "netconn_wifi.h"
#endif

/***********************************************************
 *                    Macro Definitions
 ***********************************************************/
#define WIFI_SSID     "NETGEAR26"
#define WIFI_PASSWORD "fluffyteapot856"

#define UUID     "uuid829250ee292d71ae"
#define AUTH_KEY "S7z7gE12BKjt4NE5Gbc0vTzwz0Z2uMYP"

#define API_PATH_FACE_PROPERTIES "/v1.0/iot-03/ai-vision/face-properties"
#define API_NAME_FACE_PROPERTIES "ai.vision.face.properties" // Guessing

#define API_PATH_FACE_KEYPOINTS "/v1.0/iot-03/ai-vision/face-keypoints/actions/detect"
#define API_NAME_FACE_KEYPOINTS "ai.vision.face.keypoints.detect" // Guessing

/***********************************************************
 *                    Global Variables
 ***********************************************************/
static bool g_wifi_connected = false;

/***********************************************************
 *                    Helper Functions
 ***********************************************************/

static void api_test_thread(void *arg)
{
    PR_NOTICE("API Test Thread Started");

    // Wait for WiFi connection
    while (!g_wifi_connected) {
        tal_system_sleep(1000);
        PR_NOTICE("Waiting for WiFi connection...");
    }

    PR_NOTICE("WiFi Connected. Starting API Test...");

    // Base64 Encode Image
    int   b64_len = (dummy_face_jpg_len + 2) / 3 * 4 + 1;
    char *b64_img = tal_malloc(b64_len);
    if (b64_img == NULL) {
        PR_ERR("Failed to allocate memory for base64 image");
        return;
    }
    tuya_base64_encode(dummy_face_jpg, b64_img, dummy_face_jpg_len);
    PR_NOTICE("Image Base64 Encoded. Length: %d", strlen(b64_img));

    // Construct JSON Body
    // {"image": "<base64>"}
    // We need to be careful with JSON construction.
    // cJSON might be safer but let's try manual if simple, or cJSON if available.
    // cJSON is available in SDK.

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "image", b64_img);
    char *json_body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    tal_free(b64_img); // Free base64 buffer as it's copied into json_body

    if (json_body == NULL) {
        PR_ERR("Failed to create JSON body");
        return;
    }
    PR_NOTICE("JSON Body created. Length: %d", strlen(json_body));

    // 1. Test Face Properties API
    PR_NOTICE("Testing Face Properties API: %s", API_PATH_FACE_PROPERTIES);

    atop_base_request_t  req  = {0};
    atop_base_response_t resp = {0};

    req.uuid = UUID;
    req.key  = AUTH_KEY;
    req.path = API_PATH_FACE_PROPERTIES;
    req.api =
        API_NAME_FACE_PROPERTIES; // This might be ignored if path is used by atop_base, but atop_base requires it.
    req.version   = "1.0";
    req.timestamp = tal_time_get_posix();
    req.data      = json_body;
    req.datalen   = strlen(json_body);

    // Note: atop_base_request encrypts the data.
    // If the API expects raw JSON in body, atop_base_request might not be suitable if it wraps it in `data=...`.
    // But let's try it.

    int rt = atop_base_request(&req, &resp);
    if (rt != OPRT_OK) {
        PR_ERR("Face Properties API (ATOP) Request Failed: %d", rt);
    } else {
        PR_NOTICE("Face Properties API (ATOP) Request Success: %d", resp.success);
        if (resp.success && resp.result) {
            char *res_str = cJSON_Print(resp.result);
            PR_NOTICE("Response: %s", res_str);
            tal_free(res_str);
        }
        atop_base_response_free(&resp);
    }

    // 1.b Test Face Properties API (Raw HTTP)
    PR_NOTICE("Testing Face Properties API (Raw HTTP): %s", API_PATH_FACE_PROPERTIES);

    http_client_response_t http_resp = {0};
    http_client_header_t   headers[] = {
        {.key = "Content-Type", .value = "application/json"},
        // Add other headers if needed, e.g. Authorization if we had a token
    };

    // Construct URL: https://openapi.tuyacn.com/v1.0/...
    // We need to use http_client_request with host and path.
    // Host: openapi.tuyacn.com (or tuyaus.com)

    rt = http_client_request(
        &(const http_client_request_t){.host   = "openapi.tuyacn.com", // Try China first, or maybe "openapi.tuyaus.com"
                                       .port   = 443,
                                       .method = "POST",
                                       .path   = API_PATH_FACE_PROPERTIES,
                                       .headers       = headers,
                                       .headers_count = 1,
                                       .body          = (uint8_t *)json_body,
                                       .body_length   = strlen(json_body),
                                       .timeout_ms    = 10000},
        &http_resp);

    if (rt != OPRT_OK) {
        PR_ERR("Face Properties API (Raw HTTP) Request Failed: %d", rt);
    } else {
        PR_NOTICE("Face Properties API (Raw HTTP) Request Success. Status: %d", http_resp.status_code);
        if (http_resp.body) {
            PR_NOTICE("Response: %s", http_resp.body);
        }
        http_client_free(&http_resp);
    }

    // 2. Test Face Keypoints API (ATOP)
    PR_NOTICE("Testing Face Keypoints API (ATOP): %s", API_PATH_FACE_KEYPOINTS);

    req.path      = API_PATH_FACE_KEYPOINTS;
    req.api       = API_NAME_FACE_KEYPOINTS;
    req.timestamp = tal_time_get_posix();
    req.data      = json_body; // Re-use body
    req.datalen   = strlen(json_body);

    rt = atop_base_request(&req, &resp);
    if (rt != OPRT_OK) {
        PR_ERR("Face Keypoints API (ATOP) Request Failed: %d", rt);
    } else {
        PR_NOTICE("Face Keypoints API (ATOP) Request Success: %d", resp.success);
        if (resp.success && resp.result) {
            char *res_str = cJSON_Print(resp.result);
            PR_NOTICE("Response: %s", res_str);
            tal_free(res_str);
        }
        atop_base_response_free(&resp);
    }

    // 2.b Test Face Keypoints API (Raw HTTP)
    PR_NOTICE("Testing Face Keypoints API (Raw HTTP): %s", API_PATH_FACE_KEYPOINTS);

    rt = http_client_request(&(const http_client_request_t){.host          = "openapi.tuyacn.com",
                                                            .port          = 443,
                                                            .method        = "POST",
                                                            .path          = API_PATH_FACE_KEYPOINTS,
                                                            .headers       = headers,
                                                            .headers_count = 1,
                                                            .body          = (uint8_t *)json_body,
                                                            .body_length   = strlen(json_body),
                                                            .timeout_ms    = 10000},
                             &http_resp);

    if (rt != OPRT_OK) {
        PR_ERR("Face Keypoints API (Raw HTTP) Request Failed: %d", rt);
    } else {
        PR_NOTICE("Face Keypoints API (Raw HTTP) Request Success. Status: %d", http_resp.status_code);
        if (http_resp.body) {
            PR_NOTICE("Response: %s", http_resp.body);
        }
        http_client_free(&http_resp);
    }

    tal_free(json_body);
    PR_NOTICE("API Test Finished");

    // Delete thread
    tal_thread_delete(NULL);
}

static void link_status_cb(void *data)
{
    netmgr_status_e status = (netmgr_status_e)data;
    if (status == NETMGR_LINK_UP) {
        PR_NOTICE("Network Link UP");
        g_wifi_connected = true;
    } else {
        PR_NOTICE("Network Link DOWN");
        g_wifi_connected = false;
    }
}

void user_main(void)
{
    // Init Logging
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 1024, (TAL_LOG_OUTPUT_CB)tkl_log_output);
    PR_NOTICE("Starting API Test Example");

    // Init KV (Required for some SDK functions)
    tal_kv_init(&(tal_kv_cfg_t){
        .seed = "vmlkasdh93dlvlcy",
        .key  = "dflfuap134ddlduq",
    });

    // Init System
    tal_sw_timer_init();
    tal_workq_init();

    // Init Network Manager
    netmgr_type_e type = 0;
#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
    type |= NETCONN_WIFI;
#endif
    netmgr_init(type);

    // Subscribe to Link Status
    tal_event_subscribe(EVENT_LINK_STATUS_CHG, "api_test", link_status_cb, SUBSCRIBE_TYPE_NORMAL);

#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
    // Connect to WiFi
    netconn_wifi_info_t wifi_info = {0};
    strcpy(wifi_info.ssid, WIFI_SSID);
    strcpy(wifi_info.pswd, WIFI_PASSWORD);
    PR_NOTICE("Connecting to WiFi: %s", WIFI_SSID);
    netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_SSID_PSWD, &wifi_info);
#endif

    // Start API Test Thread
    THREAD_CFG_T thrd_param = {8192, 4, "api_test"}; // Increased stack size for base64/json
    tal_thread_create_and_start(NULL, NULL, NULL, api_test_thread, NULL, &thrd_param);
}

#if OPERATING_SYSTEM == SYSTEM_LINUX
void main(int argc, char *argv[])
{
    user_main();
    while (1) {
        tal_system_sleep(500);
    }
}
#else
static THREAD_HANDLE ty_app_thread = NULL;
static void          tuya_app_thread(void *arg)
{
    user_main();
    tal_thread_delete(ty_app_thread);
    ty_app_thread = NULL;
}
void tuya_app_main(void)
{
    THREAD_CFG_T thrd_param = {4096, 4, "tuya_app_main"};
    tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thrd_param);
}
#endif
