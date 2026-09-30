#pragma once
// calc_engine.h -- hardware/display independent evaluator (portable to ESP32).
//
// Every value is carried twice: a double (always available) and, when the
// calculation stays "exact", an exact form: a sum of terms
//        c * sqrt(radicand) * pi^(0|1)         c = rational
// That is what lets the display show 7/12, 45*sqrt(3)+10*sqrt(2) or (1/6)*pi
// the way the fx-570ES PLUS does, and toggle to decimal with S<=>D.
#include <cstdint>
#include <array>
#include <string>
#include <vector>

namespace calc {

enum class AngleMode { Degrees, Radians, Grads };

struct Rational { int64_t n = 0, d = 1; };                 // d > 0, reduced
struct Term { Rational c; int64_t radicand = 1; bool pi = false; };
struct Exact { std::vector<Term> terms; };                 // canonical, empty == 0

struct EvalResult {
    bool ok = false;
    double value = 0.0;
    double imaginary = 0.0;
    bool complex = false;
    bool exact = false;        // `ex` is valid
    Exact ex;
    std::string error;         // "Syntax ERROR" / "Math ERROR"
};

struct MemoryValue {
    bool defined = false;
    double value = 0.0;
    double imaginary = 0.0;
    bool complex = false;
    bool exact = false;
    Exact ex;
};

using Memory = std::array<MemoryValue, 26>;

struct EvalContext {
    AngleMode angle = AngleMode::Degrees;
    double ans = 0.0;
    double ansImaginary = 0.0;
    bool ansComplex = false;
    const Exact *ansExact = nullptr;
    const Memory *memory = nullptr;
};

// expr uses ASCII engine syntax: + - * / ^ ( ) sin( asin( sqrt( abs( ln( log( pi e Ans ! %
EvalResult evaluate(const std::string &expr, AngleMode mode, double ans,
                    const Exact *ansExact = nullptr);
EvalResult evaluate(const std::string &expr, const EvalContext &context);

// Stateful facade suitable for firmware callers; independent of SDL/GPIO/U8g2.
class Engine {
public:
    EvalResult evaluate(const std::string &expr);
    void setAngleMode(AngleMode mode) { angle_ = mode; }
    AngleMode angleMode() const { return angle_; }
    double answer() const { return ans_; }
    const Memory &memory() const { return memory_; }
    bool store(char name, const EvalResult &value);
    const MemoryValue *recall(char name) const;
    bool addToMemory(char name, const EvalResult &value, bool subtract = false);
    bool clearMemory(char name);

private:
    AngleMode angle_ = AngleMode::Degrees;
    double ans_ = 0.0;
    double ansImaginary_ = 0.0;
    bool ansComplex_ = false;
    Exact ansExact_;
    bool ansIsExact_ = false;
    Memory memory_{};
};

bool exactIsRational(const Exact &x, Rational &out);       // no radicals, no pi

} // namespace calc
