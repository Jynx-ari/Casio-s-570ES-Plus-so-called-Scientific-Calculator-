#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace calc {

enum class Radix : uint8_t { Binary = 2, Octal = 8, Decimal = 10, Hexadecimal = 16 };
bool parseInteger(const std::string &text, Radix radix, int64_t &value);
std::string formatInteger(int64_t value, Radix radix);
enum class IntegerOperation : uint8_t { Add, Subtract, Multiply, Divide, And, Or, Xor, Xnor, Not };
bool integerOperation(int64_t a, int64_t b, IntegerOperation operation, int64_t &result);

struct Complex {
    double real = 0.0;
    double imag = 0.0;
};
Complex complexAdd(Complex a, Complex b);
Complex complexSubtract(Complex a, Complex b);
Complex complexMultiply(Complex a, Complex b);
bool complexDivide(Complex a, Complex b, Complex &result);
Complex complexSqrt(Complex value);
double complexMagnitude(Complex value);
double complexArgument(Complex value);
Complex complexConjugate(Complex value);
Complex complexExp(Complex value);
bool complexLog(Complex value, Complex &result);
Complex complexPower(Complex base, Complex exponent);
Complex complexSin(Complex value);
Complex complexCos(Complex value);
bool complexTan(Complex value, Complex &result);

class Statistics {
public:
    void clear();
    bool add(double x, double y = 0.0, uint32_t frequency = 1);
    uint32_t count() const { return count_; }
    double meanX() const;
    double meanY() const;
    double varianceX(bool sample = true) const;
    double varianceY(bool sample = true) const;
    double standardDeviationX(bool sample = true) const;
    bool linearRegression(double &slope, double &intercept, double &correlation) const;

private:
    uint32_t count_ = 0;
    double sumX_ = 0.0, sumY_ = 0.0;
    double sumXX_ = 0.0, sumYY_ = 0.0, sumXY_ = 0.0;
};

struct EquationResult {
    bool ok = false;
    bool complexRoots = false;
    uint8_t count = 0;
    Complex roots[2]{};
};
EquationResult solveLinear(double a, double b);
EquationResult solveQuadratic(double a, double b, double c);

struct CubicResult {
    bool ok = false;
    bool hasComplexRoots = false;
    Complex roots[3]{};
};
CubicResult solveCubic(double a, double b, double c, double d);

enum class AngleUnit : uint8_t { Radians, Degrees, Grads };
struct Polar { double radius = 0.0; double angle = 0.0; };
Polar rectangularToPolar(double x, double y, AngleUnit angleUnit);
bool polarToRectangular(double radius, double angle, AngleUnit angleUnit, double &x, double &y);
bool dmsToDecimal(double degrees, double minutes, double seconds, double &result);
void decimalToDms(double value, double &degrees, uint8_t &minutes, double &seconds);

enum class Unit : uint8_t {
    Meter, Kilometer, Centimeter, Millimeter, Inch, Foot, Yard, Mile,
    Gram, Kilogram, Pound,
    Second, Minute, Hour,
    Pascal, Kilopascal, Bar, Atmosphere,
    Joule, Calorie, KilowattHour,
    Watt, Kilowatt
};
bool convertUnit(double value, Unit from, Unit to, double &result);

enum class Constant : uint8_t {
    SpeedOfLight, GravitationalConstant, PlanckConstant, AvogadroConstant,
    BoltzmannConstant, ElectronMass, ProtonMass, NeutronMass,
    ElementaryCharge, StandardGravity
};
bool physicalConstant(Constant constant, double &value);

struct PrimeFactorization {
    bool ok = false;
    uint8_t count = 0;
    struct Factor { uint64_t prime = 0; uint8_t exponent = 0; } factors[64]{};
};
PrimeFactorization primeFactorize(uint64_t value);

struct Matrix3 {
    uint8_t rows = 0, cols = 0;
    double values[3][3]{};
};
bool matrixAdd(const Matrix3 &a, const Matrix3 &b, Matrix3 &result);
bool matrixMultiply(const Matrix3 &a, const Matrix3 &b, Matrix3 &result);
bool matrixDeterminant(const Matrix3 &matrix, double &result);
bool matrixInverse(const Matrix3 &matrix, Matrix3 &result);

struct Vector3 { double x = 0.0, y = 0.0, z = 0.0; };
Vector3 vectorAdd(Vector3 a, Vector3 b);
Vector3 vectorSubtract(Vector3 a, Vector3 b);
Vector3 vectorScale(Vector3 value, double scalar);
double vectorDot(Vector3 a, Vector3 b);
Vector3 vectorCross(Vector3 a, Vector3 b);
double vectorMagnitude(Vector3 value);
bool vectorAngle(Vector3 a, Vector3 b, double &radians);

using TableFunction = bool (*)(double x, void *userData, double &result);
struct TableRow { double x = 0.0, y = 0.0; };
struct TableResult { size_t count = 0; TableRow rows[21]{}; };
bool makeTable(TableFunction function, void *userData, double start, double end,
               double step, TableResult &result);

} // namespace calc
