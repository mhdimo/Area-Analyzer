#include "Analyzer.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace {
constexpr double pi = 3.14159265358979323846;
constexpr std::size_t minimum_samples = 20;

struct Point { double x, y; };

double quantile(const std::vector<double>& sorted, double fraction) {
    const double index = fraction * static_cast<double>(sorted.size() - 1);
    const auto lower = static_cast<std::size_t>(index);
    const auto upper = std::min(lower + 1, sorted.size() - 1);
    return sorted[lower] + (sorted[upper] - sorted[lower]) * (index - lower);
}

std::pair<double, double> outlier_bounds(std::vector<double> values) {
    std::sort(values.begin(), values.end());
    const double q1 = quantile(values, 0.25), q3 = quantile(values, 0.75);
    const double padding = 1.5 * (q3 - q1);
    return {q1 - padding, q3 + padding};
}
} // namespace

Analyzer::Analyzer(const Tablet& t, int sw, int sh, AnalysisOptions settings)
    : tablet(t), screen_width(sw), screen_height(sh), options(settings) {
    if (sw <= 0 || sh <= 0) {
        throw std::invalid_argument("Mapped screen dimensions must be positive.");
    }
    if (!std::isfinite(options.trim_fraction) || options.trim_fraction < 0 || options.trim_fraction >= 0.25) {
        throw std::invalid_argument("Trim fraction must be in [0, 0.25).");
    }
    if (!std::isfinite(options.safety_margin) || options.safety_margin < 0 || options.safety_margin > 1) {
        throw std::invalid_argument("Safety margin must be in [0, 1].");
    }
}

AnalysisResult Analyzer::calculate(const std::vector<std::pair<int, int>>& data) const {
    AnalysisResult result;
    result.total_samples = data.size();
    std::vector<Point> points;
    points.reserve(data.size());
    std::vector<double> xs, ys;
    xs.reserve(data.size()); ys.reserve(data.size());
    std::optional<std::pair<int, int>> previous;
    std::optional<std::pair<int, int>> last_movement;
    std::size_t edge_samples = 0;

    for (const auto& sample : data) {
        if (previous && sample == *previous) {
            ++result.stationary_samples;
            continue;
        }
        previous = sample;
        const double x = static_cast<double>(sample.first) - options.screen_origin_x;
        const double y = static_cast<double>(sample.second) - options.screen_origin_y;
        if (x < 0 || x >= screen_width || y < 0 || y >= screen_height) {
            ++result.discarded_samples;
            continue;
        }
        if (x <= 1 || x >= screen_width - 2 || y <= 1 || y >= screen_height - 2) ++edge_samples;
        // A pen resting in place can alternate among neighboring pixels. Use
        // the last RETAINED position so slow, real movement still accumulates.
        if (last_movement && std::hypot(static_cast<double>(sample.first) - last_movement->first,
                                        static_cast<double>(sample.second) - last_movement->second) < 3.0) {
            ++result.stationary_samples;
            continue;
        }
        last_movement = sample;
        // Convert to physical space BEFORE estimating rotation. Pixel axes can
        // have different scales when the tablet and monitor aspects differ.
        Point p{x / screen_width * tablet.getWidth(), y / screen_height * tablet.getHeight()};
        points.push_back(p); xs.push_back(p.x); ys.push_back(p.y);
    }

    if (points.size() < minimum_samples) {
        result.message = "Not enough movement samples on the mapped screen. Record a longer section with movement in both directions.";
        return result;
    }

    const auto x_bounds = outlier_bounds(xs);
    const auto y_bounds = outlier_bounds(ys);
    // Keep or discard WHOLE samples; independently filtering axes destroys the
    // covariance and can leave differently sized coordinate arrays.
    points.erase(std::remove_if(points.begin(), points.end(), [&](Point p) {
        if (p.x < x_bounds.first || p.x > x_bounds.second || p.y < y_bounds.first || p.y > y_bounds.second) {
            ++result.discarded_samples;
            return true;
        }
        return false;
    }), points.end());
    result.used_samples = points.size();
    if (points.size() < minimum_samples) {
        result.message = "Too few movement samples remain after removing outliers. Record another section.";
        return result;
    }

    Point mean{0, 0};
    for (Point p : points) { mean.x += p.x; mean.y += p.y; }
    mean.x /= points.size(); mean.y /= points.size();
    double xx = 0, xy = 0, yy = 0;
    for (Point p : points) {
        const double dx = p.x - mean.x, dy = p.y - mean.y;
        xx += dx * dx; xy += dx * dy; yy += dy * dy;
    }
    xx /= points.size(); xy /= points.size(); yy /= points.size();
    const double total_variance = xx + yy;
    const double difference = std::hypot(xx - yy, 2 * xy);
    const double major = (total_variance + difference) / 2;
    const double minor = (total_variance - difference) / 2;
    if (!std::isfinite(total_variance) || major <= 0 || minor <= 1e-6 || minor / major < 0.01) {
        result.message = "Movement is stationary or nearly a straight line. A two-dimensional recording is needed to recommend an area.";
        return result;
    }

    result.direction_strength = difference / total_variance;
    result.rotation_reliable = result.direction_strength >= 0.15;
    double angle = 0;
    if (result.rotation_reliable) {
        angle = 0.5 * std::atan2(2 * xy, xx - yy);
        // Covariance axes are unordered. Choose the one nearest horizontal to
        // avoid reporting a 90-degree rotation for taller movement regions.
        if (angle > pi / 4) angle -= pi / 2;
        if (angle <= -pi / 4) angle += pi / 2;
    } else {
        result.warnings.push_back("Movement has no clear principal direction; rotation is undetermined. Dimensions use 0 degrees.");
    }
    result.rotation_deg = angle * 180 / pi;
    if (std::abs(result.rotation_deg) >= 40) {
        result.warnings.push_back("Rotation is near the 45-degree axis boundary; small changes can swap width and height. Check both the angle and dimensions in your driver preview.");
    }
    const double c = std::cos(angle), s = std::sin(angle);
    xs.clear(); ys.clear();
    for (Point p : points) {
        const double dx = p.x - mean.x, dy = p.y - mean.y;
        xs.push_back(c * dx + s * dy);
        ys.push_back(-s * dx + c * dy);
    }
    std::sort(xs.begin(), xs.end()); std::sort(ys.begin(), ys.end());
    const double left = quantile(xs, options.trim_fraction);
    const double right = quantile(xs, 1 - options.trim_fraction);
    const double top = quantile(ys, options.trim_fraction);
    const double bottom = quantile(ys, 1 - options.trim_fraction);
    result.used_width_mm = right - left;
    result.used_height_mm = bottom - top;
    if (result.used_width_mm < 1 || result.used_height_mm < 1) {
        result.message = "Movement is too small to recommend an area. Record actual gameplay rather than cursor jitter.";
        return result;
    }
    const double cx = (left + right) / 2, cy = (top + bottom) / 2;
    result.center_x_mm = mean.x + c * cx - s * cy;
    result.center_y_mm = mean.y + s * cx + c * cy;

    // The default osu! playfield is 512 x 384 inside a fitted 640 x 480
    // region, not 60% of every monitor's width. See README for assumptions.
    const double game_scale = std::min(screen_width / 640.0, screen_height / 480.0);
    const double x_fraction = 512 * game_scale / screen_width;
    const double y_fraction = 384 * game_scale / screen_height;
    double width = result.used_width_mm / x_fraction * (1 + options.safety_margin);
    double height = result.used_height_mm / y_fraction * (1 + options.safety_margin);
    if (options.preserve_aspect_ratio) {
        const double aspect = static_cast<double>(screen_width) / screen_height;
        width = std::max(width, height * aspect);
        height = width / aspect;
    }

    // Uniformly reduce the recommendation if its rotated corners would extend
    // beyond the tablet, preserving both its aspect and proposed orientation.
    const double bounds_width = width * std::abs(c) + height * std::abs(s);
    const double bounds_height = width * std::abs(s) + height * std::abs(c);
    const double fit = std::min({1.0, tablet.getWidth() / bounds_width, tablet.getHeight() / bounds_height});
    result.area_limited = fit < 1;
    result.recommended_width_mm = width * fit;
    result.recommended_height_mm = height * fit;
    if (result.area_limited) {
        result.warnings.push_back("The proposed area exceeds the tablet's physical bounds and was reduced to fit. It cannot preserve the full measured movement range.");
    }
    if (edge_samples > 0) {
        result.warnings.push_back("The cursor reached a screen edge; clipping can hide pen movement and underestimate the area. Check the recording setup.");
    }
    result.valid = true;
    return result;
}

