#include "Infrastructure/Random/TimeSeededRandomSource.hpp"

#include <chrono>
#include <random>

namespace DemonRealm
{
namespace
{

/// [0, 1) 区间被切分的份数，与 Decimal 的 4 位小数精度一致。
const unsigned long long kUnitValueStepCount = 10000;

}  // namespace

TimeSeededRandomSource::TimeSeededRandomSource()
    : _sampleIndex(0)
{
}

Decimal TimeSeededRandomSource::nextUnitValue()
{
    const std::chrono::high_resolution_clock::duration sinceEpoch =
        std::chrono::high_resolution_clock::now().time_since_epoch();
    const unsigned long long timeSeed = static_cast<unsigned long long>(sinceEpoch.count());

    ++_sampleIndex;
    std::mt19937_64 engine(timeSeed ^ _sampleIndex);
    std::uniform_int_distribution<unsigned long long> distribution(0, kUnitValueStepCount - 1);
    const unsigned long long step = distribution(engine);

    // step / 10000 在 4 位小数下可以精确表示，不会因取整丢掉概率精度。
    Decimal unitValue;
    if (!Decimal::fromCount(step).tryDivide(Decimal::fromCount(kUnitValueStepCount), unitValue))
    {
        return Decimal::zero();
    }

    return unitValue;
}

}  // namespace DemonRealm
