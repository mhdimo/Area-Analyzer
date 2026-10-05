#include "Analyzer.hpp"
#include "TabletFinder.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr double pi = 3.14159265358979323846;
using Samples = std::vector<std::pair<int, int>>;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void near(double actual, double expected, double tolerance, const std::string& label) {
    require(std::isfinite(actual) && std::abs(actual - expected) <= tolerance,
            label + ": expected " + std::to_string(expected) + ", got " + std::to_string(actual));
}

template <typename F> void invalid(F action) {
    try { action(); } catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("Expected invalid_argument");
}

// Known physical rectangle, independently mapped to a 200 x 100 mm tablet.
Samples rectangle(double degrees = 0, int sw = 1920, int sh = 1080,
                  double width = 40, double height = 20, double cx = 100, double cy = 50) {
    Samples points;
    const double c = std::cos(degrees * pi / 180), s = std::sin(degrees * pi / 180);
    for (int ix = 0; ix <= 40; ++ix) {
        for (int iy = 0; iy <= 40; ++iy) {
            const double x = width * (ix / 40.0 - 0.5);
            const double y = height * (iy / 40.0 - 0.5);
            points.emplace_back(static_cast<int>(std::lround((cx + c*x - s*y) * sw / 200)),
                                static_cast<int>(std::lround((cy + s*x + c*y) * sh / 100)));
        }
    }
    return points;
}

Tablet testTablet() { return Tablet("Test", "200 x 100", 200, 100); }

void invalid_dimensions() {
    for (double value : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::quiet_NaN()}) {
        invalid([&] { Tablet t("Test", "invalid", value, 100); });
        invalid([&] { Tablet t("Test", "invalid", 200, value); });
    }
    invalid([] { Analyzer a(testTablet(), 0, 1080); });
    invalid([] { Analyzer a(testTablet(), 1920, -1); });
}

void invalid_options() {
    AnalysisOptions options;
    for (double value : {-0.1, 0.5, std::numeric_limits<double>::quiet_NaN()}) {
        options.trim_fraction = value;
        invalid([&] { Analyzer a(testTablet(), 1920, 1080, options); });
    }
    options = {};
    for (double value : {-0.1, 2.0, std::numeric_limits<double>::infinity()}) {
        options.safety_margin = value;
        invalid([&] { Analyzer a(testTablet(), 1920, 1080, options); });
    }
}

void insufficient_recordings() {
    Analyzer analyzer(testTablet(), 1920, 1080);
    for (const auto& points : {Samples{}, Samples{{100, 100}}, Samples(1000, {960, 540})}) {
        auto result = analyzer.calculate(points);
        require(!result.valid && !result.message.empty(), "Insufficient data must explain why it cannot recommend an area");
    }
    Samples horizontal, vertical, diagonal, jitter;
    for (int i = 0; i < 100; ++i) {
        horizontal.emplace_back(500 + i, 500);
        vertical.emplace_back(500, 500 + i);
        diagonal.emplace_back(500 + i, 500 + i);
        jitter.emplace_back(960 + i%3, 540 + (i/3)%3);
    }
    for (const auto& points : {horizontal, vertical, diagonal, jitter})
        require(!analyzer.calculate(points).valid, "Lines and tiny jitter cannot establish a two-dimensional play area");
}

void physical_rotation() {
    for (double angle : {-40.0, -30.0, 0.0, 30.0, 40.0}) {
        auto result = Analyzer(testTablet(), 1920, 1080).calculate(rectangle(angle));
        require(result.valid && result.rotation_reliable, "A two-dimensional rectangle should have a usable direction");
        near(result.rotation_deg, angle, 0.2, "physical rotation");
        near(result.used_width_mm, 40, 0.3, "derotated movement width");
        near(result.used_height_mm, 20, 0.3, "derotated movement height");
        near(result.recommended_width_mm, 70, 0.6, "recommended width includes full-screen scaling and 5% margin");
        near(result.recommended_height_mm, 26.25, 0.4, "recommended height");
    }
}

void non_widescreen_scaling() {
    struct Case { int width, height; double expected_width; };
    for (auto c : {Case{1600, 1200, 52.5}, Case{3440, 1440, 94.0625}, Case{900, 1600, 52.5}}) {
        auto result = Analyzer(testTablet(), c.width, c.height).calculate(rectangle(0, c.width, c.height));
        require(result.valid, "Different monitor aspect ratios must be supported");
        near(result.recommended_width_mm, c.expected_width, 0.5, "aspect-correct driver width");
        near(result.recommended_height_mm, c.width == 900 ? 62.222222 : 26.25, 0.5, "aspect-correct driver height");
    }
}

