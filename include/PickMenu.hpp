#pragma once
#include "Input.hpp"
#include <limits>
#include <string>
#include <vector>

// Numbered menus work in terminals, IDE consoles and redirected input without
// changing terminal modes or invoking a shell to clear the screen.
inline int pick_menu(const std::vector<std::string>& options, const std::string& title,
                     std::istream& input = std::cin, std::ostream& output = std::cout) {
    if (options.empty() || options.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("Menu must contain a supported number of options.");
    }
    output << '\n' << title << '\n';
    for (std::size_t i = 0; i < options.size(); ++i) output << "  " << i + 1 << ". " << options[i] << '\n';
    return read_number<int>("Selection", 1, static_cast<int>(options.size()), std::nullopt, input, output) - 1;
}
