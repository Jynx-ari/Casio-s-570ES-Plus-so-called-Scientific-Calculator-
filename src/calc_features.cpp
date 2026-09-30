#include "calc_features.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace calc {
namespace {

unsigned radixValue(Radix radix) { return static_cast<unsigned>(radix); }

bool validRadix(Radix radix) {
    unsigned value = radixValue(radix);
    return value == 2 || value == 8 || value == 10 || value == 16;
}

bool finite(double value) { return std::isfinite(value); }

} // namespace

bool parseInteger(const std::string &text, Radix radix, int64_t &value) {
    if (text.empty() || !validRadix(radix)) return false;
    size_t index = 0;
    bool negative = false;
    if (text[index] == '+' || text[index] == '-') {
        negative = text[index] == '-';
        if (++index == text.size()) return false;
    }
    uint64_t magnitude = 0;
    const uint64_t limit = negative ? (uint64_t)INT64_MAX + 1 : (uint64_t)INT64_MAX;
    for (; index < text.size(); ++index) {
        unsigned char c = (unsigned char)text[index];
        unsigned digit = c >= '0' && c <= '9' ? c - '0'
            : c >= 'A' && c <= 'F' ? c - 'A' + 10
            : c >= 'a' && c <= 'f' ? c - 'a' + 10 : 255;
        if (digit >= radixValue(radix) || magnitude > (limit - digit) / radixValue(radix)) return false;
        magnitude = magnitude * radixValue(radix) + digit;
    }
    if (negative && magnitude == (uint64_t)INT64_MAX + 1) value = INT64_MIN;
    else value = negative ? -(int64_t)magnitude : (int64_t)magnitude;
    return true;
}

std::string formatInteger(int64_t value, Radix radix) {
    if (!validRadix(radix)) return {};
    static const char digits[] = "0123456789ABCDEF";
    uint64_t magnitude = value < 0 ? (uint64_t)(-(value + 1)) + 1 : (uint64_t)value;
    unsigned base = radixValue(radix);
    std::string result;
    do {
        result.push_back(digits[magnitude % base]);
        magnitude /= base;
    } while (magnitude != 0);
    if (value < 0) result.push_back('-');
    std::reverse(result.begin(), result.end());
    return result;
}

bool integerOperation(int64_t a, int64_t b, IntegerOperation operation, int64_t &result) {
    switch (operation) {
        case IntegerOperation::Add:
            if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) return false;
            result = a + b; return true;
        case IntegerOperation::Subtract:
            if ((b < 0 && a > INT64_MAX + b) || (b > 0 && a < INT64_MIN + b)) return false;
            result = a - b; return true;
        case IntegerOperation::Multiply:
            if (a == 0 || b == 0) { result = 0; return true; }
            if ((a == -1 && b == INT64_MIN) || (b == -1 && a == INT64_MIN)) return false;
            if (a > 0 ? (b > 0 ? a > INT64_MAX / b : b < INT64_MIN / a)
                      : (b > 0 ? a < INT64_MIN / b : a < INT64_MAX / b)) return false;
            result = a * b; return true;
        case IntegerOperation::Divide:
            if (b == 0 || (a == INT64_MIN && b == -1)) return false;
            result = a / b; return true;
        case IntegerOperation::And: result = a & b; return true;
        case IntegerOperation::Or: result = a | b; return true;
        case IntegerOperation::Xor: result = a ^ b; return true;
        case IntegerOperation::Xnor: result = ~(a ^ b); return true;
        case IntegerOperation::Not: result = ~a; return true;
    }
    return false;
}

