#include <dispatcher/http/http_json.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

TEST(HttpJsonTests, EscapesQuotesAndBackslashes)
{
    const auto escaped =
        dispatcher::http::HttpJson::escape(
            "tag \"pressure\" uses C:\\data"
        );

    EXPECT_EQ(
        escaped,
        "tag \\\"pressure\\\" uses C:\\\\data"
    );
}

TEST(HttpJsonTests, EscapesControlCharacters)
{
    const auto escaped =
        dispatcher::http::HttpJson::escape(
            "line1\nline2\r\n\tindent"
        );

    EXPECT_EQ(
        escaped,
        "line1\\nline2\\r\\n\\tindent"
    );
}

TEST(HttpJsonTests, BuildsQuotedString)
{
    EXPECT_EQ(
        dispatcher::http::HttpJson::quoted(
            "runtime"
        ),
        "\"runtime\""
    );
}

TEST(HttpJsonTests, BuildsStringField)
{
    EXPECT_EQ(
        dispatcher::http::HttpJson::string_field(
            "status",
            "available"
        ),
        "\"status\":\"available\""
    );
}

TEST(HttpJsonTests, BuildsBoolField)
{
    EXPECT_EQ(
        dispatcher::http::HttpJson::bool_field(
            "ready",
            true
        ),
        "\"ready\":true"
    );

    EXPECT_EQ(
        dispatcher::http::HttpJson::bool_field(
            "ready",
            false
        ),
        "\"ready\":false"
    );
}

TEST(HttpJsonTests, BuildsIntegerFields)
{
    EXPECT_EQ(
        dispatcher::http::HttpJson::int_field(
            "active_alarm_count",
            static_cast<std::int64_t>(-1)
        ),
        "\"active_alarm_count\":-1"
    );

    EXPECT_EQ(
        dispatcher::http::HttpJson::uint_field(
            "check_count",
            static_cast<std::uint64_t>(2)
        ),
        "\"check_count\":2"
    );
}

TEST(HttpJsonTests, BuildsNullField)
{
    EXPECT_EQ(
        dispatcher::http::HttpJson::null_field(
            "last_error"
        ),
        "\"last_error\":null"
    );
}

TEST(HttpJsonTests, BuildsRawField)
{
    EXPECT_EQ(
        dispatcher::http::HttpJson::raw_field(
            "items",
            "[]"
        ),
        "\"items\":[]"
    );
}

TEST(HttpJsonTests, BuildsObjectFromInitializerList)
{
    const auto json =
        dispatcher::http::HttpJson::object(
            {
                dispatcher::http::HttpJson::string_field(
                    "status",
                    "available"
                ),
                dispatcher::http::HttpJson::bool_field(
                    "ready",
                    true
                )
            }
        );

    EXPECT_EQ(
        json,
        "{\"status\":\"available\",\"ready\":true}"
    );
}

TEST(HttpJsonTests, BuildsArrayFromInitializerList)
{
    const auto json =
        dispatcher::http::HttpJson::array(
            {
                dispatcher::http::HttpJson::quoted(
                    "health"
                ),
                dispatcher::http::HttpJson::quoted(
                    "runtime"
                )
            }
        );

    EXPECT_EQ(
        json,
        "[\"health\",\"runtime\"]"
    );
}

TEST(HttpJsonTests, BuildsNestedObject)
{
    const auto checks =
        dispatcher::http::HttpJson::array(
            {
                dispatcher::http::HttpJson::object(
                    {
                        dispatcher::http::HttpJson::string_field(
                            "name",
                            "runtime"
                        ),
                        dispatcher::http::HttpJson::string_field(
                            "status",
                            "healthy"
                        )
                    }
                )
            }
        );

    const auto json =
        dispatcher::http::HttpJson::object(
            {
                dispatcher::http::HttpJson::string_field(
                    "status",
                    "healthy"
                ),
                dispatcher::http::HttpJson::raw_field(
                    "checks",
                    checks
                )
            }
        );

    EXPECT_EQ(
        json,
        "{\"status\":\"healthy\",\"checks\":[{\"name\":\"runtime\",\"status\":\"healthy\"}]}"
    );
}

TEST(HttpJsonTests, BuildsArrayFromVector)
{
    const std::vector<std::string> values{
        dispatcher::http::HttpJson::quoted(
            "a"
        ),
        dispatcher::http::HttpJson::quoted(
            "b"
        ),
        dispatcher::http::HttpJson::quoted(
            "c"
        )
    };

    EXPECT_EQ(
        dispatcher::http::HttpJson::array(
            values
        ),
        "[\"a\",\"b\",\"c\"]"
    );
}