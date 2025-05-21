#pragma once
#include "GraphicTablet.hpp"
#include <optional>
#include <string_view>
#include <vector>

class TabletFinder {
public:
    TabletFinder();
    std::optional<Tablet> find(std::string_view brand, std::string_view model) const;
    const std::vector<Tablet>& getTablets() const { return tablets; }

private:
    std::vector<Tablet> tablets;
    void loadTablets();
    
};