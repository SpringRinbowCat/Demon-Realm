#include "Domain/Modifier/ModifierCollection.hpp"

#include <algorithm>
#include <cstddef>

namespace DemonRealm
{

ModifierCollection::ModifierCollection()
    : _modifiers()
    , _aggregates()
    , _aggregatesValid(true)
    , _revision(0)
{
}

void ModifierCollection::add(const Modifier& modifier)
{
    _modifiers.push_back(modifier);
    _aggregatesValid = false;
    ++_revision;
}

bool ModifierCollection::removeBySource(const std::string& sourceId)
{
    const auto removeBegin = std::remove_if(_modifiers.begin(),
                                            _modifiers.end(),
                                            [&sourceId](const Modifier& modifier)
                                            { return modifier.sourceId == sourceId; });
    if (removeBegin == _modifiers.end())
    {
        return false;
    }

    _modifiers.erase(removeBegin, _modifiers.end());
    _aggregatesValid = false;
    ++_revision;
    return true;
}

void ModifierCollection::clear()
{
    if (_modifiers.empty())
    {
        return;
    }

    _modifiers.clear();
    _aggregatesValid = false;
    ++_revision;
}

const ModifierAggregate& ModifierCollection::getAggregate(ModifierTarget target) const
{
    if (!_aggregatesValid)
    {
        _rebuildAggregates();
    }

    return _aggregates[static_cast<std::size_t>(target)];
}

unsigned long long ModifierCollection::getRevision() const
{
    return _revision;
}

void ModifierCollection::_rebuildAggregates() const
{
    for (ModifierAggregate& aggregate : _aggregates)
    {
        aggregate = ModifierAggregate();
    }

    for (const Modifier& modifier : _modifiers)
    {
        _aggregates[static_cast<std::size_t>(modifier.target)].accumulate(modifier);
    }

    _aggregatesValid = true;
}

}  // namespace DemonRealm