void paired_outliers() {
    auto points = rectangle(30);
    const auto clean = Analyzer(testTablet(), 1920, 1080).calculate(points);
    points.insert(points.begin() + 10, {960, 1000000});
    points.insert(points.begin() + 100, {-1000000, 540});
    points.insert(points.begin() + 200, {1800, 540});
    auto result = Analyzer(testTablet(), 1920, 1080).calculate(points);
    require(result.valid, "Independent-axis outliers must not corrupt pairs");
    near(result.rotation_deg, clean.rotation_deg, 0.1, "outlier-resistant angle");
    near(result.recommended_width_mm, clean.recommended_width_mm, 0.2, "outlier-resistant width");
    require(result.discarded_samples >= 3, "Report excluded samples");
}

void stationary_dwell() {
    auto points = rectangle(20);
    const auto clean = Analyzer(testTablet(), 1920, 1080).calculate(points);
    points.insert(points.begin() + 100, 20000, points[99]);
    const auto result = Analyzer(testTablet(), 1920, 1080).calculate(points);
    require(result.valid, "A stationary pause should not overwhelm real movement");
    near(result.rotation_deg, clean.rotation_deg, 0.001, "dwell-independent angle");
    near(result.used_width_mm, clean.used_width_mm, 0.001, "dwell-independent width");
}

void jittering_dwell() {
    const auto points = rectangle(20);
    const auto clean = Analyzer(testTablet(), 1920, 1080).calculate(points);
    for (int count : {1000, 3000}) {
        auto with_pause = points;
        for (int i = 0; i < count; ++i) with_pause.emplace_back(960 + i%3 - 1, 540 + (i/3)%3 - 1);
        const auto result = Analyzer(testTablet(), 1920, 1080).calculate(with_pause);
        require(result.valid, "Small pen jitter during a pause must not erase actual movement");
        near(result.rotation_deg, clean.rotation_deg, 0.1, "jitter-pause-independent angle");
        near(result.recommended_width_mm, clean.recommended_width_mm, 0.2, "jitter-pause-independent width");
        near(result.recommended_height_mm, clean.recommended_height_mm, 0.2, "jitter-pause-independent height");
    }
}

void ambiguous_axis_boundary() {
    for (double angle : {44.99, 45.01, -44.99, -45.01}) {
        const auto result = Analyzer(testTablet(), 1920, 1080).calculate(rectangle(angle));
        require(result.valid, "A diagonal movement region still has measurable dimensions");
        require(!result.warnings.empty(), "Explain possible width/height axis swaps near 45 degrees");
        require(result.rotation_deg > -45 && result.rotation_deg <= 45, "Keep the documented angle range");
    }
}

void slow_movement() {
    Samples points;
    for (int x = 768; x < 1152; ++x) points.emplace_back(x, 432);
    for (int y = 432; y < 648; ++y) points.emplace_back(1152, y);
    for (int x = 1152; x > 768; --x) points.emplace_back(x, 648);
    for (int y = 648; y > 432; --y) points.emplace_back(768, y);
    const auto result = Analyzer(testTablet(), 1920, 1080).calculate(points);
    require(result.valid, "One-pixel steps must accumulate into real movement rather than be discarded as jitter");
    near(result.rotation_deg, 0, 0.2, "slow perimeter rotation");
    near(result.used_width_mm, 40, 0.2, "slow perimeter width");
    near(result.used_height_mm, 20, 0.2, "slow perimeter height");
}

void clipping_warning() {
    auto points = rectangle();
    points.emplace_back(1917, 540);
    points.emplace_back(1919, 540);
    const auto result = Analyzer(testTablet(), 1920, 1080).calculate(points);
    require(result.valid, "Interior motion still yields a result when one edge point is filtered out");
    require(std::any_of(result.warnings.begin(), result.warnings.end(), [](const std::string& warning) {
        return warning.find("screen edge") != std::string::npos;
    }), "An actual screen-edge sample must warn about clipping even if ignored as jitter");
}

void uncertain_rotation() {
    auto result = Analyzer(testTablet(), 1920, 1080).calculate(rectangle(20, 1920, 1080, 20, 20));
    require(result.valid && !result.rotation_reliable, "Near-isotropic data must not claim a reliable angle");
    near(result.rotation_deg, 0, 0.001, "ambiguous rotation falls back to zero");
    require(!result.warnings.empty(), "Explain ambiguous rotation");
}

void vertical_principal_axis() {
    auto result = Analyzer(testTablet(), 1920, 1080).calculate(rectangle(15, 1920, 1080, 20, 40));
    require(result.valid, "Tall movement regions are valid");
    near(result.rotation_deg, 15, 0.2, "avoid a spurious 90-degree flip");
    near(result.used_width_mm, 20, 0.3, "tall region width");
    near(result.used_height_mm, 40, 0.3, "tall region height");
}

