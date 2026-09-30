#include "calc_engine.h"

#include <cmath>
#include <iostream>

namespace {

bool expectValue(const char *expression, double expected) {
    calc::EvalResult result = calc::evaluate(expression, calc::AngleMode::Radians, 0);
    if (!result.ok || std::fabs(result.value - expected) > 1e-10) {
        std::cerr << expression << " expected " << expected << ", got "
                  << (result.ok ? std::to_string(result.value) : result.error) << '\n';
        return false;
    }
    return true;
}

} // namespace

int main() {
    calc::Engine engine;
    calc::Engine complexEngine;
    calc::EvalResult complexSaved = complexEngine.evaluate("1+2i");
    calc::EvalResult complexAns = complexEngine.evaluate("Ans*Ans");
    if (!complexSaved.ok || !complexSaved.complex || !complexAns.ok
        || std::fabs(complexAns.value + 3.0) > 1e-12 || std::fabs(complexAns.imaginary - 4.0) > 1e-12) return 27;
    if (!complexEngine.store('B', complexSaved)) return 28;
    calc::EvalResult complexMemory = complexEngine.evaluate("B+1");
    if (!complexMemory.ok || std::fabs(complexMemory.value - 2.0) > 1e-12
        || std::fabs(complexMemory.imaginary - 2.0) > 1e-12) return 29;
    calc::EvalResult complexM = complexEngine.evaluate("1+2i");
    if (!complexEngine.addToMemory('M', complexM)) return 30;
    calc::EvalResult complexMRead = complexEngine.evaluate("M*2");
    if (!complexMRead.ok || std::fabs(complexMRead.value - 2.0) > 1e-12
        || std::fabs(complexMRead.imaginary - 4.0) > 1e-12) return 31;
    calc::EvalResult firstAnswer = engine.evaluate("5+5");
    calc::EvalResult chainedAnswer = engine.evaluate("Ans*2");
    if (!firstAnswer.ok || firstAnswer.value != 10 || !chainedAnswer.ok || chainedAnswer.value != 20) return 26;
    calc::EvalResult defaultA = engine.evaluate("A");
    calc::EvalResult defaultC = engine.evaluate("C");
    calc::EvalResult defaultM = engine.evaluate("M");
    if (!defaultA.ok || !defaultA.exact || defaultA.value != 0
        || !defaultC.ok || !defaultC.exact || defaultC.value != 0
        || !defaultM.ok || !defaultM.exact || defaultM.value != 0) return 23;
    if (engine.recall('M')) return 24;
    calc::EvalResult saved = engine.evaluate("7/3");
    if (!engine.store('A', saved)) return 10;
    calc::EvalResult recalled = engine.evaluate("A+1");
    if (!recalled.ok || std::fabs(recalled.value - 10.0 / 3.0) > 1e-10 || !recalled.exact) return 11;
    calc::EvalResult defaultZ = engine.evaluate("Z");
    if (!defaultZ.ok || !defaultZ.exact || defaultZ.value != 0) return 12;
    calc::EvalResult third = engine.evaluate("1/3");
    double answerBeforeMemory = engine.answer();
    if (!engine.addToMemory('M', third)) return 13;
    if (std::fabs(engine.answer() - answerBeforeMemory) > 1e-10) return 20;
    calc::EvalResult sixth = engine.evaluate("1/6");
    answerBeforeMemory = engine.answer();
    if (!engine.addToMemory('M', sixth)) return 14;
    if (std::fabs(engine.answer() - answerBeforeMemory) > 1e-10) return 21;
    const calc::MemoryValue *memory = engine.recall('M');
    calc::Rational memoryValue;
    if (!memory || !memory->exact || !calc::exactIsRational(memory->ex, memoryValue)
        || memoryValue.n != 1 || memoryValue.d != 2) return 15;
    calc::EvalResult quarter = engine.evaluate("1/4");
    answerBeforeMemory = engine.answer();
    if (!engine.addToMemory('M', quarter, true)) return 16;
    if (std::fabs(engine.answer() - answerBeforeMemory) > 1e-10) return 22;
    memory = engine.recall('M');
    if (!memory || !calc::exactIsRational(memory->ex, memoryValue) || memoryValue.n != 1 || memoryValue.d != 4) return 17;
    if (!engine.clearMemory('M') || engine.recall('M')) return 19;
    calc::EvalResult clearedM = engine.evaluate("M");
    if (!clearedM.ok || !clearedM.exact || clearedM.value != 0) return 25;
    return expectValue("nPr(5,2)", 20)
        && expectValue("nCr(10,3)", 120)
        && expectValue("5P2", 20)
        && expectValue("5C2", 10)
        && expectValue("gcd(12,8)", 4)
        && expectValue("lcm(12,8)", 24)
        && expectValue("lcm(0,8)", 0)
        && !calc::evaluate("gcd(2.5,5)", calc::AngleMode::Radians, 0).ok
        && std::fabs(calc::evaluate("e", calc::AngleMode::Radians, 0).value - 2.718281828459045) < 1e-12
        && expectValue("10P0", 1)
        && expectValue("10C0", 1)
        && expectValue("mod(7,4)", 3)
        && expectValue("25%", 0.25)
        && expectValue("floor(2.9)", 2)
        && expectValue("ceil(2.1)", 3)
        && std::fabs(calc::evaluate("abs(3+4i)", calc::AngleMode::Radians, 0).value - 5.0) < 1e-12
        && std::fabs(calc::evaluate("arg(i)", calc::AngleMode::Degrees, 0).value - 90.0) < 1e-12
        && !calc::evaluate("arg(0i)", calc::AngleMode::Degrees, 0).ok
        && std::fabs(calc::evaluate("(2+3i)*(1-2i)", calc::AngleMode::Radians, 0).value - 8.0) < 1e-12
        && std::fabs(calc::evaluate("(2+3i)*(1-2i)", calc::AngleMode::Radians, 0).imaginary + 1.0) < 1e-12
        && calc::evaluate("(2+3i)*(1-2i)", calc::AngleMode::Radians, 0).complex
        && std::fabs(calc::evaluate("(1+2i)^2", calc::AngleMode::Radians, 0).imaginary - 4.0) < 1e-12
        && expectValue("asinh(0)", 0)
        && expectValue("acosh(1)", 0)
        && expectValue("atanh(0)", 0)
        && std::fabs(calc::evaluate("sinh(1)", calc::AngleMode::Radians, 0).value - 1.1752011936438014) < 1e-12
        && std::fabs(calc::evaluate("cosh(1)", calc::AngleMode::Radians, 0).value - 1.5430806348152437) < 1e-12
        && std::fabs(calc::evaluate("tanh(1)", calc::AngleMode::Radians, 0).value - 0.7615941559557649) < 1e-12
        && !calc::evaluate("nCr(3,4)", calc::AngleMode::Radians, 0).ok
        && !calc::evaluate("acosh(0.5)", calc::AngleMode::Radians, 0).ok
        && !calc::evaluate("atanh(1)", calc::AngleMode::Radians, 0).ok ? 0 : 1;
}