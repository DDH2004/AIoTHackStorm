/**
 * @file audio_capture.h
 * @brief Audio capture with VAD for VR Avatar
 */

#ifndef __AUDIO_CAPTURE_H__
#define __AUDIO_CAPTURE_H__

#include "tuya_cloud_types.h"

// --- VOICE STATUS ---
typedef enum {
    VOICE_STATUS_SILENCE = 0,
    VOICE_STATUS_SPEAKING
} voice_status_t;

// --- CALLBACK TYPE ---
typedef void (*voice_status_cb_t)(voice_status_t status);

// --- API FUNCTIONS ---

/**
 * @brief Initialize audio capture system
 * @return 0 on success, -1 on failure
 */
int audio_capture_init(void);

/**
 * @brief Start audio capture with voice detection
 * @param callback Function to call when voice status changes
 * @return 0 on success, -1 on failure
 */
int audio_capture_start(voice_status_cb_t callback);

/**
 * @brief Stop audio capture
 * @return 0 on success, -1 on failure
 */
int audio_capture_stop(void);

/**
 * @brief Check if audio capture is running
 * @return TRUE if running, FALSE otherwise
 */
BOOL_T audio_capture_is_running(void);

/**
 * @brief Get current audio level (RMS)
 * @return Current audio level value
 */
uint32_t audio_capture_get_level(void);

/**
 * @brief Get peak audio level since last reset
 * @return Peak audio level value
 */
uint32_t audio_capture_get_peak_level(void);

/**
 * @brief Reset peak audio level tracking
 */
void audio_capture_reset_peak(void);

/**
 * @brief Get the count of how many times the callback has been invoked
 * @return Number of times the callback has been called
 */
uint32_t audio_capture_get_callback_count(void);

#endif /* __AUDIO_CAPTURE_H__ */
