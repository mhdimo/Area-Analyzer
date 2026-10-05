#include "Recorder.hpp"
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif __linux__
#include <X11/Xlib.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace {
// A single capture session owns platform resources for the entire recording.
class CursorReader {
public:
    CursorReader() {
#ifdef __linux__
        const char* session = std::getenv("XDG_SESSION_TYPE");
        const char* wayland = std::getenv("WAYLAND_DISPLAY");
        if ((session && std::string(session) == "wayland") || (wayland && *wayland)) {
            throw std::runtime_error("Global cursor recording requires an X11 session; Wayland/XWayland is not supported.");
        }
        display = XOpenDisplay(nullptr);
        if (!display) throw std::runtime_error("Cannot open the X11 display. Run from a graphical X11 session.");
#endif
    }

    ~CursorReader() {
#ifdef __linux__
        if (display) XCloseDisplay(display);
#endif
    }
    CursorReader(const CursorReader&) = delete;
    CursorReader& operator=(const CursorReader&) = delete;

    std::pair<int, int> position() const {
#ifdef _WIN32
        POINT p{};
        // Physical coordinates are independent of thread/process DPI awareness,
        // including on desktops with monitors using different scaling factors.
        if (!GetPhysicalCursorPos(&p)) throw std::runtime_error("Cannot read the Windows cursor position.");
        return {p.x, p.y};
#elif __linux__
        Window root_return{}, child_return{};
        int root_x = 0, root_y = 0, window_x = 0, window_y = 0;
        unsigned int mask = 0;
        if (!XQueryPointer(display, DefaultRootWindow(display), &root_return, &child_return,
                           &root_x, &root_y, &window_x, &window_y, &mask)) {
            throw std::runtime_error("Cannot read the cursor on this X11 screen.");
        }
        return {root_x, root_y};
#elif __APPLE__
        CGEventRef event = CGEventCreate(nullptr);
        if (!event) throw std::runtime_error("Cannot read the macOS cursor position. Check system privacy permissions.");
        const CGPoint p = CGEventGetLocation(event);
        CFRelease(event);
        if (!std::isfinite(p.x) || !std::isfinite(p.y) ||
            p.x < std::numeric_limits<int>::min() || p.x > std::numeric_limits<int>::max() ||
            p.y < std::numeric_limits<int>::min() || p.y > std::numeric_limits<int>::max()) {
            throw std::runtime_error("macOS returned an invalid cursor position.");
        }
        return {static_cast<int>(std::lround(p.x)), static_cast<int>(std::lround(p.y))};
#else
        throw std::runtime_error("Cursor recording is not supported on this platform.");
#endif
    }

private:
#ifdef __linux__
    Display* display = nullptr;
#endif
};
} // namespace

Recorder::Recorder(int duration, int start_delay)
    : duration_sec(duration), start_delay_sec(start_delay) {
    if (duration < 1 || duration > 3600) throw std::invalid_argument("Recording duration must be between 1 and 3600 seconds.");
    if (start_delay < 0 || start_delay > 60) throw std::invalid_argument("Recording delay must be between 0 and 60 seconds.");
}

std::vector<std::pair<int, int>> Recorder::record() const {
    using Clock = std::chrono::steady_clock;
    using namespace std::chrono_literals;
    CursorReader cursor;
    cursor.position(); // Detect unavailable capture before waiting for gameplay.
    for (int remaining = start_delay_sec; remaining > 0; --remaining) {
        std::cout << "Recording starts in " << remaining << "...\n" << std::flush;
        std::this_thread::sleep_for(1s);
    }

    std::vector<std::pair<int, int>> points;
    points.reserve(static_cast<std::size_t>(duration_sec) * 100);
    std::cout << "Recording cursor for " << duration_sec << " seconds...\n" << std::flush;
    auto next_sample = Clock::now();
    const auto end = next_sample + std::chrono::seconds(duration_sec);
    while (Clock::now() < end) {
        points.push_back(cursor.position());
        next_sample += 10ms;
        // Do not manufacture a burst of samples to catch up after a stall.
        if (next_sample < Clock::now()) next_sample = Clock::now() + 10ms;
        std::this_thread::sleep_until(next_sample < end ? next_sample : end);
    }
    std::cout << "Recording complete: " << points.size() << " samples.\n";
    return points;
}
