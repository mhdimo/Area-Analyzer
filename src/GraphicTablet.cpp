#include "GraphicTablet.hpp"

Tablet::Tablet(std::string_view b, std::string_view m, float w, float h)
    : brand(b), model(m), width_mm(w), height_mm(h) {}

std::string_view Tablet::getBrand() const { return brand; }
std::string_view Tablet::getModel() const { return model; }
float Tablet::getWidth() const { return width_mm; }
float Tablet::getHeight() const { return height_mm; }


