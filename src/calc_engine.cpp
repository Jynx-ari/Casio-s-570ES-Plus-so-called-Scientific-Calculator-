#include "calc_engine.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <complex>
#include <numeric>
#include <stdexcept>

namespace calc {
namespace {

struct CalcError : std::runtime_error {
    explicit CalcError(const char *m) : std::runtime_error(m) {}
};
[[noreturn]] void mathErr() { throw CalcError("Math ERROR"); }
[[noreturn]] void syntaxErr() { throw CalcError("Syntax ERROR"); }

// ---------------------------------------------------------------- exact math
constexpr __int128 LIM = 1000000000000LL;   // 1e12: beyond this we give up exactness

__int128 gcd128(__int128 a, __int128 b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b) { __int128 t = a % b; a = b; b = t; }
    return a;
}
bool mkRat(__int128 n, __int128 d, Rational &r) {
    if (d == 0) return false;
    if (d < 0) { n = -n; d = -d; }
    __int128 g = gcd128(n, d);
    if (g > 1) { n /= g; d /= g; }
    if (n > LIM || n < -LIM || d > LIM) return false;
    r.n = (int64_t)n; r.d = (int64_t)d;
    return true;
}
bool ratAdd(const Rational &a, const Rational &b, Rational &r) {
    return mkRat((__int128)a.n * b.d + (__int128)b.n * a.d, (__int128)a.d * b.d, r);
}
bool ratMul(const Rational &a, const Rational &b, Rational &r) {
    return mkRat((__int128)a.n * b.n, (__int128)a.d * b.d, r);
}
bool ratDiv(const Rational &a, const Rational &b, Rational &r) {
    if (b.n == 0) return false;
    return mkRat((__int128)a.n * b.d, (__int128)a.d * b.n, r);
}

void splitSquare(int64_t r, int64_t &k, int64_t &m) {      // r = k^2 * m, m squarefree
    k = 1; m = r;
    for (int64_t p = 2; p * p <= m; ++p)
        while (m % (p * p) == 0) { m /= p * p; k *= p; }
}

bool normalize(Exact &x) {
    std::sort(x.terms.begin(), x.terms.end(), [](const Term &a, const Term &b) {
        if (a.pi != b.pi) return a.pi < b.pi;
        return a.radicand < b.radicand;
    });
    std::vector<Term> out;
    for (auto &t : x.terms) {
        if (!out.empty() && out.back().pi == t.pi && out.back().radicand == t.radicand) {
            if (!ratAdd(out.back().c, t.c, out.back().c)) return false;
        } else out.push_back(t);
    }
    out.erase(std::remove_if(out.begin(), out.end(), [](const Term &t) { return t.c.n == 0; }), out.end());
    x.terms = std::move(out);
    return true;
}
bool exAdd(const Exact &a, const Exact &b, Exact &r, bool negB = false) {
    r = a;
    for (Term t : b.terms) { if (negB) t.c.n = -t.c.n; r.terms.push_back(t); }
    return normalize(r);
}
bool exMul(const Exact &a, const Exact &b, Exact &r) {
    r.terms.clear();
    for (auto &x : a.terms)
        for (auto &y : b.terms) {
            if (x.pi && y.pi) return false;                 // pi^2 not representable
            __int128 rad = (__int128)x.radicand * y.radicand;
            if (rad > LIM) return false;
            int64_t k, m; splitSquare((int64_t)rad, k, m);
            Term t; t.radicand = m; t.pi = x.pi || y.pi;
            Rational c;
            if (!ratMul(x.c, y.c, c)) return false;
            if (!ratMul(c, Rational{k, 1}, t.c)) return false;
            r.terms.push_back(t);
        }
    return normalize(r);
}
bool exDiv(const Exact &a, const Exact &b, Exact &r) {
    if (b.terms.size() != 1) return false;                  // no rationalising of sums
    const Term &t = b.terms[0];
    Exact inv;                                              // 1 / (c sqrt(r) pi^p)
    Term u; Rational cr;
    if (!ratMul(t.c, Rational{t.radicand, 1}, cr)) return false;   // c*r
    if (!ratDiv(Rational{1, 1}, cr, u.c)) return false;            // 1/(c r)
    u.radicand = t.radicand; u.pi = false;                          // * sqrt(r)
    inv.terms.push_back(u);
    Exact prod;
    if (t.pi) {                                             // divide out one pi
        Exact a2 = a;
        for (auto &x : a2.terms) { if (!x.pi) return false; x.pi = false; }
        return exMul(a2, inv, r);
    }
    return exMul(a, inv, r);
}
Exact exRat(const Rational &q) {
    Exact e; if (q.n != 0) e.terms.push_back(Term{q, 1, false}); return e;
}

struct Val { double d = 0; bool ex = false; Exact x; };
Val inexact(double d) { Val v; v.d = d; return v; }
Val fromRat(const Rational &q) { Val v; v.d = (double)q.n / q.d; v.ex = true; v.x = exRat(q); return v; }

Val vAdd(const Val &a, const Val &b, bool sub) {
    Val r; r.d = sub ? a.d - b.d : a.d + b.d;
    if (a.ex && b.ex) r.ex = exAdd(a.x, b.x, r.x, sub);
    return r;
}
Val vMul(const Val &a, const Val &b) {
    Val r; r.d = a.d * b.d;
    if (a.ex && b.ex) r.ex = exMul(a.x, b.x, r.x);
    return r;
}
Val vDiv(const Val &a, const Val &b) {
    if (b.d == 0.0) mathErr();
    Val r; r.d = a.d / b.d;
    if (a.ex && b.ex) r.ex = exDiv(a.x, b.x, r.x);
    return r;
}
bool valRational(const Val &v, Rational &q) { return v.ex && exactIsRational(v.x, q); }

Val vNeg(const Val &a) { return vMul(a, fromRat(Rational{-1, 1})); }

Val vSqrt(const Val &a) {
    if (a.d < 0) mathErr();
    Val r; r.d = std::sqrt(a.d);
    Rational q;
    if (valRational(a, q) || (a.ex && a.x.terms.empty())) {
        if (a.x.terms.empty()) { r.ex = true; return r; }
        __int128 pq = (__int128)q.n * q.d;
        if (pq <= LIM) {
            int64_t k, m; splitSquare((int64_t)pq, k, m);
            Term t; t.radicand = m; t.pi = false;
            if (mkRat(k, q.d, t.c)) { r.ex = true; r.x.terms.push_back(t); normalize(r.x); }
        }
    }
    return r;
}

Val vPow(const Val &a, const Val &b) {
    double d = std::pow(a.d, b.d);
    if (std::isnan(d) || std::isinf(d)) mathErr();
    Val r = inexact(d);
    Rational e;
    if (a.ex && valRational(b, e)) {
        if (e.d == 1 && std::llabs(e.n) <= 20) {
            Val acc = fromRat(Rational{1, 1});
            for (int i = 0; i < std::llabs(e.n) && acc.ex; ++i) acc = vMul(acc, a);
            if (acc.ex) {
                if (e.n < 0) { if (a.d == 0) mathErr(); acc = vDiv(fromRat(Rational{1, 1}), acc); }
                if (acc.ex) { acc.d = d; return acc; }
            }
        } else if (e.n == 1 && e.d == 2 && a.d >= 0) {
            Val s = vSqrt(a); s.d = d; return s;
        }
    }
    return r;
}

// ---------------------------------------------------------------- tokenizer
enum class T { Num, Ident, Op, LP, RP, Comma, Bang, Pct, End };
struct Tok { T type; std::string text; double num = 0; bool ratOk = false; Rational rat; };

std::vector<Tok> tokenize(const std::string &s) {
    std::vector<Tok> out; size_t i = 0, n = s.size();
    while (i < n) {
        char c = s[i];
        if (isspace((unsigned char)c)) { ++i; continue; }
        if (isdigit((unsigned char)c) || c == '.') {
            size_t st = i; bool dot = false;
            while (i < n && (isdigit((unsigned char)s[i]) || s[i] == '.')) {
                if (s[i] == '.') { if (dot) break; dot = true; }
                ++i;
            }
            bool sci = false; int ex = 0;
            if (i < n && s[i] == 'E') {
                size_t sv = i; ++i; bool neg = false;
                if (i < n && (s[i] == '+' || s[i] == '-')) { neg = s[i] == '-'; ++i; }
                if (i < n && isdigit((unsigned char)s[i])) {
                    while (i < n && isdigit((unsigned char)s[i])) { ex = ex * 10 + (s[i] - '0'); if (ex > 400) ex = 400; ++i; }
                    if (neg) ex = -ex;
                    sci = true;
                } else i = sv;
            }
            Tok t; t.type = T::Num; t.text = s.substr(st, i - st);
            try { t.num = std::stod(t.text); } catch (...) { syntaxErr(); }
            // exact only for plain integers (decimal input => decimal result, like the real unit)
            if (!dot && !sci && t.text.size() <= 15) {
                t.ratOk = mkRat(std::stoll(t.text), 1, t.rat);
            } else if (!dot && sci && ex >= 0 && ex <= 9 && t.text.find('E') > 0) {
                t.ratOk = false;
            }
            out.push_back(t); continue;
        }
        if (isalpha((unsigned char)c)) {
            if (c == 'P' || c == 'C') {
                Tok t; t.type = T::Ident; t.text = std::string(1, c); out.push_back(t); ++i; continue;
            }
            size_t st = i; while (i < n && isalnum((unsigned char)s[i])) ++i;
            Tok t; t.type = T::Ident; t.text = s.substr(st, i - st);
            out.push_back(t); continue;
        }
        switch (c) {
            case '(': { Tok t; t.type = T::LP; t.text = "("; out.push_back(t); break; }
            case ')': { Tok t; t.type = T::RP; t.text = ")"; out.push_back(t); break; }
            case ',': { Tok t; t.type = T::Comma; t.text = ","; out.push_back(t); break; }
            case '!': { Tok t; t.type = T::Bang; t.text = "!"; out.push_back(t); break; }
            case '%': { Tok t; t.type = T::Pct; t.text = "%"; out.push_back(t); break; }
            case '+': case '-': case '*': case '/': case '^':
                { Tok t; t.type = T::Op; t.text = std::string(1, c); out.push_back(t); break; }
            default: syntaxErr();
        }
        ++i;
    }
    { Tok t; t.type = T::End; out.push_back(t); }
    return out;
}

// ------------------------------------------------------------------- parser
class Parser {
public:
    Parser(std::vector<Tok> t, const EvalContext &context, const Val &ans)
        : t_(std::move(t)), m_(context.angle), ans_(ans), memory_(context.memory) {}
    Val all() { Val v = expr(); if (cur().type != T::End) syntaxErr(); return v; }
private:
    std::vector<Tok> t_; size_t p_ = 0; AngleMode m_; Val ans_; const Memory *memory_;
    const Tok &cur() const { return t_[p_]; }
    const Tok &adv() { const Tok &x = t_[p_]; if (p_ + 1 < t_.size()) ++p_; return x; }
    bool isOp(char c) const { return cur().type == T::Op && cur().text[0] == c; }
    bool startsPrimary() const { return cur().type == T::Num || cur().type == T::Ident || cur().type == T::LP; }

