#include "Recorder.hpp"
#include <iostream>
#include <stdexcept>

int main() {
    for (int duration : {-1, 0, 3601}) {
        try {
            Recorder recorder(duration);
            std::cerr << "FAIL accepted recording duration: " << duration << '\n';
            return 1;
        } catch (const std::invalid_argument&) {}
    }
    for (int delay : {-1, 61}) {
        try {
            Recorder recorder(1, delay);
            std::cerr << "FAIL accepted recording delay: " << delay << '\n';
            return 1;
        } catch (const std::invalid_argument&) {}
    }
    std::cout << "PASS recording duration validation\n";
}
