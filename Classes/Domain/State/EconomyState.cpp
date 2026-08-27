#include "Domain/State/EconomyState.hpp"

namespace DemonRealm
{

EconomyState::EconomyState()
    : _goldAmount(Decimal::zero())
    , _modifiers()
{
}

Decimal EconomyState::addGoldFromDamage(const Decimal& damage, const ModifierAggregate& globalGoldAggregate)
{
    const ModifierAggregate goldAggregate =
        _modifiers.getAggregate(ModifierTarget::GoldGain).combinedWith(globalGoldAggregate);
    const Decimal reward = goldAggregate.applyTo(damage);
    _goldAmount = _goldAmount.add(reward);
    return reward;
}

void EconomyState::addGold(const Decimal& amount)
{
    _goldAmount = _goldAmount.add(amount);
}

bool EconomyState::trySpendGold(const Decimal& amount)
{
    if (_goldAmount.compare(amount) < 0)
    {
        return false;
    }

    _goldAmount = _goldAmount.subtractClampedToZero(amount);
    return true;
}

const Decimal& EconomyState::getGoldAmount() const
{
    return _goldAmount;
}

ModifierCollection& EconomyState::getModifiers()
{
    return _modifiers;
}

const ModifierCollection& EconomyState::getModifiers() const
{
    return _modifiers;
}

}  // namespace DemonRealm