    Val expr() {
        Val v = term();
        while (isOp('+') || isOp('-')) { bool sub = isOp('-'); adv(); v = vAdd(v, term(), sub); }
        return v;
    }
    Val term() {
        Val v = unary();
        for (;;) {
            if (cur().type == T::Ident && (cur().text == "P" || cur().text == "C")
                && (t_[p_ + 1].type == T::Num || t_[p_ + 1].type == T::Ident || t_[p_ + 1].type == T::LP)) {
                std::string operation = cur().text == "P" ? "nPr" : "nCr";
                adv();
                Val right = unary();
                v = call(operation, v, &right);
                continue;
            }
            if (isOp('*')) { adv(); v = vMul(v, unary()); continue; }
            if (isOp('/')) { adv(); v = vDiv(v, unary()); continue; }
            if (startsPrimary()) { v = vMul(v, unary()); continue; }
            break;
        }
        return v;
    }
    Val unary() {                       // unary minus binds looser than ^  (-3^2 = -9)
        if (isOp('-')) { adv(); return vNeg(unary()); }
        if (isOp('+')) { adv(); return unary(); }
        return power();
    }
    Val power() {
        Val b = postfix();
        if (isOp('^')) { adv(); Val e = unary(); return vPow(b, e); }
        return b;
    }
    Val postfix() {
        Val v = primary();
        for (;;) {
            if (cur().type == T::Bang) {
                adv();
                if (v.d < 0 || v.d != std::floor(v.d) || v.d > 69) mathErr();
                double r = 1; for (int k = 2; k <= (int)v.d; ++k) r *= k;
                Val f = inexact(r);
                if (v.d <= 20) { int64_t q = 1; for (int k = 2; k <= (int)v.d; ++k) q *= k; f = fromRat(Rational{q > LIM ? 0 : q, 1}); if (q > LIM) f = inexact(r); }
                v = f; continue;
            }
            if (cur().type == T::Pct) { adv(); v = vMul(v, fromRat(Rational{1, 100})); continue; }
            break;
        }
        return v;
    }
    double toRad(double x) const { return m_ == AngleMode::Degrees ? x * M_PI / 180 : m_ == AngleMode::Grads ? x * M_PI / 200 : x; }
    double fromRad(double x) const { return m_ == AngleMode::Degrees ? x * 180 / M_PI : m_ == AngleMode::Grads ? x * 200 / M_PI : x; }
    Val call(const std::string &f, const Val &a, const Val *b = nullptr) {
        double x = a.d;
        if (f == "nPr" || f == "nCr" || f == "mod" || f == "gcd" || f == "lcm") {
            if (!b) syntaxErr();
            if (f == "mod") {
                if (b->d == 0) mathErr();
                return inexact(std::fmod(x, b->d));
            }
            if (f == "gcd" || f == "lcm") {
                constexpr double MAX_INTEGER = 1000000000000.0;
                if (x < 0 || b->d < 0 || x > MAX_INTEGER || b->d > MAX_INTEGER
                    || x != std::floor(x) || b->d != std::floor(b->d)) mathErr();
                uint64_t left = (uint64_t)x, right = (uint64_t)b->d;
                uint64_t divisor = std::gcd(left, right);
                uint64_t result = f == "gcd" ? divisor : (divisor == 0 ? 0 : (left / divisor) * right);
                if (result > (uint64_t)MAX_INTEGER) mathErr();
                return fromRat(Rational{(int64_t)result, 1});
            }
            double y = b->d;
            if (x < 0 || y < 0 || x != std::floor(x) || y != std::floor(y) || x > 69 || y > x) mathErr();
            int n = (int)x, k = (int)y;
            double result = 1;
            if (f == "nPr") {
                for (int i = 0; i < k; ++i) result *= n - i;
            } else {
                k = std::min(k, n - k);
                for (int i = 1; i <= k; ++i) result = result * (n - k + i) / i;
            }
            Val value = inexact(result);
            if (result <= 1000000000000.0 && result == std::floor(result))
                value = fromRat(Rational{(int64_t)result, 1});
            return value;
        }
        if (f == "sqrt") return vSqrt(a);
        if (f == "abs") { Val r = a; if (a.d < 0) r = vNeg(a); r.d = std::fabs(x); return r; }
        if (f == "sin") return inexact(std::sin(toRad(x)));
        if (f == "cos") return inexact(std::cos(toRad(x)));
        if (f == "tan") return inexact(std::tan(toRad(x)));
        if (f == "asin") { if (x < -1 || x > 1) mathErr(); return inexact(fromRad(std::asin(x))); }
        if (f == "acos") { if (x < -1 || x > 1) mathErr(); return inexact(fromRad(std::acos(x))); }
        if (f == "atan") return inexact(fromRad(std::atan(x)));
        if (f == "sinh") return inexact(std::sinh(x));
        if (f == "cosh") return inexact(std::cosh(x));
        if (f == "tanh") return inexact(std::tanh(x));
        if (f == "asinh") return inexact(std::asinh(x));
        if (f == "acosh") { if (x < 1) mathErr(); return inexact(std::acosh(x)); }
        if (f == "atanh") { if (x <= -1 || x >= 1) mathErr(); return inexact(std::atanh(x)); }
        if (f == "log") { if (x <= 0) mathErr(); return inexact(std::log10(x)); }
        if (f == "ln") { if (x <= 0) mathErr(); return inexact(std::log(x)); }
        if (f == "floor") return inexact(std::floor(x));
        if (f == "ceil") return inexact(std::ceil(x));
        if (f == "cbrt") return inexact(std::cbrt(x));
        syntaxErr();
    }
    Val primary() {
        if (cur().type == T::Num) {
            Tok t = adv();
            return t.ratOk ? fromRat(t.rat) : inexact(t.num);
        }
        if (cur().type == T::LP) {
            adv(); Val v = expr();
            if (cur().type != T::RP) syntaxErr();
            adv(); return v;
        }
        if (cur().type == T::Ident) {
            std::string name = adv().text;
            if (name == "pi") { Val v; v.d = M_PI; v.ex = true; v.x.terms.push_back(Term{Rational{1, 1}, 1, true}); return v; }
            if (name == "e") return inexact(M_E);
            if (name == "Ans") return ans_;
            if (name.size() == 1 && name[0] >= 'A' && name[0] <= 'Z' && memory_) {
                const MemoryValue &saved = (*memory_)[(size_t)(name[0] - 'A')];
                if (!saved.defined) return fromRat(Rational{0, 1});
                Val v; v.d = saved.value; v.ex = saved.exact; if (v.ex) v.x = saved.ex;
                return v;
            }
            if (cur().type != T::LP) syntaxErr();
            adv(); Val a = expr();
            if (cur().type == T::Comma) {
                adv(); Val b = expr();
                if (cur().type != T::RP) syntaxErr();
                adv(); return call(name, a, &b);
            }
            if (cur().type != T::RP) syntaxErr();
            adv(); return call(name, a);
        }
        syntaxErr();
    }
};

class ComplexParser {
public:
    ComplexParser(std::vector<Tok> tokens, const EvalContext &context)
        : tokens_(std::move(tokens)), context_(context) {}

