import time
import json
import numpy as np
from pynput.mouse import Listener

# Get tablet dimensions
TABLET_WIDTH_MM = int(input("Enter active tablet width in mm: "))
TABLET_HEIGHT_MM = int(input("Enter active tablet height in mm: "))
TABLET_WIDTH_PX = 4096
TABLET_HEIGHT_PX = 2560
INNERGAMEPLAY_WIDTH_PX = 1152
INNERGAMEPLAY_HEIGHT_PX = 864

data = []

def on_move(x, y):
    """Records mouse movements"""
    data.append((x, y))

def record_movements(duration):
    """Records mouse movements for the given duration"""
    with Listener(on_move=on_move) as listener:
        print(f"Recording started for {duration} seconds...")
        start_time = time.time()
        while time.time() - start_time < duration:
            time.sleep(0.01)  # Sampling every 10ms
        listener.stop()
    
    with open("mouse_data.json", "w") as f:
        json.dump(data, f)
    
    print("Recording completed. Data saved in 'mouse_data.json'.")
    return data

def find_peak_near_extremes(values, min_val, max_val, threshold_percentage=5):
    """Finds the most used point near the detected min/max values"""
    threshold_range = (max_val - min_val) * (threshold_percentage / 100)
    near_min = values[values <= min_val + threshold_range]
    near_max = values[values >= max_val - threshold_range]

    # Remove negative values
    near_min = near_min[near_min >= 0]
    near_max = near_max[near_max >= 0]

    if len(near_min) > 0:
        min_peak = np.bincount(near_min.astype(int)).argmax()
    else:
        min_peak = int(min_val)

    if len(near_max) > 0:
        max_peak = np.bincount(near_max.astype(int)).argmax()
    else:
        max_peak = int(max_val)

    return min_peak, max_peak

def analyze_data(data):
    """Analyzes the movement data and finds dimensions & peak points"""
    x_values = np.array([point[0] for point in data])
    y_values = np.array([point[1] for point in data])
    
    # Remove soft outliers (0.001 - 99.99 percentiles)
    x_1, x_99 = np.percentile(x_values, [0.01, 99.99])
    y_1, y_99 = np.percentile(y_values, [0.01, 99.99])
    
    width_px_filtered = x_99 - x_1
    height_px_filtered = y_99 - y_1

    # Convert to mm
    width_mmC_filtered = (width_px_filtered * TABLET_WIDTH_MM) / INNERGAMEPLAY_WIDTH_PX
    height_mmC_filtered = (height_px_filtered * TABLET_HEIGHT_MM) / INNERGAMEPLAY_HEIGHT_PX

    # Find peak usage near the filtered extremes
    x_min_peak, x_max_peak = find_peak_near_extremes(x_values, x_1, x_99)
    y_min_peak, y_max_peak = find_peak_near_extremes(y_values, y_1, y_99)

    # Convert to mm
    x_min_peak_mm = (x_min_peak * TABLET_WIDTH_MM) / TABLET_WIDTH_PX
    x_max_peak_mm = (x_max_peak * TABLET_WIDTH_MM) / TABLET_WIDTH_PX
    y_min_peak_mm = (y_min_peak * TABLET_HEIGHT_MM) / TABLET_HEIGHT_PX
    y_max_peak_mm = (y_max_peak * TABLET_HEIGHT_MM) / TABLET_HEIGHT_PX

    x_distance_px = x_max_peak - x_min_peak
    y_distance_px = y_max_peak - y_min_peak
    x_distance_mm = (x_distance_px * TABLET_WIDTH_MM) / INNERGAMEPLAY_WIDTH_PX
    y_distance_mm = (y_distance_px * TABLET_HEIGHT_MM) / INNERGAMEPLAY_HEIGHT_PX

    print("\n==== RESULTS ====")
    print(f"Max used area converted with inner gameplay (removed soft outliers): {width_mmC_filtered:.2f} x {height_mmC_filtered:.2f} mm")
    print(f"Area calculated with most used points near extremes (removed soft outliers): {x_distance_mm:.2f} x {y_distance_mm:.2f} mm")

    return width_mmC_filtered, height_mmC_filtered, x_min_peak_mm, x_max_peak_mm, y_min_peak_mm, y_max_peak_mm, x_distance_mm, y_distance_mm

# Run program
duration = int(input("Enter map duration in seconds: "))
data = record_movements(duration)
analyze_data(data)
input("Press Enter to exit...")
