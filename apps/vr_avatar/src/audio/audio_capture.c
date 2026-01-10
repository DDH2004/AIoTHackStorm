/**
 * @file audio_capture.c
 * @brief Audio capture with VAD and visual level feedback
 */

#include "audio_capture.h"
#include "tal_api.h"
#include "tdl_audio_manage.h"
#include "tdl_audio_driver.h"
#include "tkl_vad.h"
#include "board_com_api.h"

// --- CONFIGURATION ---
#define VAD_THRESHOLD_LEVEL    100000   // RMS squared threshold
#define VAD_SILENCE_TIMEOUT_MS 500

// --- MODULE STATE ---
static TDL_AUDIO_HANDLE_T sg_audio_hdl = NULL;
static voice_status_cb_t sg_voice_callback = NULL;
static volatile BOOL_T sg_is_running = FALSE;
static volatile voice_status_t sg_last_status = VOICE_STATUS_SILENCE;
static volatile uint32_t sg_last_voice_time = 0;

// Audio level monitoring - IMPORTANT for debugging
static volatile uint32_t sg_current_audio_level = 0;
static volatile uint32_t sg_peak_audio_level = 0;
static volatile uint32_t sg_frame_count = 0;
static volatile uint32_t sg_callback_count = 0;  // Track if callback is being called

// --- CALCULATE AUDIO LEVEL (RMS squared) ---
static uint32_t calculate_audio_level(uint8_t *data, uint32_t len)
{
    if (data == NULL || len < 2) return 0;
    
    int16_t *samples = (int16_t *)data;
    uint32_t num_samples = len / sizeof(int16_t);
    uint64_t sum_squares = 0;
    
    for (uint32_t i = 0; i < num_samples; i++) {
        int32_t sample = (int32_t)samples[i];
        sum_squares += (uint64_t)(sample * sample);
    }
    
    return (uint32_t)(sum_squares / num_samples);
}

// --- AUDIO FRAME CALLBACK (called by audio driver) ---
static void audio_mic_callback(TDL_AUDIO_FRAME_FORMAT_E type, 
                                TDL_AUDIO_STATUS_E status, 
                                uint8_t *data, 
                                uint32_t len)
{
    sg_callback_count++;  // Track that we're receiving callbacks
    
    // Log first few callbacks to confirm mic is working
    if (sg_callback_count <= 5) {
        PR_NOTICE(">>> MIC CALLBACK #%u: type=%d, status=%d, len=%u <<<", 
                  sg_callback_count, type, status, len);
    }
    
    // Log periodically to show mic is still active
    if (sg_callback_count % 100 == 0) {
        PR_DEBUG("MIC: %u callbacks, level=%u, peak=%u", 
                 sg_callback_count, sg_current_audio_level, sg_peak_audio_level);
    }
    
    if (data == NULL || len == 0) {
        PR_WARN("MIC callback: NULL data or zero len");
        return;
    }
    
    sg_frame_count++;
    
    // Calculate audio level
    sg_current_audio_level = calculate_audio_level(data, len);
    
    // Track peak
    if (sg_current_audio_level > sg_peak_audio_level) {
        sg_peak_audio_level = sg_current_audio_level;
        PR_NOTICE("NEW PEAK LEVEL: %u", sg_peak_audio_level);
    }
    
    // Detect voice based on level threshold
    uint32_t now = tal_system_get_millisecond();
    voice_status_t new_status = sg_last_status;
    
    if (sg_current_audio_level > VAD_THRESHOLD_LEVEL) {
        sg_last_voice_time = now;
        if (sg_last_status != VOICE_STATUS_SPEAKING) {
            new_status = VOICE_STATUS_SPEAKING;
            PR_NOTICE(">>> VOICE DETECTED! Level: %u (threshold: %u) <<<", 
                      sg_current_audio_level, VAD_THRESHOLD_LEVEL);
        }
    } else {
        if (sg_last_status == VOICE_STATUS_SPEAKING) {
            if ((now - sg_last_voice_time) > VAD_SILENCE_TIMEOUT_MS) {
                new_status = VOICE_STATUS_SILENCE;
                PR_NOTICE(">>> SILENCE DETECTED <<<");
            }
        }
    }
    
    // Notify on change
    if (new_status != sg_last_status) {
        sg_last_status = new_status;
        if (sg_voice_callback) {
            sg_voice_callback(new_status);
        }
    }
}

