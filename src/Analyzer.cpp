#include "Analyzer.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <numeric>

Analyzer::Analyzer(const Tablet& t, int sw, int sh)
    : tablet(t), screen_width(sw), screen_height(sh) {}

void Analyzer::analyze(const std::vector<std::pair<int, int>>& data) const {
    if (data.empty()) return;

    std::vector<int> x, y;
    for (const auto& [px, py] : data) {
        x.push_back(px);
        y.push_back(py);
    }

    auto [min_x, max_x] = std::minmax_element(x.begin(), x.end());
    auto [min_y, max_y] = std::minmax_element(y.begin(), y.end());

    int x_distance = *max_x - *min_x;
    int y_distance = *max_y - *min_y;

    int inner_width_px = static_cast<int>((1152.0 / 1920.0) * screen_width);
    int inner_height_px = static_cast<int>((864.0 / 1080.0) * screen_height);

    float x_mm = (x_distance * tablet.getWidth()) / inner_width_px;
    float y_mm = (y_distance * tablet.getHeight()) / inner_height_px;

    std::cout << "\n==== RESULTS ====\n";
    std::cout << "Used Area: " << x_mm << " x " << y_mm << " mm\n";
    std::cout << "=================\n";
}