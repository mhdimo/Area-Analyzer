#pragma once
#include <string>
#include <string_view>

class Tablet {
    private:
        std::string brand;
        std::string model;
        double width_mm;
        double height_mm;
    public:
        Tablet(std::string_view brand, std::string_view model, double width, double height);

        std::string_view getBrand() const;
        std::string_view getModel() const;
        double getWidth() const;
        double getHeight() const;
};
