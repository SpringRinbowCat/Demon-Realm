#include "Presentation/Format/NumberFormatter.hpp"

#include <cstddef>

namespace DemonRealm
{
namespace NumberFormatter
{
namespace
{

/// 千分位分组长度。
const std::size_t kThousandsGroupSize = 3;

/// 秒数展示保留的最少小数位。
const std::size_t kMinimumSecondsFractionDigits = 2;

/// 千分位分隔符。
const char kGroupSeparator = ',';

/// 小数点。
const char kDecimalPoint = '.';

/// 数字字符 '0'。
const char kZeroDigit = '0';

/// 数字字符 '9'。
const char kNineDigit = '9';

/// 整数部分为空或非法时的兜底文字。
const char* const kZeroInteger = "0";

/// 秒数非法时的兜底文字。
const char* const kZeroSeconds = "0.00";

/// 取小数点前的整数部分。
/// 参数 canonicalDecimal：规范化定点小数字符串。
/// 返回值：整数部分；不含小数点时返回整串。
std::string takeIntegerPart(const std::string& canonicalDecimal)
{
    const std::size_t decimalPointPosition = canonicalDecimal.find(kDecimalPoint);
    if (decimalPointPosition == std::string::npos)
    {
        return canonicalDecimal;
    }

    return canonicalDecimal.substr(0, decimalPointPosition);
}

/// 取小数点后的小数部分。
/// 参数 canonicalDecimal：规范化定点小数字符串。
/// 返回值：小数部分；不含小数点时返回空串。
std::string takeFractionPart(const std::string& canonicalDecimal)
{
    const std::size_t decimalPointPosition = canonicalDecimal.find(kDecimalPoint);
    if (decimalPointPosition == std::string::npos)
    {
        return std::string();
    }

    return canonicalDecimal.substr(decimalPointPosition + 1);
}

/// 判断字符串是否只由数字组成且非空。
bool isDigitsOnly(const std::string& text)
{
    if (text.empty())
    {
        return false;
    }

    for (const char character : text)
    {
        if (character < kZeroDigit || character > kNineDigit)
        {
            return false;
        }
    }

    return true;
}

}  // namespace

std::string formatIntegerWithGroups(const std::string& canonicalDecimal)
{
    const std::string integerPart = takeIntegerPart(canonicalDecimal);
    if (!isDigitsOnly(integerPart))
    {
        return std::string(kZeroInteger);
    }

    std::string formatted;
    formatted.reserve(integerPart.size() + integerPart.size() / kThousandsGroupSize);
    for (std::size_t index = 0; index < integerPart.size(); ++index)
    {
        const std::size_t remaining = integerPart.size() - index;
        if (index > 0 && remaining % kThousandsGroupSize == 0)
        {
            formatted.push_back(kGroupSeparator);
        }

        formatted.push_back(integerPart[index]);
    }

    return formatted;
}

std::string formatCompactValue(const std::string& canonicalDecimal)
{
    const std::string integerPart = takeIntegerPart(canonicalDecimal);
    if (!isDigitsOnly(integerPart))
    {
        return std::string(kZeroInteger);
    }

    std::string fractionPart = takeFractionPart(canonicalDecimal);
    while (!fractionPart.empty() && fractionPart.back() == kZeroDigit)
    {
        fractionPart.pop_back();
    }

    const std::string groupedInteger = formatIntegerWithGroups(canonicalDecimal);
    if (fractionPart.empty())
    {
        return groupedInteger;
    }

    return groupedInteger + std::string(1, kDecimalPoint) + fractionPart;
}

std::string formatSecondsWithTrimmedFraction(const std::string& canonicalDecimal)
{
    const std::string integerPart = takeIntegerPart(canonicalDecimal);
    std::string fractionPart = takeFractionPart(canonicalDecimal);
    if (!isDigitsOnly(integerPart))
    {
        return std::string(kZeroSeconds);
    }

    // 小数位不足时补零，保证至少显示 2 位。
    if (fractionPart.size() < kMinimumSecondsFractionDigits)
    {
        fractionPart.resize(kMinimumSecondsFractionDigits, kZeroDigit);
    }

    // 去掉末尾多余的 0，但不低于最少位数。
    while (fractionPart.size() > kMinimumSecondsFractionDigits && fractionPart.back() == kZeroDigit)
    {
        fractionPart.pop_back();
    }

    return integerPart + std::string(1, kDecimalPoint) + fractionPart;
}

}  // namespace NumberFormatter

}  // namespace DemonRealm
