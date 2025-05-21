#pragma once
#include "GraphicTablet.hpp"
#include <vector>
#include <utility>

class Analyzer {
public:
    Analyzer(const Tablet& tablet, int screen_width, int screen_height);
    void analyze(const std::vector<std::pair<int, int>>& data) const;

private:
    const Tablet& tablet;
    int screen_width;
    int screen_height;
};