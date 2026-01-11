# VR Avatar - Real-Time Emotion Tracking Display

A real-time emotion-tracking avatar system that displays expressive face animations on a Tuya T5AI embedded device. The system uses computer vision for face detection and emotion recognition on a host PC, streaming the detected emotions and face position to the embedded display via HTTP.

![VR Avatar Demo](../../docs/images/vr_avatar_demo.gif)

## 🎯 Features

- **Real-Time Emotion Detection**: Uses FER (Facial Expression Recognition) with MTCNN for accurate face detection
- **6 Expressive Emotions**: Happy, Sad, Angry, Confused, Surprised, and Neutral
- **Face Position Tracking**: Avatar follows your face position on screen
- **Double Buffering**: Smooth 20 FPS animation rendering
- **WiFi Communication**: HTTP-based communication between server and device
- **MCP Architecture**: Modular Component Protocol for clean code organization

## 📁 Project Structure

```
vr_avatar/
├── CMakeLists.txt           # Build configuration
├── app_default.config       # Default board configuration (T5AI)
├── README.md                # This file
│
├── include/
│   └── emotion_manager.h    # Public emotion API header
│
├── server/
│   └── emotion_server.py    # Python emotion detection server
│
└── src/
    ├── vr_avatar_main.c     # Main application entry point
    ├── dummy_face_data.h    # Fallback face data
    │
    ├── ai/
    │   ├── ai_manager.c     # AI service integration
    │   └── ai_manager.h
    │
    ├── camera/
    │   ├── camera_manager.c # Camera handling (optional)
    │   └── camera_manager.h
    │
    ├── emotions/
    │   ├── emotion_manager.c    # Core emotion rendering
    │   ├── emotion_manager.h
    │   ├── emotion_neutral.h    # 😐 Neutral face params
    │   ├── emotion_happy.h      # 😊 Happy face params
    │   ├── emotion_sad.h        # 😢 Sad face params
    │   ├── emotion_angry.h      # 😠 Angry face params
    │   ├── emotion_confused.h   # 😕 Confused face params
    │   └── emotion_surprised.h  # 😲 Surprised face params
    │
    └── mcp/
        ├── avatar_mcp.c     # HTTP client & emotion sync
        └── avatar_mcp.h
```

## 🛠️ Hardware Requirements

- **Tuya T5AI Development Board** with LCD display (320x480)
- **Host PC** with webcam (for emotion detection server)
- Both devices on the same WiFi network

## 📦 Software Dependencies

### Embedded (Tuya T5AI)
- TuyaOS SDK
- LVGL display drivers (via `tdl_display`)
- HTTP client library
- cJSON parser

### Server (Python)
```bash
pip install opencv-python fer flask
```

## 🚀 Quick Start

### 1. Configure WiFi Credentials

Edit `src/vr_avatar_main.c` and update:
```c
#define WIFI_SSID     "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
```

### 2. Configure Server IP

Edit `src/mcp/avatar_mcp.c` and set your host PC's IP:
```c
#define SERVER_HOST "192.168.x.x"  // Your PC's IP address
#define SERVER_PORT 5001
```

### 3. Start the Emotion Server

On your host PC:
```bash
cd server/
python emotion_server.py
```

The server will:
- Open your webcam
- Detect faces and emotions in real-time
- Serve emotion data at `http://0.0.0.0:5001/listener/emotion`

### 4. Build and Flash

From the SDK root directory:
```bash
# Build for T5AI board
./build.sh apps/vr_avatar DNESP32S3_BOX

# Flash to device
./flash.sh
```

## 🔧 Configuration

### Board Configuration (`app_default.config`)

```ini
CONFIG_BOARD_CHOICE_T5AI=y
CONFIG_TUYA_T5AI_BOARD_EX_MODULE_35565LCD=y
CONFIG_ENABLE_EX_MODULE_CAMERA=y
```

### Emotion Parameters

Each emotion is defined in its own header file with customizable parameters:
- Eye size, position, and shape
- Eyebrow angle and position
- Mouth curve and dimensions
- Colors and animation

## 🌐 API Reference

### HTTP Endpoint

**GET** `/listener/emotion`

Response:
```json
{
  "emotion": "happy",
  "x": 0.65,
  "y": 0.42
}
```

| Field | Type | Description |
|-------|------|-------------|
| `emotion` | string | Detected emotion: `happy`, `sad`, `angry`, `confused`, `surprised`, `neutral`, `fear` |
| `x` | float | Normalized face X position (0.0 - 1.0) |
| `y` | float | Normalized face Y position (0.0 - 1.0) |

### Emotion Types

```c
typedef enum {
    EMOTION_NEUTRAL = 0,
    EMOTION_HAPPY,
    EMOTION_SAD,
    EMOTION_ANGRY,
    EMOTION_CONFUSED,
    EMOTION_SURPRISED
} emotion_type_t;
```

## 🎨 Customization

### Adding a New Emotion

1. Create `src/emotions/emotion_newtype.h` with face parameters
2. Add the enum value to `emotion_type_t`
3. Register in `emotion_manager.c`:
   ```c
   static const emotion_face_params_t *sg_emotion_params[] = {
       // ... existing emotions
       &EMOTION_PARAMS_NEWTYPE
   };
   ```

### Adjusting Animation Speed

In `vr_avatar_main.c`:
```c
tal_system_sleep(50);  // 50ms = 20 FPS, lower = faster
```

### Changing Display Size

The system auto-detects display dimensions from the board driver. For manual override:
```c
sg_screen_width  = 320;
sg_screen_height = 480;
```

## 🐛 Troubleshooting

| Issue | Solution |
|-------|----------|
| No face detected | Ensure good lighting and face the webcam directly |
| Display shows nothing | Check WiFi connection and server IP configuration |
| Slow response | Reduce polling interval in `http_update_thread()` |
| Build errors | Ensure all TuyaOS dependencies are installed |

## 📄 License

This project is part of the TuyaOS Open SDK and follows its licensing terms.

## 🙏 Acknowledgments

- [FER (Facial Expression Recognition)](https://github.com/justinshenk/fer) - Emotion detection
- [MTCNN](https://github.com/ipazc/mtcnn) - Face detection
- [TuyaOS](https://developer.tuya.com/) - Embedded platform SDK
