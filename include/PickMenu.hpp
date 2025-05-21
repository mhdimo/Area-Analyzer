#pragma once
#include <vector>
#include <string>
#include <iostream>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

// Reads a single character from stdin without waiting for Enter
inline int getch() {
#ifdef _WIN32
    return _getch();
#else
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
#endif
}

// Returns the index of the selected item
inline int pick_menu(const std::vector<std::string>& options, const std::string& title) {
    int selected = 0;
    while (true) {
        // Clear screen (simple way)
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
        std::cout << title << "\n";
        for (size_t i = 0; i < options.size(); ++i) {
            if ((int)i == selected)
                std::cout << " > " << options[i] << "\n";
            else
                std::cout << "   " << options[i] << "\n";
        }
        std::cout << "\nUse Up/Down arrows and Enter to select.\n";

        int ch = getch();
#ifdef _WIN32
        if (ch == 224 || ch == 0) { // Arrow or function key
            ch = getch();
            if (ch == 72 && selected > 0) selected--;         // Up
            else if (ch == 80 && selected < (int)options.size() - 1) selected++; // Down
        } else if (ch == 13) { // Enter
            return selected;
        }
#else
        if (ch == 27) { // Escape sequence
            if (getch() == 91) {
                ch = getch();
                if (ch == 65 && selected > 0) selected--;         // Up
                else if (ch == 66 && selected < (int)options.size() - 1) selected++; // Down
            }
        } else if (ch == 10) { // Enter
            return selected;
        }
#endif
    }
}