#include <dispatcher/api/alarm_acknowledgement_api_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::api
{
    AlarmAcknowledgementApiResult AlarmAcknowledgementApiResult::success(
        dispatcher::alarm::AlarmAcknowledgementResult acknowledgement
    )
    {
        return AlarmAcknowledgementApiResult(
            ApiResult::success(),
            std::move(acknowledgement)
        );
    }

    AlarmAcknowledgementApiResult AlarmAcknowledgementApiResult::failure(
        ApiStatus status,
        std::string operation,
        std::string resource,
        std::string field,
        std::string message
    )
    {
        return AlarmAcknowledgementApiResult(
            ApiResult::failure(
                status,
                std::move(operation),
                std::move(resource),
                std::move(field),
                std::move(message)
            ),
            std::nullopt
        );
    }

    bool AlarmAcknowledgementApiResult::ok() const noexcept
    {
        return result_.ok();
    }

    bool AlarmAcknowledgementApiResult::failed() const noexcept
    {
        return result_.failed();
    }

    ApiStatus AlarmAcknowledgementApiResult::status() const noexcept
    {
        return result_.status();
    }

    const ApiResult& AlarmAcknowledgementApiResult::result() const noexcept
    {
        return result_;
    }

    const ApiError& AlarmAcknowledgementApiResult::error() const noexcept
    {
        return result_.error();
    }

    bool AlarmAcknowledgementApiResult::has_acknowledgement() const noexcept
    {
        return acknowledgement_.has_value();
    }

    const dispatcher::alarm::AlarmAcknowledgementResult&
        AlarmAcknowledgementApiResult::acknowledgement() const
    {
        if (!acknowledgement_.has_value())
        {
            throw std::logic_error(
                "AlarmAcknowledgementApiResult does not contain an acknowledgement result"
            );
        }

        return acknowledgement_.value();
    }

    AlarmAcknowledgementApiResult::AlarmAcknowledgementApiResult(
        ApiResult result,
        std::optional<dispatcher::alarm::AlarmAcknowledgementResult>
        acknowledgement
    )
        : result_(std::move(result))
        , acknowledgement_(std::move(acknowledgement))
    {
    }
}