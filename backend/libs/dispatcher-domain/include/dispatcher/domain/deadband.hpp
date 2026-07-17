#pragma once

#include <cmath>

namespace dispatcher::domain
{
    class Deadband
    {
    public:
        explicit Deadband(double value = 0.0)
            : value_(value)
        {
        }

        [[nodiscard]] double value() const noexcept
        {
            return value_;
        }

        [[nodiscard]] bool enabled() const noexcept
        {
            return value_ > 0.0;
        }

        [[nodiscard]] bool accepts_change(double previous, double current) const noexcept
        {
            if (!enabled())
            {
                return previous != current;
            }

            return std::abs(current - previous) >= value_;
        }

    private:
        double value_;
    };
}