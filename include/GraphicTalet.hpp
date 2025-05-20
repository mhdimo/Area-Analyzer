#pragma once
#include <string>
#include <nlohmann/json.hpp>

class GraphicTablet {
public:
    std::string Tablet_model;
    double width_mm;
    double height_mm;

    GraphicTablet() = default;
    GraphicTablet(const std::string& model, double width, double height)
        : model(model), width_mm(width), height_mm(height) {}

    static GraphicTablet from_json(const nlohmann::json& j, const std::string& model_name);
};