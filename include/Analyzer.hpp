#pragma once
#include "GraphicTablet.hpp"
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

struct AnalysisOptions {
    // Coordinates and dimensions must use the same units as the OS cursor API.
    int screen_origin_x = 0;
    int screen_origin_y = 0;
    double trim_fraction = 0.01; // Trim each tail after projection, in [0, 0.25).
    double safety_margin = 0.05; // Total extra width/height, in [0, 1].
    bool preserve_aspect_ratio = false;
};

struct AnalysisResult {
    bool valid = false;
    std::string message;
    std::vector<std::string> warnings;
    std::size_t total_samples = 0;
    std::size_t used_samples = 0;
    std::size_t stationary_samples = 0;
    std::size_t discarded_samples = 0;
    double used_width_mm = 0;
    double used_height_mm = 0;
    double recommended_width_mm = 0;
    double recommended_height_mm = 0;
    // Measured movement center, relative to the tablet's top-left corner.
    double center_x_mm = 0;
    double center_y_mm = 0;
    // Nearest-horizontal principal axis in (-45, 45], positive clockwise.
    double rotation_deg = 0;
    double direction_strength = 0; // Covariance anisotropy, not a probability.
    bool rotation_reliable = false;
    bool area_limited = false;
};

class Analyzer {
public:
    Analyzer(const Tablet& tablet, int screen_width, int screen_height,
             AnalysisOptions options = {});
    AnalysisResult calculate(const std::vector<std::pair<int, int>>& data) const;
    bool analyze(const std::vector<std::pair<int, int>>& data) const;

private:
    Tablet tablet;
    int screen_width;
    int screen_height;
    AnalysisOptions options;
};
