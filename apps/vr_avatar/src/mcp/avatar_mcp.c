/**
 * @file avatar_mcp.c
 * @brief MCP Tool Registration for Remote Avatar Control
 */

#include "avatar_mcp.h"
#include "tal_api.h"
#include "wukong_ai_mcp_server.h" // Standard MCP Library
#include "emotion_manager.h"
#include <string.h>

// --- TOOL CALLBACK ---
static OPERATE_RET __set_emotion_cb(const MCP_PROPERTY_LIST_T *properties, MCP_RETURN_VALUE_T *ret_val, void *user_data)
{
    char *emotion_str = "neutral";

    for (int i = 0; i < properties->count; i++) {
        MCP_PROPERTY_T *prop = properties->properties[i];
        if (strcmp(prop->name, "emotion") == 0 && prop->type == MCP_PROPERTY_TYPE_STRING) {

            // FIX: Use 'default_val' (based on your app_mcp.c example)
            emotion_str = prop->default_val.str_val;

            break;
        }
    }

    PR_NOTICE(">>> MCP COMMAND: Set Emotion to '%s' <<<", emotion_str);

    // 2. Map String -> Enum
    emotion_type_t target = EMOTION_NEUTRAL;
    if (strcmp(emotion_str, "happy") == 0)
        target = EMOTION_HAPPY;
    else if (strcmp(emotion_str, "sad") == 0)
        target = EMOTION_SAD;
    else if (strcmp(emotion_str, "angry") == 0)
        target = EMOTION_ANGRY;
    else if (strcmp(emotion_str, "confused") == 0)
        target = EMOTION_CONFUSED;

    // 3. Update the Avatar Immediately
    emotion_set_current(target);

    // 4. Return "True" to the AI Agent
    wukong_mcp_return_value_set_bool(ret_val, TRUE);
    return OPRT_OK;
}

// --- INIT MCP SERVER ---
static OPERATE_RET __avatar_mcp_start(void *data)
{
    // Initialize the MCP Server
    wukong_mcp_server_init("VR Avatar Host", "1.0");

    // Add the "set_emotion" tool
    WUKONG_MCP_TOOL_ADD("device.avatar.set_emotion",
                        "Sets the avatar facial expression. Use this to react to user voice or events.",
                        __set_emotion_cb, NULL,
                        MCP_PROP_STR("emotion", "Target emotion: 'happy', 'sad', 'angry', 'confused', 'neutral'"));

    PR_NOTICE("Avatar MCP Tool Registered!");
    return OPRT_OK;
}

// Public Init
OPERATE_RET avatar_mcp_init(void)
{
    // Wait for MQTT to connect before registering tools
    return tal_event_subscribe(EVENT_MQTT_CONNECTED, "avatar_mcp_start", __avatar_mcp_start, SUBSCRIBE_TYPE_ONETIME);
}