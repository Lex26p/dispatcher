#include <dispatcher/common/version.hpp>

#include <gtest/gtest.h>

#include <string_view>

TEST(CommonVersionTests, ReturnsProjectVersion)
{
    constexpr std::string_view expected_version = "0.1.0";

    EXPECT_EQ(dispatcher::common::version(), expected_version);
}

TEST(CommonVersionTests, VersionIsNotEmpty)
{
    EXPECT_FALSE(dispatcher::common::version().empty());
}