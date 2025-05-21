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
        if (val <= min_val + threshold_range) {
            min_counts[val]++;
        }
        if (val >= max_val - threshold_range) {
            max_counts[val]++;
        }
    }

    int min_peak = min_val;
    int max_peak = max_val;
    int max_min_freq = 0;
    int max_max_freq = 0;

    for (const auto& [val, freq] : min_counts) {
        if (freq > max_min_freq) {
            max_min_freq = freq;
            min_peak = val;
        }
    }

    for (const auto& [val, freq] : max_counts) {
        if (freq > max_max_freq) {
            max_max_freq = freq;
            max_peak = val;
        }
    }

    return {min_peak, max_peak};
}

float compute_rotation_deg(const std::vector<int>& x, const std::vector<int>& y) {
    float mean_x = std::accumulate(x.begin(), x.end(), 0.0f) / x.size();
    float mean_y = std::accumulate(y.begin(), y.end(), 0.0f) / y.size();

    float sxx = 0.0f, sxy = 0.0f;
    for (size_t i = 0; i < x.size(); ++i) {
        float dx = x[i] - mean_x;
        float dy = y[i] - mean_y;
        sxx += dx * dx;
        sxy += dx * dy;
    }

    float angle_rad = std::atan2(2 * sxy, sxx - (std::accumulate(y.begin(), y.end(), 0.0f,
                      [&](float acc, float yi) {
                          float dy = yi - mean_y;
                          return acc + dy * dy;
                      })));
    angle_rad *= 0.5f;

    float angle_deg = angle_rad * (180.0f / M_PI);
    return angle_deg;
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
        for (float val : v) {
            sum += (val - mean) * (val - mean);
        }
        float stddev = std::sqrt(sum / v.size());
        return {mean, stddev};
    };

    // Filter ±3σ
    auto filter_sigma = [](const std::vector<int>& input, float mean, float stddev) {
        std::vector<int> filtered;
        for (int val : input) {
            if (val > mean - 3 * stddev && val < mean + 3 * stddev) {
                filtered.push_back(val);
            }
        }
        return filtered;
    };

    auto [x_mean, x_std] = mean_stddev(x);
    auto [y_mean, y_std] = mean_stddev(y);

    std::vector<int> x_filtered = filter_sigma(x, x_mean, x_std);
    std::vector<int> y_filtered = filter_sigma(y, y_mean, y_std);
    float rotation_deg = compute_rotation_deg(x_filtered, y_filtered);

    auto [x_min_it, x_max_it] = std::minmax_element(x_filtered.begin(), x_filtered.end());
    auto [y_min_it, y_max_it] = std::minmax_element(y_filtered.begin(), y_filtered.end());

    int x_min = *x_min_it, x_max = *x_max_it;
    int y_min = *y_min_it, y_max = *y_max_it;

    auto [x_min_peak, x_max_peak] = find_peak_near_extremes(x_filtered, x_min, x_max);
    auto [y_min_peak, y_max_peak] = find_peak_near_extremes(y_filtered, y_min, y_max);

    int x_distance_px = x_max_peak - x_min_peak;
    int y_distance_px = y_max_peak - y_min_peak;

    int inner_width_px = static_cast<int>((1152.0 / 1920.0) * screen_width);
    int inner_height_px = static_cast<int>((864.0 / 1080.0) * screen_height);

    float x_mm = (x_distance_px * tablet.getWidth()) / inner_width_px;
    float y_mm = (y_distance_px * tablet.getHeight()) / inner_height_px;

    std::cout << "\n==== RESULTS ====\n";
    std::cout << "Used Area (filtered and peak-aligned): " << x_mm << " x " << y_mm << " mm\n";
    std::cout << "Rotation angle (degrees): " << rotation_deg << "°\n";
    std::cout << "=================\n";
}
