# Tablet Area Analyzer

A C++17 command-line tool that records your OS cursor movement during osu! gameplay and estimates a comfortable tablet area and rotation.

The output is a **starting point**, not an optimal setting or a calibration. Beatmap patterns and how you move affect the estimate. Compare several representative recordings before changing your driver settings.

## Features

- Windows, macOS, and Linux/X11 cursor recording, with a configurable start delay.
- Built-in tablet catalogue and manual active-area dimensions for other tablets.
- Rotation estimated in physical tablet coordinates, with paired outlier removal and stationary-pause handling.
- Separate measured movement and suggested driver-area dimensions, with a configurable margin and optional screen aspect ratio.
- Monitor offsets, including negative desktop coordinates, and support for different monitor aspect ratios.
- Clear errors for invalid input, unavailable capture, and recordings without enough two-dimensional movement.

## Build and test

Requires CMake 3.20 or newer and a C++17 compiler. Linux additionally needs X11 development files (`libx11-dev` on Debian/Ubuntu); macOS needs Xcode Command Line Tools; Windows can use Visual Studio's C++ tools.

```sh
git clone https://github.com/mhdimo/Area-Analyzer.git
cd Area-Analyzer
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

On macOS/Linux, run `./build/tablet_analyzer`. With Visual Studio's generator, run `build\Release\tablet_analyzer.exe`.

For memory and undefined-behavior checks with GCC/Clang:

```sh
cmake -S . -B build-sanitized -DCMAKE_BUILD_TYPE=Debug -DTABLET_ANALYZER_ENABLE_SANITIZERS=ON
cmake --build build-sanitized --parallel
ctest --test-dir build-sanitized --output-on-failure
```

Tests use synthetic recordings and require no tablet or graphical session. The GitHub Actions workflow builds and tests Windows, macOS, and Linux; hardware capture still needs checking on the target system.

## Recording setup

1. Set your driver to **full active tablet area**, **0-degree rotation**, **absolute mode**, and **no forced proportions**. Disable filters. Use the tablet's active dimensions, not its outer case dimensions.
2. Map the tablet to **one monitor**. Run osu! fullscreen on that monitor, at its desktop size, with sensitivity **1x** and default playfield scaling. Windowed play, letterboxing, custom lazer scaling, and mapping to the entire multi-monitor desktop are not supported by the conversion.
3. Run the analyzer. Enter menu numbers to choose a tablet, or choose the custom option and enter its active dimensions.
4. Enter the mapped monitor's width, height, and desktop origin in **OS cursor coordinate units**. For a primary monitor the origin is usually `(0, 0)`; a monitor to its left can have a negative X origin. On macOS use the logical display size, which can differ from Retina hardware pixels. On Windows use the display's pixel dimensions.
5. Choose whether the **recommended** area should match the screen aspect ratio, and set a margin (default: 5% added to each dimension). These options do not change the required recording setup.
6. Enter the recording duration (1–3600 seconds) and start delay (0–60 seconds, default: 5). Choose enough delay to switch back to the game and start a representative section.
7. Enable **Autopilot**, start with your pen comfortably centered, and move it as you would during normal play. Avoid including menus, long breaks, or unrelated cursor movement.
8. Read the results and any notes. If rotation is undetermined, use the unrotated dimensions as a starting point and collect more representative movement.

Blank input accepts a displayed default. Invalid numbers are re-prompted; EOF exits cleanly. Ctrl+C stops the program using the terminal's normal behavior.

Linux recording requires an X11 session. Wayland/XWayland cannot provide reliable global cursor recording here and is rejected. A failed cursor query aborts the recording rather than inserting fake `(0, 0)` samples.

## Reading the results

**Measured movement** is the physical width and height of the filtered movement, aligned with the estimated rotation. **Suggested driver area** expands those dimensions to account for the fraction of the screen occupied by the osu! playfield, then adds the requested margin. The two numbers describe different things.

**Rotation** is positive clockwise with X to the right and Y down. The estimator chooses the principal axis nearest horizontal, in `(-45, 45]` degrees, so a tall movement pattern does not automatically suggest a 90-degree turn. This assumes an upright tablet and modest area rotation. Movement alone cannot resolve larger orientation changes, nor distinguish a tilted hand position from a beatmap with mostly diagonal patterns. Check the direction in your driver's preview.

**Measured movement center** is in millimetres from the tablet's top-left corner. It is not a calibrated driver offset: playfield placement and the distribution of hit objects affect it. Position the suggested area manually so its rotated corners fit inside the tablet. If the suggested dimensions are physically too large, the analyzer reduces them uniformly and reports the loss of movement coverage.

## How the estimate works

1. Remove identical samples and movements smaller than three cursor coordinate units from the last retained position, so a stationary or slightly jittering pause does not dominate the statistics. Slow movement accumulates until it crosses that threshold. Exclude samples outside the configured monitor.
2. Convert complete `(x, y)` pairs to millimetres using the full tablet-to-monitor mapping. This avoids changing the angle when horizontal and vertical pixel scales differ.
3. Discard an entire sample when either axis falls beyond a 1.5-IQR outlier fence. Never filter X and Y independently.
4. Use double-precision covariance/PCA to estimate orientation. Nearly straight-line recordings are rejected; near-isotropic movement gets an undetermined angle and unrotated dimensions.
5. Rotate the retained points into the estimated area axes and measure the 1st–99th percentile span on each axis. This replaces the previous nondeterministic frequency peaks near extrema.
6. Convert movement spans to full-screen driver dimensions using:

   ```text
   scale = min(screen_width / 640, screen_height / 480)
   playfield_width  = 512 * scale
   playfield_height = 384 * scale
   driver_width  = movement_width  / (playfield_width  / screen_width)
   driver_height = movement_height / (playfield_height / screen_height)
   ```

7. Add the selected margin. If aspect lock is requested, expand the smaller dimension to match the screen aspect. Uniformly reduce an oversized rectangle until its rotated bounding box fits the tablet.

The playfield geometry follows the [osu! playfield documentation](https://osu.ppy.sh/wiki/en/Client/Playfield) and [osu!'s playfield adjustment code](https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/UI/OsuPlayfieldAdjustmentContainer.cs). For example, a 1920×1080 screen has a 1152×864 default playfield, while 1600×1200 has a 1280×960 playfield. A fixed 60%-of-screen-width calculation is incorrect for other aspect ratios.

This tool measures the OS cursor, not raw tablet reports. It cannot reconstruct pen movement clipped at a screen edge, validate the driver's existing mapping, or infer hit-object coverage from cursor samples alone. Recordings with little movement, bad setup, or heavy clipping should be repeated. Rotation direction strength is a covariance statistic, not a statistical confidence level.

## License

MIT. See [LICENSE](LICENSE).