Complex complexAdd(Complex a, Complex b) { return {a.real + b.real, a.imag + b.imag}; }
Complex complexSubtract(Complex a, Complex b) { return {a.real - b.real, a.imag - b.imag}; }
Complex complexMultiply(Complex a, Complex b) {
    return {a.real * b.real - a.imag * b.imag, a.real * b.imag + a.imag * b.real};
}
bool complexDivide(Complex a, Complex b, Complex &result) {
    if (b.real == 0.0 && b.imag == 0.0) return false;
    double scale = std::max(std::fabs(b.real), std::fabs(b.imag));
    double real = b.real / scale, imag = b.imag / scale;
    double denominator = real * real + imag * imag;
    result = {(a.real * real + a.imag * imag) / (scale * denominator),
              (a.imag * real - a.real * imag) / (scale * denominator)};
    return finite(result.real) && finite(result.imag);
}
Complex complexSqrt(Complex value) {
    double magnitude = std::hypot(value.real, value.imag);
    double real = std::sqrt(std::max(0.0, (magnitude + value.real) / 2.0));
    double imag = std::sqrt(std::max(0.0, (magnitude - value.real) / 2.0));
    if (value.imag < 0.0) imag = -imag;
    return {real, imag};
}
double complexMagnitude(Complex value) { return std::hypot(value.real, value.imag); }
double complexArgument(Complex value) { return std::atan2(value.imag, value.real); }
Complex complexConjugate(Complex value) { return {value.real, -value.imag}; }
Complex complexExp(Complex value) {
    double scale = std::exp(value.real);
    return {scale * std::cos(value.imag), scale * std::sin(value.imag)};
}
bool complexLog(Complex value, Complex &result) {
    double magnitude = complexMagnitude(value);
    if (magnitude == 0.0 || !finite(magnitude)) return false;
    result = {std::log(magnitude), complexArgument(value)};
    return finite(result.real) && finite(result.imag);
}
Complex complexPower(Complex base, Complex exponent) {
    Complex logarithm;
    if (!complexLog(base, logarithm)) {
        if (base.real == 0.0 && base.imag == 0.0 && exponent.real > 0.0) return {};
        double nan = std::numeric_limits<double>::quiet_NaN();
        return {nan, nan};
    }
    return complexExp(complexMultiply(exponent, logarithm));
}
Complex complexSin(Complex value) {
    return {std::sin(value.real) * std::cosh(value.imag), std::cos(value.real) * std::sinh(value.imag)};
}
Complex complexCos(Complex value) {
    return {std::cos(value.real) * std::cosh(value.imag), -std::sin(value.real) * std::sinh(value.imag)};
}
bool complexTan(Complex value, Complex &result) {
    return complexDivide(complexSin(value), complexCos(value), result);
}

void Statistics::clear() { *this = Statistics{}; }
bool Statistics::add(double x, double y, uint32_t frequency) {
    if (!finite(x) || !finite(y) || frequency == 0 || frequency > UINT32_MAX - count_) return false;
    double weight = frequency;
    double sx = sumX_ + x * weight, sy = sumY_ + y * weight;
    double sxx = sumXX_ + x * x * weight, syy = sumYY_ + y * y * weight, sxy = sumXY_ + x * y * weight;
    if (!finite(sx) || !finite(sy) || !finite(sxx) || !finite(syy) || !finite(sxy)) return false;
    count_ += frequency; sumX_ = sx; sumY_ = sy; sumXX_ = sxx; sumYY_ = syy; sumXY_ = sxy;
    return true;
}
double Statistics::meanX() const { return count_ ? sumX_ / count_ : std::numeric_limits<double>::quiet_NaN(); }
double Statistics::meanY() const { return count_ ? sumY_ / count_ : std::numeric_limits<double>::quiet_NaN(); }
double Statistics::varianceX(bool sample) const {
    if (count_ < (sample ? 2u : 1u)) return std::numeric_limits<double>::quiet_NaN();
    double centered = std::max(0.0, sumXX_ - sumX_ * sumX_ / count_);
    return centered / (count_ - (sample ? 1 : 0));
}
double Statistics::varianceY(bool sample) const {
    if (count_ < (sample ? 2u : 1u)) return std::numeric_limits<double>::quiet_NaN();
    double centered = std::max(0.0, sumYY_ - sumY_ * sumY_ / count_);
    return centered / (count_ - (sample ? 1 : 0));
}
double Statistics::standardDeviationX(bool sample) const { return std::sqrt(varianceX(sample)); }
bool Statistics::linearRegression(double &slope, double &intercept, double &correlation) const {
    if (count_ < 2) return false;
    double n = (double)count_;
    double xx = n * sumXX_ - sumX_ * sumX_;
    double yy = n * sumYY_ - sumY_ * sumY_;
    double xy = n * sumXY_ - sumX_ * sumY_;
    if (xx == 0.0 || yy == 0.0) return false;
    slope = xy / xx;
    intercept = (sumY_ - slope * sumX_) / n;
    correlation = std::clamp(xy / std::sqrt(xx * yy), -1.0, 1.0);
    return finite(slope) && finite(intercept) && finite(correlation);
}

