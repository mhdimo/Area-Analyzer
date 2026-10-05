#include "TabletFinder.hpp"
#include "Recorder.hpp"
#include "Analyzer.hpp"
#include "PickMenu.hpp"
#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace {
Tablet select_tablet() {
    TabletFinder finder;
    std::set<std::string> brand_set;
    for (const auto& tablet : finder.getTablets()) brand_set.emplace(tablet.getBrand());
    std::vector<std::string> brands(brand_set.begin(), brand_set.end());
    brands.emplace_back("Custom / tablet not listed");
    const auto brand_index = static_cast<std::size_t>(pick_menu(brands, "Select your tablet brand:"));
    if (brand_index == brands.size() - 1) {
        const double width = read_number<double>("Active tablet width (mm)", 1, 2000);
        const double height = read_number<double>("Active tablet height (mm)", 1, 2000);
        return Tablet("Custom", "Manual dimensions", width, height);
    }

    const auto& brand = brands[brand_index];
    std::vector<std::string> models;
    for (const auto& tablet : finder.getTablets()) {
        if (tablet.getBrand() == brand) models.emplace_back(tablet.getModel());
    }
    std::sort(models.begin(), models.end());
    const auto model_index = static_cast<std::size_t>(pick_menu(models, "Select your tablet model:"));
    const auto tablet = finder.find(brand, models[model_index]);
    if (!tablet) throw std::runtime_error("Selected tablet was not found.");
    return *tablet;
}
} // namespace

int main() {
    try {
        std::cout << "Tablet Area Analyzer\n"
                  << "Before recording: use the full tablet area, 0-degree driver rotation, absolute mode,\n"
                  << "no forced proportions or filters, and osu! sensitivity 1x. Use fullscreen osu!\n"
                  << "with default playfield scaling on the single monitor mapped by the driver.\n";
        const Tablet tablet = select_tablet();
        std::cout << "\nSelected: " << tablet.getBrand() << ' ' << tablet.getModel() << " ("
                  << tablet.getWidth() << " x " << tablet.getHeight() << " mm active area)\n"
                  << "Use OS cursor coordinates for the monitor size and desktop origin.\n"
                  << "On macOS this is the logical display size, which may differ from Retina pixels.\n";
        const int width = read_number<int>("Mapped monitor width", 1, 100000);
        const int height = read_number<int>("Mapped monitor height", 1, 100000);
        AnalysisOptions options;
        options.screen_origin_x = read_number<int>("Monitor left on desktop (negative is allowed)", -100000, 100000, 0);
        options.screen_origin_y = read_number<int>("Monitor top on desktop (negative is allowed)", -100000, 100000, 0);
        options.preserve_aspect_ratio = read_number<int>("Preserve screen aspect ratio in recommendation? 1=yes, 0=no", 0, 1, 0) == 1;
        options.safety_margin = read_number<double>("Extra area margin (%)", 0, 100, 5) / 100;
        const int duration = read_number<int>("Recording duration (seconds)", 1, 3600);
        const int delay = read_number<int>("Delay before recording (seconds)", 0, 60, 5);
        const Analyzer analyzer(tablet, width, height, options);
        const Recorder recorder(duration, delay);
        std::cout << "Switch to osu!, enable Autopilot and move your pen as you would during normal play.\n";
        return analyzer.analyze(recorder.record()) ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << '\n';
        return 1;
    }
}
