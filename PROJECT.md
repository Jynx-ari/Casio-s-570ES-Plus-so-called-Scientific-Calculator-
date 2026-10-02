# ESP32-S3 Graphing Calculator Project

## Product Direction

Build a graphing calculator in a familiar physical calculator form, powered by the existing ESP32-S3 N16R8 development board. Graphing is a primary offline workflow; scientific calculations remain available as a supporting mode. The same physical keypad must serve calculator and concealed text modes, with a discreet CALC/TEXT selector rather than a separate keyboard. Wi-Fi and cloud AI are optional services, not dependencies of local calculation.

The desktop simulator is the working UI prototype, not a throwaway mockup. The target is a 2-inch, 240x320 ST7789-class color TFT in landscape orientation, giving the application a 320x240 logical coordinate system. The color display and graphing-calculator software direction are intentional. Keep the familiar calculator body/keypad proportions, but do not constrain the UI to a monochrome 128x64 display. Physical parts and enclosure dimensions remain provisional until the exact display, development board, switches, and battery are measured.

## Current Simulator

- C++20 application with an SDL2 desktop window and scripted/headless rendering.
- Logical framebuffer is 320x240; the inherited scientific-calculator expression/result, status, menu, and mode-work layouts have been reflowed for it.
- Existing natural-expression editor and portable calculation engine remain in place.
- The virtual display currently uses U8g2's 1-bit drawing model. It verifies layout and interaction, but does not simulate TFT color, the ST7789 SPI driver, or hardware refresh behavior.
- A first graph mode is implemented: MODE 9 accepts one expression, samples it through `calc_core`, and renders a grid/axes, curve, and trace readout. Multiple functions, editable ranges, and table view are not implemented yet.
- Text mode, Wi-Fi provisioning, cloud AI screens, physical keypad scanning, and ESP32 firmware integration are not implemented yet.

## Architecture Boundary

Keep calculation and interaction independent of SDL and hardware:

```text
Calculator engine -> application/UI state -> Display abstraction
                                           /                   \
                                  SDL simulator       ESP32 TFT backend
```

`calc_core` owns expression evaluation and reusable math features. Graph sampling must reuse this evaluator, binding the independent variable `x` for each sample rather than creating a second expression language or evaluator. The application maps logical key events into calculator and graph state. The display interface owns dimensions, text metrics, pixels/primitives, and presentation; SDL and the eventual ST7789 backend implement that interface. Key scanning, mode-switch GPIO, networking, storage, and AI clients must not leak into the math engine.

The current `Display` wrapper is U8g2-backed. Before firmware UI integration, retain the drawing API while allowing the TFT backend to use an ST7789-compatible library and the color capabilities of the panel. Keep UI geometry behind `Display::width()`/`height()` and avoid adding display dimensions to calculator evaluation code.

## Graphing Workflow

The graph screen is a first-class calculator mode, not a replacement for scientific calculation. Its first useful version should support:

- A small editable function list, initially starting with `f(x)`.
- Expression parsing/evaluation through the existing `calc_core` with the current angle mode and an `x` value for each sample.
- A plot viewport with axes, configurable ranges, and grid lines.
- Function traces with invalid/discontinuous samples handled without joining across gaps.
- Key-driven trace/cursor readout showing the current `x` and `y` values.
- Zoom and pan, followed by multiple functions and a sampled-value/table view.

The 320x240 layout should reserve clear space for the graph and compact function/coordinate information. Physical keys remain the primary controls; do not depend on touch input.

## Implementation Order

1. **Graph interaction:** add editable axis ranges, robust discontinuity handling, multiple functions, and sampled-value/table support to the initial MODE 9 graph screen.
2. **Scientific calculator integration:** preserve COMP-style expression editing/evaluation and link naturally between calculation and graph workflows.
3. **Graph-first simulator polish:** refine function editing, trace movement, zoom/pan controls, graph labels, and 320x240 composition.
4. **Math layout and simulator polish:** refine structured powers, fractions, roots, symbols, status, menus, cursor, and errors for 320x240.
5. **Text mode:** model the discreet CALC/TEXT selector and same-keypad text entry, cursor/editing, and scrolling.
6. **Network UI and services:** simulate Wi-Fi provisioning and AI request/response/error states first. Keep credentials and API configuration out of the renderer and calculator engine.
7. **ESP32 integration:** add the measured board, TFT, keypad, mode switch, settings storage, Wi-Fi, and power paths behind hardware-specific adapters.
8. **Mechanical integration:** measure the actual selected parts before final CAD; preserve battery clearance and ESP32 antenna clearance.

Do not freeze enclosure CAD or commit to a battery/switch stack using generic product dimensions. Keep offline graphing and scientific calculation usable when Wi-Fi or AI is unavailable. The color TFT is the target; the current monochrome simulator raster is only a temporary backend limitation, not a product requirement.

## Build and Verify

Arch Linux prerequisites:

```sh
sudo pacman -S --needed gcc cmake pkgconf sdl2
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/casio_ui_sim
```

The simulator can render scripted input without opening a window. For example, enter `sin(X)` in MODE 9 and save the graph frame:

```sh
./build/casio_ui_sim --keys "mode 9 sin X ) = @graph.bmp"
```

For development, validate calculator behavior with CTest and verify the rendered logical frame remains 320x240. See [README.md](README.md) for current controls and [ESP32_CORE_GUIDE.md](ESP32_CORE_GUIDE.md) for the portable engine boundary.