EquationResult solveLinear(double a, double b) {
    EquationResult result;
    if (!finite(a) || !finite(b) || a == 0.0) return result;
    double root = -b / a;
    if (!finite(root)) return result;
    result.ok = true; result.count = 1; result.roots[0] = {root, 0.0};
    return result;
}
EquationResult solveQuadratic(double a, double b, double c) {
    if (a == 0.0) return solveLinear(b, c);
    EquationResult result;
    if (!finite(a) || !finite(b) || !finite(c)) return result;
    double discriminant = b * b - 4.0 * a * c;
    if (!finite(discriminant)) return result;
    result.ok = true; result.count = 2;
    if (discriminant >= 0.0) {
        double root = std::sqrt(discriminant);
        double q = -0.5 * (b + std::copysign(root, b));
        if (q == 0.0) result.roots[0] = result.roots[1] = {-b / (2.0 * a), 0.0};
        else { result.roots[0] = {q / a, 0.0}; result.roots[1] = {c / q, 0.0}; }
    } else {
        result.complexRoots = true;
        result.roots[0] = {-b / (2.0 * a), std::sqrt(-discriminant) / std::fabs(2.0 * a)};
        result.roots[1] = {result.roots[0].real, -result.roots[0].imag};
    }
    if (!finite(result.roots[0].real) || !finite(result.roots[0].imag)
        || !finite(result.roots[1].real) || !finite(result.roots[1].imag)) return EquationResult{};
    return result;
}

CubicResult solveCubic(double a, double b, double c, double d) {
    CubicResult result;
    if (!finite(a) || !finite(b) || !finite(c) || !finite(d) || a == 0.0) return result;
    double aa = b / a, bb = c / a, cc = d / a;
    double p = bb - aa * aa / 3.0;
    double q = 2.0 * aa * aa * aa / 27.0 - aa * bb / 3.0 + cc;
    double halfQ = q / 2.0, thirdP = p / 3.0;
    double discriminant = halfQ * halfQ + thirdP * thirdP * thirdP;
    if (!finite(discriminant)) return result;
    double tolerance = 1e-14 * (std::fabs(halfQ * halfQ) + std::fabs(thirdP * thirdP * thirdP) + 1.0);
    if (discriminant > tolerance) {
        double root = std::sqrt(discriminant);
        double u = std::cbrt(-halfQ + root), v = std::cbrt(-halfQ - root);
        double shift = aa / 3.0;
        result.roots[0] = {u + v - shift, 0.0};
        double real = -(u + v) / 2.0 - shift;
        double imag = std::sqrt(3.0) * (u - v) / 2.0;
        result.roots[1] = {real, imag}; result.roots[2] = {real, -imag};
        result.hasComplexRoots = imag != 0.0;
    } else if (discriminant >= -tolerance) {
        double u = std::cbrt(-halfQ), shift = aa / 3.0;
        result.roots[0] = {2.0 * u - shift, 0.0};
        result.roots[1] = result.roots[2] = {-u - shift, 0.0};
    } else {
        double radius = 2.0 * std::sqrt(-thirdP);
        double argument = std::acos(std::clamp(-halfQ / std::sqrt(-(thirdP * thirdP * thirdP)), -1.0, 1.0));
        double shift = aa / 3.0;
        for (int k = 0; k < 3; ++k)
            result.roots[k] = {radius * std::cos((argument + 2.0 * 3.14159265358979323846 * k) / 3.0) - shift, 0.0};
    }
    for (const Complex &root : result.roots)
        if (!finite(root.real) || !finite(root.imag)) return CubicResult{};
    result.ok = true;
    return result;
}

