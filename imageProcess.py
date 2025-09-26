import time
import cv2
import numpy as np
import pyapriltags
# Initial values for thresholds and Canny
lower_threshold = 50
upper_threshold = 255
canny1 = 240
canny2 = 240

# Initial values for black filter in HSV
lower_hue = 0
lower_saturation = 0
lower_value = 0
upper_hue = 180
upper_saturation = 255
upper_value = 50

def on_canny1_change(val):
    global canny1
    canny1 = val

def on_canny2_change(val):
    global canny2
    canny2 = val

def on_lower_threshold_change(val):
    global lower_threshold
    lower_threshold = val

def on_upper_threshold_change(val):
    global upper_threshold
    upper_threshold = val

def on_lower_hue_change(val):
    global lower_hue
    lower_hue = val

def on_lower_saturation_change(val):
    global lower_saturation
    lower_saturation = val

def on_lower_value_change(val):
    global lower_value
    lower_value = val

def on_upper_hue_change(val):
    global upper_hue
    upper_hue = val

def on_upper_saturation_change(val):
    global upper_saturation
    upper_saturation = max(1, val)  # Ensure non-zero to avoid invalid range

def on_upper_value_change(val):
    global upper_value
    upper_value = max(1, val)  # Ensure non-zero to avoid invalid range

def erosion_then_dilation(img):
    imgcopy = img.copy()
    kernel = np.ones((10, 10))
    erosion = cv2.erode(imgcopy, kernel, iterations=1)
    dilation = cv2.dilate(erosion, kernel, iterations=1)
    return dilation

def dilation_then_erosion(img):
    imgcopy = img.copy()
    kernel = np.ones((10, 10))
    dilation = cv2.dilate(imgcopy, kernel, iterations=1)
    erosion = cv2.erode(dilation, kernel, iterations=1)
    return erosion

def erosion(img):
    imgcopy = img.copy()
    kernel = np.ones((5, 5))
    erosion = cv2.erode(imgcopy, kernel, iterations=1)
    return erosion

def filter_black(img):
    # Convert image to HSV color space
    hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)

    # Define range of black color in HSV
    lower_black = np.array([lower_hue, lower_saturation, lower_value])
    upper_black = np.array([upper_hue, upper_saturation, upper_value])

    # Create mask for black pixels
    mask = cv2.inRange(hsv, lower_black, upper_black)

    # Apply mask to keep only black pixels
    result = cv2.bitwise_and(img, img, mask=mask)
    
    return result, mask

def read_mjpeg_stream(url):
    # Open the MJPEG stream with OpenCV
    cap = cv2.VideoCapture(url)
    
    if not cap.isOpened():
        print("Failed to open stream")
        return

    """# Create windows for display
    cv2.namedWindow('thresh')
    cv2.namedWindow('canny')
    cv2.namedWindow('black_filtered')
    cv2.namedWindow('Controls', cv2.WINDOW_NORMAL)
    


    # Create trackbars for binary threshold (optional, if still needed)
    cv2.createTrackbar('Lower Threshold', 'Controls', lower_threshold, 255, on_lower_threshold_change)
    cv2.createTrackbar('Upper Threshold', 'Controls', upper_threshold, 255, on_upper_threshold_change)
    cv2.createTrackbar('canny1', 'Controls', canny1, 255, on_canny1_change)
    cv2.createTrackbar('canny2', 'Controls', canny2, 255, on_canny2_change)

    # Create trackbars for black filter (HSV ranges)
    cv2.createTrackbar('Lower Hue', 'Controls', lower_hue, 179, on_lower_hue_change)
    cv2.createTrackbar('Lower Saturation', 'Controls', lower_saturation, 255, on_lower_saturation_change)
    cv2.createTrackbar('Lower Value', 'Controls', lower_value, 255, on_lower_value_change)
    cv2.createTrackbar('Upper Hue', 'Controls', upper_hue, 179, on_upper_hue_change)
    cv2.createTrackbar('Upper Saturation', 'Controls', upper_saturation, 255, on_upper_saturation_change)
    cv2.createTrackbar('Upper Value', 'Controls', upper_value, 255, on_upper_value_change)"""

    while True:
        ret, frame = cap.read()
        if not ret:
            print("Failed to capture frame")
            break

        # Apply bilateral filter
        bilateral = cv2.bilateralFilter(frame, 9, 75, 75)
        
        # Apply black filter
        black_filtered, mask = filter_black(bilateral)
        
        # Apply morphological operations to the mask
        thresh1 = dilation_then_erosion(mask)
        thresh1 = erosion(thresh1)
        
        # Optional: Convert to grayscale if needed elsewhere
        gray = cv2.cvtColor(bilateral, cv2.COLOR_BGR2GRAY)
        
        # Optional: Canny on the processed mask (but for contours, we use the binary mask directly)
        canny = cv2.Canny(thresh1, canny1, canny2)
        
        # Find contours directly on the processed mask for better blob detection
        contours, hierarchy = cv2.findContours(thresh1, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        # Create canvas for drawing
        canvas = np.zeros_like(frame)

        # Draw the largest contour
        
        if len(contours) > 0:
            # Filter contours by minimum area to ignore noise (adjust min_area as needed)
            min_area = 100  # Example threshold; tune based on your setup
            large_contours = [c for c in contours if cv2.contourArea(c) > min_area]
            
            if len(large_contours) > 0:
                largest_contour = max(large_contours, key=cv2.contourArea)
                # Draw only the largest contour in green
                cv2.drawContours(canvas, [largest_contour], -1, (0, 255, 0), 1)
                # Print area and number of points in the largest contour
                area = cv2.contourArea(largest_contour)
                """print(f"Area of largest contour: {area}")
                print(f"Number of points in largest contour: {len(largest_contour)}")"""
                # Compute centroid using moments
                M = cv2.moments(largest_contour)
                if M["m00"] != 0:  # Avoid division by zero
                    cx = int(M["m10"] / M["m00"])  # Centroid x-coordinate
                    cy = int(M["m01"] / M["m00"])  # Centroid y-coordinate
                    print(f"Centroid of largest contour: ({cx}, {cy})")
                    # Draw centroid as a red dot
                    cv2.circle(canvas, (cx, cy), 5, (0, 0, 255), -1)
                """"else:
                    print("Centroid not computed (area is zero)")
            else:
                print("No large contours detected")
        else:
            print("No contours detected")"""

        # Display the frames
        #cv2.imshow('original', frame)
        cv2.imshow('bilateral', bilateral)
        #cv2.imshow('black_filtered', black_filtered)
        cv2.imshow('gray', gray)
        #cv2.imshow('thresh', thresh1)
        #cv2.imshow('canny', canny)
        cv2.imshow('biggestContour', canvas)

        detector = pyapriltags.Detector(families='tag36h11')
        # Detect AprilTags in the image
        detections = detector.detect(gray)

        # Check if any tags were detected
        if len(detections) == 0:
            pass
        else:
            # Iterate through detected tags and print their IDs
            for tag in detections:
                print(f"Detected AprilTag ID: {tag.tag_id}")
        # Press 'q' to quit
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
        time.sleep(0.05)

    # Clean up
    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    stream_url = "http://172.20.10.5:3585/stream"  # Update with actual IP
    read_mjpeg_stream(stream_url)