#include <dispatcher/api/api_page.hpp>
#include <dispatcher/api/api_page_request.hpp>

#include <gtest/gtest.h>

TEST(ApiPageRequestTests, DefaultPageRequestWorks)
{
    const dispatcher::api::ApiPageRequest request;

    EXPECT_FALSE(request.has_offset());
    EXPECT_TRUE(request.has_limit());

    EXPECT_EQ(request.start_index(), 0);
    EXPECT_EQ(request.end_index_exclusive(), 100);
}

TEST(ApiPageRequestTests, CustomPageRequestWorks)
{
    const dispatcher::api::ApiPageRequest request{
        .offset = 50,
        .limit = 25
    };

    EXPECT_TRUE(request.has_offset());
    EXPECT_TRUE(request.has_limit());

    EXPECT_EQ(request.start_index(), 50);
    EXPECT_EQ(request.end_index_exclusive(), 75);
}

TEST(ApiPageTests, DefaultPageWorks)
{
    const dispatcher::api::ApiPage page;

    EXPECT_EQ(page.offset(), 0);
    EXPECT_EQ(page.limit(), 100);
    EXPECT_EQ(page.returned_count(), 0);
    EXPECT_EQ(page.total_count(), 0);

    EXPECT_TRUE(page.empty());
    EXPECT_FALSE(page.has_next_page());
    EXPECT_FALSE(page.has_previous_page());

    EXPECT_EQ(page.next_offset(), 0);
    EXPECT_EQ(page.previous_offset(), 0);
}

TEST(ApiPageTests, PageWithNextAndPreviousWorks)
{
    const dispatcher::api::ApiPage page(
        50,
        25,
        25,
        100
    );

    EXPECT_EQ(page.offset(), 50);
    EXPECT_EQ(page.limit(), 25);
    EXPECT_EQ(page.returned_count(), 25);
    EXPECT_EQ(page.total_count(), 100);

    EXPECT_FALSE(page.empty());
    EXPECT_TRUE(page.has_next_page());
    EXPECT_TRUE(page.has_previous_page());

    EXPECT_EQ(page.next_offset(), 75);
    EXPECT_EQ(page.previous_offset(), 25);
}

TEST(ApiPageTests, LastPageHasNoNextPage)
{
    const dispatcher::api::ApiPage page(
        75,
        25,
        25,
        100
    );

    EXPECT_FALSE(page.has_next_page());
    EXPECT_TRUE(page.has_previous_page());

    EXPECT_EQ(page.next_offset(), 75);
    EXPECT_EQ(page.previous_offset(), 50);
}

TEST(ApiPageTests, PartialLastPageHasNoNextPage)
{
    const dispatcher::api::ApiPage page(
        90,
        25,
        10,
        100
    );

    EXPECT_FALSE(page.has_next_page());
    EXPECT_TRUE(page.has_previous_page());

    EXPECT_EQ(page.next_offset(), 90);
    EXPECT_EQ(page.previous_offset(), 65);
}

TEST(ApiPageTests, FromRequestWorks)
{
    const dispatcher::api::ApiPageRequest request{
        .offset = 20,
        .limit = 10
    };

    const auto page = dispatcher::api::ApiPage::from_request(
        request,
        10,
        45
    );

    EXPECT_EQ(page.offset(), 20);
    EXPECT_EQ(page.limit(), 10);
    EXPECT_EQ(page.returned_count(), 10);
    EXPECT_EQ(page.total_count(), 45);

    EXPECT_TRUE(page.has_next_page());
    EXPECT_TRUE(page.has_previous_page());
    EXPECT_EQ(page.next_offset(), 30);
    EXPECT_EQ(page.previous_offset(), 10);
}