namespace {
double angleScale(AngleUnit unit) {
    if (unit == AngleUnit::Degrees) return 180.0 / 3.14159265358979323846;
    if (unit == AngleUnit::Grads) return 200.0 / 3.14159265358979323846;
    return 1.0;
}
enum class Dimension : uint8_t { Length, Mass, Time, Pressure, Energy, Power, Invalid };
bool unitInfo(Unit unit, Dimension &dimension, double &scale) {
    switch (unit) {
        case Unit::Meter: dimension = Dimension::Length; scale = 1.0; break;
        case Unit::Kilometer: dimension = Dimension::Length; scale = 1000.0; break;
        case Unit::Centimeter: dimension = Dimension::Length; scale = 0.01; break;
        case Unit::Millimeter: dimension = Dimension::Length; scale = 0.001; break;
        case Unit::Inch: dimension = Dimension::Length; scale = 0.0254; break;
        case Unit::Foot: dimension = Dimension::Length; scale = 0.3048; break;
        case Unit::Yard: dimension = Dimension::Length; scale = 0.9144; break;
        case Unit::Mile: dimension = Dimension::Length; scale = 1609.344; break;
        case Unit::Gram: dimension = Dimension::Mass; scale = 0.001; break;
        case Unit::Kilogram: dimension = Dimension::Mass; scale = 1.0; break;
        case Unit::Pound: dimension = Dimension::Mass; scale = 0.45359237; break;
        case Unit::Second: dimension = Dimension::Time; scale = 1.0; break;
        case Unit::Minute: dimension = Dimension::Time; scale = 60.0; break;
        case Unit::Hour: dimension = Dimension::Time; scale = 3600.0; break;
        case Unit::Pascal: dimension = Dimension::Pressure; scale = 1.0; break;
        case Unit::Kilopascal: dimension = Dimension::Pressure; scale = 1000.0; break;
        case Unit::Bar: dimension = Dimension::Pressure; scale = 100000.0; break;
        case Unit::Atmosphere: dimension = Dimension::Pressure; scale = 101325.0; break;
        case Unit::Joule: dimension = Dimension::Energy; scale = 1.0; break;
        case Unit::Calorie: dimension = Dimension::Energy; scale = 4.184; break;
        case Unit::KilowattHour: dimension = Dimension::Energy; scale = 3600000.0; break;
        case Unit::Watt: dimension = Dimension::Power; scale = 1.0; break;
        case Unit::Kilowatt: dimension = Dimension::Power; scale = 1000.0; break;
        default: dimension = Dimension::Invalid; scale = 0.0; return false;
    }
    return true;
}
} // namespace

