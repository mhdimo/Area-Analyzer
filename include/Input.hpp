#pragma once
#include <cmath>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

template <typename T>
T read_number(const std::string& prompt, T minimum, T maximum,
              std::optional<T> default_value = std::nullopt,
              std::istream& input = std::cin, std::ostream& output = std::cout) {
    for (;;) {
        output << prompt;
        if (default_value) output << " [" << *default_value << ']';
        output << ": " << std::flush;
        std::string line;
        if (!std::getline(input, line)) throw std::runtime_error("Input ended; recording cancelled.");
        std::istringstream parser(line);
        parser >> std::ws;
        if (parser.eof() && default_value) return *default_value;
        T value{};
        if (parser >> value && std::isfinite(static_cast<double>(value)) && value >= minimum && value <= maximum) {
            parser >> std::ws;
            if (parser.eof()) return value;
        }
        output << "Enter a number from " << minimum << " to " << maximum << ".\n";
    }
}