// --- PUBLIC API ---

int audio_capture_init(void)
{
    OPERATE_RET rt = OPRT_OK;
    
    PR_NOTICE("========================================");
    PR_NOTICE("  AUDIO CAPTURE INITIALIZATION");
    PR_NOTICE("========================================");
    PR_NOTICE("Looking for audio device: '%s'", AUDIO_CODEC_NAME);
    
    // Find audio device
    rt = tdl_audio_find(AUDIO_CODEC_NAME, &sg_audio_hdl);
    if (rt != OPRT_OK || sg_audio_hdl == NULL) {
        PR_ERR("!!! AUDIO DEVICE '%s' NOT FOUND (err=%d) !!!", AUDIO_CODEC_NAME, rt);
        PR_ERR("Check: 1) Device name in Kconfig  2) board_register_hardware()");
        return -1;
    }
    
    PR_NOTICE(">>> Audio device FOUND: %s <<<", AUDIO_CODEC_NAME);
    PR_NOTICE("Audio handle: %p", sg_audio_hdl);
    PR_NOTICE("VAD threshold: %u", VAD_THRESHOLD_LEVEL);
    PR_NOTICE("========================================");
    
    return 0;
}

int audio_capture_start(voice_status_cb_t callback)
{
    OPERATE_RET rt = OPRT_OK;
    
    if (sg_audio_hdl == NULL) {
        PR_ERR("Audio not initialized!");
        return -1;
    }
    
    if (sg_is_running) {
        PR_WARN("Audio capture already running");
        return 0;
    }
    
    sg_voice_callback = callback;
    sg_last_status = VOICE_STATUS_SILENCE;
    sg_frame_count = 0;
    sg_callback_count = 0;
    sg_peak_audio_level = 0;
    sg_current_audio_level = 0;
    
    PR_NOTICE("========================================");
    PR_NOTICE("  STARTING AUDIO CAPTURE");
    PR_NOTICE("========================================");
    
    // Open audio with mic callback
    rt = tdl_audio_open(sg_audio_hdl, audio_mic_callback);
    if (rt != OPRT_OK) {
        PR_ERR("!!! Failed to open audio: %d !!!", rt);
        return -1;
    }
    
    sg_is_running = TRUE;
    
    PR_NOTICE(">>> AUDIO CAPTURE STARTED <<<");
    PR_NOTICE("Waiting for microphone data...");
    PR_NOTICE("Speak into the microphone!");
    PR_NOTICE("Watch for 'MIC CALLBACK' messages in serial output");
    PR_NOTICE("========================================");
    
    return 0;
}

int audio_capture_stop(void)
{
    if (!sg_is_running) {
        return 0;
    }
    
    tdl_audio_close(sg_audio_hdl);
    sg_is_running = FALSE;
    sg_voice_callback = NULL;
    
    PR_NOTICE("Audio stopped. Total callbacks: %u, Peak level: %u", 
              sg_callback_count, sg_peak_audio_level);
    
    return 0;
}

BOOL_T audio_capture_is_running(void)
{
    return sg_is_running;
}

uint32_t audio_capture_get_level(void)
{
    return sg_current_audio_level;
}

uint32_t audio_capture_get_peak_level(void)
{
    return sg_peak_audio_level;
}

uint32_t audio_capture_get_callback_count(void)
{
    return sg_callback_count;
}

void audio_capture_reset_peak(void)
{
    sg_peak_audio_level = 0;
}
