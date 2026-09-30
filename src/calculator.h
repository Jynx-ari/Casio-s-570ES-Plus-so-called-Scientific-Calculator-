#pragma once
// calculator.h -- fx-570ES PLUS behaviour + screens, independent of SDL / GPIO.
// Firmware: feed press() from the keypad scan, call draw() ~every 500 ms for cursor blink.
#include <memory>
#include <string>
#include <vector>
#include "calc_engine.h"
#include "calc_features.h"
#include "display.h"
#include "natural.h"

namespace app {

enum class Key {
    N0, N1, N2, N3, N4, N5, N6, N7, N8, N9, Dot,
    Plus, Minus, Mul, Div, Neg, LPar, RPar,
    Frac, Sqrt, Sq, Pow, Recip, Log, LogB, Ln, Sin, Cos, Tan, Abs, Exp10, Ans,
    Eq, SD, Del, Ac, Left, Right, Up, Down, Shift, Alpha, Hyp, Mode,
    Comma, NPr, NCr, Mod, Pct, Floor, Ceil, Imaginary, Arg, Asinh, Acosh, Atanh,
    BitAnd, BitOr, BitXor, BitXnor, BitNot, MPlus, MR, Gcd, Lcm
};

struct Settings {
    enum class Fmt { Norm1, Norm2, Fix, Sci } fmt = Fmt::Norm1;
    enum class ComplexFmt { Rectangular, Polar, Real } complexFmt = ComplexFmt::Rectangular;
    int digits = 0;                                   // for Fix / Sci
    bool mixedFractions = false;                      // ab/c vs d/c
    bool mathIO = true;                               // Natural Display results
    calc::AngleMode angle = calc::AngleMode::Degrees;
    calc::Radix radix = calc::Radix::Decimal;
    bool statFrequency = false;
    bool statBivariate = false;
    bool decimalComma = false;
    int contrast = 50;
    bool showBatteryPlaceholder = false;
};

class Calculator {
public:
    Calculator();
    void press(Key k);
    void draw(Display &d, bool cursorOn);

private:
    enum class Screen { Calc, Mode, Setup1, Setup2, MathIOSel, NormSel, BaseNSel,
                        ComplexSel, StatSel, DispSel, ContrastSel, BatterySel, StatType, MatrixOp, TableFn, ModeWork, Prompt } scr_ = Screen::Calc;
    enum class Phase { Edit, Result, Error } phase_ = Phase::Edit;
    struct Hist { nat::Seq expr; calc::EvalResult res; };

    Settings set_;
    calc::Engine engine_;
    nat::Editor ed_;
    nat::Seq feedbackExpr_;
    bool showFeedbackExpr_ = false;
    bool shift_ = false, alpha_ = false, hyp_ = false;
    calc::EvalResult res_;
    bool showDec_ = false;
    std::string err_;
    bool storePending_ = false;
    std::vector<std::unique_ptr<Hist>> hist_;
    int hidx_ = -1;                     // >= 0: browsing calculation history
    int scrollX_ = 0, resScroll_ = 0, resMaxScroll_ = 0;
    std::string modeInd_;               // "", "CMPLX", "STAT", "MAT", "VCT"
    bool promptSci_ = false;
    bool promptComplex_ = false;
    std::vector<double> modeValues_;
    std::vector<std::string> modeResults_;
    std::string modeInput_;
    calc::Statistics modeStats_;
    int modePage_ = 0;
    int modeOperation_ = -1;
    bool modeHasResult_ = false;
    bool modeAlpha_ = false;
    int64_t baseFirst_ = 0;
    std::string baseToken_;
    int baseOperation_ = -1;
    bool baseHasFirst_ = false;

    void editKey(Key k, bool sh, bool al, bool hyp);
    void menuKey(Key k);
    void modeKey(Key k);
    void finishModeEntry();
    void showMemoryFeedback(const std::string &suffix);
    void equals();
    void newInputIfResult(Key k);
    bool canToggle() const;

    void drawIndicators(Display &d);
    void drawCalc(Display &d, bool cursorOn);
    void drawError(Display &d);
    void drawMenu(Display &d);
    void drawModeWork(Display &d);
    void buildResult(nat::Seq &out, const calc::EvalResult &r, bool decimal) const;
};

} // namespace app
