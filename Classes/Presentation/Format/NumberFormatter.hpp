#pragma once

#include <string>

namespace DemonRealm
{

/// 数值展示格式化。
///
/// 输入统一是业务层产出的规范化定点小数字符串（固定 4 位小数，例如 "99999.0000"）。
/// 业务层只负责算准数值，"怎么显示"全部收口在这里，避免各个页面各写一套格式规则。
///
/// 线程要求：纯函数，无状态，可在任意线程调用。
namespace NumberFormatter
{

/// 按整数展示数值，整数部分加千分位分隔符。
///
/// 小数部分被丢弃而不是四舍五入：业务数值本身已经向下取整，展示继续向下取整才不会
/// 出现"显示 100 金币但实际不够花"的错位。
///
/// 参数 canonicalDecimal：规范化定点小数字符串。
/// 返回值：形如 "0"、"99,999" 的展示文字；输入非法时返回 "0"。
std::string formatIntegerWithGroups(const std::string& canonicalDecimal);

/// 按秒数展示数值，小数位最少 2 位、最多 4 位。
///
/// 规则：先去掉末尾多余的 0，再保证至少保留 2 位。例如 "4.0000" 显示为 "4.00"，
/// "4.0050" 显示为 "4.005"，"2.4328" 保留 4 位。
///
/// 参数 canonicalDecimal：规范化定点小数字符串。
/// 返回值：形如 "4.00"、"2.4328" 的展示文字；输入非法时返回 "0.00"。
std::string formatSecondsWithTrimmedFraction(const std::string& canonicalDecimal);

}  // namespace NumberFormatter

}  // namespace DemonRealm
