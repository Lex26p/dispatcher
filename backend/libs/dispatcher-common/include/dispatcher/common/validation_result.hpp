#pragma once

#include <string>
#include <utility>
#include <vector>

namespace dispatcher::common
{
    struct ValidationError
    {
        std::string field;
        std::string message;
    };

    class ValidationResult
    {
    public:
        [[nodiscard]] bool valid() const noexcept
        {
            return errors_.empty();
        }

        [[nodiscard]] bool has_errors() const noexcept
        {
            return !errors_.empty();
        }

        void add_error(std::string field, std::string message)
        {
            errors_.push_back(
                ValidationError{
                    std::move(field),
                    std::move(message)
                }
            );
        }

        [[nodiscard]] const std::vector<ValidationError>& errors() const noexcept
        {
            return errors_;
        }

    private:
        std::vector<ValidationError> errors_;
    };
}