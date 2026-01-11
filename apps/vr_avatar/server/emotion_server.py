import cv2
from fer.fer import FER
from flask import Flask, jsonify
import threading
import time

app = Flask(__name__)

# Global variable to store the latest emotion
# Global variable to store the latest emotion and position
current_data = {"emotion": "neutral", "x": 0.5, "y": 0.5}
lock = threading.Lock()

def emotion_detector():
    global current_data
    
    # Initialize detector
    detector = FER(mtcnn=True)
    
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
            
        # Detect emotion
        # analyze() returns a list of dictionaries
        result = detector.detect_emotions(frame)
        
        if result:
            # Get the top emotion
            emotions = result[0]['emotions']
            dominant_emotion = max(emotions, key=emotions.get)
            print(f"Face detected! Emotion: {dominant_emotion}")
            

            # Get bounding box
            box = result[0]['box'] # [x, y, w, h]
            x, y, w, h = box
            
            # Calculate center (normalized 0.0 - 1.0)
            # Webcam frame is usually 640x480 or similar, but we can get it from frame.shape
            height, width, _ = frame.shape
            
            center_x = (x + w / 2) / width
            center_y = (y + h / 2) / height
            
            # Mirror X because it's a webcam (selfie view)
            center_x = 1.0 - center_x
            
            print(f"Face: {dominant_emotion} at ({center_x:.2f}, {center_y:.2f})")
            
            with lock:
                current_data = {
                    "emotion": dominant_emotion,
                    "x": center_x,
                    "y": center_y
                }
        else:
            print("No face detected", end='\r')
        
        time.sleep(0.1) # Don't hog CPU

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
