#include "Presentation/Format/SkillDescriptionFormatter.hpp"

#include <cstddef>
#include <string>
#include <vector>

#include "Domain/Numeric/Decimal.hpp"
#include "Presentation/Format/NumberFormatter.hpp"

namespace DemonRealm
{
namespace SkillDescriptionFormatter
{
namespace
{

/// 占位符的起止字符。
const char kPlaceholderBegin = '{';
const char kPlaceholderEnd = '}';

/// 支持的运算符。
const char kMultiplyOperator = '*';
const char kDivideOperator = '/';

/// 变量名的英雄前缀模板部分。
const char* const kHeroPrefix = "hero_";
const char kHeroPrefixSeparator = '_';

/// 变量名里可用的字段名。
const char* const kAttackField = "Attack";
const char* const kAttackIntervalField = "AttackInterval";
const char* const kHeroLevelField = "HeroLevel";
const char* const kAttackLevelField = "AttackLevel";
const char* const kAttackIntervalLevelField = "AttackIntervalLevel";

/// 去掉首尾空白。
std::string trim(const std::string& text)
{
    const std::size_t first = text.find_first_not_of(" \t");
    if (first == std::string::npos)
    {
        return std::string();
    }

    const std::size_t last = text.find_last_not_of(" \t");
    return text.substr(first, last - first + 1);
}

/// 把变量名解析成数值。
/// 参数 name：变量名。
/// 参数 values：英雄数值。
/// 参数 result：解析结果。
/// 返回值：变量属于该英雄且字段已知时返回 true。
bool resolveVariable(const std::string& name, const HeroValues& values, Decimal& result)
{
    const std::string prefix = std::string(kHeroPrefix) + values.heroId + std::string(1, kHeroPrefixSeparator);
    if (name.size() <= prefix.size() || name.compare(0, prefix.size(), prefix) != 0)
    {
        return false;
    }

    const std::string field = name.substr(prefix.size());
    if (field == kAttackField)
    {
        return Decimal::tryParse(values.attack, result);
    }
    if (field == kAttackIntervalField)
    {
        return Decimal::tryParse(values.attackIntervalSeconds, result);
    }
    if (field == kHeroLevelField)
    {
        return Decimal::tryParse(values.heroLevel, result);
    }
    if (field == kAttackLevelField)
    {
        return Decimal::tryParse(values.attackLevel, result);
    }
    if (field == kAttackIntervalLevelField)
    {
        return Decimal::tryParse(values.attackIntervalLevel, result);
    }

    return false;
}

/// 把一个操作数解析成数值：既可能是变量名，也可能是直接写的数字。
bool resolveOperand(const std::string& token, const HeroValues& values, Decimal& result)
{
    const std::string operand = trim(token);
    if (operand.empty())
    {
        return false;
    }

    if (Decimal::tryParse(operand, result))
    {
        return true;
    }

    return resolveVariable(operand, values, result);
}

/// 求值一个只含变量、数字、乘号和除号的表达式。
/// 参数 expression：大括号内的原始内容。
/// 参数 values：英雄数值。
/// 参数 result：求值结果。
/// 返回值：表达式合法且可求值时返回 true。
bool evaluate(const std::string& expression, const HeroValues& values, Decimal& result)
{
    std::vector<std::string> operands;
    std::vector<char> operators;

    std::string current;
    for (const char character : expression)
    {
        if (character == kMultiplyOperator || character == kDivideOperator)
        {
            operands.push_back(current);
            operators.push_back(character);
            current.clear();
            continue;
        }

        current.push_back(character);
    }
    operands.push_back(current);

    Decimal accumulated;
    if (!resolveOperand(operands.front(), values, accumulated))
    {
        return false;
    }

    // 只有乘除，优先级相同，按从左到右依次结算。
    for (std::size_t index = 0; index < operators.size(); ++index)
    {
        Decimal operand;
        if (!resolveOperand(operands[index + 1], values, operand))
        {
            return false;
        }

        if (operators[index] == kMultiplyOperator)
        {
            accumulated = accumulated.multiply(operand);
            continue;
        }

        Decimal quotient;
        if (!accumulated.tryDivide(operand, quotient))
        {
            return false;
        }

        accumulated = quotient;
    }

    result = accumulated;
    return true;
}

}  // namespace

std::string format(const std::string& description, const HeroValues& values)
{
    std::string formatted;
    formatted.reserve(description.size());

    std::size_t cursor = 0;
    while (cursor < description.size())
    {
        const std::size_t placeholderBegin = description.find(kPlaceholderBegin, cursor);
        if (placeholderBegin == std::string::npos)
        {
            formatted.append(description, cursor, std::string::npos);
            break;
        }

        const std::size_t placeholderEnd = description.find(kPlaceholderEnd, placeholderBegin + 1);
        if (placeholderEnd == std::string::npos)
        {
            // 没有闭合的大括号，剩余内容原样保留。
            formatted.append(description, cursor, std::string::npos);
            break;
        }

        formatted.append(description, cursor, placeholderBegin - cursor);

        const std::string expression =
            description.substr(placeholderBegin + 1, placeholderEnd - placeholderBegin - 1);
        Decimal value;
        if (evaluate(expression, values, value))
        {
            formatted += NumberFormatter::formatCompactValue(value.toString());
        }
        else
        {
            // 求值失败时保留原样，让配置里的问题在界面上直接可见。
            formatted.append(description, placeholderBegin, placeholderEnd - placeholderBegin + 1);
        }

        cursor = placeholderEnd + 1;
    }

    return formatted;
}

}  // namespace SkillDescriptionFormatter

}  // namespace DemonRealm
