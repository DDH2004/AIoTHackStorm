import cv2
from fer.fer import FER
from flask import Flask, jsonify
import threading
import time

app = Flask(__name__)

# Global variable to store the latest emotion
current_emotion = "neutral"
lock = threading.Lock()

def emotion_detector():
    global current_emotion
    
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
            
            with lock:
                current_emotion = dominant_emotion
        else:
            print("No face detected", end='\r')
        
        time.sleep(0.1) # Don't hog CPU

    cap.release()
    cv2.destroyAllWindows()

@app.route('/listener/emotion', methods=['GET'])
def get_emotion():
    with lock:
        return current_emotion

if __name__ == '__main__':
    # Start detector in background thread
    t = threading.Thread(target=emotion_detector, daemon=True)
    t.start()
    
    # Run Flask server
    # Host 0.0.0.0 to be accessible from other devices (Tuya Board)
    app.run(host='0.0.0.0', port=5001)
