#include "Domain/Modifier/ModifierAggregate.hpp"

namespace DemonRealm
{

ModifierAggregate::ModifierAggregate()
    : _flatBonus(Decimal::zero())
    , _multiplier(Decimal::one())
{
}

void ModifierAggregate::accumulate(const Modifier& modifier)
{
    switch (modifier.operation)
    {
        case ModifierOperation::Add:
            _flatBonus = _flatBonus.add(modifier.value);
            break;

        case ModifierOperation::Multiply:
            _multiplier = _multiplier.multiply(modifier.value);
            break;
    }
}

ModifierAggregate ModifierAggregate::combinedWith(const ModifierAggregate& other) const
{
    ModifierAggregate combined;
    combined._flatBonus = _flatBonus.add(other._flatBonus);
    combined._multiplier = _multiplier.multiply(other._multiplier);
    return combined;
}

Decimal ModifierAggregate::applyTo(const Decimal& baseValue) const
{
    return baseValue.add(_flatBonus).multiply(_multiplier);
}

}  // namespace DemonRealm
