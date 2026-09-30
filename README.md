# casio_ui_sim

fx-570ES PLUS-style **LCD simulator** for the ESP32-S3 calculator project.
No calculator body/keypad graphics here on purpose (see chat) -- just the
128x64 glass, driven by real U8g2, reproducing the display features the
570ES PLUS actually has: stacked fractions (incl. mixed a b/c), radicals
with a proper index and overbar, nested exponents, |abs| bars, log with a
subscript base, the pi glyph, x10^n scientific notation, S<=>D (exact <->
decimal) toggle, MODE/SETUP menus, calculation history (up/down), and the
Math ERROR screen with [AC]/[<|][|>] recovery.

## Architecture

    calc_engine.*   exact-value evaluator and stateful answer/register memory
    calc_features.* portable complex, statistics, BASE-N, linear/quadratic/cubic
                    equations, matrix, vector, table, conversion and constant APIs
    calc_core       standalone library target without SDL/U8g2 dependencies
    natural.*       Natural-Display expression TREE + editor (cursor, del,
                    fractions/roots/powers as real nested nodes, not strings)
    natural_draw.*  lays out & draws that tree with U8g2 primitives only
    compact_font.h  standalone 364-byte 5x7 uppercase/lowercase alphabet table
    calculator.*    fx-570ES PLUS behaviour: keys, screens, indicators,
                    MODE/SETUP workflows, history, S<=>D, Fix/Sci/Norm
    display.*       thin wrapper over real U8g2 (fonts, lines, boxes, clip)
    lcd_hal.*       simulated "controller driver" (stands in for
                    u8x8_d_st7565_*; not an SPI-protocol emulation)
    main.cpp        SDL2 window that paints the 128x64 glass, or a
                    scripted/headless mode for testing (see below)

Firmware swaps one line in `Display::Display()`:
`u8g2_Setup_calcsim_128x64_f(...)` -> `u8g2_Setup_st7565_ea_dogm128_f(...)`.
Everything else -- fonts, natural-display layout, calculator logic -- is
unchanged.

## Build

    sudo pacman -S --needed gcc cmake pkgconf sdl2
    cmake -S . -B build      # downloads U8g2 2.36.19 on first configure
    cmake --build build      # first build is slow (U8g2's font table is huge C)
    ./build/casio_ui_sim

Offline: `cmake -S . -B build -DFETCHCONTENT_SOURCE_DIR_U8G2=/path/to/u8g2`.

Firmware can include `src/calc.h` and compile `src/calc_engine.cpp` and
`src/calc_features.cpp` directly (or link the CMake `calc_core` target).
Matrix, vector,
and table storage is fixed-size. The expression parser uses `std::string` and
`std::vector`, so it needs the normal C++ runtime and heap.

## Controls (interactive window)
For the complete categorized quick reference, including desktop keys, calculator
SHIFT/ALPHA sequences, script tokens, current functions, and planned add-ons, see
[KEYBINDINGS.md](KEYBINDINGS.md).

