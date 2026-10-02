#include "calculator.h"
#include "natural_draw.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <numeric>
#include <sstream>

namespace app {
using nat::Kind;

namespace {
const char *const UP_ROWS[3] = {"..#..", ".###.", "#####"};
const char *const DN_ROWS[3] = {"#####", ".###.", "..#.."};
const char *const TL[5] = {"..#", ".##", "###", ".##", "..#"};
const char *const TR[5] = {"#..", "##.", "###", "##.", "#.."};
const std::string MUL = "\xC3\x97", DIV = "\xC3\xB7", PI = "\xCF\x80";
constexpr int GRAPH_X = 20, GRAPH_Y = 60, GRAPH_W = 280, GRAPH_H = 144;
constexpr int GRAPH_INSET = 3;

std::string num(long long v) { return std::to_string(v); }

std::string modeNumber(double value) {
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%.8g", value);
    return buffer;
}

std::string complexComponent(double value, bool imaginary) {
    if (!imaginary || value >= 0) return (imaginary ? "+" : "") + modeNumber(value);
    return modeNumber(value);
}

double fromRadians(double radians, calc::AngleMode angle) {
    if (angle == calc::AngleMode::Degrees) return radians * 180.0 / 3.14159265358979323846;
    if (angle == calc::AngleMode::Grads) return radians * 200.0 / 3.14159265358979323846;
    return radians;
}

// Draws text where ':' is a hand-drawn two-dot colon (this font's colon is a plus-shaped blob).
void menuText(Display &d, int x, int by, const std::string &t) {
    std::string run;
    auto flush = [&]() { if (!run.empty()) { d.text(x, by, run, Display::Font::Main); x += d.textWidth(run, Display::Font::Main); run.clear(); } };
    for (char c : t) {
        if (c == ':') { flush(); d.box(x + 1, by - 6, 2, 2); d.box(x + 1, by - 2, 2, 2); x += 5; }
        else run += c;
    }
    flush();
}


// ---- decimal number formatting (Norm1/Norm2/Fix/Sci, 10-digit mantissa) ----
void formatDecimal(double v, const Settings &s, std::string &mant, bool &hasExp, int &exp) {
    hasExp = false; exp = 0;
    if (v == 0) {
        if (s.fmt == Settings::Fmt::Fix) {
            char buf[64];
            std::snprintf(buf, sizeof buf, "%.*f", std::clamp(s.digits, 0, 9), 0.0);
            mant = buf;
        } else if (s.fmt == Settings::Fmt::Sci) {
            int sig = s.digits > 0 ? std::clamp(s.digits, 1, 9) : 10;
            mant = "0";
            if (sig > 1) mant += "." + std::string((size_t)(sig - 1), '0');
            hasExp = true; exp = 0;
        } else mant = "0";
        return;
    }
    double mag = std::fabs(v);
    bool expForm = false;
    switch (s.fmt) {
        case Settings::Fmt::Norm1: expForm = mag < 1e-2 || mag >= 1e10; break;
        case Settings::Fmt::Norm2: expForm = mag < 1e-9 || mag >= 1e10; break;
        case Settings::Fmt::Fix:   expForm = mag >= 1e10; break;
        case Settings::Fmt::Sci:   expForm = true; break;
    }
    char buf[64];
    if (s.fmt == Settings::Fmt::Fix && !expForm) {
        std::snprintf(buf, sizeof buf, "%.*f", std::clamp(s.digits, 0, 9), v);
        mant = buf;
        if (mant.find_first_not_of("-0.") == std::string::npos) mant = mant.substr(mant[0] == '-' ? 1 : 0);
        return;
    }
    if (expForm) {
        int sig = (s.fmt == Settings::Fmt::Sci && s.digits > 0) ? std::clamp(s.digits, 1, 9) : 10;
        std::snprintf(buf, sizeof buf, "%.*e", sig - 1, v);
        std::string t = buf; size_t e = t.find('e');
        mant = t.substr(0, e); exp = std::atoi(t.c_str() + e + 1); hasExp = true;
        if (s.fmt != Settings::Fmt::Sci && mant.find('.') != std::string::npos) {
            while (mant.back() == '0') mant.pop_back();
            if (mant.back() == '.') mant.pop_back();
        }
        return;
    }
    int e10 = (int)std::floor(std::log10(mag));
    int dec = std::clamp(9 - e10, 0, 15);
    std::snprintf(buf, sizeof buf, "%.*f", dec, v);
    mant = buf;
    if (mant.find('.') != std::string::npos) {
        while (mant.back() == '0') mant.pop_back();
        if (mant.back() == '.') mant.pop_back();
    }
}

void addSigned(nat::Seq &s, bool negative) { if (negative) nat::addText(s, "-"); }

void emitRadTerm(nat::Seq &dst, long long coef, long long radicand, bool showSign, bool neg) {
    if (showSign) nat::addText(dst, neg ? "-" : "+");
    if (radicand == 1) { nat::addDigits(dst, num(coef)); return; }
    if (coef != 1) nat::addDigits(dst, num(coef));
    nat::Node *sq = dst.push(nat::makeContainer(Kind::Sqrt));
    nat::addDigits(sq->slots[0], num(radicand));
}

bool buildExact(nat::Seq &out, const calc::Exact &ex, const Settings &st) {
    using calc::Rational;
    Rational q;
    if (ex.terms.empty()) { nat::addText(out, "0"); return true; }
    if (calc::exactIsRational(ex, q)) {
        if (std::llabs(q.n) >= 10000000000LL || q.d >= 10000000000LL) return false;
        bool neg = q.n < 0; long long n = std::llabs(q.n);
        addSigned(out, neg);
        if (q.d == 1) { nat::addDigits(out, num(n)); return true; }
        if (st.mixedFractions && n > q.d) {
            nat::Node *m = out.push(nat::makeContainer(Kind::Mixed));
            nat::addDigits(m->slots[0], num(n / q.d));
            nat::addDigits(m->slots[1], num(n % q.d));
            nat::addDigits(m->slots[2], num(q.d));
            return true;
        }
        nat::Node *f = out.push(nat::makeContainer(Kind::Frac));
        nat::addDigits(f->slots[0], num(n)); nat::addDigits(f->slots[1], num(q.d));
        return true;
    }
    if (ex.terms.size() == 1 && ex.terms[0].pi && ex.terms[0].radicand == 1) {
        const calc::Term &t = ex.terms[0];
        double val = std::fabs((double)t.c.n / t.c.d * M_PI);
        if (val >= 1e6) return false;
        long long n = std::llabs(t.c.n);
        addSigned(out, t.c.n < 0);
        if (t.c.d == 1) { if (n != 1) nat::addDigits(out, num(n)); }
        else { nat::Node *f = out.push(nat::makeContainer(Kind::Frac)); nat::addDigits(f->slots[0], num(n)); nat::addDigits(f->slots[1], num(t.c.d)); }
        nat::addText(out, PI, "pi");
        return true;
    }
    // sqrt forms: at most two terms, one common denominator
    for (auto &t : ex.terms) if (t.pi) return false;
    if (ex.terms.size() > 2) return false;
    long long L = 1;
    for (auto &t : ex.terms) L = std::lcm(L, t.c.d);
    if (L >= 100) return false;
    struct T { long long a, r; };
    std::vector<T> ts;
    for (auto &t : ex.terms) {
        long long a = t.c.n * (L / t.c.d);
        if (t.radicand == 1 ? std::llabs(a) >= 100 : (std::llabs(a) >= 100 || t.radicand >= 1000)) return false;
        ts.push_back({a, t.radicand});
    }
    std::sort(ts.begin(), ts.end(), [](const T &x, const T &y) {           // integer first, then radicands descending
        if ((x.r == 1) != (y.r == 1)) return x.r == 1;
        return x.r > y.r;
    });
    bool single = ts.size() == 1;
    nat::Seq *dst = &out;
    if (L > 1) {
        if (single && ts[0].a < 0) nat::addText(out, "-");
        nat::Node *f = out.push(nat::makeContainer(Kind::Frac));
        nat::addDigits(f->slots[1], num(L));
        dst = &f->slots[0];
    }
    for (size_t i = 0; i < ts.size(); ++i) {
        bool neg = ts[i].a < 0;
        bool signInside = (L > 1 && single) ? false : neg;
        if (i == 0) { if (signInside) nat::addText(*dst, "-"); emitRadTerm(*dst, std::llabs(ts[i].a), ts[i].r, false, false); }
        else emitRadTerm(*dst, std::llabs(ts[i].a), ts[i].r, true, neg);
    }
    return true;
}
} // namespace

void Calculator::buildResult(nat::Seq &out, const calc::EvalResult &r, bool decimal) const {
    if (r.complex) {
        auto appendScalar = [&](double value) {
            calc::EvalResult scalar; scalar.ok = true; scalar.value = value;
            buildResult(out, scalar, true);
        };
        if (set_.complexFmt == Settings::ComplexFmt::Polar) {
            double angle = std::atan2(r.imaginary, r.value);
            if (set_.angle == calc::AngleMode::Degrees) angle *= 180.0 / M_PI;
            else if (set_.angle == calc::AngleMode::Grads) angle *= 200.0 / M_PI;
            nat::addText(out, "r="); appendScalar(std::hypot(r.value, r.imaginary));
            nat::addText(out, " arg="); appendScalar(angle);
        } else if (set_.complexFmt == Settings::ComplexFmt::Real) {
            appendScalar(r.value);
        } else {
            if (r.value != 0.0 || r.imaginary == 0.0) appendScalar(r.value);
            if (r.imaginary != 0.0) {
                if (r.imaginary > 0 && (r.value != 0.0)) nat::addText(out, "+");
                else if (r.imaginary < 0) nat::addText(out, "-");
                appendScalar(std::fabs(r.imaginary));
                nat::addText(out, "i", "i");
            }
        }
        return;
    }
    if (modeInd_ == "BASE" && r.value >= -9223372036854775808.0 && r.value < 9223372036854775808.0
        && r.value == std::floor(r.value)) {
        nat::addText(out, calc::formatInteger((int64_t)r.value, set_.radix));
        return;
    }
    bool fixedFormat = set_.fmt == Settings::Fmt::Fix || set_.fmt == Settings::Fmt::Sci;
    if (!decimal && !fixedFormat && r.exact && set_.mathIO && buildExact(out, r.ex, set_)) return;
    std::string mant; bool hasExp; int exp;
    formatDecimal(r.value, set_, mant, hasExp, exp);
    if (set_.decimalComma) std::replace(mant.begin(), mant.end(), '.', ',');
    nat::addDigits(out, mant);
    if (hasExp) {
        nat::addText(out, MUL, "*"); nat::addText(out, "1"); nat::addText(out, "0");
        nat::Node *p = out.push(nat::makeContainer(Kind::Pow));
        if (exp < 0) nat::addText(p->slots[0], "-");
        nat::addDigits(p->slots[0], num(std::abs(exp)));
    }
}

bool Calculator::canToggle() const {
    if (!res_.ok || !res_.exact || !set_.mathIO) return false;
    calc::Rational q;
    return !(calc::exactIsRational(res_.ex, q) && q.d == 1);
}

Calculator::Calculator() {}

void Calculator::showMemoryFeedback(const std::string &suffix) {
    feedbackExpr_.clear();
    nat::clone(ed_.root, feedbackExpr_);
    nat::addText(feedbackExpr_, suffix);
    showFeedbackExpr_ = true;
    phase_ = Phase::Result;
    showDec_ = false;
    resScroll_ = 0;
    scrollX_ = 0;
}

// ------------------------------------------------------------------ input
void Calculator::equals() {
    showFeedbackExpr_ = false;
    std::string e = ed_.engine();
    int open = 0; for (char c : e) { if (c == '(') ++open; else if (c == ')') --open; }
    for (int i = 0; i < open; ++i) e += ")";
    engine_.setAngleMode(set_.angle);
    calc::EvalResult r = engine_.evaluate(e);
    if (r.ok && modeInd_ == "BASE" && (r.value < -9223372036854775808.0 || r.value >= 9223372036854775808.0
        || r.value != std::floor(r.value))) { r.ok = false; r.error = "Math ERROR"; }
    if (!r.ok) { phase_ = Phase::Error; err_ = r.error; return; }
    res_ = r;
    showDec_ = false; phase_ = Phase::Result; resScroll_ = 0; scrollX_ = 0;
    auto h = std::make_unique<Hist>();
    nat::clone(ed_.root, h->expr); h->res = r;
    hist_.push_back(std::move(h));
    if (hist_.size() > 10) hist_.erase(hist_.begin());
}

void Calculator::press(Key k) {
    if (scr_ == Screen::GraphEntry || scr_ == Screen::GraphPlot) { graphKey(k); return; }
    if (scr_ == Screen::ModeWork) { modeKey(k); return; }
    if (scr_ != Screen::Calc) { menuKey(k); return; }
    if (k == Key::Shift) { shift_ = !shift_; alpha_ = false; return; }
    if (k == Key::Alpha) { alpha_ = !alpha_; shift_ = false; return; }
    if (k == Key::Hyp) { hyp_ = !hyp_; return; }
    bool sh = shift_, al = alpha_, hyp = hyp_; shift_ = alpha_ = hyp_ = false;

    if (storePending_) {
        char variable = 0;
        if (k >= Key::N1 && k <= Key::N6) variable = (char)('A' + (int)k - (int)Key::N1);
        else if (k == Key::N7) variable = 'X';
        else if (k == Key::N8) variable = 'Y';
        else if (k == Key::N9) variable = 'M';
        if (variable) {
            engine_.store(variable, res_);
            storePending_ = false;
            showMemoryFeedback(" -> " + std::string(1, variable));
            return;
        }
        storePending_ = false;
    }

    if (k == Key::MPlus) {
        if (phase_ == Phase::Edit) equals();
        if (phase_ == Phase::Result && res_.ok) {
            bool subtract = sh;
            engine_.addToMemory('M', res_, subtract);
            showMemoryFeedback(subtract ? " M-" : " M+");
        }
        return;
    }
    if (k == Key::MR && sh) { engine_.clearMemory('M'); return; }
    if (k == Key::MR) {
        if (phase_ == Phase::Result) ed_.reset();
        ed_.type("M", "M"); phase_ = Phase::Edit; ed_.toEnd();
        equals();
        return;
    }

    if (k == Key::Ans && sh) { storePending_ = true; return; }

    if (k == Key::Mode) { scr_ = sh ? Screen::Setup1 : Screen::Mode; return; }

    if (phase_ == Phase::Error) {
        if (k == Key::Ac) { ed_.reset(); phase_ = Phase::Edit; }
        else if (k == Key::Left || k == Key::Right) { phase_ = Phase::Edit; ed_.toEnd(); }
        return;
    }
    if (hidx_ >= 0) {                                   // browsing history
        if (k == Key::Up) { if (hidx_ > 0) --hidx_; return; }
        if (k == Key::Down) { if (++hidx_ >= (int)hist_.size()) hidx_ = -1; return; }
        if (k == Key::Ac) { hidx_ = -1; return; }
        ed_.loadFrom(hist_[hidx_]->expr); hidx_ = -1; phase_ = Phase::Edit;
    }
    if (phase_ == Phase::Result) {
        if ((k == Key::Left || k == Key::Right) && resMaxScroll_ > 0) {          // long result: scroll it
            resScroll_ += (k == Key::Right) ? 12 : -12;
            resScroll_ = std::clamp(resScroll_, 0, resMaxScroll_);
            return;
        }
        switch (k) {
            case Key::SD: if (canToggle()) showDec_ = !showDec_; return;
            case Key::Up: if (!hist_.empty()) hidx_ = (int)hist_.size() - 1; return;
            case Key::Down: return;
            case Key::Ac: ed_.reset(); showFeedbackExpr_ = false; phase_ = Phase::Edit; return;
            case Key::Left: showFeedbackExpr_ = false; phase_ = Phase::Edit; ed_.toEnd(); scrollX_ = 1 << 20; return;
            case Key::Right: showFeedbackExpr_ = false; phase_ = Phase::Edit; ed_.toStart(); scrollX_ = 0; return;
            case Key::Del: phase_ = Phase::Edit; ed_.toEnd(); break;
            case Key::Plus: case Key::Minus: case Key::Mul: case Key::Div: case Key::Pow: case Key::Sq: case Key::Recip:
                showFeedbackExpr_ = false; ed_.reset(); ed_.type("Ans", "Ans"); phase_ = Phase::Edit; break;
            case Key::Eq: return;
            default: showFeedbackExpr_ = false; ed_.reset(); phase_ = Phase::Edit; break;
        }
    }
    editKey(k, sh, al, hyp);
}

void Calculator::finishModeEntry() {
    if (modeInput_.empty() || modeHasResult_) return;
    char *end = nullptr;
    double value = std::strtod(modeInput_.c_str(), &end);
    if (!end || *end || !std::isfinite(value)) { modeInput_.clear(); return; }
    modeValues_.push_back(value);
    modeInput_.clear();

    if (modeInd_ == "STAT") {
        size_t rowValues = (set_.statBivariate ? 2u : 1u) + (set_.statFrequency ? 1u : 0u);
        if (modeValues_.size() < rowValues) return;
        double x = modeValues_[0], y = set_.statBivariate ? modeValues_[1] : 0.0;
        double f = set_.statFrequency ? modeValues_[rowValues - 1] : 1.0;
        if (f < 1 || f > 1000000 || f != std::floor(f)) { modeValues_.clear(); return; }
        if (!modeStats_.add(x, y, (uint32_t)f)) { modeValues_.clear(); return; }
        modeValues_.clear();
        modeResults_ = {"n=" + std::to_string(modeStats_.count()), "MeanX=" + modeNumber(modeStats_.meanX()),
                        "Sx=" + modeNumber(modeStats_.standardDeviationX()),
                        "PopVar=" + modeNumber(modeStats_.varianceX(false))};
        if (set_.statBivariate) {
            double slope, intercept, correlation;
            modeResults_.push_back("MeanY=" + modeNumber(modeStats_.meanY()));
            if (modeStats_.linearRegression(slope, intercept, correlation)) {
                modeResults_.push_back("a=" + modeNumber(intercept));
                modeResults_.push_back("b=" + modeNumber(slope));
                modeResults_.push_back("r=" + modeNumber(correlation));
            } else modeResults_.push_back("Regression ERROR");
        }
        modeHasResult_ = true;
        return;
    }

    size_t required = modeInd_ == "CMPLX" ? 4 : modeInd_ == "EQN" ? 3
                    : modeInd_ == "MAT" ? (modeOperation_ >= 2 ? 8 : 4) : modeInd_ == "VCT" ? 6 : 3;
    if (modeValues_.size() < required) return;
    modeHasResult_ = true;
    if (modeInd_ == "EQN") {
        calc::EquationResult result = calc::solveQuadratic(modeValues_[0], modeValues_[1], modeValues_[2]);
        if (!result.ok) modeResults_ = {"Math ERROR"};
        else if (result.count == 1) modeResults_ = {"x=" + modeNumber(result.roots[0].real)};
        else modeResults_ = {"x1=" + modeNumber(result.roots[0].real) + (result.complexRoots ? complexComponent(result.roots[0].imag, true) + "i" : ""),
                     "x2=" + modeNumber(result.roots[1].real) + (result.complexRoots ? complexComponent(result.roots[1].imag, true) + "i" : "")};
    } else if (modeInd_ == "MAT") {
        calc::Matrix3 a; a.rows = a.cols = 2;
        for (size_t i = 0; i < 4; ++i) a.values[i / 2][i % 2] = modeValues_[i];
        calc::Matrix3 result; double determinant = 0;
        if (modeOperation_ == 0) {
            if (!calc::matrixDeterminant(a, determinant)) modeResults_ = {"Math ERROR"};
            else modeResults_ = {"det=" + modeNumber(determinant)};
        } else if (modeOperation_ == 1) {
            if (!calc::matrixInverse(a, result)) modeResults_ = {"Singular"};
            else modeResults_ = {"inv11=" + modeNumber(result.values[0][0]), "inv12=" + modeNumber(result.values[0][1]),
                                 "inv21=" + modeNumber(result.values[1][0]), "inv22=" + modeNumber(result.values[1][1])};
        } else {
            calc::Matrix3 b; b.rows = b.cols = 2;
            for (size_t i = 0; i < 4; ++i) b.values[i / 2][i % 2] = modeValues_[i + 4];
            bool ok = modeOperation_ == 2 ? calc::matrixAdd(a, b, result) : calc::matrixMultiply(a, b, result);
            if (!ok) modeResults_ = {"Math ERROR"};
            else for (uint8_t row = 0; row < 2; ++row) for (uint8_t col = 0; col < 2; ++col)
                modeResults_.push_back(modeNumber(result.values[row][col]));
        }
    } else if (modeInd_ == "VCT") {
        calc::Vector3 a{modeValues_[0], modeValues_[1], modeValues_[2]};
        calc::Vector3 b{modeValues_[3], modeValues_[4], modeValues_[5]};
        calc::Vector3 cross = calc::vectorCross(a, b); double angle = 0;
        bool angleOk = calc::vectorAngle(a, b, angle);
        modeResults_ = {"A.B=" + modeNumber(calc::vectorDot(a, b)),
                        "Ax=" + modeNumber(cross.x), "Ay=" + modeNumber(cross.y), "Az=" + modeNumber(cross.z),
                "|A|=" + modeNumber(calc::vectorMagnitude(a)), "|B|=" + modeNumber(calc::vectorMagnitude(b)),
                        "Angle=" + (angleOk ? modeNumber(fromRadians(angle, set_.angle)) : "ERROR")};
    } else if (modeInd_ == "TABLE") {
        struct TableContext { int function; calc::AngleMode angle; } context{modeOperation_, set_.angle};
        auto fn = [](double x, void *data, double &y) -> bool {
            auto *context = static_cast<TableContext *>(data);
            int function = context->function;
            if (function == 0) y = x;
            else if (function == 1) y = x * x;
            else {
                double radians = x;
                if (context->angle == calc::AngleMode::Degrees) radians *= 3.14159265358979323846 / 180.0;
                else if (context->angle == calc::AngleMode::Grads) radians *= 3.14159265358979323846 / 200.0;
                y = function == 2 ? std::sin(radians) : std::cos(radians);
            }
            return std::isfinite(y);
        };
        calc::TableResult table;
        if (!calc::makeTable(fn, &context, modeValues_[0], modeValues_[1], modeValues_[2], table)) modeResults_ = {"Math ERROR"};
        else {
            modeResults_.clear();
            for (size_t i = 0; i < table.count; ++i)
                modeResults_.push_back("x=" + modeNumber(table.rows[i].x) + " y=" + modeNumber(table.rows[i].y));
        }
    }
    modePage_ = 0;
}

bool Calculator::graphValueAt(double x, double &y) const {
    calc::Memory memory{};
    calc::MemoryValue &xValue = memory[static_cast<size_t>('X' - 'A')];
    xValue.defined = true;
    xValue.value = x;
    calc::EvalContext context;
    context.angle = set_.angle;
    context.memory = &memory;
    calc::EvalResult result = calc::evaluate(graphExpression_, context);
    if (!result.ok || result.complex || !std::isfinite(result.value)) return false;
    y = result.value;
    return true;
}

void Calculator::rebuildGraphSamples() {
    const int count = GRAPH_W - 2 * GRAPH_INSET;
    graphSamples_.assign(static_cast<size_t>(count), std::numeric_limits<double>::quiet_NaN());
    graphValidSamples_ = 0;
    for (int i = 0; i < count; ++i) {
        double x = graphXMin_ + (graphXMax_ - graphXMin_) * i / (count - 1);
        double y = 0.0;
        if (graphValueAt(x, y)) {
            graphSamples_[static_cast<size_t>(i)] = y;
            ++graphValidSamples_;
        }
    }
    graphTrace_ = std::clamp(graphTrace_, 0, count - 1);
}

void Calculator::graphKey(Key k) {
    if (k == Key::Ac) {
        scr_ = Screen::Calc;
        modeInd_.clear();
        graphError_.clear();
        ed_.reset();
        phase_ = Phase::Edit;
        shift_ = false;
        return;
    }
    if (scr_ == Screen::GraphEntry) {
        if (k == Key::Eq) {
            graphExpression_ = ed_.engine();
            int open = 0;
            for (char c : graphExpression_) {
                if (c == '(') ++open;
                else if (c == ')') --open;
            }
            for (int i = 0; i < open; ++i) graphExpression_ += ')';
            if (graphExpression_.empty()) { graphError_ = "Enter f(x) first"; return; }
            rebuildGraphSamples();
            if (graphValidSamples_ == 0) { graphError_ = "No plottable points"; return; }
            graphError_.clear();
            scr_ = Screen::GraphPlot;
            return;
        }
        if (k == Key::Del) { ed_.del(); graphError_.clear(); return; }
        if (k == Key::Left) { ed_.left(); graphError_.clear(); return; }
        if (k == Key::Right) { ed_.right(); graphError_.clear(); return; }
        if (k == Key::Up) { ed_.up(); return; }
        if (k == Key::Down) { ed_.down(); return; }
        if (k == Key::Shift) { shift_ = !shift_; return; }
        if (k == Key::Alpha) { alpha_ = !alpha_; shift_ = false; return; }
        if (k == Key::Hyp) { hyp_ = !hyp_; return; }
        bool sh = shift_, al = alpha_, hyp = hyp_;
        shift_ = alpha_ = hyp_ = false;
        editKey(k, sh, al, hyp);
        graphError_.clear();
        return;
    }

    if (k == Key::Eq || k == Key::Del) { scr_ = Screen::GraphEntry; graphError_.clear(); return; }
    if (k == Key::Shift) { shift_ = !shift_; return; }
    if (k == Key::Left || k == Key::Right) {
        int direction = k == Key::Right ? 1 : -1;
        if (shift_) {
            double offset = (graphXMax_ - graphXMin_) * 0.1 * direction;
            graphXMin_ += offset;
            graphXMax_ += offset;
            rebuildGraphSamples();
        } else graphTrace_ = std::clamp(graphTrace_ + direction, 0, static_cast<int>(graphSamples_.size()) - 1);
        shift_ = false;
        return;
    }
    if (k == Key::Up || k == Key::Down) {
        double offset = (graphYMax_ - graphYMin_) * 0.1 * (k == Key::Up ? 1.0 : -1.0);
        graphYMin_ += offset;
        graphYMax_ += offset;
        return;
    }
    if (k == Key::Plus || k == Key::Minus) {
        double factor = k == Key::Plus ? 0.8 : 1.25;
        double xCenter = (graphXMin_ + graphXMax_) / 2.0;
        double yCenter = (graphYMin_ + graphYMax_) / 2.0;
        double halfX = (graphXMax_ - graphXMin_) * factor / 2.0;
        double halfY = (graphYMax_ - graphYMin_) * factor / 2.0;
        graphXMin_ = xCenter - halfX; graphXMax_ = xCenter + halfX;
        graphYMin_ = yCenter - halfY; graphYMax_ = yCenter + halfY;
        rebuildGraphSamples();
    }
}

void Calculator::modeKey(Key k) {
    if (k == Key::Ac) { scr_ = Screen::Calc; return; }
    bool startsNumber = (k >= Key::N0 && k <= Key::N9) || k == Key::Dot || k == Key::Neg || k == Key::Minus;
    if (modeInd_ == "BASE") {
        if (k == Key::Alpha) { modeAlpha_ = !modeAlpha_; return; }
        if (modeAlpha_) {
            modeAlpha_ = false;
            if (k >= Key::N1 && k <= Key::N6 && set_.radix == calc::Radix::Hexadecimal)
                baseToken_.push_back((char)('A' + (int)k - (int)Key::N1));
            return;
        }
        if (k == Key::Del) { if (!baseToken_.empty()) baseToken_.pop_back(); return; }
        if (k == Key::Minus && baseHasFirst_) { baseOperation_ = 1; return; }
        if (k == Key::Neg || k == Key::Minus) {
            if (baseToken_.empty()) baseToken_ = "-";
            else if (baseToken_[0] == '-') baseToken_.erase(0, 1);
            return;
        }
        int operation = -1;
        if (k == Key::Plus) operation = 0;
        else if (k == Key::Minus) operation = 1;
        else if (k == Key::Mul) operation = 2;
        else if (k == Key::Div) operation = 3;
        else if (k == Key::BitAnd) operation = 4;
        else if (k == Key::BitOr) operation = 5;
        else if (k == Key::BitXor) operation = 6;
        else if (k == Key::BitXnor) operation = 7;
        else if (k == Key::BitNot) operation = 8;
        if (operation == 8) {
            int64_t value;
            if (calc::parseInteger(baseToken_, set_.radix, value)) {
                int64_t result;
                modeResults_ = calc::integerOperation(value, 0, calc::IntegerOperation::Not, result)
                    ? std::vector<std::string>{calc::formatInteger(result, set_.radix)}
                    : std::vector<std::string>{"Math ERROR"};
                modeHasResult_ = true;
            }
            return;
        }
        if (operation >= 0) {
            if (!baseHasFirst_) {
                if (calc::parseInteger(baseToken_, set_.radix, baseFirst_)) {
                    baseHasFirst_ = true; baseOperation_ = operation; baseToken_.clear();
                }
            } else baseOperation_ = operation;
            return;
        }
        if (k >= Key::N0 && k <= Key::N9) {
            char digit = (char)('0' + (int)k - (int)Key::N0);
            if ((unsigned)(digit - '0') < (unsigned)set_.radix) baseToken_.push_back(digit);
            return;
        }
        if (k == Key::Eq) {
            int64_t value;
            if (!calc::parseInteger(baseToken_, set_.radix, value)) return;
            if (!baseHasFirst_) { baseFirst_ = value; baseHasFirst_ = true; baseToken_.clear(); return; }
            int64_t result = 0;
            bool valid = baseOperation_ >= 0 && baseOperation_ <= 7
                && calc::integerOperation(baseFirst_, value, static_cast<calc::IntegerOperation>(baseOperation_), result);
            modeResults_ = {valid ? calc::formatInteger(result, set_.radix) : "Math ERROR"};
            modeHasResult_ = true; baseHasFirst_ = false; baseOperation_ = -1; baseToken_.clear();
            return;
        }
        if (startsNumber) modeHasResult_ = false;
        return;
    }
    if (k == Key::Del) {
        if (!modeInput_.empty()) modeInput_.pop_back();
        return;
    }
    if (modeHasResult_ && modeInd_ == "STAT" && startsNumber) {
        modeHasResult_ = false;
        modeResults_.clear();
    }
    if (modeHasResult_) {
        if (k == Key::Up) modePage_ = std::max(0, modePage_ - 1);
        else if (k == Key::Down) modePage_ = std::min((int)modeResults_.size() - 1, modePage_ + 1);
        else if (modeInd_ == "CMPLX" && (k == Key::Plus || k == Key::Minus || k == Key::Mul || k == Key::Div)) {
            modeOperation_ = k == Key::Plus ? 0 : k == Key::Minus ? 1 : k == Key::Mul ? 2 : 3;
            calc::Complex a{modeValues_[0], modeValues_[1]}, b{modeValues_[2], modeValues_[3]}, result;
            bool ok = true;
            if (modeOperation_ == 0) result = calc::complexAdd(a, b);
            else if (modeOperation_ == 1) result = calc::complexSubtract(a, b);
            else if (modeOperation_ == 2) result = calc::complexMultiply(a, b);
            else ok = calc::complexDivide(a, b, result);
            if (!ok) modeResults_ = {"Math ERROR"};
            else if (set_.complexFmt == Settings::ComplexFmt::Real) modeResults_ = {"Re=" + modeNumber(result.real)};
            else if (set_.complexFmt == Settings::ComplexFmt::Polar)
                modeResults_ = {"r=" + modeNumber(calc::complexMagnitude(result)),
                                "theta=" + modeNumber(fromRadians(calc::complexArgument(result), set_.angle))};
            else modeResults_ = {"Re=" + modeNumber(result.real), "Im=" + modeNumber(result.imag)};
            modePage_ = 0;
        } else if (k == Key::Eq && modeInd_ == "STAT") {
            modeResults_.clear(); modeHasResult_ = false;
        } else if (k == Key::Ac) { modeHasResult_ = false; modeValues_.clear(); }
        return;
    }
    if (k >= Key::N0 && k <= Key::N9) { modeInput_ += (char)('0' + (int)k - (int)Key::N0); return; }
    if (k == Key::Dot && modeInput_.find('.') == std::string::npos) { modeInput_ += '.'; return; }
    if (k == Key::Neg || k == Key::Minus) {
        if (modeInput_.empty()) modeInput_ = "-";
        else if (modeInput_[0] == '-') modeInput_.erase(0, 1);
        return;
    }
    if (k == Key::Eq) finishModeEntry();
}

void Calculator::editKey(Key k, bool sh, bool al, bool hyp) {
    using K = Key;
    auto dig = [&](char c) { ed_.type(std::string(1, c)); };
    if (al) {
        char variable = 0;
        if (k >= K::N1 && k <= K::N6) variable = (char)('A' + (int)k - (int)K::N1);
        else if (k == K::N7) variable = 'X';
        else if (k == K::N8) variable = 'Y';
        else if (k == K::N9) variable = 'M';
        if (variable) { ed_.type(std::string(1, variable), std::string(1, variable)); return; }
    }
    switch (k) {
        case K::N0: dig('0'); break; case K::N1: if (al) ed_.type("A", "A"); else dig('1'); break;
        case K::N2: if (al) ed_.type("B", "B"); else dig('2'); break;
        case K::N3: if (al) ed_.type("C", "C"); else dig('3'); break;
        case K::N4: if (al) ed_.type("D", "D"); else dig('4'); break;
        case K::N5: if (al) ed_.type("E", "E"); else dig('5'); break;
        case K::N6: if (al) ed_.type("F", "F"); else dig('6'); break;
        case K::N7: if (al) ed_.type("X", "X"); else dig('7'); break;
        case K::N8: if (al) ed_.type("Y", "Y"); else dig('8'); break;
        case K::N9: if (al) ed_.type("M", "M"); else dig('9'); break;
        case K::Dot: dig('.'); break;
        case K::Plus: ed_.type("+"); break;
        case K::Minus: case K::Neg: ed_.type("-"); break;
        case K::Mul: ed_.type(MUL, "*"); break;
        case K::Div: ed_.type(DIV, "/"); break;
        case K::LPar: ed_.type("("); break;
        case K::RPar: ed_.type(")"); break;
        case K::Frac: if (sh) ed_.mixed(); else ed_.frac(); break;
        case K::Sqrt: if (sh) ed_.rootT("3"); else ed_.sqrtT(); break;
        case K::Sq: ed_.powT(sh ? "3" : "2"); break;
        case K::Pow: if (sh) ed_.rootT(""); else ed_.powT(""); break;
        case K::Recip: if (sh) ed_.type("!"); else ed_.powT("-1"); break;
        case K::Log: if (sh) { ed_.type("1"); ed_.type("0"); ed_.powT(""); } else ed_.typeFunc("log", "log("); break;
        case K::LogB: ed_.logB(); break;
        case K::Ln:  if (sh) { ed_.type("e"); ed_.powT(""); } else ed_.typeFunc("ln", "ln("); break;
        case K::Sin:
            if (hyp) ed_.typeFunc("sinh", "sinh(");
            else if (sh) ed_.typeFunc("sin", "asin(", "-1");
            else ed_.typeFunc("sin", "sin(");
            break;
        case K::Cos:
            if (hyp) ed_.typeFunc("cosh", "cosh(");
            else if (sh) ed_.typeFunc("cos", "acos(", "-1");
            else ed_.typeFunc("cos", "cos(");
            break;
        case K::Tan:
            if (hyp) ed_.typeFunc("tanh", "tanh(");
            else if (sh) ed_.typeFunc("tan", "atan(", "-1");
            else ed_.typeFunc("tan", "tan(");
            break;
        case K::Abs: ed_.absT(); break;
        case K::Exp10: if (sh) ed_.type(PI, "pi"); else if (al) ed_.type("e"); else ed_.exp10(); break;
        case K::Ans: if (sh) storePending_ = true; else ed_.type("Ans", "Ans"); break;
        case K::Comma: ed_.type(","); break;
        case K::NPr: ed_.type("P", "P"); break;
        case K::NCr: ed_.type("C", "C"); break;
        case K::Mod: ed_.typeFunc("mod", "mod("); break;
        case K::Pct: ed_.type("%"); break;
        case K::Floor: ed_.typeFunc("floor", "floor("); break;
        case K::Ceil: ed_.typeFunc("ceil", "ceil("); break;
        case K::Imaginary: ed_.type("i", "i"); break;
        case K::Arg: ed_.typeFunc("arg", "arg("); break;
        case K::Gcd: ed_.typeFunc("gcd", "gcd("); break;
        case K::Lcm: ed_.typeFunc("lcm", "lcm("); break;
        case K::Asinh: ed_.typeFunc("asinh", "asinh("); break;
        case K::Acosh: ed_.typeFunc("acosh", "acosh("); break;
        case K::Atanh: ed_.typeFunc("atanh", "atanh("); break;
        case K::Eq: equals(); break;
        case K::SD: break;
        case K::Del: ed_.del(); break;
        case K::Ac: ed_.reset(); break;
        case K::Left: ed_.left(); break;
        case K::Right: ed_.right(); break;
        case K::Up: if (!ed_.up() && ed_.empty() && !hist_.empty()) hidx_ = (int)hist_.size() - 1; break;
        case K::Down: ed_.down(); break;
        default: break;
    }
}

void Calculator::menuKey(Key k) {
    int n = -1;
    if (k >= Key::N1 && k <= Key::N9) n = (int)k - (int)Key::N0;
    if (k == Key::Ac) { scr_ = Screen::Calc; return; }
    switch (scr_) {
        case Screen::Mode:
            if (k == Key::Down) return;
            if (n >= 1 && n <= 9) {
                if (n == 1) { modeInd_.clear(); scr_ = Screen::Calc; }
                else if (n == 4) { modeInd_ = "BASE"; scr_ = Screen::BaseNSel; }
                else if (n == 3) { modeInd_ = "STAT"; scr_ = Screen::StatType; }
                else if (n == 6) { modeInd_ = "MAT"; scr_ = Screen::MatrixOp; }
                else if (n == 7) { modeInd_ = "TABLE"; scr_ = Screen::TableFn; }
                else if (n == 9) { modeInd_ = "GRAPH"; scr_ = Screen::GraphEntry; }
                else {
                    static const char *ind[] = {"", "", "CMPLX", "STAT", "", "EQN", "MAT", "", "VCT"};
                    modeInd_ = ind[n]; scr_ = Screen::ModeWork;
                }
                modeValues_.clear(); modeResults_.clear(); modeInput_.clear();
                modePage_ = 0; modeOperation_ = modeInd_ == "TABLE" ? 1 : -1;
                modeHasResult_ = false;
                if (modeInd_ == "STAT") modeStats_.clear();
                if (modeInd_ == "GRAPH") {
                    graphError_.clear();
                    graphXMin_ = -10.0; graphXMax_ = 10.0;
                    graphYMin_ = -5.0; graphYMax_ = 5.0;
                    graphTrace_ = 0;
                }
                ed_.reset(); phase_ = Phase::Edit;
            }
            break;
        case Screen::Setup1:
            if (k == Key::Down) { scr_ = Screen::Setup2; return; }
            if (n == 1) scr_ = Screen::MathIOSel;
            else if (n == 2) { set_.mathIO = false; scr_ = Screen::Calc; }
            else if (n == 3) { set_.angle = calc::AngleMode::Degrees; scr_ = Screen::Calc; }
            else if (n == 4) { set_.angle = calc::AngleMode::Radians; scr_ = Screen::Calc; }
            else if (n == 5) { set_.angle = calc::AngleMode::Grads; scr_ = Screen::Calc; }
            else if (n == 6) { promptSci_ = false; scr_ = Screen::Prompt; }
            else if (n == 7) { promptSci_ = true; scr_ = Screen::Prompt; }
            else if (n == 8) scr_ = Screen::NormSel;
            break;
        case Screen::Setup2:
            if (k == Key::Up) { scr_ = Screen::Setup1; return; }
            if (n == 1) { set_.mixedFractions = true; scr_ = Screen::Calc; }
            else if (n == 2) { set_.mixedFractions = false; scr_ = Screen::Calc; }
            else if (n == 3) scr_ = Screen::ComplexSel;
            else if (n == 4) scr_ = Screen::StatSel;
            else if (n == 5) scr_ = Screen::DispSel;
            else if (n == 6) scr_ = Screen::ContrastSel;
            else if (n == 7) scr_ = Screen::BatterySel;
            break;
        case Screen::MathIOSel:
            if (n == 1) { set_.mathIO = true; scr_ = Screen::Calc; }
            else if (n == 2) { set_.mathIO = false; scr_ = Screen::Calc; }
            break;
        case Screen::NormSel:
            if (n == 1) { set_.fmt = Settings::Fmt::Norm1; scr_ = Screen::Calc; }
            else if (n == 2) { set_.fmt = Settings::Fmt::Norm2; scr_ = Screen::Calc; }
            break;
        case Screen::BaseNSel:
            if (n == 1) set_.radix = calc::Radix::Decimal;
            else if (n == 2) set_.radix = calc::Radix::Hexadecimal;
            else if (n == 3) set_.radix = calc::Radix::Binary;
            else if (n == 4) set_.radix = calc::Radix::Octal;
            else break;
            modeValues_.clear(); modeResults_.clear(); modeInput_.clear(); baseToken_.clear();
            baseHasFirst_ = false; modeHasResult_ = false; modeAlpha_ = false;
            scr_ = Screen::ModeWork;
            break;
        case Screen::ComplexSel:
            if (n == 1) set_.complexFmt = Settings::ComplexFmt::Rectangular;
            else if (n == 2) set_.complexFmt = Settings::ComplexFmt::Polar;
            else if (n == 3) set_.complexFmt = Settings::ComplexFmt::Real;
            else break;
            scr_ = Screen::Calc;
            break;
        case Screen::StatSel:
            if (n == 1) set_.statFrequency = true;
            else if (n == 2) set_.statFrequency = false;
            else break;
            scr_ = Screen::Calc;
            break;
        case Screen::StatType:
            if (n == 1) set_.statBivariate = false;
            else if (n == 2) set_.statBivariate = true;
            else break;
            modeInd_ = "STAT"; modeStats_.clear(); modeValues_.clear(); modeResults_.clear(); modeInput_.clear();
            scr_ = Screen::ModeWork;
            break;
        case Screen::MatrixOp:
            if (n < 1 || n > 4) break;
            modeOperation_ = n - 1;
            modeInd_ = "MAT"; modeValues_.clear(); modeResults_.clear(); modeInput_.clear();
            modeHasResult_ = false; modePage_ = 0;
            scr_ = Screen::ModeWork;
            break;
        case Screen::TableFn:
            if (n < 1 || n > 4) break;
            modeOperation_ = n - 1;
            modeInd_ = "TABLE"; modeValues_.clear(); modeResults_.clear(); modeInput_.clear();
            modeHasResult_ = false; modePage_ = 0;
            scr_ = Screen::ModeWork;
            break;
        case Screen::DispSel:
            if (n == 1) set_.decimalComma = false;
            else if (n == 2) set_.decimalComma = true;
            else break;
            scr_ = Screen::Calc;
            break;
        case Screen::ContrastSel:
            if (k == Key::Left || k == Key::Down) set_.contrast = std::max(0, set_.contrast - 5);
            else if (k == Key::Right || k == Key::Up) set_.contrast = std::min(100, set_.contrast + 5);
            else if (k == Key::Eq || k == Key::Ac) scr_ = Screen::Calc;
            break;
        case Screen::BatterySel:
            if (n == 1) { set_.showBatteryPlaceholder = true; scr_ = Screen::Calc; }
            else if (n == 2) { set_.showBatteryPlaceholder = false; scr_ = Screen::Calc; }
            break;
        case Screen::Prompt:
            if (k >= Key::N0 && k <= Key::N9) {
                set_.digits = (int)k - (int)Key::N0;
                set_.fmt = promptSci_ ? Settings::Fmt::Sci : Settings::Fmt::Fix;
                scr_ = Screen::Calc;
            }
            break;
        default: break;
    }
}

// ------------------------------------------------------------------ drawing
void Calculator::drawIndicators(Display &d) {
    using F = Display::Font;
    const int by = 18;
    int x = 8;
    auto indicator = [&](const std::string &value) {
        d.text(x, by, value, F::Small);
        x += d.textWidth(value, F::Small) + 12;
    };
    if (shift_) indicator("SHIFT");
    if (alpha_) indicator("ALPHA");
    if (storePending_) indicator("STO");
    if (hyp_) indicator("HYP");
    if (!modeInd_.empty()) indicator(modeInd_);
    const char *ang = set_.angle == calc::AngleMode::Degrees ? "D" : set_.angle == calc::AngleMode::Radians ? "R" : "G";
    indicator(ang);
    if (set_.fmt == Settings::Fmt::Fix) indicator("FIX");
    if (set_.fmt == Settings::Fmt::Sci) indicator("SCI");
    if (set_.decimalComma) indicator(",");
    if (set_.mathIO) indicator("Math");
    if (engine_.recall('M')) indicator("M");
    if (set_.showBatteryPlaceholder) indicator("100%");
    else {
        bool up = !hist_.empty() && (hidx_ == -1 || hidx_ > 0);
        bool dn = hidx_ >= 0;
        if (up) d.glyph(Display::width() - 24, 8, UP_ROWS, 3);
        if (dn) d.glyph(Display::width() - 12, 8, DN_ROWS, 3);
    }
}

void Calculator::drawError(Display &d) {
    using F = Display::Font;
    d.text(12, 88, err_, F::Main);
    menuText(d, 12, 150, "[AC]  :Cancel");
    d.text(12, 205, "[", F::Main); d.glyph(22, 200, TL, 5); d.text(30, 205, "][", F::Main); d.glyph(48, 200, TR, 5);
    menuText(d, 56, 205, "]:Goto");
}

void Calculator::drawMenu(Display &d) {
    auto line = [&](int i, const char *a, const char *b = nullptr) {
        int by = 52 + i * 36;
        menuText(d, 12, by, a);
        if (b) menuText(d, Display::width() / 2 + 8, by, b);
    };
    switch (scr_) {
        case Screen::Mode:
            line(0, "1:COMP", "2:CMPLX"); line(1, "3:STAT", "4:BASE-N"); line(2, "5:EQN", "6:MATRIX"); line(3, "7:TABLE", "8:VECTOR"); line(4, "9:GRAPH");
            break;
        case Screen::Setup1:
            line(0, "1:MthIO 2:LineIO"); line(1, "3:Deg 4:Rad 5:Gra"); line(2, "6:Fix 7:Sci 8:Norm");
            d.glyph(Display::width() - 16, Display::height() - 14, DN_ROWS, 3);
            break;
        case Screen::Setup2:
            line(0, "1:ab/c  2:d/c"); line(1, "3:CMPLX 4:STAT"); line(2, "5:Disp  6:CONT"); line(3, "7:Battery");
            d.glyph(Display::width() - 16, 28, UP_ROWS, 3);
            break;
        case Screen::MathIOSel: line(0, "1:MathO"); line(1, "2:LineO"); break;
        case Screen::StatType: line(0, "1:1-Var"); line(1, "2:A+BX"); break;
        case Screen::MatrixOp: line(0, "1:det 2:inv"); line(1, "3:A+B 4:AxB"); break;
        case Screen::TableFn: line(0, "1:X 2:X^2"); line(1, "3:sin 4:cos"); break;
        case Screen::BaseNSel:
            line(0, "1:DEC 2:HEX"); line(1, "3:BIN 4:OCT");
            break;
        case Screen::ComplexSel: line(0, "1:a+bi 2:r<theta"); line(1, "3:Real"); break;
        case Screen::StatSel: line(0, "1:FreqOn"); line(1, "2:FreqOff"); break;
        case Screen::DispSel: line(0, "1:Dot"); line(1, "2:Comma"); break;
        case Screen::ContrastSel: {
            line(0, "Contrast");
            std::string value = std::to_string(set_.contrast);
            d.text(Display::width() / 2, 130, value, Display::Font::Main);
            d.glyph(16, 180, TL, 5); d.glyph(Display::width() - 24, 180, TR, 5);
            break;
        }
        case Screen::BatterySel:
            line(0, "1:Show 100%"); line(1, "2:Hide");
            break;
        case Screen::NormSel: line(0, "1:Norm1"); line(1, "2:Norm2"); break;
        case Screen::Prompt: line(0, promptSci_ ? "Sci 0~9?" : "Fix 0~9?"); break;
        default: break;
    }
}

void Calculator::drawModeWork(Display &d) {
    using F = Display::Font;
    const int left = 12;
    d.text(left, 38, modeInd_, F::Main);
    std::string prompt;
    if (modeInd_ == "BASE") {
        const char *radix = set_.radix == calc::Radix::Binary ? "BIN" : set_.radix == calc::Radix::Octal ? "OCT"
                         : set_.radix == calc::Radix::Hexadecimal ? "HEX" : "DEC";
        d.text(left, 70, radix, F::Small);
        d.text(left, 128, modeHasResult_ ? (modeResults_.empty() ? "" : modeResults_[0])
                                      : baseToken_.empty() ? "_" : baseToken_, F::Main);
        d.text(left, 212, "AND OR XOR XNOR NOT", F::Small);
        return;
    } else if (modeInd_ == "CMPLX") {
        static const char *fields[] = {"A real", "A imag", "B real", "B imag", "Select + - * /"};
        prompt = modeValues_.size() < 4 ? fields[modeValues_.size()] : fields[4];
    } else if (modeInd_ == "STAT") {
        size_t rowSize = (set_.statBivariate ? 2u : 1u) + (set_.statFrequency ? 1u : 0u);
        size_t entry = modeValues_.size() % rowSize;
        prompt = set_.statBivariate ? (entry == 0 ? "X value" : set_.statFrequency && entry == 2 ? "Frequency" : "Y value")
                : (set_.statFrequency && entry == 1 ? "Frequency" : "X value");
    }
    else if (modeInd_ == "EQN") prompt = modeValues_.size() < 3 ? "Enter a, b, c" : "Roots";
    else if (modeInd_ == "MAT") {
        size_t cell = modeValues_.size() % 4;
        const char *cells[] = {"11", "12", "21", "22"};
        prompt = std::string(modeOperation_ >= 2 && modeValues_.size() >= 4 ? "B " : "A ") + cells[cell];
    } else if (modeInd_ == "VCT") {
        static const char *fields[] = {"A.x", "A.y", "A.z", "B.x", "B.y", "B.z"};
        prompt = modeValues_.size() < 6 ? fields[modeValues_.size()] : "Dot / cross / angle";
    }
    else if (modeInd_ == "TABLE") {
        const char *function = modeOperation_ == 0 ? "X" : modeOperation_ == 1 ? "X^2" : modeOperation_ == 2 ? "sin(X)" : "cos(X)";
        prompt = std::string("f=") + function + " start/end/step";
    }
    d.text(left, 76, prompt, F::Small);
    if (!modeHasResult_) {
        std::string input = modeInput_.empty() ? "_" : modeInput_;
        d.text(left, 132, input, F::Main);
        d.text(left, 212, "Enter value: =", F::Small);
        return;
    }
    if (modeResults_.empty()) {
        d.text(left, 132, "Select operation", F::Main);
        d.text(left, 212, "+  -  *  /", F::Small);
        return;
    }
    int lastPage = (int)modeResults_.size() - 1;
    modePage_ = std::clamp(modePage_, 0, std::max(0, lastPage));
    std::string shown = modeResults_[(size_t)modePage_];
    if (set_.decimalComma) std::replace(shown.begin(), shown.end(), '.', ',');
    d.text(left, 132, shown, F::Main);
    if (modeResults_.size() > 1) {
        d.text(left, 212, "Up/Down", F::Small);
        d.text(Display::width() - 72, 212, std::to_string(modePage_ + 1) + "/" + std::to_string(modeResults_.size()), F::Small);
    } else d.text(left, 212, modeInd_ == "STAT" ? "=: next sample" : "AC: exit", F::Small);
}

void Calculator::drawGraphEntry(Display &d, bool cursorOn) {
    using F = Display::Font;
    d.text(12, 48, "f(x) =", F::Main);
    nat::Renderer renderer(d);
    renderer.cursor = &ed_.cur;
    renderer.dry = true;
    renderer.draw(ed_.root, 64, 48);
    renderer.dry = false;
    renderer.curFound = false;
    d.setClip(64, 36, Display::width() - 12, 132);
    renderer.draw(ed_.root, 64, 84);
    d.clearClip();
    if (cursorOn && renderer.curFound)
        d.vLine(renderer.curX, renderer.curTop, renderer.curBot - renderer.curTop + 1);
    if (!graphError_.empty()) d.text(12, 164, graphError_, F::Small);
    d.text(12, 212, "Enter: plot   DEL: edit   AC: exit", F::Small);
}

void Calculator::drawGraph(Display &d) {
    using F = Display::Font;
    const int left = GRAPH_X + GRAPH_INSET;
    const int top = GRAPH_Y + GRAPH_INSET;
    const int plotWidth = GRAPH_W - 2 * GRAPH_INSET - 1;
    const int plotHeight = GRAPH_H - 2 * GRAPH_INSET - 1;
    const int right = left + plotWidth;
    const int bottom = top + plotHeight;
    auto mapX = [&](double x) { return left + (int)std::lround((x - graphXMin_) / (graphXMax_ - graphXMin_) * plotWidth); };
    auto mapY = [&](double y) { return bottom - (int)std::lround((y - graphYMin_) / (graphYMax_ - graphYMin_) * plotHeight); };
    auto gridStep = [](double span) {
        double raw = span / 10.0;
        double power = std::pow(10.0, std::floor(std::log10(raw)));
        double unit = raw / power;
        return (unit <= 1.0 ? 1.0 : unit <= 2.0 ? 2.0 : unit <= 5.0 ? 5.0 : 10.0) * power;
    };

    d.text(12, 42, "GRAPH", F::Main);
    d.text(82, 42, "f(x)=", F::Small);
    d.text(112, 42, graphExpression_, F::Small);
    d.frame(GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H);

    double xStep = gridStep(graphXMax_ - graphXMin_);
    double yStep = gridStep(graphYMax_ - graphYMin_);
    for (double x = std::ceil(graphXMin_ / xStep) * xStep; x <= graphXMax_; x += xStep) {
        int px = mapX(x);
        if (px < left || px > right) continue;
        for (int py = top; py <= bottom; py += 4) d.pixel(px, py);
    }
    for (double y = std::ceil(graphYMin_ / yStep) * yStep; y <= graphYMax_; y += yStep) {
        int py = mapY(y);
        if (py < top || py > bottom) continue;
        for (int px = left; px <= right; px += 4) d.pixel(px, py);
    }
    if (graphXMin_ <= 0.0 && graphXMax_ >= 0.0) d.vLine(mapX(0.0), top, plotHeight + 1);
    if (graphYMin_ <= 0.0 && graphYMax_ >= 0.0) d.hLine(left, mapY(0.0), plotWidth + 1);

    int previousX = -1, previousY = -1;
    for (size_t i = 0; i < graphSamples_.size(); ++i) {
        double y = graphSamples_[i];
        if (!std::isfinite(y) || y < graphYMin_ || y > graphYMax_) { previousX = previousY = -1; continue; }
        int px = left + (int)i;
        int py = mapY(y);
        if (previousX >= 0 && std::abs(py - previousY) < plotHeight * 2 / 3) d.line(previousX, previousY, px, py);
        else d.pixel(px, py);
        previousX = px; previousY = py;
    }

    if (!graphSamples_.empty()) {
        graphTrace_ = std::clamp(graphTrace_, 0, static_cast<int>(graphSamples_.size()) - 1);
        int traceX = left + graphTrace_;
        for (int py = top; py <= bottom; py += 5) d.vLine(traceX, py, std::min(2, bottom - py + 1));
        double x = graphXMin_ + (graphXMax_ - graphXMin_) * graphTrace_ / (graphSamples_.size() - 1);
        double y = graphSamples_[static_cast<size_t>(graphTrace_)];
        std::string readout = "x=" + modeNumber(x) + "  y=" + (std::isfinite(y) ? modeNumber(y) : "undefined");
        d.text(12, 222, readout, F::Small);
    }
    d.text(12, 238, "Left/Right trace  Up/Down pan Y  +/- zoom  SHIFT+arrows pan X  = edit", F::Tiny);
}

void Calculator::drawCalc(Display &d, bool cursorOn) {
    nat::Renderer r(d);
    const nat::Seq *src; const calc::EvalResult *rs; bool showRes, dec;
    if (hidx_ >= 0) { src = &hist_[hidx_]->expr; rs = &hist_[hidx_]->res; showRes = true; dec = false; }
        else { src = phase_ == Phase::Result && showFeedbackExpr_ ? &feedbackExpr_ : &ed_.root;
            rs = &res_; showRes = phase_ == Phase::Result; dec = showDec_; }
    const bool editing = phase_ == Phase::Edit && hidx_ < 0;

    const int X0 = 12, VW = Display::width() - 24, TOP = 48;
    nat::Box ib = r.measure(*src);
    int inBase = TOP + std::max(ib.asc, r.fontAsc(0));

    // result geometry first, so the input can be clipped above it
    nat::Seq rseq; nat::Box rb; int resBase = 0, resTop = Display::height() - 12;
    if (showRes) {
        buildResult(rseq, *rs, dec);
        rb = r.measure(rseq);
        resBase = Display::height() - 16 - rb.desc; resTop = resBase - rb.asc;
    }

    // horizontal scroll so the cursor stays visible
    int total = ib.w + 2;
    if (editing) {
        r.cursor = &ed_.cur; r.dry = true;
        r.draw(*src, X0, inBase);
        r.dry = false;
        if (r.curFound) {
            int rel = r.curX - X0;
            if (rel - scrollX_ > VW - 4) scrollX_ = rel - (VW - 4);
            if (rel - scrollX_ < 0) scrollX_ = rel;
        }
    }
    scrollX_ = std::clamp(scrollX_, 0, std::max(0, total - VW));
    if (!editing) scrollX_ = 0;
    r.curFound = false;
    r.cursor = editing ? &ed_.cur : nullptr;

    d.setClip(X0, 36, X0 + VW, std::max(37, showRes ? resTop - 12 : Display::height() - 12));
    r.draw(*src, X0 - scrollX_, inBase);
    d.clearClip();
    int yc = TOP + 16;
    if (scrollX_ > 0) r.arrowLeft(4, yc);
    if (total - scrollX_ > VW) r.arrowRight(Display::width() - 12, yc);
    if (editing && cursorOn && r.curFound) {
        d.vLine(std::max(0, r.curX), r.curTop, r.curBot - r.curTop + 1);
    }

    resMaxScroll_ = 0;
    if (showRes) {
        int rx;
        if (rb.w <= VW + 2) rx = Display::width() - 12 - rb.w;
        else {
            int maxScroll = rb.w - VW;
            resMaxScroll_ = maxScroll;
            resScroll_ = std::clamp(resScroll_, 0, maxScroll);
            rx = X0 - resScroll_;
            if (resScroll_ > 0) r.arrowLeft(4, resBase - 3);
            if (resScroll_ < maxScroll) r.arrowRight(Display::width() - 12, resBase - 3);
        }
        d.setClip(0, 36, Display::width(), Display::height());
        r.cursor = nullptr;
        r.draw(rseq, rx, resBase);
        d.clearClip();
    }
}

void Calculator::draw(Display &d, bool cursorOn) {
    d.setContrast(set_.contrast);
    d.clear();
    drawIndicators(d);
    if (scr_ == Screen::GraphEntry) drawGraphEntry(d, cursorOn);
    else if (scr_ == Screen::GraphPlot) drawGraph(d);
    else if (scr_ == Screen::ModeWork) drawModeWork(d);
    else if (scr_ != Screen::Calc) drawMenu(d);
    else if (phase_ == Phase::Error) drawError(d);
    else drawCalc(d, cursorOn);
    d.present();
}

} // namespace app
