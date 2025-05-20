#include "GraphicTablet.hpp"

GraphicTablet GraphicTablet::from_json(const nlohmann::json& j, const std::string& model_name) {
    for (const auto& item : j["tablets"]) {
        if (item["model"] == model_name) {
            return GraphicTablet(
                item["model"],
                item["width_mm"],
                item["height_mm"]
            );
        }
    }
    throw std::runtime_error("Tablet model not found in JSON.");
}