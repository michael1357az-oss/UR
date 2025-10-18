import cv2
import apriltag

# --- CONFIGURATION ---
# Set the URL of your ESP32-CAM video stream
VIDEO_STREAM_URL = "http://172.20.10.5/stream"

# --- INITIALIZATION ---
# Initialize the video capture
cap = cv2.VideoCapture(VIDEO_STREAM_URL)
if not cap.isOpened():
    print("Error: Could not open video stream.")
    exit()

# Initialize the AprilTag detector
# Common families include: 'tag36h11', 'tag25h9', 'tag16h5'
detector = apriltag.Detector(families='tag36h11')

print("Starting AprilTag detection... Press 'q' to quit.")

# --- MAIN LOOP ---
while True:
    # Read a frame from the video stream
    ret, frame = cap.read()
    if not ret:
        print("Error: Could not read frame from stream.")
        break

    # Convert the frame to grayscale (required for AprilTag detection)
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    # Detect AprilTags in the grayscale image
    detections = detector.detect(gray)

    # Loop over any detections and draw the results on the frame
    for det in detections:
        # Get the bounding box corners and convert them to integers
        (ptA, ptB, ptC, ptD) = det.corners
        ptA = (int(ptA[0]), int(ptA[1]))
        ptB = (int(ptB[0]), int(ptB[1]))
        ptC = (int(ptC[0]), int(ptC[1]))
        ptD = (int(ptD[0]), int(ptD[1]))

        # Draw the bounding box around the detected tag
        cv2.line(frame, ptA, ptB, (0, 255, 0), 2)
        cv2.line(frame, ptB, ptC, (0, 255, 0), 2)
        cv2.line(frame, ptC, ptD, (0, 255, 0), 2)
        cv2.line(frame, ptD, ptA, (0, 255, 0), 2)

        # Draw the center of the tag as a red circle
        (cX, cY) = (int(det.center[0]), int(det.center[1]))
        cv2.circle(frame, (cX, cY), 5, (0, 0, 255), -1)

        # Draw the tag ID on the image
        tag_id = str(det.tag_id)
        cv2.putText(frame, f"ID: {tag_id}", (ptA[0], ptA[1] - 15),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
        
        # Print the detected tag ID to the console
        print(f"[INFO] Detected AprilTag ID: {tag_id}")

    # Display the resulting frame
    cv2.imshow("AprilTag Detections", frame)

    # Check for the 'q' key to exit the loop
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# --- CLEANUP ---
# Release the video capture and close all windows
print("Shutting down...")
cap.release()
cv2.destroyAllWindows()