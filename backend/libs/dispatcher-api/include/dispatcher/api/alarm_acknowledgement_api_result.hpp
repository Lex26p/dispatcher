#pragma once

#include <dispatcher/alarm/alarm_acknowledgement_result.hpp>
#include <dispatcher/api/api_result.hpp>
#include <dispatcher/api/api_status.hpp>

#include <optional>
#include <string>

namespace dispatcher::api
{
    class AlarmAcknowledgementApiResult
    {
    public:
        [[nodiscard]] static AlarmAcknowledgementApiResult success(
            dispatcher::alarm::AlarmAcknowledgementResult acknowledgement
        );

        [[nodiscard]] static AlarmAcknowledgementApiResult failure(
            ApiStatus status,
            std::string operation = {},
            std::string resource = {},
            std::string field = {},
            std::string message = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] ApiStatus status() const noexcept;

        [[nodiscard]] const ApiResult& result() const noexcept;

        [[nodiscard]] const ApiError& error() const noexcept;

        [[nodiscard]] bool has_acknowledgement() const noexcept;

        [[nodiscard]] const dispatcher::alarm::AlarmAcknowledgementResult&
            acknowledgement() const;

    private:
        AlarmAcknowledgementApiResult(
            ApiResult result,
            std::optional<dispatcher::alarm::AlarmAcknowledgementResult>
            acknowledgement
        );

        ApiResult result_;
        std::optional<dispatcher::alarm::AlarmAcknowledgementResult>
            acknowledgement_;
    };
}