bool Analyzer::analyze(const std::vector<std::pair<int, int>>& data) const {
    const auto result = calculate(data);
    if (!result.valid) {
        std::cout << "\nNo area recommendation: " << result.message << '\n';
        return false;
    }
    std::ostringstream output;
    output << std::fixed << std::setprecision(2)
           << "\n==== RESULTS ====\n"
           << "Measured movement (trimmed, rotation-aligned): " << result.used_width_mm << " x " << result.used_height_mm << " mm\n"
           << "Suggested driver area: " << result.recommended_width_mm << " x " << result.recommended_height_mm << " mm\n"
           << "Area margin: " << options.safety_margin * 100 << "%"
           << (options.preserve_aspect_ratio ? ", screen aspect ratio preserved\n" : ", independent width/height\n")
           << "Measured movement center (from tablet top-left): " << result.center_x_mm << ", " << result.center_y_mm << " mm\n";
    if (result.rotation_reliable) {
        output << "Estimated area rotation: " << result.rotation_deg << " degrees (positive = clockwise)\n";
    } else {
        output << "Estimated area rotation: undetermined\n";
    }
    output << "Samples: " << result.used_samples << " used / " << result.total_samples
           << " recorded; " << result.stationary_samples << " stationary/jitter samples, " << result.discarded_samples << " outside/outliers\n";
    for (const auto& warning : result.warnings) output << "Note: " << warning << '\n';
    output << "Use these as starting settings. Map patterns affect the estimate; compare several representative recordings.\n"
           << "Center the driver area so all rotated corners fit on the tablet. The measured movement center is not a calibrated driver offset.\n"
           << "=================\n";
    std::cout << output.str();
    return true;
}
