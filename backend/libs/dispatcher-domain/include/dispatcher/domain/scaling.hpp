#pragma once

namespace dispatcher::domain
{
    class Scaling
    {
    public:
        Scaling(double multiplier = 1.0, double offset = 0.0)
            : multiplier_(multiplier)
            , offset_(offset)
        {
        }

        [[nodiscard]] double multiplier() const noexcept
        {
            return multiplier_;
        }

        [[nodiscard]] double offset() const noexcept
        {
            return offset_;
        }

        [[nodiscard]] double apply(double raw_value) const noexcept
        {
            return raw_value * multiplier_ + offset_;
        }

    private:
        double multiplier_;
        double offset_;
    };
}