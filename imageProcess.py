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
shape_history = deque(maxlen=5)

# 回調函數
def nothing(x):
    pass


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

    # 分析每個輪廓
    for contour in contours:
        # 忽略面積小於 min_area 的輪廓
        if cv2.contourArea(contour) < min_area:
            continue

        # 近似輪廓
        epsilon = 0.02 * cv2.arcLength(contour, True)
        approx = cv2.approxPolyDP(contour, epsilon, True)

        # 獲取頂點數（邊數）
        num_vertices = len(approx)

        # 判斷形狀
        shape = "Quadrilateral" if num_vertices == 4 else f"Polygon ({num_vertices} sides)"
        if num_vertices <= 7:
            # 繪製輪廓
            cv2.drawContours(contour_frame, [approx], -1, (0, 255, 0), 2)

            # 標註形狀
            x, y = approx[0][0]  # 使用第一個頂點作為標註位置
            cv2.putText(contour_frame, shape, (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

            # 儲存當前形狀（假設每個幀只考慮一個主要輪廓）
            current_shape = num_vertices


    # 將當前形狀加入歷史記錄
    shape_history.append(current_shape)

    # 檢查最近 5 幀是否都是矩形
    if len(shape_history) == 5 and sum(1 for shape in shape_history if shape == 4 or shape==5) >= 4:
        print("forward")
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