#pragma once
#include <string_view>

class Tablet {
    private:
        std::string_view brand;
        std::string_view model;
        float width_mm;
        float height_mm;
    public:
        Tablet(std::string_view brand, std::string_view model, float width, float height);

        std::string_view getBrand() const;
        std::string_view getModel() const;
        float getWidth() const;
        float getHeight() const;
};