Polar rectangularToPolar(double x, double y, AngleUnit angleUnit) {
    return {std::hypot(x, y), std::atan2(y, x) * angleScale(angleUnit)};
}
bool polarToRectangular(double radius, double angle, AngleUnit angleUnit, double &x, double &y) {
    if (!finite(radius) || !finite(angle)) return false;
    double radians = angle / angleScale(angleUnit);
    x = radius * std::cos(radians); y = radius * std::sin(radians);
    return finite(x) && finite(y);
}
bool dmsToDecimal(double degrees, double minutes, double seconds, double &result) {
    if (!finite(degrees) || !finite(minutes) || !finite(seconds) || minutes < 0 || minutes >= 60 || seconds < 0 || seconds >= 60) return false;
    double magnitude = std::fabs(degrees) + minutes / 60.0 + seconds / 3600.0;
    result = std::signbit(degrees) ? -magnitude : magnitude;
    return finite(result);
}
void decimalToDms(double value, double &degrees, uint8_t &minutes, double &seconds) {
    double magnitude = std::fabs(value);
    double wholeDegrees = std::floor(magnitude);
    double totalMinutes = (magnitude - wholeDegrees) * 60.0;
    double wholeMinutes = std::floor(totalMinutes);
    degrees = std::copysign(wholeDegrees, value);
    minutes = (uint8_t)wholeMinutes;
    seconds = (totalMinutes - wholeMinutes) * 60.0;
    if (seconds >= 59.999999999) { seconds = 0.0; if (++minutes == 60) { minutes = 0; degrees += std::copysign(1.0, value); } }
}

bool convertUnit(double value, Unit from, Unit to, double &result) {
    Dimension fromDimension, toDimension; double fromScale, toScale;
    if (!finite(value) || !unitInfo(from, fromDimension, fromScale) || !unitInfo(to, toDimension, toScale)
        || fromDimension != toDimension) return false;
    result = value * fromScale / toScale;
    return finite(result);
}

bool physicalConstant(Constant constant, double &value) {
    switch (constant) {
        case Constant::SpeedOfLight: value = 299792458.0; break;
        case Constant::GravitationalConstant: value = 6.67430e-11; break;
        case Constant::PlanckConstant: value = 6.62607015e-34; break;
        case Constant::AvogadroConstant: value = 6.02214076e23; break;
        case Constant::BoltzmannConstant: value = 1.380649e-23; break;
        case Constant::ElectronMass: value = 9.1093837139e-31; break;
        case Constant::ProtonMass: value = 1.67262192595e-27; break;
        case Constant::NeutronMass: value = 1.67492750056e-27; break;
        case Constant::ElementaryCharge: value = 1.602176634e-19; break;
        case Constant::StandardGravity: value = 9.80665; break;
        default: return false;
    }
    return true;
}

PrimeFactorization primeFactorize(uint64_t value) {
    PrimeFactorization result;
    if (value == 0 || value > 1000000000000ULL) return result;
    result.ok = true;
    for (uint64_t prime = 2; prime <= value / prime; prime += prime == 2 ? 1 : 2) {
        if (value % prime != 0) continue;
        uint8_t exponent = 0;
        do { value /= prime; ++exponent; } while (value % prime == 0);
        result.factors[result.count++] = {prime, exponent};
    }
    if (value > 1) result.factors[result.count++] = {value, 1};
    return result;
}