    std::complex<double> all() {
        std::complex<double> value = expression();
        if (current().type != T::End) syntaxErr();
        return value;
    }

private:
    std::vector<Tok> tokens_;
    size_t pos_ = 0;
    const EvalContext &context_;

    const Tok &current() const { return tokens_[pos_]; }
    const Tok &advance() { const Tok &token = tokens_[pos_]; if (pos_ + 1 < tokens_.size()) ++pos_; return token; }
    bool op(char c) const { return current().type == T::Op && current().text[0] == c; }
    bool startsValue() const { return current().type == T::Num || current().type == T::Ident || current().type == T::LP; }

    std::complex<double> expression() {
        auto value = product();
        while (op('+') || op('-')) {
            bool subtract = op('-'); advance();
            value += (subtract ? -1.0 : 1.0) * product();
        }
        return value;
    }
    std::complex<double> product() {
        auto value = unary();
        for (;;) {
            if (op('*')) { advance(); value *= unary(); }
            else if (op('/')) { advance(); auto divisor = unary(); if (std::abs(divisor) == 0) mathErr(); value /= divisor; }
            else if (startsValue()) value *= unary();
            else break;
        }
        return value;
    }
    std::complex<double> unary() {
        if (op('-')) { advance(); return -unary(); }
        if (op('+')) { advance(); return unary(); }
        return power();
    }
    std::complex<double> power() {
        auto base = postfix();
        if (op('^')) { advance(); return std::pow(base, unary()); }
        return base;
    }
    std::complex<double> postfix() {
        auto value = primary();
        for (;;) {
            if (current().type == T::Pct) { advance(); value /= 100.0; }
            else if (current().type == T::Bang) {
                advance();
                if (value.imag() != 0 || value.real() < 0 || value.real() != std::floor(value.real()) || value.real() > 69) mathErr();
                double factorial = 1;
                for (int n = 2; n <= (int)value.real(); ++n) factorial *= n;
                value = factorial;
            } else break;
        }
        return value;
    }
    std::complex<double> call(const std::string &name, std::complex<double> value) {
        constexpr double PI = 3.14159265358979323846;
        double toRadians = context_.angle == AngleMode::Degrees ? PI / 180.0
                         : context_.angle == AngleMode::Grads ? PI / 200.0 : 1.0;
        double fromRadians = 1.0 / toRadians;
        if (name == "sqrt") return std::sqrt(value);
        if (name == "abs") return std::abs(value);
        if (name == "arg") {
            if (std::abs(value) == 0.0) mathErr();
            return std::arg(value) * fromRadians;
        }
        if (name == "conj") return std::conj(value);
        if (name == "real") return value.real();
        if (name == "imag") return value.imag();
        if (name == "exp") return std::exp(value);
        if (name == "ln") return std::log(value);
        if (name == "log") return std::log10(value);
        if (name == "sin") return std::sin(value * toRadians);
        if (name == "cos") return std::cos(value * toRadians);
        if (name == "tan") return std::tan(value * toRadians);
        if (name == "sinh") return std::sinh(value);
        if (name == "cosh") return std::cosh(value);
        if (name == "tanh") return std::tanh(value);
        if (name == "asin") return std::asin(value) * fromRadians;
        if (name == "acos") return std::acos(value) * fromRadians;
        if (name == "atan") return std::atan(value) * fromRadians;
        if (name == "floor" || name == "ceil") {
            if (value.imag() != 0) mathErr();
            return name == "floor" ? std::floor(value.real()) : std::ceil(value.real());
        }
        syntaxErr();
    }
    std::complex<double> primary() {
        if (current().type == T::Num) return advance().num;
        if (current().type == T::LP) {
            advance(); auto value = expression();
            if (current().type != T::RP) syntaxErr();
            advance(); return value;
        }
        if (current().type != T::Ident) syntaxErr();
        std::string name = advance().text;
        if (name == "i" || name == "I") return {0, 1};
        if (name == "pi") return {3.14159265358979323846, 0};
        if (name == "e") return {2.71828182845904523536, 0};
        if (name == "Ans") return {context_.ans, context_.ansImaginary};
        if (name.size() == 1 && name[0] >= 'A' && name[0] <= 'Z' && context_.memory) {
            const MemoryValue &saved = (*context_.memory)[(size_t)(name[0] - 'A')];
            return {saved.defined ? saved.value : 0.0, saved.defined ? saved.imaginary : 0.0};
        }
        if (current().type != T::LP) syntaxErr();
        advance(); auto argument = expression();
        if (current().type != T::RP) syntaxErr();
        advance(); return call(name, argument);
    }
};

bool containsImaginaryUnit(const std::vector<Tok> &tokens) {
    return std::any_of(tokens.begin(), tokens.end(), [](const Tok &token) {
        return token.type == T::Ident && (token.text == "i" || token.text == "I");
    });
}
bool requiresComplexEvaluation(const std::vector<Tok> &tokens, const EvalContext &context) {
    for (const Tok &token : tokens) {
        if (token.type != T::Ident) continue;
        if (token.text == "i" || token.text == "I") return true;
        if (token.text == "Ans" && context.ansComplex) return true;
        if (context.memory && token.text.size() == 1 && token.text[0] >= 'A' && token.text[0] <= 'Z'
            && (*context.memory)[(size_t)(token.text[0] - 'A')].complex) return true;
    }
    return false;
}

} // namespace

