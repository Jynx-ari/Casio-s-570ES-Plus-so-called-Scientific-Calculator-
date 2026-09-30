# Integrating the Calculator Core on ESP32-S3

This guide describes using the math engine without the SDL simulator, U8g2 display, or desktop keyboard. The ESP32 application owns the keypad scan, display drawing, settings persistence, and any AI/network features. The core only evaluates expressions and provides calculator algorithms.

## Core Files

Compile these files into the firmware target:

- `src/calc_engine.cpp`: expression parser, exact arithmetic, Ans, and A-F/X/Y/M registers.
- `src/calc_features.cpp`: BASE-N integer operations, complex math, statistics, equations, matrices, vectors, tables, constants, unit/coordinate/DMS conversion, and prime factors.

Include `src/calc.h` for the combined API, or include `calc_engine.h` and `calc_features.h` separately. Do not compile `calculator.cpp`, `natural_draw.cpp`, `display.cpp`, `lcd_hal.c`, or `main.cpp` into a firmware-only core build; those belong to the simulator/UI layer.

The project currently uses C++20 in CMake. The core uses standard library facilities available in C++17 plus the GCC `__int128` extension for bounded exact rational arithmetic. ESP-IDF's Xtensa GCC supports `__int128`; verify the exact compiler/runtime configuration used by the firmware project. The parser uses `std::string`, `std::vector`, exceptions, and dynamic allocation. Enable C++ exceptions and provide a working heap, or refactor the parser for fixed buffers before disabling exceptions or targeting very constrained memory conditions.

## Basic Evaluator Use

```cpp
#include "calc.h"

calc::Engine engine;
engine.setAngleMode(calc::AngleMode::Degrees);

calc::EvalResult result = engine.evaluate("sin(30)+1/3");
if (result.ok) {
    double numericAnswer = result.value;
    bool hasExactForm = result.exact;
    (void)numericAnswer;
    (void)hasExactForm;
} else {
    // result.error is "Syntax ERROR" or "Math ERROR".
}
```

A successful `Engine::evaluate()` updates Ans automatically. Failed evaluations leave the previous Ans unchanged. Variables are single capital letters; use the named register API for reads/writes:

```cpp
calc::EvalResult stored = engine.evaluate("7/3");
engine.store('A', stored);
const calc::MemoryValue *a = engine.recall('A');
if (a && a->defined) {
    calc::EvalResult useA = engine.evaluate("A*6");
}
```

Supported engine expression syntax includes arithmetic, implicit multiplication, powers, parentheses, trig/inverse/hyperbolic functions, logs, roots, floor/ceiling, absolute value, factorial, postfix percent, `pi`, `e`, `i`, `Ans`, stored capital-letter variables, complex arithmetic/functions (`abs`, `arg`, `conj`, `real`, `imag`), two-argument `nPr`, `nCr`, `mod`, `gcd`, and `lcm` calls, and infix P/C permutation/combination operators. Engine mode state is intentionally independent of the UI settings struct.

## M Memory Keys

`Engine` also exposes explicit M-register operations. They preserve an exact rational/radical representation when the inputs permit it, and they do not change Ans:

```cpp
calc::EvalResult two = engine.evaluate("2");
engine.addToMemory('M', two);            // M+
calc::EvalResult oneHalf = engine.evaluate("1/2");
engine.addToMemory('M', oneHalf);         // M = 2.5
engine.addToMemory('M', two, true);       // M-; M = 0.5
const calc::MemoryValue *memory = engine.recall('M');
engine.clearMemory('M');                  // MC
```

On the desktop simulator F6 is M+, Shift+F6 is M−, F8 is MR, and Shift+F8 is MC. For firmware using `app::Calculator`, send `app::Key::MPlus` or `app::Key::MR`; precede the key with `app::Key::Shift` for M− or MC. Alternatively, call `Engine::addToMemory('M', result, subtract)` and `Engine::clearMemory('M')` directly. MR in the simulator inserts the variable token `M` into the current expression; a custom firmware may recall and render the stored value directly.

To store a result in a variable using `app::Calculator`, send `app::Key::Shift`, then `app::Key::Ans`, then the variable number key (`N1` through `N9`). The SHIFT state applies to the next key only. This mirrors the calculator's yellow secondary-function layer; do not send a standalone STO key event.

## Keypad and Display Boundary

