import cv2
from fer.fer import FER
from flask import Flask, jsonify
import threading
import time
import mediapipe as mp
import math

app = Flask(__name__)

# Global variable to store the latest emotion
# Global variable to store the latest emotion and position
current_data = {"emotion": "neutral", "x": 0.5, "y": 0.5, "mouth_open": 0.0}
lock = threading.Lock()

def emotion_detector():
    global current_data
    
    # Initialize detector
    detector = FER(mtcnn=True)
    
    # Initialize MediaPipe Face Landmarker (Tasks API)
    # Using the downloaded model file
    BaseOptions = mp.tasks.BaseOptions
    FaceLandmarker = mp.tasks.vision.FaceLandmarker
    FaceLandmarkerOptions = mp.tasks.vision.FaceLandmarkerOptions
    VisionRunningMode = mp.tasks.vision.RunningMode

    options = FaceLandmarkerOptions(
        base_options=BaseOptions(model_asset_path='face_landmarker.task'),
        running_mode=VisionRunningMode.VIDEO,
        num_faces=1,
        min_face_detection_confidence=0.5,
        min_face_presence_confidence=0.5,
        min_tracking_confidence=0.5)
    
    landmarker = FaceLandmarker.create_from_options(options)

    frame_count = 0
    skip_frames = 10 # Run emotion detection every 10 frames
    prev_time = time.time()
    
    # Open webcam
    cap = cv2.VideoCapture(0)
    
    if not cap.isOpened():
        print("Error: Could not open webcam.")
        return

    print("Webcam opened. detecting emotions...")
    
    while True:
        ret, frame = cap.read()
        if not ret:
            continue
            
        height, width, _ = frame.shape
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        
        # 1. EMOTION DETECTION (Throttled)
        dominant_emotion = current_data["emotion"] # Default to last known
        if frame_count % skip_frames == 0:
            result = detector.detect_emotions(frame)
            if result:
                emotions = result[0]['emotions']
                dominant_emotion = max(emotions, key=emotions.get)
                # print(f"Face detected! Emotion: {dominant_emotion}")

        # 2. FACE MESH (Every frame for smooth tracking)
        # Convert to MediaPipe Image
        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)
        
        # Detect asynchronously/synchronously. For VIDEO mode, use detect_for_video
        timestamp_ms = int(time.time() * 1000)
        results = landmarker.detect_for_video(mp_image, timestamp_ms)
        
        center_x = 0.5
        center_y = 0.5
        mouth_open = 0.0
        
        if results.face_landmarks:
            for face_landmarks in results.face_landmarks:
                # face_landmarks is a list of normalized landmarks
                
                # --- POSITION ---
                # Use nose tip (1) position
                nose = face_landmarks[1]
                center_x = 1.0 - nose.x # Mirror
                center_y = nose.y
                
                # --- MOUTH OPENNESS ---
                # Upper lip top: 13, Lower lip bottom: 14 (inner)
                upper_lip = face_landmarks[13]
                lower_lip = face_landmarks[14]
                
                # Calculate distance
                mouth_dist = math.hypot(upper_lip.x - lower_lip.x, upper_lip.y - lower_lip.y)
                
                # Normalize by face scale
                top_head = face_landmarks[10]
                chin = face_landmarks[152]
                face_height = math.hypot(top_head.x - chin.x, top_head.y - chin.y)
                
                if face_height > 0:
                    ratio = mouth_dist / face_height
                    # Empirical thresholds
                    min_ratio = 0.02
                    max_ratio = 0.15
                    mouth_open = (ratio - min_ratio) / (max_ratio - min_ratio)
                    mouth_open = max(0.0, min(1.0, mouth_open))
                
                with lock:
                    current_data = {
                        "emotion": dominant_emotion,
                        "x": center_x,
                        "y": center_y,
                        "mouth_open": mouth_open
                    }
        else:
             pass

        frame_count += 1
        
        # Display preview (optional, helpful for debugging)
        # cv2.imshow('Webcam', frame)
        # if cv2.waitKey(1) & 0xFF == ord('q'):
        #     break
        
        # Calculate FPS
        current_time = time.time()
        fps = 1.0 / (current_time - prev_time) if frame_count > 0 else 0
        prev_time = current_time
        
        # print(f"FPS: {fps:.1f}", end='\r')
        
        time.sleep(0.001) # Minimal sleep to yield 

    cap.release()
    cv2.destroyAllWindows()


@app.route('/listener/emotion', methods=['GET'])
def get_emotion():
    with lock:
        return jsonify(current_data)

if __name__ == '__main__':
    # Start detector in background thread
    t = threading.Thread(target=emotion_detector, daemon=True)
    t.start()
    
    # Run Flask server
    # Host 0.0.0.0 to be accessible from other devices (Tuya Board)
    app.run(host='0.0.0.0', port=5001)
