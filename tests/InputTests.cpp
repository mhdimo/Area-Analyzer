#include "Input.hpp"
#include "PickMenu.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main() {
    try {
        std::ostringstream output;
        std::istringstream numbers("oops\n0\n-3\n12garbage\n999999999999999999999\n42\n");
        require(read_number<int>("Width: ", 1, 100, std::nullopt, numbers, output) == 42,
                "Invalid input must be rejected as a complete line before accepting a valid value");
        std::istringstream defaults("   \n");
        require(read_number<int>("Delay: ", 0, 60, 5, defaults, output) == 5, "Blank line accepts the stated default");
        std::istringstream floats("nan\ninf\n-5\n12.5\n");
        require(read_number<double>("Size: ", 1.0, 500.0, std::nullopt, floats, output) == 12.5,
                "Tablet dimensions must reject non-finite or negative numbers");
        std::istringstream origin("-1920\n");
        require(read_number<int>("Origin: ", -10000, 10000, 0, origin, output) == -1920,
                "Signed monitor offsets are valid");
        std::istringstream menu("bad\n0\n3\n2\n");
        require(pick_menu({"first", "second"}, "Choose", menu, output) == 1, "Numbered menu validates its bounds");
        bool cancelled = false;
        std::istringstream eof;
        try { pick_menu({"first"}, "Choose", eof, output); }
        catch (const std::runtime_error&) { cancelled = true; }
        require(cancelled, "EOF must exit instead of looping forever");
        bool empty_rejected = false;
        try { pick_menu({}, "Empty", eof, output); }
        catch (const std::invalid_argument&) { empty_rejected = true; }
        require(empty_rejected, "Empty menus must fail safely");
        std::cout << "PASS input validation, menu bounds, defaults and EOF\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL input: " << e.what() << '\n';
        return 1;
    }
}
