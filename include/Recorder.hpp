#pragma once
#include <vector>
#include <utility>

class Recorder {
public:
    Recorder(int duration);
    std::vector<std::pair<int, int>> record() const;

private:
    int duration_sec;
    std::pair<int, int> getCursorPosition() const;
};