bool exactIsRational(const Exact &x, Rational &out) {
    if (x.terms.empty()) { out = Rational{0, 1}; return true; }
    if (x.terms.size() == 1 && x.terms[0].radicand == 1 && !x.terms[0].pi) { out = x.terms[0].c; return true; }
    return false;
}

EvalResult evaluate(const std::string &expr, AngleMode mode, double ans, const Exact *ansEx) {
    EvalContext context; context.angle = mode; context.ans = ans; context.ansExact = ansEx;
    return evaluate(expr, context);
}

EvalResult evaluate(const std::string &expr, const EvalContext &context) {
    EvalResult r;
    if (expr.empty()) { r.error = "Syntax ERROR"; return r; }
    try {
        std::vector<Tok> tokens = tokenize(expr);
        if (containsImaginaryUnit(tokens) || requiresComplexEvaluation(tokens, context)) {
            ComplexParser parser(std::move(tokens), context);
            std::complex<double> value = parser.all();
            if (!std::isfinite(value.real()) || !std::isfinite(value.imag())
                || std::abs(value.real()) >= 1e100 || std::abs(value.imag()) >= 1e100) mathErr();
            r.ok = true; r.value = value.real(); r.imaginary = value.imag();
            r.complex = true;
            return r;
        }
        Val a; a.d = context.ans;
        if (context.ansExact) { a.ex = true; a.x = *context.ansExact; }
        Parser p(tokenize(expr), context, a);
        Val v = p.all();
        if (std::isnan(v.d) || std::isinf(v.d)) mathErr();
        if (std::fabs(v.d) >= 1e100) mathErr();
        r.ok = true; r.value = v.d; r.exact = v.ex; r.ex = v.x;
    } catch (const CalcError &e) {
        r.ok = false; r.error = e.what();
    } catch (...) {
        r.ok = false; r.error = "Syntax ERROR";
    }
    return r;
}

