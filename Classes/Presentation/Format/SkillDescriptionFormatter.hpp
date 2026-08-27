#pragma once

#include <string>

namespace DemonRealm
{

/// 技能说明文字里的数值占位符替换。
///
/// 配置里的技能说明可以用大括号写出数值来源，例如
/// `造成等同于{hero_1_Attack}的伤害`、`永久增加{hero_1_AttackLevel*hero_1_HeroLevel/50}`。
/// 这里把大括号内的内容按当前英雄的实际数值求值，再嵌回句子里。
///
/// 支持的写法：
/// - 变量：`hero_<英雄id>_Attack`、`hero_<英雄id>_AttackInterval`、`hero_<英雄id>_HeroLevel`、
///   `hero_<英雄id>_AttackLevel`、`hero_<英雄id>_AttackIntervalLevel`。
/// - 数字：直接写十进制数字，例如 `50`、`0.5`。
/// - 运算：乘号与除号，按从左到右结合；不支持加减与嵌套括号。
///
/// 求值失败（未知变量、非法字符、除数为 0）时保留原样的大括号内容，让问题在界面上可见，
/// 而不是悄悄显示成 0。
///
/// 线程要求：纯函数，无状态，可在任意线程调用。
namespace SkillDescriptionFormatter
{

/// 求值所需的英雄数值。
///
/// 数值字段是规范化定点小数字符串，等级字段是十进制整数字符串，都直接来自展示快照。
struct HeroValues
{
    /// 英雄配置 id，用于匹配变量名里的英雄前缀。
    std::string heroId;

    /// 当前攻击力。
    std::string attack;

    /// 当前攻击间隔，单位秒。
    std::string attackIntervalSeconds;

    /// 当前英雄等级。
    std::string heroLevel;

    /// 当前攻击力等级。
    std::string attackLevel;

    /// 当前攻击速度等级。
    std::string attackIntervalLevel;
};

/// 把说明文字里的占位符替换成实际数值。
/// 参数 description：原始说明文字。
/// 参数 values：当前英雄的数值。
/// 返回值：替换后的说明文字。
std::string format(const std::string& description, const HeroValues& values);

}  // namespace SkillDescriptionFormatter

}  // namespace DemonRealm
