#include "Domain/Numeric/Decimal.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <vector>

namespace DemonRealm
{
namespace
{

/// 十进制的基数。
const int kDecimalBase = 10;

/// 数字字符 '0'，用于数字与字符之间的换算。
const char kZeroDigit = '0';

/// 数字字符 '9'，用于合法性判断。
const char kNineDigit = '9';

/// 小数点字符。
const char kDecimalPoint = '.';

/// 值为 0 的规范数字串。
const char* const kZeroDigits = "0";

/// 去掉前导零。
/// 参数 digits：十进制数字串，允许含前导零。
/// 返回值：无前导零的数字串；全为零时返回 "0"。
std::string stripLeadingZeros(const std::string& digits)
{
    const std::size_t firstNonZero = digits.find_first_not_of(kZeroDigit);
    if (firstNonZero == std::string::npos)
    {
        return std::string(kZeroDigits);
    }

    return digits.substr(firstNonZero);
}

/// 数字串相加。
/// 参数 left、right：无前导零的十进制数字串。
/// 返回值：和的数字串。
std::string addDigits(const std::string& left, const std::string& right)
{
    std::string result;
    result.reserve(std::max(left.size(), right.size()) + 1);

    int carry = 0;
    std::size_t leftIndex = left.size();
    std::size_t rightIndex = right.size();
    while (leftIndex > 0 || rightIndex > 0 || carry != 0)
    {
        int sum = carry;
        if (leftIndex > 0)
        {
            --leftIndex;
            sum += left[leftIndex] - kZeroDigit;
        }
        if (rightIndex > 0)
        {
            --rightIndex;
            sum += right[rightIndex] - kZeroDigit;
        }

        carry = sum / kDecimalBase;
        result.push_back(static_cast<char>(kZeroDigit + sum % kDecimalBase));
    }

    std::reverse(result.begin(), result.end());
    return result;
}

/// 比较两个无前导零的数字串。
/// 参数 left、right：无前导零的十进制数字串。
/// 返回值：小于返回 -1，等于返回 0，大于返回 1。
int compareDigits(const std::string& left, const std::string& right)
{
    if (left.size() != right.size())
    {
        return left.size() < right.size() ? -1 : 1;
    }
    if (left == right)
    {
        return 0;
    }

    return left < right ? -1 : 1;
}

/// 数字串相减。
/// 参数 left：被减数，必须大于或等于 right。
/// 参数 right：减数。
/// 返回值：差的数字串，无前导零。
std::string subtractDigits(const std::string& left, const std::string& right)
{
    std::string result;
    result.reserve(left.size());

    int borrow = 0;
    std::size_t leftIndex = left.size();
    std::size_t rightIndex = right.size();
    while (leftIndex > 0)
    {
        --leftIndex;
        int digit = (left[leftIndex] - kZeroDigit) - borrow;
        if (rightIndex > 0)
        {
            --rightIndex;
            digit -= right[rightIndex] - kZeroDigit;
        }

        if (digit < 0)
        {
            digit += kDecimalBase;
            borrow = 1;
        }
        else
        {
            borrow = 0;
        }

        result.push_back(static_cast<char>(kZeroDigit + digit));
    }

    std::reverse(result.begin(), result.end());
    return stripLeadingZeros(result);
}

/// 数字串相乘。
/// 参数 left、right：无前导零的十进制数字串。
/// 返回值：积的数字串，无前导零。
std::string multiplyDigits(const std::string& left, const std::string& right)
{
    if (left == kZeroDigits || right == kZeroDigits)
    {
        return std::string(kZeroDigits);
    }

    // 按竖式逐位累加，再统一进位；下标 0 是最高位。
    std::vector<int> columns(left.size() + right.size(), 0);
    for (std::size_t leftIndex = left.size(); leftIndex > 0; --leftIndex)
    {
        for (std::size_t rightIndex = right.size(); rightIndex > 0; --rightIndex)
        {
            const int product = (left[leftIndex - 1] - kZeroDigit) * (right[rightIndex - 1] - kZeroDigit);
            columns[leftIndex + rightIndex - 1] += product;
        }
    }

    for (std::size_t index = columns.size(); index > 1; --index)
    {
        columns[index - 2] += columns[index - 1] / kDecimalBase;
        columns[index - 1] %= kDecimalBase;
    }

    std::string result;
    result.reserve(columns.size());
    for (const int digit : columns)
    {
        result.push_back(static_cast<char>(kZeroDigit + digit));
    }

    return stripLeadingZeros(result);
}

/// 数字串相除，取整数商。
/// 参数 dividend：被除数，无前导零的十进制数字串。
/// 参数 divisor：除数，无前导零且不为 "0"。
/// 返回值：向下取整的商，无前导零。
std::string divideDigits(const std::string& dividend, const std::string& divisor)
{
    // 竖式除法：逐位把下一位带进余数，再用减法试出该位的商。
    std::string quotient;
    quotient.reserve(dividend.size());

    std::string remainder(kZeroDigits);
    for (const char digit : dividend)
    {
        remainder.push_back(digit);
        remainder = stripLeadingZeros(remainder);

        int quotientDigit = 0;
        while (compareDigits(remainder, divisor) >= 0)
        {
            remainder = subtractDigits(remainder, divisor);
            ++quotientDigit;
        }

        quotient.push_back(static_cast<char>(kZeroDigit + quotientDigit));
    }

    return stripLeadingZeros(quotient);
}

/// 去掉数字串末尾若干位，等价于向下取整的按位除法。
/// 参数 digits：无前导零的十进制数字串。
/// 参数 count：要去掉的低位个数。
/// 返回值：截断后的数字串；位数不足时返回 "0"。
std::string removeLowestDigits(const std::string& digits, std::size_t count)
{
    if (digits.size() <= count)
    {
        return std::string(kZeroDigits);
    }

    return stripLeadingZeros(digits.substr(0, digits.size() - count));
}

}  // namespace

bool Decimal::tryParse(const std::string& text, Decimal& value)
{
    if (text.empty())
    {
        return false;
    }

    std::string integerPart;
    std::string fractionPart;
    bool seenDecimalPoint = false;
    for (const char character : text)
    {
        if (character == kDecimalPoint)
        {
            if (seenDecimalPoint)
            {
                return false;
            }

            seenDecimalPoint = true;
            continue;
        }

        if (character < kZeroDigit || character > kNineDigit)
        {
            return false;
        }

        if (seenDecimalPoint)
        {
            fractionPart.push_back(character);
        }
        else
        {
            integerPart.push_back(character);
        }
    }

    if (integerPart.empty())
    {
        return false;
    }

    // resize 同时完成两件事：小数位不足时补零，多于 4 位时按向下取整截断。
    fractionPart.resize(static_cast<std::size_t>(kFractionDigits), kZeroDigit);
    value._scaledDigits = stripLeadingZeros(integerPart + fractionPart);
    return true;
}

Decimal Decimal::zero()
{
    return Decimal();
}

Decimal Decimal::one()
{
    return fromCount(1);
}

Decimal Decimal::fromCount(unsigned long long count)
{
    Decimal value;
    value._scaledDigits =
        stripLeadingZeros(std::to_string(count) + std::string(static_cast<std::size_t>(kFractionDigits), kZeroDigit));
    return value;
}

Decimal::Decimal()
    : _scaledDigits(kZeroDigits)
{
}

Decimal Decimal::add(const Decimal& other) const
{
    Decimal result;
    result._scaledDigits = addDigits(_scaledDigits, other._scaledDigits);
    return result;
}

Decimal Decimal::subtractClampedToZero(const Decimal& other) const
{
    if (compareDigits(_scaledDigits, other._scaledDigits) <= 0)
    {
        return Decimal();
    }

    Decimal result;
    result._scaledDigits = subtractDigits(_scaledDigits, other._scaledDigits);
    return result;
}

Decimal Decimal::multiply(const Decimal& other) const
{
    // 两个 10^4 放大值相乘得到 10^8 放大值，去掉多出的 4 位小数即向下取整回 4 位。
    const std::string product = multiplyDigits(_scaledDigits, other._scaledDigits);

    Decimal result;
    result._scaledDigits = removeLowestDigits(product, static_cast<std::size_t>(kFractionDigits));
    return result;
}

bool Decimal::tryDivide(const Decimal& divisor, Decimal& quotient) const
{
    if (divisor.isZero())
    {
        return false;
    }

    // 两个 10^4 放大值相除会把放大倍数抵消掉，先把被除数再放大 10^4 才能得到 4 位小数的商。
    const std::string scaledDividend =
        _scaledDigits == kZeroDigits
            ? std::string(kZeroDigits)
            : _scaledDigits + std::string(static_cast<std::size_t>(kFractionDigits), kZeroDigit);

    quotient._scaledDigits = divideDigits(scaledDividend, divisor._scaledDigits);
    return true;
}

int Decimal::compare(const Decimal& other) const
{
    return compareDigits(_scaledDigits, other._scaledDigits);
}

bool Decimal::isZero() const
{
    return _scaledDigits == kZeroDigits;
}

std::string Decimal::toString() const
{
    const std::size_t minimumLength = static_cast<std::size_t>(kFractionDigits) + 1;
    std::string padded = _scaledDigits;
    if (padded.size() < minimumLength)
    {
        padded.insert(0, minimumLength - padded.size(), kZeroDigit);
    }

    const std::size_t decimalPointPosition = padded.size() - static_cast<std::size_t>(kFractionDigits);
    return padded.substr(0, decimalPointPosition) + std::string(1, kDecimalPoint) + padded.substr(decimalPointPosition);
}

double Decimal::toDouble() const
{
    // 数值过大时 strtod 会返回 HUGE_VAL，调用方只允许在小量级场景使用该结果。
    const std::string text = toString();
    return std::strtod(text.c_str(), nullptr);
}

}  // namespace DemonRealm