bool matrixAdd(const Matrix3 &a, const Matrix3 &b, Matrix3 &result) {
    if (a.rows == 0 || a.cols == 0 || a.rows > 3 || a.cols > 3 || a.rows != b.rows || a.cols != b.cols) return false;
    result = {}; result.rows = a.rows; result.cols = a.cols;
    for (uint8_t r = 0; r < a.rows; ++r) for (uint8_t c = 0; c < a.cols; ++c) result.values[r][c] = a.values[r][c] + b.values[r][c];
    return true;
}
bool matrixMultiply(const Matrix3 &a, const Matrix3 &b, Matrix3 &result) {
    if (a.rows == 0 || a.cols == 0 || b.rows == 0 || b.cols == 0 || a.rows > 3 || a.cols > 3 || b.rows > 3 || b.cols > 3 || a.cols != b.rows) return false;
    result = {}; result.rows = a.rows; result.cols = b.cols;
    for (uint8_t r = 0; r < a.rows; ++r) for (uint8_t c = 0; c < b.cols; ++c)
        for (uint8_t k = 0; k < a.cols; ++k) result.values[r][c] += a.values[r][k] * b.values[k][c];
    return true;
}
bool matrixDeterminant(const Matrix3 &m, double &result) {
    if (m.rows == 0 || m.rows != m.cols || m.rows > 3) return false;
    if (m.rows == 1) result = m.values[0][0];
    else if (m.rows == 2) result = m.values[0][0] * m.values[1][1] - m.values[0][1] * m.values[1][0];
    else result = m.values[0][0] * (m.values[1][1] * m.values[2][2] - m.values[1][2] * m.values[2][1])
                - m.values[0][1] * (m.values[1][0] * m.values[2][2] - m.values[1][2] * m.values[2][0])
                + m.values[0][2] * (m.values[1][0] * m.values[2][1] - m.values[1][1] * m.values[2][0]);
    return finite(result);
}
bool matrixInverse(const Matrix3 &m, Matrix3 &result) {
    double determinant;
    if (!matrixDeterminant(m, determinant) || determinant == 0.0) return false;
    result = {}; result.rows = m.rows; result.cols = m.cols;
    if (m.rows == 1) result.values[0][0] = 1.0 / determinant;
    else if (m.rows == 2) {
        result.values[0][0] = m.values[1][1] / determinant; result.values[0][1] = -m.values[0][1] / determinant;
        result.values[1][0] = -m.values[1][0] / determinant; result.values[1][1] = m.values[0][0] / determinant;
    } else {
        for (uint8_t r = 0; r < 3; ++r) for (uint8_t c = 0; c < 3; ++c) {
            double minor[4];
            size_t i = 0;
            for (uint8_t sourceRow = 0; sourceRow < 3; ++sourceRow)
                for (uint8_t sourceCol = 0; sourceCol < 3; ++sourceCol)
                    if (sourceRow != r && sourceCol != c) minor[i++] = m.values[sourceRow][sourceCol];
            double cofactor = minor[0] * minor[3] - minor[1] * minor[2];
            result.values[c][r] = ((r + c) % 2 ? -cofactor : cofactor) / determinant;
        }
    }
    for (uint8_t r = 0; r < result.rows; ++r)
        for (uint8_t c = 0; c < result.cols; ++c)
            if (!finite(result.values[r][c])) return false;
    return true;
}

Vector3 vectorAdd(Vector3 a, Vector3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vector3 vectorSubtract(Vector3 a, Vector3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vector3 vectorScale(Vector3 v, double scalar) { return {v.x * scalar, v.y * scalar, v.z * scalar}; }
double vectorDot(Vector3 a, Vector3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vector3 vectorCross(Vector3 a, Vector3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
double vectorMagnitude(Vector3 v) { return std::sqrt(vectorDot(v, v)); }
bool vectorAngle(Vector3 a, Vector3 b, double &radians) {
    double denominator = vectorMagnitude(a) * vectorMagnitude(b);
    if (denominator == 0.0 || !finite(denominator)) return false;
    radians = std::acos(std::clamp(vectorDot(a, b) / denominator, -1.0, 1.0));
    return finite(radians);
}

bool makeTable(TableFunction function, void *userData, double start, double end, double step, TableResult &result) {
    if (!function || !finite(start) || !finite(end) || !finite(step) || step == 0.0) return false;
    if ((end - start) * step < 0.0) return false;
    result = {};
    double x = start;
    for (size_t i = 0; i < 21; ++i) {
        double y;
        if (!function(x, userData, y) || !finite(y)) return false;
        result.rows[i] = {x, y}; result.count = i + 1;
        if ((step > 0.0 && x >= end) || (step < 0.0 && x <= end)) return true;
        double next = x + step;
        if (!finite(next) || next == x) return false;
        x = next;
        if ((step > 0.0 && x > end) || (step < 0.0 && x < end)) return true;
    }
    return false;
}

} // namespace calc
