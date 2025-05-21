#include "TabletFinder.hpp"
#include "Recorder.hpp"
#include "Analyzer.hpp"
#include "PickMenu.hpp"
#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <algorithm>

int main() {
    TabletFinder finder;

    // 1. Gather unique brands
    std::set<std::string> brand_set;
    for (const auto& t : finder.getTablets()) {
        brand_set.insert(std::string(t.getBrand()));
    }
    std::vector<std::string> brands(brand_set.begin(), brand_set.end());
    std::sort(brands.begin(), brands.end());

    // 2. Pick brand
    int brand_idx = pick_menu(brands, "Select your tablet brand:");
    std::string selected_brand = brands[brand_idx];

    // 3. Gather models for that brand
    std::vector<std::string> models;
    for (const auto& t : finder.getTablets()) {
        if (t.getBrand() == selected_brand) {
            models.push_back(std::string(t.getModel()));
        }
    }
    std::sort(models.begin(), models.end());

    // 4. Pick model
    int model_idx = pick_menu(models, "Select your tablet model:");
    std::string selected_model = models[model_idx];

    // 5. Find the tablet
    auto tablet_opt = finder.find(selected_brand, selected_model);
    if (!tablet_opt) {
        std::cerr << "Tablet not found.\n";
        return 1;
    }

    int screen_w, screen_h;
    std::cout << "Screen width (px): ";
    std::cin >> screen_w;
    std::cout << "Screen height (px): ";
    std::cin >> screen_h;

    int duration;
    std::cout << "Duration (seconds): ";
    std::cin >> duration;

    Recorder recorder(duration);
    auto points = recorder.record();

    Analyzer analyzer(*tablet_opt, screen_w, screen_h);
    analyzer.analyze(points);

    return 0;
}