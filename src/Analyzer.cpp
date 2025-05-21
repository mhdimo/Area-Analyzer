#include "Analyzer.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <numeric>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Analyzer::Analyzer(const Tablet& t, int sw, int sh)
    : tablet(t), screen_width(sw), screen_height(sh) {}

static std::pair<int, int> find_peak_near_extremes(
    const std::vector<int>& values,
    int min_val,
    int max_val,
    float threshold_percentage = 5.0f
) {
    float threshold_range = (max_val - min_val) * (threshold_percentage / 100.0f);
    std::unordered_map<int, int> min_counts, max_counts;

    for (int val : values) {
        if (val <= min_val + threshold_range) min_counts[val]++;
        if (val >= max_val - threshold_range) max_counts[val]++;
    }

    int min_peak = min_val, max_peak = max_val;
    int max_min_freq = 0, max_max_freq = 0;

    for (const auto& [val, freq] : min_counts)
        if (freq > max_min_freq) { max_min_freq = freq; min_peak = val; }

    for (const auto& [val, freq] : max_counts)
        if (freq > max_max_freq) { max_max_freq = freq; max_peak = val; }

    return {min_peak, max_peak};
}

void Analyzer::analyze(const std::vector<std::pair<int, int>>& data) const {
    if (data.empty()) return;

    std::vector<int> x, y;
    for (const auto& [px, py] : data) {
        x.push_back(px);
        y.push_back(py);
    }

    auto mean_stddev = [](const std::vector<int>& v) -> std::pair<float, float> {
        float mean = std::accumulate(v.begin(), v.end(), 0.0f) / v.size();
        float sum = 0.0f;
        for (float val : v)
            sum += (val - mean) * (val - mean);
        float stddev = std::sqrt(sum / v.size());
        return {mean, stddev};
    };

    auto filter_sigma = [](const std::vector<int>& input, float mean, float stddev) {
        std::vector<int> filtered;
        for (int val : input)
            if (val > mean - 3 * stddev && val < mean + 3 * stddev)
                filtered.push_back(val);
        return filtered;
    };

    auto [x_mean, x_std] = mean_stddev(x);
    auto [y_mean, y_std] = mean_stddev(y);

    std::vector<int> x_filtered = filter_sigma(x, x_mean, x_std);
    std::vector<int> y_filtered = filter_sigma(y, y_mean, y_std);

    if (x_filtered.size() < 2 || y_filtered.size() < 2) return;

    auto [x_min_it, x_max_it] = std::minmax_element(x_filtered.begin(), x_filtered.end());
    auto [y_min_it, y_max_it] = std::minmax_element(y_filtered.begin(), y_filtered.end());

    int x_min = *x_min_it, x_max = *x_max_it;
    int y_min = *y_min_it, y_max = *y_max_it;

    auto [x_min_peak, x_max_peak] = find_peak_near_extremes(x_filtered, x_min, x_max);
    auto [y_min_peak, y_max_peak] = find_peak_near_extremes(y_filtered, y_min, y_max);

    int inner_width_px = static_cast<int>((1152.0 / 1920.0) * screen_width);
    int inner_height_px = static_cast<int>((864.0 / 1080.0) * screen_height);

    // Compute dominant movement angle (rotation)
    float dx_sum = 0.0f, dy_sum = 0.0f;
    for (size_t i = 1; i < x_filtered.size(); ++i) {
        dx_sum += static_cast<float>(x_filtered[i] - x_filtered[i - 1]);
        dy_sum += static_cast<float>(y_filtered[i] - y_filtered[i - 1]);
    }

    float angle_rad = std::atan2(dy_sum, dx_sum);
    float cos_theta = std::cos(-angle_rad);  // counter-rotate
    float sin_theta = std::sin(-angle_rad);

    // Rotate points around mean center
    std::vector<float> x_rot, y_rot;
    for (size_t i = 0; i < x_filtered.size(); ++i) {
        float xc = x_filtered[i] - x_mean;
        float yc = y_filtered[i] - y_mean;
        float xr = xc * cos_theta - yc * sin_theta;
        float yr = xc * sin_theta + yc * cos_theta;
        x_rot.push_back(xr);
        y_rot.push_back(yr);
    }

    float x_rot_min = *std::min_element(x_rot.begin(), x_rot.end());
    float x_rot_max = *std::max_element(x_rot.begin(), x_rot.end());
    float y_rot_min = *std::min_element(y_rot.begin(), y_rot.end());
    float y_rot_max = *std::max_element(y_rot.begin(), y_rot.end());

    float x_distance_px = x_rot_max - x_rot_min;
    float y_distance_px = y_rot_max - y_rot_min;

    float x_mm = (x_distance_px * tablet.getWidth()) / inner_width_px;
    float y_mm = (y_distance_px * tablet.getHeight()) / inner_height_px;

    std::cout << "\n==== RESULTS ====\n";
    std::cout << "Used Area (rotated and filtered): " << x_mm << " x " << y_mm << " mm\n";
    std::cout << "Dominant Rotation Angle: " << angle_rad * (180.0f / M_PI) << " degrees\n";
    std::cout << "=================\n";
}
