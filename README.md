# Tablet Area Analyzer

A tool for analyzing your tablet area usage in osu! by recording your play area during gameplay. This helps you optimize your tablet settings for comfort and performance.



> [!WARNING]  
> The area calculation is not accurate currently. The program is still in development and the area calculation will be improved in future releases.

## Features

- Cross-platform support (Windows, macOS, Linux)
- Records and analyzes your actual play area
- Simple, interactive command-line interface

## Building

1. Clone the repository:
    ```sh
    git clone https://github.com/mhdimo/AreaAnalyzer.git
    cd AreaAnalyzer
    ```

2. Generate build files with CMake:
    ```sh
    cd ./Build/
    cmake ..
    ```

3. Build the executable:
    ```sh
    cmake --build .
    ```

4. The resulting executable will be named `tablet_analyzer` (or `tablet_analyzer.exe` on Windows in the folder `build/debug`).

## Usage

1. Set the full area in your tablet driver (absolute mode, no forced proportion).
2. Disable all filters and set osu! sensitivity to 1x.
3. Run the program:
    ```sh
    ./tablet_analyzer

    OR

    ./tablet_analyzer.exe
    ```
4. Select your tablet and input the prompted monitor resolution.
5. Locate a long osu! map and input its duration.
6. Select the "Autopilot" mode.
7. Before starting, position your pen as you would during normal play, with the cursor centered.
8. Play as usual; the program will record your play area.
9. Review the results to adjust your tablet area settings.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
