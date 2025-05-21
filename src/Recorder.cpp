#include "Recorder.hpp"
#include <iostream>
#include <chrono>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif

Recorder::Recorder(int duration) : duration_sec(duration) {}

std::pair<int, int> Recorder::getCursorPosition() const {
#ifdef _WIN32
    POINT p;
    if (GetCursorPos(&p)) {
        return {p.x, p.y};
    }
    return {0, 0};

#elif __linux__
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) return {0, 0};

    Window root = DefaultRootWindow(dpy);
    Window ret_root, ret_child;
    int x, y;
    int win_x, win_y;
    unsigned int mask;

    XQueryPointer(dpy, root, &ret_root, &ret_child, &x, &y, &win_x, &win_y, &mask);
    XCloseDisplay(dpy);
    return {x, y};

#elif __APPLE__
    CGEventRef event = CGEventCreate(nullptr);
    CGPoint point = CGEventGetLocation(event);
    CFRelease(event);
    return {static_cast<int>(point.x), static_cast<int>(point.y)};

#else
    return {0, 0}; // Unsupported platform
#endif
}

std::vector<std::pair<int, int>> Recorder::record() const {
    std::vector<std::pair<int, int>> points;
    using namespace std::chrono_literals;

    std::cout << "Recording cursor for " << duration_sec << " seconds...\n";
    auto start = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(duration_sec)) {
        points.push_back(getCursorPosition());
        std::this_thread::sleep_for(10ms);
    }

    return points;
}