#!/usr/bin/env python3
"""
Emotion Detection Server for VR Avatar
Uses local webcam for face detection (for testing)
"""

from flask import Flask, request, jsonify
import numpy as np
import cv2
import logging
import socket
import threading
import time

app = Flask(__name__)
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(message)s')

# Global state
deepface_ready = False
face_cascade = None
frame_count = 0

# Webcam state
webcam = None
webcam_lock = threading.Lock()
last_webcam_frame = None
last_emotion = "neutral"
last_face = (0, 0, 0, 0)
last_confidence = 50

# Emotions list
EMOTIONS = ["happy", "sad", "angry", "neutral", "surprise", "fear", "disgust"]

def load_models():
    """Load face detection models"""
    global face_cascade, deepface_ready, webcam
    
    # Load OpenCV cascade
    cascade_path = cv2.data.haarcascades + 'haarcascade_frontalface_default.xml'
    face_cascade = cv2.CascadeClassifier(cascade_path)
    logging.info("Face cascade loaded")
    
    # Try to open webcam
    try:
        webcam = cv2.VideoCapture(0)
        if webcam.isOpened():
            webcam.set(cv2.CAP_PROP_FRAME_WIDTH, 320)
            webcam.set(cv2.CAP_PROP_FRAME_HEIGHT, 240)
            logging.info("Webcam opened successfully")
        else:
            logging.warning("Could not open webcam")
            webcam = None
    except Exception as e:
        logging.warning(f"Webcam error: {e}")
        webcam = None
    
    # Try to load DeepFace
    try:
        from deepface import DeepFace
        logging.info("Loading DeepFace model (this may take a moment)...")
        test_img = np.zeros((48, 48, 3), dtype=np.uint8)
        try:
            DeepFace.analyze(test_img, actions=['emotion'], enforce_detection=False, silent=True)
        except:
            pass
        deepface_ready = True
        logging.info("DeepFace ready!")
    except ImportError:
        logging.warning("DeepFace not installed - using position-based simulation")
        deepface_ready = False

def get_local_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except:
        return "127.0.0.1"

def detect_from_webcam():
    """Capture from webcam and detect emotion"""
    global last_emotion, last_face, last_confidence, last_webcam_frame
    
    if webcam is None or not webcam.isOpened():
        return None, "neutral", (80, 60, 80, 80), 50
    
    with webcam_lock:
        ret, frame = webcam.read()
    
    if not ret or frame is None:
        return None, last_emotion, last_face, last_confidence
    
    last_webcam_frame = frame.copy()
    img_h, img_w = frame.shape[:2]
    
    # Convert to grayscale for face detection
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    
    # Detect faces
    faces = face_cascade.detectMultiScale(gray, 1.1, 4, minSize=(30, 30))
    
    if len(faces) == 0:
        return frame, last_emotion, last_face, last_confidence
    
    # Get largest face
    x, y, w, h = max(faces, key=lambda f: f[2] * f[3])
    last_face = (int(x), int(y), int(w), int(h))
    
    logging.info(f"Webcam face at ({x},{y}) size {w}x{h}")
    
    # Detect emotion
    emotion = "neutral"
    confidence = 50
    
    if deepface_ready:
        try:
            from deepface import DeepFace
            margin = 20
            fx1 = max(0, x - margin)
            fy1 = max(0, y - margin)
            fx2 = min(img_w, x + w + margin)
            fy2 = min(img_h, y + h + margin)
            face_img = frame[fy1:fy2, fx1:fx2]
            
            result = DeepFace.analyze(face_img, actions=['emotion'], 
                                     enforce_detection=False, silent=True)
            if isinstance(result, list):
                result = result[0]
            emotion = result.get('dominant_emotion', 'neutral')
            emotions_dict = result.get('emotion', {})
            confidence = int(emotions_dict.get(emotion, 50))
            logging.info(f"DeepFace emotion: {emotion} ({confidence}%)")
        except Exception as e:
            logging.warning(f"DeepFace error: {e}")
            # Fall back to position-based
            center_x = x + w // 2
            if center_x < img_w * 0.33:
                emotion = "sad"
            elif center_x > img_w * 0.66:
                emotion = "happy"
            else:
                emotion = "neutral"
            confidence = 60
    else:
        # Position-based emotion (for testing without DeepFace)
        center_x = x + w // 2
        center_y = y + h // 2
        
        if center_y < img_h * 0.4:
            emotion = "surprise"
        elif center_x < img_w * 0.33:
            emotion = "sad"
        elif center_x > img_w * 0.66:
            emotion = "happy"
        else:
            emotion = "neutral"
        confidence = 65
    
    last_emotion = emotion
    last_confidence = confidence
    
    return frame, emotion, last_face, confidence

@app.route('/health', methods=['GET'])
def health():
    return jsonify({
        "status": "ok",
        "deepface_ready": deepface_ready,
        "webcam_ready": webcam is not None and webcam.isOpened(),
        "frame_count": frame_count,
        "last_emotion": last_emotion
    })

@app.route('/detect_simple', methods=['POST'])
def detect_simple():
    """
    When T5AI sends a frame, we use the local webcam instead
    (since T5AI is sending test patterns, not real camera data)
    """
    global frame_count
    frame_count += 1
    
    try:
        data = request.get_data()
        data_len = len(data)
        logging.info(f"Received {data_len} bytes from T5AI (frame #{frame_count})")
        
        # Use webcam for face detection
        frame, emotion, face, confidence = detect_from_webcam()
        x, y, w, h = face
        
        response = f"{emotion}:{x}:{y}:{w}:{h}:{confidence}"
        logging.info(f">>> {response}")
        return response
        
    except Exception as e:
        logging.error(f"Error: {e}")
        import traceback
        traceback.print_exc()
        return "error:0:0:0:0:0"

@app.route('/detect_webcam', methods=['GET'])
def detect_webcam_direct():
    """Direct webcam detection endpoint (for testing)"""
    frame, emotion, face, confidence = detect_from_webcam()
    x, y, w, h = face
    return jsonify({
        "emotion": emotion,
        "face": {"x": x, "y": y, "w": w, "h": h},
        "confidence": confidence
    })

def cleanup():
    global webcam
    if webcam is not None:
        webcam.release()

if __name__ == '__main__':
    import atexit
    atexit.register(cleanup)
    
    load_models()
    
    local_ip = get_local_ip()
    port = 5000
    
    print("\n" + "="*60)
    print("  EMOTION DETECTION SERVER (Webcam Mode)")
    print("="*60)
    print(f"  Local IP:   {local_ip}")
    print(f"  Port:       {port}")
    print(f"  Endpoint:   http://{local_ip}:{port}/detect_simple")
    print(f"  Webcam:     {'Ready' if webcam and webcam.isOpened() else 'Not available'}")
    print(f"  DeepFace:   {'Ready' if deepface_ready else 'Using position-based detection'}")
    print("="*60)
    print("  Point your face at the webcam!")
    print("  The T5AI avatar will mirror your detected emotion.")
    print("="*60 + "\n")
    
    app.run(host='0.0.0.0', port=port, threaded=True)