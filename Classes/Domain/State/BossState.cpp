#include "Domain/State/BossState.hpp"

namespace DemonRealm
{

BossState::BossState()
    : _maxHp(Decimal::zero())
    , _remainingHp(Decimal::zero())
{
}

BossState::BossState(const Decimal& maxHp)
    : _maxHp(maxHp)
    , _remainingHp(maxHp)
{
}

void BossState::resetTo(const Decimal& maxHp)
{
    _maxHp = maxHp;
    _remainingHp = maxHp;
}

Decimal BossState::applyDamage(const Decimal& damage)
{
    // 伤害超过剩余血量时只扣到 0，实际扣除量按剩余血量返回，金币产出据此结算。
    const Decimal appliedDamage = damage.compare(_remainingHp) > 0 ? _remainingHp : damage;
    _remainingHp = _remainingHp.subtractClampedToZero(appliedDamage);
    return appliedDamage;
}

const Decimal& BossState::getRemainingHp() const
{
    return _remainingHp;
}

const Decimal& BossState::getMaxHp() const
{
    return _maxHp;
}

bool BossState::isDefeated() const
{
    return _remainingHp.isZero();
}

}  // namespace DemonRealm