The desktop's `app::Calculator` is an example UI controller, not a dependency of the evaluator. Its `press(app::Key)` method accepts logical key events and its `draw(Display&, bool)` method renders to the simulator display. For a firmware port, use this separation:

1. Scan the ESP32-S3 keypad and debounce it in firmware code.
2. Convert physical keys to logical calculator commands.
3. Let the evaluator handle expressions or call the portable feature functions for specialized modes.
4. Render the resulting `EvalResult`/feature structs using the firmware display driver.
5. Persist settings and memories through NVS if restart retention is required; the core itself is RAM-only.

The Natural Display editor/renderer currently uses `std::string`, `std::vector`, and tree nodes, and depends on the simulator's `Display` wrapper. It is not part of `calc_core`. A firmware UI can keep it out and render expression/results itself, or port the display abstraction separately.

For a tiny text/AI response display, `src/compact_font.h` provides constexpr
5x7 A-Z/a-z glyph rows: 52 glyphs use 364 bytes total. Use `lcd_font::glyph(ch)`
to get the seven 5-bit row masks, `CellWidth`/`GlyphHeight` for layout, and
`textWidth(count)` for fixed-cell strings. Draw each set bit through the chosen
LCD driver. The calculator renderer deliberately draws permutation/combination
P/C operators with separate larger symbols, so those are not confused with
ordinary variable letters rendered from this table.

## Specialized Feature Examples

```cpp
calc::Matrix3 matrix{};
matrix.rows = matrix.cols = 2;
matrix.values[0][0] = 4; matrix.values[0][1] = 7;
matrix.values[1][0] = 2; matrix.values[1][1] = 6;
calc::Matrix3 inverse{};
bool invertible = calc::matrixInverse(matrix, inverse);

calc::CubicResult roots = calc::solveCubic(1, -6, 11, -6);

calc::PrimeFactorization factors = calc::primeFactorize(360);

calc::Polar polar = calc::rectangularToPolar(3, 4, calc::AngleUnit::Degrees);

double feet = 0;
bool converted = calc::convertUnit(1, calc::Unit::Mile, calc::Unit::Foot, feet);
```

Check each function's boolean/`ok` result before using outputs. Matrix/vector/table work buffers are fixed-size. Prime factorization currently accepts positive integers through $10^{12}$. `Statistics` uses fixed accumulators and supports frequency weights, means, variance/deviation, and linear regression; it does not retain every sample, so median/order statistics are not available from that class.

## Memory, Persistence, and AI

`calc_core` has no Wi-Fi, HTTP, filesystem, display, microphone, or AI dependencies. Keep AI requests outside evaluator calls: evaluate deterministic local math first, then pass a bounded textual request/result to an application service. Never let remote output bypass the parser's validation or write memory without an explicit policy. For a low-memory firmware build, monitor heap fragmentation from repeated `std::string`/`std::vector` expressions and consider a fixed-capacity tokenizer/AST before long-running production use.

Memory registers and settings reset when `Engine`/`Calculator` is recreated. Save finite numeric register values and settings to ESP-IDF NVS as application data; reconstruct stored values using `EvalResult` and `Engine::store()` during boot. The exact symbolic form is not serialized by the current API, so values restored from NVS should be marked inexact unless a future serialization format is added.

## Current Core Limits

- Exact symbolic results are intentionally bounded and support rationals plus a limited sum of square-root/π terms.
- Base integer operations are checked signed 64-bit, not the fx-570ES PLUS fixed-width signed 32-bit word behavior.
- Matrix storage is fixed at 3x3; vector helpers are 3D; table results hold at most 21 rows.
- Equation helpers cover linear, quadratic, and real-coefficient cubic polynomials, not systems or higher orders.
- Statistics does not retain raw observations and implements only basic summaries/regression.
- Unit conversions/constants are a selected SI-oriented set, not the calculator's complete catalog.
- The interactive simulator does not expose every core API as a MODE workflow.

## Build and Test on Desktop

Use the repo CMake build to validate the portable core before adding it to firmware:

```sh
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

The standalone CMake target is `calc_core`; link it without `casio_ui_sim`, SDL2, or U8g2 in a native firmware-oriented test target. For ESP-IDF, add the two core `.cpp` files and `src` include directory to the component's CMake configuration, and set the C++ standard/exception policy explicitly.
