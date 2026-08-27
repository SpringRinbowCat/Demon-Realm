#pragma once

#include <string>

namespace DemonRealm
{

/// 非负定点十进制数值，按十进制字符串存储和计算，固定保留 4 位小数。
///
/// 职责：承载攻击力、伤害、金币、剩余血量、攻击间隔和各类 buff 系数。挂机游戏的数值
/// 会持续膨胀，浮点会丢精度、64 位整数会溢出，因此这里用十进制数字串保存，加减乘和比较
/// 都在数字串上完成，位数只受内存限制。
///
/// 取整：只保留 4 位小数，超出部分一律向下取整（直接截断），不做四舍五入。乘法先算出
/// 8 位小数的中间结果，再截断回 4 位。
///
/// 取值范围：只表示非负数。减法结果为负时钳制为 0；因此"减少类"效果（例如缩短攻击
/// 间隔）必须用小于 1 的乘法系数表达，不要试图用负的加法项。
///
/// 性能：单次运算的代价与数字位数成线性，位数在数值膨胀到天文量级前都只有几十位。
/// 内部表示对外不可见，后续若需要提速，可改成按 9 位一组的紧凑分组存储而不影响调用方。
/// 调用方仍应避免在每帧对同一份数据反复重算，应把结果缓存下来。
///
/// 线程要求：值类型，不持有外部状态，可在任意线程使用。
class Decimal
{
public:
    /// 保留的小数位数，也是对外的取整精度。
    static const int kFractionDigits = 4;

    /// 解析非负十进制字符串。
    /// 参数 text：形如 "0"、"12"、"4.0"、"2.43281" 的字符串，小数点前必须有数字。
    /// 参数 value：解析结果，仅在返回 true 时被写入。
    /// 返回值：格式合法返回 true；空串、含非数字字符或出现多个小数点返回 false。
    /// 说明：小数位多于 4 位时按向下取整截断，少于 4 位时补零。
    static bool tryParse(const std::string& text, Decimal& value);

    /// 返回数值 0。
    static Decimal zero();

    /// 返回数值 1，可作为乘法系数的初始值。
    static Decimal one();

    /// 由非负整数计数构造数值，例如把"本次补算的攻击次数"转成数值参与乘法。
    /// 参数 count：非负计数。
    /// 返回值：等于 count 的数值。
    static Decimal fromCount(unsigned long long count);

    /// 构造数值 0。
    Decimal();

    /// 相加。
    /// 参数 other：加数。
    /// 返回值：和。
    Decimal add(const Decimal& other) const;

    /// 相减，结果为负时钳制为 0。
    /// 参数 other：减数。
    /// 返回值：差，或 0。
    Decimal subtractClampedToZero(const Decimal& other) const;

    /// 相乘，结果向下取整到 4 位小数。
    /// 参数 other：乘数。
    /// 返回值：积。
    Decimal multiply(const Decimal& other) const;

    /// 比较大小。
    /// 参数 other：比较对象。
    /// 返回值：小于返回 -1，等于返回 0，大于返回 1。
    int compare(const Decimal& other) const;

    /// 是否为 0。
    bool isZero() const;

    /// 转成固定 4 位小数的十进制字符串，例如 "0.0000"、"4.0000"、"2.4328"。
    /// 返回值：规范化字符串，整数部分无多余前导零。展示格式化由表现层负责。
    std::string toString() const;

    /// 转成 double，仅用于与引擎帧时间这类小量级的比较。
    /// 返回值：近似值；数值很大时会丢失精度甚至溢出，禁止用于金币、血量等会膨胀的数值。
    double toDouble() const;

private:
    /// 数值放大 10^4 后的十进制数字串，无前导零，值为 0 时是 "0"。
    std::string _scaledDigits;
};

}  // namespace DemonRealm
