#include "GraphicTablet.hpp"
#include <cmath>
#include <stdexcept>

Tablet::Tablet(std::string_view b, std::string_view m, double w, double h)
    : brand(b), model(m), width_mm(w), height_mm(h) {
    if (!std::isfinite(w) || !std::isfinite(h) || w <= 0 || h <= 0) {
        throw std::invalid_argument("Tablet dimensions must be finite and positive.");
    }
}

std::string_view Tablet::getBrand() const { return brand; }
std::string_view Tablet::getModel() const { return model; }
double Tablet::getWidth() const { return width_mm; }
double Tablet::getHeight() const { return height_mm; }