void monitor_origin() {
    auto points = rectangle(-20);
    for (auto& point : points) { point.first -= 1920; point.second += 300; }
    AnalysisOptions options;
    options.screen_origin_x = -1920;
    options.screen_origin_y = 300;
    auto result = Analyzer(testTablet(), 1920, 1080, options).calculate(points);
    require(result.valid, "A monitor with a negative desktop origin must work");
    near(result.rotation_deg, -20, 0.2, "translated angle");
    near(result.center_x_mm, 100, 0.3, "tablet-space center x");
    near(result.center_y_mm, 50, 0.3, "tablet-space center y");
}

void recommendation_fits_tablet() {
    auto result = Analyzer(testTablet(), 1920, 1080).calculate(rectangle(30, 1920, 1080, 130, 35));
    require(result.valid && result.area_limited, "Oversized rotated recommendation must be reported and constrained");
    double c = std::abs(std::cos(result.rotation_deg*pi/180));
    double s = std::abs(std::sin(result.rotation_deg*pi/180));
    require(result.recommended_width_mm*c + result.recommended_height_mm*s <= 200.0001,
            "All rotated corners must fit the tablet width");
    require(result.recommended_width_mm*s + result.recommended_height_mm*c <= 100.0001,
            "All rotated corners must fit the tablet height");
}

void aspect_ratio_option() {
    AnalysisOptions options;
    options.preserve_aspect_ratio = true;
    auto result = Analyzer(testTablet(), 1920, 1080, options).calculate(rectangle());
    require(result.valid, "Aspect-locked recommendation is valid");
    near(result.recommended_width_mm, 70, 0.4, "aspect-locked width");
    near(result.recommended_height_mm, 39.375, 0.4, "aspect-locked height expands to fit movement");
}

void deterministic_recommendation() {
    auto points = rectangle(25);
    const auto first = Analyzer(testTablet(), 1920, 1080).calculate(points);
    std::reverse(points.begin(), points.end());
    const auto second = Analyzer(testTablet(), 1920, 1080).calculate(points);
    near(first.rotation_deg, second.rotation_deg, 1e-8, "order-independent angle");
    near(first.recommended_width_mm, second.recommended_width_mm, 1e-8, "order-independent dimensions");
}

void owned_tablet_names() {
    std::string brand(100, 'b'), model(100, 'm');
    Tablet tablet(brand, model, 200, 100);
    brand.assign(100, 'x'); model.assign(100, 'y');
    require(tablet.getBrand() == std::string(100, 'b') && tablet.getModel() == std::string(100, 'm'),
            "Tablet metadata must own its text");
}

void catalogue_integrity() {
    TabletFinder finder;
    for (const auto& tablet : finder.getTablets()) {
        require(std::isfinite(tablet.getWidth()) && tablet.getWidth() > 0 &&
                std::isfinite(tablet.getHeight()) && tablet.getHeight() > 0,
                "Catalogue dimensions must be finite and positive");
        auto found = finder.find(tablet.getBrand(), tablet.getModel());
        require(found && found->getWidth() == tablet.getWidth() && found->getHeight() == tablet.getHeight(),
                "Catalogue lookup must match its listed tablet");
    }
}
} // namespace

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"invalid_dimensions", invalid_dimensions}, {"invalid_options", invalid_options},
        {"insufficient_recordings", insufficient_recordings}, {"physical_rotation", physical_rotation},
        {"non_widescreen_scaling", non_widescreen_scaling}, {"paired_outliers", paired_outliers},
        {"stationary_dwell", stationary_dwell}, {"uncertain_rotation", uncertain_rotation},
        {"jittering_dwell", jittering_dwell}, {"ambiguous_axis_boundary", ambiguous_axis_boundary},
        {"slow_movement", slow_movement}, {"clipping_warning", clipping_warning},
        {"vertical_principal_axis", vertical_principal_axis}, {"monitor_origin", monitor_origin},
        {"recommendation_fits_tablet", recommendation_fits_tablet}, {"aspect_ratio_option", aspect_ratio_option},
        {"deterministic_recommendation", deterministic_recommendation}, {"owned_tablet_names", owned_tablet_names},
        {"catalogue_integrity", catalogue_integrity}
    };
    int failures = 0;
    for (const auto& [name, test] : tests) {
        if (argc > 1 && name != argv[1]) continue;
        try { test(); std::cout << "PASS " << name << '\n'; }
        catch (const std::exception& e) { ++failures; std::cerr << "FAIL " << name << ": " << e.what() << '\n'; }
    }
    return failures ? 1 : 0;
}
