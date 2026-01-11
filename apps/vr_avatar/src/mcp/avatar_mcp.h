#ifndef __AVATAR_MCP_H__
#define __AVATAR_MCP_H__

/**
 * @brief WiFi Status Enum (compatible with legacy/custom definitions)
 */
typedef enum {
    STAT_WIFI_UNCONNECTED = 0,
    STAT_WIFI_CONNECTED   = 3, // Connected to AP
    STAT_IP_GOT           = 4, // Got IP
    STAT_CLOUD_CONNECTED  = 5  // Connected to Cloud
} GW_WIFI_STAT_E;

typedef struct {
    float x; // Normalized 0.0 - 1.0
    float y; // Normalized 0.0 - 1.0
} face_position_t;

/**
 * @brief Get the current WiFi/Network status
 * @return GW_WIFI_STAT_E
 */
GW_WIFI_STAT_E get_wf_status(void);

/**
 * @brief Polls the Python server for the current emotion state
 */
void update_emotion_from_server(void);

/**
 * @brief Get the latest face position
 */
face_position_t avatar_get_position(void);

#endif // __AVATAR_MCP_H__