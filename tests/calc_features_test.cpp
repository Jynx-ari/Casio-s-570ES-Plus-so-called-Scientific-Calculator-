#include "calc_features.h"
#include "compact_font.h"

#include <cmath>
#include <iostream>

namespace {
bool close(double a, double b) { return std::fabs(a - b) < 1e-9; }
bool square(double x, void *, double &y) { y = x * x; return true; }
}

int main() {
    for (char letter = 'A'; letter <= 'Z'; ++letter) {
        if (!lcd_font::glyph(letter) || !lcd_font::glyph((char)(letter - 'A' + 'a'))) return 39;
    }
    if (lcd_font::glyph('[') || lcd_font::glyphWidth('A') != lcd_font::CellWidth
        || lcd_font::glyphWidth('z') != lcd_font::CellWidth || lcd_font::textWidth(3) != 17) return 40;
    if (lcd_font::glyph('P')->rows[0] == lcd_font::glyph('p')->rows[0]) return 41;

    int64_t integer = 0;
    if (!calc::parseInteger("-7F", calc::Radix::Hexadecimal, integer) || integer != -127) return 1;
    if (calc::formatInteger(-127, calc::Radix::Binary) != "-1111111") return 2;
    if (!calc::parseInteger("FF", calc::Radix::Hexadecimal, integer) || integer != 255) return 15;
    if (calc::parseInteger("2", calc::Radix::Binary, integer)) return 16;
    if (!calc::parseInteger("-9223372036854775808", calc::Radix::Decimal, integer) || integer != INT64_MIN) return 25;
    int64_t integerResult;
    if (!calc::integerOperation(42, 1, calc::IntegerOperation::Add, integerResult) || integerResult != 43) return 17;
    if (calc::integerOperation(INT64_MAX, 1, calc::IntegerOperation::Add, integerResult)) return 18;
    if (calc::integerOperation(INT64_MAX, 2, calc::IntegerOperation::Multiply, integerResult)) return 26;
    if (calc::integerOperation(INT64_MIN, -1, calc::IntegerOperation::Divide, integerResult)) return 19;
    if (!calc::integerOperation(0x5A, 0x3C, calc::IntegerOperation::And, integerResult) || integerResult != 0x18) return 20;
    if (!calc::integerOperation(0x5A, 0x3C, calc::IntegerOperation::Or, integerResult) || integerResult != 0x7E) return 21;
    if (!calc::integerOperation(0x5A, 0x3C, calc::IntegerOperation::Xor, integerResult) || integerResult != 0x66) return 22;
    if (!calc::integerOperation(0x5A, 0x3C, calc::IntegerOperation::Xnor, integerResult) || integerResult != ~0x66LL) return 23;
    if (!calc::integerOperation(0x5A, 0, calc::IntegerOperation::Not, integerResult) || integerResult != ~0x5ALL) return 24;

    calc::Complex quotient;
    if (!calc::complexDivide({1, 2}, {1, -1}, quotient) || !close(quotient.real, -0.5) || !close(quotient.imag, 1.5)) return 3;
    calc::Complex root = calc::complexSqrt({-4, 0});
    if (!close(root.real, 0) || !close(root.imag, 2)) return 4;
    calc::Complex sine = calc::complexSin({1.5707963267948966, 0});
    if (!close(sine.real, 1) || !close(sine.imag, 0)) return 12;
    calc::Complex logarithm;
    if (!calc::complexLog({-1, 0}, logarithm) || !close(logarithm.real, 0)
        || !close(logarithm.imag, 3.141592653589793)) return 13;

    calc::Statistics stats;
    if (!stats.add(1, 3) || !stats.add(2, 5) || !stats.add(3, 7)) return 5;
    double slope, intercept, correlation;
    if (!stats.linearRegression(slope, intercept, correlation) || !close(slope, 2) || !close(intercept, 1) || !close(correlation, 1)) return 6;
    calc::Statistics weighted;
    if (!weighted.add(2, 0, 3) || !weighted.add(5, 0, 1) || weighted.count() != 4 || !close(weighted.meanX(), 2.75)) return 14;

    calc::EquationResult equation = calc::solveQuadratic(1, 0, 1);
    if (!equation.ok || !equation.complexRoots || !close(equation.roots[0].imag, 1)) return 7;

    calc::CubicResult cubic = calc::solveCubic(1, -6, 11, -6);
    if (!cubic.ok || cubic.hasComplexRoots) return 27;
    bool roots123[3] = {};
    for (const calc::Complex &candidate : cubic.roots)
        for (int rootValue = 1; rootValue <= 3; ++rootValue)
            if (close(candidate.real, rootValue) && close(candidate.imag, 0)) roots123[rootValue - 1] = true;
    if (!roots123[0] || !roots123[1] || !roots123[2]) return 28;
    cubic = calc::solveCubic(1, 0, 0, 1);
    if (!cubic.ok || !cubic.hasComplexRoots || !close(cubic.roots[0].real, -1)) return 29;

    calc::Polar polar = calc::rectangularToPolar(0, 1, calc::AngleUnit::Degrees);
    if (!close(polar.radius, 1) || !close(polar.angle, 90)) return 30;
    double x, y;
    if (!calc::polarToRectangular(2, 180, calc::AngleUnit::Degrees, x, y) || !close(x, -2) || !close(y, 0)) return 31;
    double decimalDegrees;
    if (!calc::dmsToDecimal(-12, 30, 0, decimalDegrees) || !close(decimalDegrees, -12.5)) return 32;
    double degrees, seconds; uint8_t minutes;
    calc::decimalToDms(-12.5, degrees, minutes, seconds);
    if (!close(degrees, -12) || minutes != 30 || !close(seconds, 0)) return 33;

    double converted;
    if (!calc::convertUnit(1, calc::Unit::Mile, calc::Unit::Foot, converted) || !close(converted, 5280)) return 34;
    if (calc::convertUnit(1, calc::Unit::Meter, calc::Unit::Second, converted)) return 35;
    if (!calc::physicalConstant(calc::Constant::SpeedOfLight, converted) || !close(converted, 299792458)) return 36;
    calc::PrimeFactorization factors = calc::primeFactorize(360);
    if (!factors.ok || factors.count != 3 || factors.factors[0].prime != 2 || factors.factors[0].exponent != 3
        || factors.factors[1].prime != 3 || factors.factors[1].exponent != 2 || factors.factors[2].prime != 5) return 37;
    if (calc::primeFactorize(0).ok || !calc::primeFactorize(1).ok) return 38;

    calc::Matrix3 matrix; matrix.rows = matrix.cols = 2;
    matrix.values[0][0] = 4; matrix.values[0][1] = 7;
    matrix.values[1][0] = 2; matrix.values[1][1] = 6;
    calc::Matrix3 inverse;
    if (!calc::matrixInverse(matrix, inverse) || !close(inverse.values[0][0], 0.6) || !close(inverse.values[1][0], -0.2)) return 8;

    calc::Matrix3 matrix3; matrix3.rows = matrix3.cols = 3;
    const double entries[3][3] = {{1, 2, 3}, {0, 1, 4}, {5, 6, 0}};
    for (int row = 0; row < 3; ++row) for (int col = 0; col < 3; ++col) matrix3.values[row][col] = entries[row][col];
    if (!calc::matrixInverse(matrix3, inverse) || !close(inverse.values[0][0], -24)
        || !close(inverse.values[1][2], -4) || !close(inverse.values[2][1], 4)) return 11;

    calc::Vector3 cross = calc::vectorCross({1, 0, 0}, {0, 1, 0});
    if (!close(cross.z, 1)) return 9;

    calc::TableResult table;
    if (!calc::makeTable(square, nullptr, -2, 2, 1, table) || table.count != 5 || !close(table.rows[3].y, 1)) return 10;
    std::cout << "portable calculator features passed\n";
}