#include <dispatcher/core/current_state_store.hpp>

#include <utility>

namespace dispatcher::core
{
    void CurrentStateStore::update(TelemetryValue value)
    {
        const auto key = value.tag_id().value();
        values_.insert_or_assign(key, std::move(value));
    }

    std::optional<CurrentStateStore::TelemetryValue> CurrentStateStore::find(const TagId& tag_id) const
    {
        const auto it = values_.find(tag_id.value());

        if (it == values_.end())
        {
            return std::nullopt;
        }

        return it->second;
    }

    bool CurrentStateStore::contains(const TagId& tag_id) const
    {
        return values_.contains(tag_id.value());
    }

    std::size_t CurrentStateStore::size() const noexcept
    {
        return values_.size();
    }

    void CurrentStateStore::clear()
    {
        values_.clear();
    }
}