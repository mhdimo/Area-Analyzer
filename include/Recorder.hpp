#pragma once
#include <vector>
#include <utility>

class Recorder {
public:
    explicit Recorder(int duration, int start_delay = 5);
    std::vector<std::pair<int, int>> record() const;

private:
    int duration_sec;
    int start_delay_sec;
};
