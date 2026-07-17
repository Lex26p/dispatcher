#include <dispatcher/http/http_alarm_contract.hpp>

#include <dispatcher/http/http_json.hpp>

#include <vector>

namespace dispatcher::http
{
    HttpAlarmSummary HttpAlarmContract::default_summary()
    {
        return HttpAlarmSummary{};
    }

    std::string HttpAlarmContract::item_to_json(
        const HttpAlarmItem& item
    )
    {
        return HttpJson::object(
            {
                HttpJson::string_field(
                    "id",
                    item.id
                ),
                HttpJson::string_field(
                    "name",
                    item.name
                ),
                HttpJson::string_field(
                    "tag",
                    item.tag
                ),
                HttpJson::string_field(
                    "severity",
                    item.severity
                ),
                HttpJson::string_field(
                    "state",
                    item.state
                ),
                HttpJson::string_field(
                    "message",
                    item.message
                ),
                HttpJson::string_field(
                    "source",
                    item.source
                ),
                HttpJson::bool_field(
                    "active",
                    item.active
                ),
                HttpJson::bool_field(
                    "acknowledged",
                    item.acknowledged
                ),
                HttpJson::bool_field(
                    "shelved",
                    item.shelved
                ),
                HttpJson::bool_field(
                    "suppressed",
                    item.suppressed
                ),
                HttpJson::bool_field(
                    "inhibited",
                    item.inhibited
                )
            }
        );
    }

    std::string HttpAlarmContract::items_to_json(
        const std::vector<HttpAlarmItem>& items
    )
    {
        std::vector<std::string> values;
        values.reserve(
            items.size()
        );

        for (const auto& item : items)
        {
            values.push_back(
                item_to_json(
                    item
                )
            );
        }

        return HttpJson::array(
            values
        );
    }

    std::string HttpAlarmContract::to_json(
        const HttpAlarmSummary& summary
    )
    {
        const auto severity_json =
            HttpJson::object(
                {
                    HttpJson::uint_field(
                        "critical_count",
                        summary.severity.critical_count
                    ),
                    HttpJson::uint_field(
                        "high_count",
                        summary.severity.high_count
                    ),
                    HttpJson::uint_field(
                        "medium_count",
                        summary.severity.medium_count
                    ),
                    HttpJson::uint_field(
                        "low_count",
                        summary.severity.low_count
                    ),
                    HttpJson::uint_field(
                        "info_count",
                        summary.severity.info_count
                    )
                }
            );

        const auto states_json =
            HttpJson::object(
                {
                    HttpJson::uint_field(
                        "active_count",
                        summary.states.active_count
                    ),
                    HttpJson::uint_field(
                        "acknowledged_count",
                        summary.states.acknowledged_count
                    ),
                    HttpJson::uint_field(
                        "unacknowledged_count",
                        summary.states.unacknowledged_count
                    ),
                    HttpJson::uint_field(
                        "shelved_count",
                        summary.states.shelved_count
                    ),
                    HttpJson::uint_field(
                        "suppressed_count",
                        summary.states.suppressed_count
                    ),
                    HttpJson::uint_field(
                        "inhibited_count",
                        summary.states.inhibited_count
                    )
                }
            );

        const auto items_json =
            items_to_json(
                summary.items
            );

        const auto summary_json =
            HttpJson::object(
                {
                    HttpJson::bool_field(
                        "available",
                        summary.available
                    ),
                    HttpJson::uint_field(
                        "total_count",
                        summary.items.size()
                    ),
                    HttpJson::uint_field(
                        "active_count",
                        summary.states.active_count
                    ),
                    HttpJson::uint_field(
                        "unacknowledged_count",
                        summary.states.unacknowledged_count
                    ),
                    HttpJson::uint_field(
                        "shelved_count",
                        summary.states.shelved_count
                    ),
                    HttpJson::uint_field(
                        "suppressed_count",
                        summary.states.suppressed_count
                    ),
                    HttpJson::uint_field(
                        "inhibited_count",
                        summary.states.inhibited_count
                    )
                }
            );

        const auto api_json =
            HttpJson::object(
                {
                    HttpJson::string_field(
                        "endpoint",
                        summary.endpoint
                    ),
                    HttpJson::string_field(
                        "path",
                        summary.path
                    ),
                    HttpJson::string_field(
                        "method",
                        summary.method
                    ),
                    HttpJson::string_field(
                        "source",
                        summary.source
                    )
                }
            );

        return HttpJson::object(
            {
                HttpJson::uint_field(
                    "schema_version",
                    summary.schema_version
                ),

                    // Backward-compatible top-level fields.
                    HttpJson::string_field(
                        "status",
                        summary.status
                    ),
                    HttpJson::string_field(
                        "endpoint",
                        summary.endpoint
                    ),
                    HttpJson::string_field(
                        "path",
                        summary.path
                    ),
                    HttpJson::string_field(
                        "method",
                        summary.method
                    ),
                    HttpJson::string_field(
                        "source",
                        summary.source
                    ),

                    // Structured contract fields.
                    HttpJson::raw_field(
                        "summary",
                        summary_json
                    ),
                    HttpJson::raw_field(
                        "severity",
                        severity_json
                    ),
                    HttpJson::raw_field(
                        "states",
                        states_json
                    ),
                    HttpJson::raw_field(
                        "items",
                        items_json
                    ),
                    HttpJson::raw_field(
                        "api",
                        api_json
                    )
            }
        );
    }

    std::string HttpAlarmContract::default_json()
    {
        return to_json(
            default_summary()
        );
    }
}