0-9 . + - * /, `j` permutation P, `k` combination C,
Shift+6 or ^ for power, Shift+0 or ) for close parenthesis,
Enter/= equals, Backspace DEL, Delete AC, arrow keys,
f fraction, r sqrt (Shift+r cube root), q x^2, h x^y, v x^-1 (Shift+v x!),
l log (Shift+l 10^x), n ln (Shift+n e^x), s/c/t sin/cos/tan (Shift+ = inverse),
 b |abs|, e x10^x (ALPHA+e Euler's number, SHIFT+e pi), y HYP, a Ans, p pi, m MODE,
 HYP then s/c/t for sinh/cosh/tanh (desktop HYP key: y)
u SETUP, Tab S<=>D, left/right SHIFT, x ALPHA,
j enters infix `P` and k enters infix `C` (for example `5 P 2 =` gives 20,
`5 C 2 =` gives 10); `[` inserts GCD and `]` inserts LCM; `%` is postfix
percent, `mod(a,b)` is remainder, and g/i/o enter asinh/acosh/atanh through the
named script tokens; comma separates function arguments.
`i` enters the imaginary unit; `F10` inserts floor, `F11` ceiling, and `F12`
inserts argument. `%` is postfix percent (for example `25 % =` gives 0.25),
while `mod(a,b)` is the separate remainder function. Complex expressions use
`i`, such as `3+4i`; `abs(3+4i)` gives magnitude and `arg(3+4i)` gives argument.
Complex answers and stored A-Z values retain their imaginary components; SETUP's
complex result preference selects rectangular, polar, or real-only output.
For example, `(1+2i)*(1+2i)` evaluates to `-3+4i`, `abs(3+4i)` to `5`, and
`arg(i)` to 90 degrees when DEG is selected. Complex `Ans`, STO registers, and M
memory operations retain both components.
In MODE 4, F1-F5 select AND/OR/XOR/XNOR/NOT; ALPHA plus 1-6 enters A-F in HEX.
Shift-minus toggles the sign in the desktop simulator.
Press calculator SHIFT (hold desktop Shift) then Ans (`a`) for STO, then
press the variable number: 1-6=A-F, 7=X, 8=Y, 9=M. ALPHA (`x`) then the same
number recalls a variable. All variables evaluate as exact zero until stored.
After a result, pressing a binary operator starts the next expression with Ans;
for example `5 + 5 = * 2 =` computes 20. In the desktop simulator `*` (or the
numeric keypad multiply key) means multiplication; keyboard `x` is ALPHA.
After STO, the display annotates the source expression (for example `2 -> A`)
and shows the result using the active FIX/SCI/Norm setting. F6=M+ and
Shift+F6=M- annotate the displayed value (`10 M+`, `3 M-`); F8=MR evaluates M
and shows its accumulated value, while Shift+F8=MC clears M back to zero.
M+ and M- also evaluate the expression currently being entered, so no `=` is
needed before either memory-add key.
STO status is shown while the destination variable is pending. The SHIFT and
ALPHA status indicators appear while their one-shot state is active. `sto`,
`mminus`, and `mc` scripted tokens are convenience macros for their SHIFT
sequences, not separate physical calculator keys.
The interactive terminal help lists the simulator's key legends as primary and
SHIFT pairs, mirroring the yellow secondary labels on the fx-570ES PLUS. SETUP
page two option 7 toggles a battery display placeholder between hidden and
`100%`; it is a UI placeholder, not a measured battery value.
MODE 4 selects BASE-N radix. SETUP page two configures fraction style, complex
display preference, statistics frequency preference, decimal separator, and
LCD contrast. F-key aliases are desktop simulator shortcuts; firmware should
map physical keys to `app::Key` events independently.

## MODE and SETUP workflows

- `MODE 2` enters complex arithmetic. Enter real/imaginary parts for A and B,
    press `=` after each number, then select `+`, `-`, `*`, or `/`. Results use
    the configured rectangular, polar, or real-only display.
- `MODE 3` selects `1-Var` or `A+BX`. Enter X (and Y for regression), pressing
    `=` after each value. If SETUP frequency is enabled, enter a positive integer
    frequency after each sample. Results page with Up/Down; start another sample
    by typing its first digit.
- `MODE 4` selects decimal, hexadecimal, binary, or octal input/output. Enter
    integer operands in the selected radix, terminate each with `=`, then choose
    `+`, `-`, `*`, `/`, `and`, `or`, `xor`, or `xnor`. `not` is unary. Arithmetic
    is checked signed 64-bit; logic operations use the platform's signed integer
    representation, so this is not yet the calculator's fixed 32-bit word model.
- `MODE 5` solves `a*x^2+b*x+c=0`; enter a, b, and c with `=` between values.
    Up/Down pages the roots. A zero leading coefficient falls back to a linear
    equation.
- `MODE 6` supports 2x2 determinant, inverse, addition, and multiplication.
    Select the operation, enter A row-major, and enter B row-major for add/multiply.
- `MODE 7` selects `x`, `x^2`, `sin(x)`, or `cos(x)`, then enter start, end,
    and step. Up/Down pages up to 21 generated rows.
- `MODE 8` enters two 3D vectors and pages dot product, cross product,
    magnitudes, and angle.
- `SHIFT MODE` opens SETUP. Page one selects MathI/LineI, angle, Fix/Sci
    precision, or Norm1/Norm2. Page two selects fraction style, complex output,
    statistics frequency, decimal separator, and contrast. Contrast uses the
    arrow keys; Enter or AC exits that adjustment.

These are functional simulator workflows, not an exact firmware clone. MATRIX
entry is limited to 2x2, EQN covers linear/quadratic equations only, and TABLE
offers four built-in functions rather than an arbitrary expression editor.
Statistics currently reports means, deviations/variance, and linear regression;
it does not yet provide the calculator's full set of regression/statistical
summaries. The library's 3x3 matrix helpers, cubic solver, physical constants,
unit and coordinate conversions, DMS conversion, and prime factorization are
available through `calc_features.h` but are not exposed as calculator screens.
Other missing areas include the calculator's fixed-width BASE-N word semantics,
additional regression/statistical summaries, distributions, a custom-expression
table editor, and other calculator-specific conversions and equation types.

For firmware integration, see [ESP32_CORE_GUIDE.md](ESP32_CORE_GUIDE.md).

Natural Display uses custom LCD-style digit glyphs with spacing, larger distinct
P/C operator glyphs, and a short blinking vertical cursor. Single-letter variables
use a normal compact 5x7 alphabet, where e.g. variable `C` is distinct from the
larger combination operator `C`. `compact_font.h` exports constexpr glyph lookup,
glyph width, and text-width helpers for future AI/LLM output screens; its 52
uppercase/lowercase glyphs occupy 364 bytes, independent of U8g2 fonts.

## Scripted / headless mode (for testing, screenshots, CI)

    ./build/casio_ui_sim --keys "2 frac 3 right + 1 frac 2 right = @out.bmp"
    ./build/casio_ui_sim --keys "1 frac 2 right = " --ascii   # dump glass as text

Key names: digits/`.`/`+`/`-`/`*`/`/`/`(`/`)`, `frac mixed sqrt cbrt sq cube
pow root recip fact log 10^ ln e^ sin cos asin acos tan atan abs exp10 pi e
ans logb npr ncr mod gcd lcm asinh acosh atanh comma eq sd del ac left right up down
shift alpha mode setup sto A B C D E F X Y M mplus mminus mr mc and or xor
xnor not`.
`@path.bmp` mid-script saves a screenshot at that point.
