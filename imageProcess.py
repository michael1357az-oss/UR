import cv2
import numpy as np
from collections import deque

# 開啟攝影機
cap = cv2.VideoCapture("http://172.20.10.5:3585/stream")  # IP 攝影機流
if not cap.isOpened():
    print("錯誤：無法開啟攝影機！")
    exit()

# 創建顯示窗口
cv2.namedWindow('Extracted Black Parts')

# 定義初始值
threshold_value = 80
min_area = 5000  # 初始最小面積

# 用於儲存最近 5 幀的形狀資訊
shape_history = deque(maxlen=10)

# 回調函數
def nothing(x):
    pass

# 分類轉彎方向的函數（基於頂部和底部平均 x 座標的啟發式方法）
def classify_turn(approx):
    points = approx.reshape(-1, 2).astype(np.float32)
    if len(points) < 6:
        return None
    
    # 計算質心
    M = cv2.moments(points)
    if M["m00"] == 0:
        return None
    cx = M["m10"] / M["m00"]
    cy = M["m01"] / M["m00"]
    
    # 分離頂部和底部點（相對於質心 y）
    top_mask = points[:, 1] < cy
    bottom_mask = points[:, 1] > cy
    top_points = points[top_mask]
    bottom_points = points[bottom_mask]
    
    # 如果沒有足夠的頂部或底部點，使用所有點的邊界
    if len(top_points) == 0:
        min_y = np.min(points[:, 1])
        height = np.max(points[:, 1]) - min_y
        top_y_threshold = min_y + height * 0.3
        top_points = points[points[:, 1] < top_y_threshold]
    if len(bottom_points) == 0:
        max_y = np.max(points[:, 1])
        height = max_y - np.min(points[:, 1])
        bottom_y_threshold = max_y - height * 0.3
        bottom_points = points[points[:, 1] > bottom_y_threshold]
    
    if len(top_points) == 0 or len(bottom_points) == 0:
        return None
    
    avg_x_top = np.mean(top_points[:, 0])
    avg_x_bottom = np.mean(bottom_points[:, 0])
    
    # 啟發式判斷：如果頂部平均 x < 底部平均 x（向右傾斜，像 "7"），則 "turn left"
    # 反之，向左傾斜，像水平反轉的 "7"，則 "turn right"
    if avg_x_top < avg_x_bottom:
        return "turn_left"
    else:
        return "turn_right"

# 創建滑桿
cv2.createTrackbar('Threshold', 'Extracted Black Parts', threshold_value, 255, nothing)
cv2.createTrackbar('Min Area', 'Extracted Black Parts', min_area, 10000, nothing)

while True:
    # 讀取攝影機的每一幀
    ret, frame = cap.read()
    if not ret:
        print("錯誤：無法讀取攝影機畫面！")
        break

    # 將畫面轉換為灰度圖
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    # 從滑桿獲取當前閾值和最小面積
    threshold_value = cv2.getTrackbarPos('Threshold', 'Extracted Black Parts')
    min_area = cv2.getTrackbarPos('Min Area', 'Extracted Black Parts')

    # 二值化處理
    _, binary = cv2.threshold(gray, threshold_value, 255, cv2.THRESH_BINARY_INV)

    # 形態學操作去除噪點
    kernel = np.ones((5,5), np.uint8)
    cleaned = cv2.morphologyEx(binary, cv2.MORPH_OPEN, kernel, iterations=2)
    cleaned = cv2.morphologyEx(cleaned, cv2.MORPH_CLOSE, kernel, iterations=2)

    # 創建結果圖片
    result = np.zeros_like(frame)
    result[cleaned == 255] = frame[cleaned == 255]

    # 輪廓檢測
    contours, _ = cv2.findContours(cleaned, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    # 複製原始畫面用於繪製輪廓
    contour_frame = frame.copy()
    current_shape = None

    if contours:
        # 找到面積最大的輪廓索引（用於決定 shape_history）
        areas = [cv2.contourArea(c) for c in contours]
        largest_idx = np.argmax(areas)
        
        # 分析每個輪廓（繪製所有符合條件的，但只用最大的決定 current_shape）
        for i, contour in enumerate(contours):
            # 忽略面積小於 min_area 的輪廓
            if cv2.contourArea(contour) < min_area:
                continue

            # 近似輪廓
            epsilon = 0.02 * cv2.arcLength(contour, True)
            approx = cv2.approxPolyDP(contour, epsilon, True)

            # 獲取頂點數（邊數）
            num_vertices = len(approx)

            # 判斷形狀
            shape_text = "Quadrilateral" if num_vertices == 4 else f"Polygon ({num_vertices} sides)"
            
            if num_vertices <= 7:
                # 繪製輪廓
                cv2.drawContours(contour_frame, [approx], -1, (0, 255, 0), 2)

                # 如果是最大的輪廓，決定 current_shape
                if i == largest_idx:
                    if num_vertices == 4 or num_vertices == 5:
                        current_shape = "forward"
                        shape_text = "Forward"
                    elif num_vertices == 6 or num_vertices == 7:
                        turn_dir = classify_turn(approx)
                        if turn_dir:
                            current_shape = turn_dir
                            shape_text = turn_dir.replace("_", " ")
                        else:
                            current_shape = None
                            shape_text = f"Polygon ({num_vertices}) - Unknown"

                # 標註形狀（使用第一個頂點作為標註位置）
                if len(approx) > 0:
                    x, y = approx[0][0]
                    cv2.putText(contour_frame, shape_text, (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

    # 將當前形狀加入歷史記錄
    shape_history.append(current_shape)

    # 檢查最近 5 幀的主導形狀並決定輸出
    if len(shape_history) == 10:
        forward_count = sum(1 for s in shape_history if s == "forward")
        left_count = sum(1 for s in shape_history if s == "turn_left")
        right_count = sum(1 for s in shape_history if s == "turn_right")
        
        if forward_count >= 8:
            print("forward")
        elif left_count >= 8:
            print("turn left")
        elif right_count >= 8:
            print("turn right")
        else:
            print("stop")
    else:
        print("stop")

    # 顯示原始畫面（帶輪廓和標註）、二值化結果和提取結果
    cv2.imshow('Original Video with Contours', contour_frame)
    cv2.imshow("Non Cleaned Image", binary)
    cv2.imshow('Binary Image', cleaned)
    cv2.imshow('Extracted Black Parts', result)

    # 按 'q' 鍵退出
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# 釋放攝影機並關閉窗口
cap.release()
cv2.destroyAllWindows()