EvalResult Engine::evaluate(const std::string &expr) {
    EvalContext context; context.angle = angle_; context.ans = ans_;
    context.ansImaginary = ansImaginary_; context.ansComplex = ansComplex_;
    context.ansExact = ansIsExact_ ? &ansExact_ : nullptr;
    context.memory = &memory_;
    EvalResult result = calc::evaluate(expr, context);
    if (result.ok) {
        ans_ = result.value;
        ansImaginary_ = result.imaginary;
        ansComplex_ = result.complex;
        ansIsExact_ = result.exact;
        if (result.exact) ansExact_ = result.ex;
    }
    return result;
}

bool Engine::store(char name, const EvalResult &value) {
    if (name >= 'a' && name <= 'z') name = (char)(name - 'a' + 'A');
    if (name < 'A' || name > 'Z' || !value.ok) return false;
    MemoryValue &saved = memory_[(size_t)(name - 'A')];
    saved.defined = true; saved.value = value.value; saved.imaginary = value.imaginary;
    saved.complex = value.complex; saved.exact = value.exact;
    if (value.exact) saved.ex = value.ex;
    return true;
}

const MemoryValue *Engine::recall(char name) const {
    if (name >= 'a' && name <= 'z') name = (char)(name - 'a' + 'A');
    if (name < 'A' || name > 'Z') return nullptr;
    const MemoryValue &saved = memory_[(size_t)(name - 'A')];
    return saved.defined ? &saved : nullptr;
}

bool Engine::addToMemory(char name, const EvalResult &value, bool subtract) {
    if (name >= 'a' && name <= 'z') name = (char)(name - 'a' + 'A');
    if (name < 'A' || name > 'Z' || !value.ok) return false;

    Memory workingMemory = memory_;
    MemoryValue &target = workingMemory[(size_t)(name - 'A')];
    if (!target.defined) { target.defined = true; target.exact = true; }
    EvalContext context;
    context.angle = angle_;
    context.ans = value.value;
    context.ansImaginary = value.imaginary;
    context.ansComplex = value.complex;
    context.ansExact = value.exact ? &value.ex : nullptr;
    context.memory = &workingMemory;
    EvalResult sum = calc::evaluate(std::string(1, name) + (subtract ? "-Ans" : "+Ans"), context);
    return store(name, sum);
}

bool Engine::clearMemory(char name) {
    if (name >= 'a' && name <= 'z') name = (char)(name - 'a' + 'A');
    if (name < 'A' || name > 'Z') return false;
    memory_[(size_t)(name - 'A')] = MemoryValue{};
    return true;
}

} // namespace calc
