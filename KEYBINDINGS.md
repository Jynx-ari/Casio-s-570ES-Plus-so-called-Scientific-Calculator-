# Calculator Keybinds and Feature Reference

This page distinguishes **desktop simulator keys**, **calculator SHIFT/ALPHA sequences**, and **script tokens**. The desktop key layout is a convenience mapping, not a picture of the fx-570ES PLUS keypad. Only left Shift toggles calculator SHIFT; right Shift is ignored.

## Desktop Keys

| Desktop key | Calculator action |
|---|---|
| `0`-`9`, `.`, `+`, `-`, `*`, `/`, `(`, `)` | Digits and arithmetic |
| `Shift+6` or `^` | Power `x^y` |
| `Shift+0` or `)` | Close parenthesis |
| `Shift+8` | Multiplication `*` |
| `Shift+9` | Open parenthesis |
| `Enter` | Equals |
| `Backspace` | DEL |
| `Delete` | AC |
| Arrow keys | Cursor, history, result paging, menu navigation |
| `Tab` | S<=>D |
| **Left Shift** | Toggle calculator SHIFT state; displays `S` |
| `x` | Toggle ALPHA state; displays `A` |
| `y` | Toggle HYP state; displays `HYP` |
| `a` | Ans; Left Shift then `a` starts STO |
| `s`, `c`, `t` | sin, cos, tan; with calculator SHIFT: asin, acos, atan; with HYP: sinh, cosh, tanh |
| `f`, `r`, `q`, `h`, `v`, `l`, `n`, `e` | Fraction, sqrt, square, power, reciprocal, log, ln, x10^x; SHIFT selects the secondary function described below |
| `p` | Pi constant |
| `m` | MODE; calculator SHIFT then `m` opens SETUP |
| `u` | SETUP shortcut |
| `j`, `k` | Permutation `P`, combination `C` |
| `%` | Postfix percentage |
| `i` | Imaginary unit `i`; Left Shift plus `i` is acosh |
| `g`, `o` | asinh, atanh |
| `[` and `]` | Insert GCD and LCM functions |
| `F1`-`F5` | BASE-N AND, OR, XOR, XNOR, NOT |
| `F6`; Left Shift+`F6` | M+; M- |
| `F8`; Left Shift+`F8` | MR; MC |
| `F10`, `F11`, `F12` | floor, ceil, arg |
| `Esc` | Quit simulator |

The calculator SHIFT layer also includes `f` mixed fraction, `r` cube root, `q` cube, `h` nth root, `v` factorial, `l` 10^x, `n` e^x, `e` pi, inverse trig on `s/c/t`, STO on Ans, M- on M+, MC on MR, and SETUP on MODE. Calculator HYP followed by `s/c/t` enters sinh/cosh/tanh.

## Calculator Memory

To store a result in a named register, press **Left Shift**, `a` (Ans/STO), then a digit: `1`-`6` selects A-F, `7` selects X, `8` selects Y, and `9` selects M. The screen shows the source expression and destination, e.g. `12 -> A`.

ALPHA followed by those same digits recalls the corresponding variable. Untouched variables evaluate as exact zero. `F6` accumulates the current expression into M without requiring `=`, `Left Shift+F6` subtracts it, `F8` recalls/evaluates M, and `Left Shift+F8` clears M to zero. M operations show feedback such as `10 M+` and `3 M-`.

## MODE and SETUP

| Menu selection | Current behavior |
|---|---|
| MODE 1 COMP | Standard scientific expression calculator |
| MODE 2 CMPLX | Complex arithmetic workflow |
| MODE 3 STAT | One-variable and linear regression entry, optional frequency |
| MODE 4 BASE-N | DEC/HEX/BIN/OCT integer input, arithmetic and bitwise operations |
| MODE 5 EQN | Linear/quadratic equations |
| MODE 6 MATRIX | 2x2 determinant, inverse, addition, multiplication |
| MODE 7 TABLE | x, x^2, sin(x), cos(x); up to 21 rows |
| MODE 8 VECTOR | 3D dot/cross products, magnitudes, angle |
| Left Shift+MODE | SETUP |

SETUP includes MathI/LineI, DEG/RAD/GRA, FIX/SCI/NORM, fraction style, complex result format, statistics frequency, decimal separator, contrast, and an optional `100%` battery placeholder. Battery percentage is only a placeholder, not a hardware reading.

## Script Tokens

Run scripts with `./build/casio_ui_sim --keys "..."`. Tokens include digits, `. + - * / ( )`, `eq`, `frac`, `mixed`, `sqrt`, `cbrt`, `sq`, `cube`, `pow`, `root`, `recip`, `fact`, `log`, `10^`, `ln`, `e^`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sinh`, `cosh`, `tanh`, `asinh`, `acosh`, `atanh`, `pi`, `e`, `ans`, `imag`, `arg`, `abs`, `npr`, `ncr`, `mod`, `pct`, `floor`, `ceil`, `gcd`, `lcm`, `comma`, `A`-`F`, `X`, `Y`, `M`, `sto`, `mplus`, `mminus`, `mr`, `mc`, `mode`, `setup`, `shift`, `alpha`, `hyp`, cursor keys, and `@path.bmp` for screenshots.

Expression examples:

```text
5 P 2 =              # 20
5 C 2 =              # 10
25 pct =             # 0.25
floor(2.9) =         # 2
ceil(2.1) =          # 3
3 + 4i =             # 3+4i
abs(3+4i) =           # 5
arg(i) =              # 90 in DEG mode
(1+2i)*(1+2i) =      # -3+4i
```

## Implemented Engine Functions

The portable evaluator supports real arithmetic, implicit multiplication, powers, fractions/roots via expression syntax, factorial, postfix percent, trig/inverse trig, hyperbolic/inverse hyperbolic, ln/log, floor/ceil, nPr/nCr, remainder, GCD/LCM, pi/e, complex arithmetic and powers, `sqrt`, `abs`, `arg`, `conj`, `real`, and `imag`. Ans and memory registers retain complex values. `calc_features.h` also provides complex helpers, weighted statistics, linear/quadratic/cubic solvers, 3x3 matrix helpers, vector helpers, table generation, selected unit/coordinate/DMS conversions, constants, and prime factorization.

## Possible Future Add-ons

These are **not yet complete fx-570ES PLUS features**:

- Full probability distributions and statistical summaries/regression families.
- Simultaneous equation systems, higher-order equation modes, and full EQN UI.
- Larger interactive matrices and vectors, native MATRIX/VECTOR data editing.
- Arbitrary-expression TABLE editor and full table navigation.
- Exact fx-570ES PLUS fixed-width BASE-N behavior and complete base-operation set.
- Complete constants/conversion catalogs and calculator-style conversion menus.
- Persistent settings and memory using ESP32 NVS.
- AI/LLM prompt and response interface. The compact A-Z/a-z 5x7 font is prepared for small text rendering; it is not itself an AI feature.

For ESP32-S3 integration, heap/toolchain notes, and portable API examples, see [ESP32_CORE_GUIDE.md](ESP32_CORE_GUIDE.md).
