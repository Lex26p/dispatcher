#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/operator_directory.hpp>
#include <dispatcher/domain/operator_directory_result.hpp>
#include <dispatcher/domain/operator_directory_status.hpp>
#include <dispatcher/domain/operator_identity.hpp>
#include <dispatcher/domain/operator_role.hpp>
#include <dispatcher/domain/operator_status.hpp>

#include <gtest/gtest.h>

#include <string>
#include <utility>

namespace
{
    dispatcher::domain::OperatorIdentity make_directory_identity(
        std::string operator_id,
        std::string username,
        dispatcher::domain::OperatorRole role =
        dispatcher::domain::OperatorRole::Operator,
        dispatcher::domain::OperatorStatus status =
        dispatcher::domain::OperatorStatus::Active
    )
    {
        return dispatcher::domain::OperatorIdentity(
            dispatcher::domain::OperatorId{ std::move(operator_id) },
            std::move(username),
            role,
            status
        );
    }
}

TEST(OperatorDirectoryStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorDirectoryStatus::Added
        ),
        "added"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorDirectoryStatus::Removed
        ),
        "removed"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorDirectoryStatus::NotFound
        ),
        "not_found"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorDirectoryStatus::DuplicateOperatorId
        ),
        "duplicate_operator_id"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorDirectoryStatus::DuplicateUsername
        ),
        "duplicate_username"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorDirectoryStatus::InvalidIdentity
        ),
        "invalid_identity"
    );
}

TEST(OperatorDirectoryStatusTests, SuccessPredicatesWork)
{
    EXPECT_TRUE(
        dispatcher::domain::is_success(
            dispatcher::domain::OperatorDirectoryStatus::Added
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_success(
            dispatcher::domain::OperatorDirectoryStatus::Removed
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_success(
            dispatcher::domain::OperatorDirectoryStatus::NotFound
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_failure(
            dispatcher::domain::OperatorDirectoryStatus::DuplicateUsername
        )
    );
}

TEST(OperatorDirectoryResultTests, SuccessResultWorks)
{
    const auto result =
        dispatcher::domain::OperatorDirectoryResult::success(
            dispatcher::domain::OperatorDirectoryStatus::Added,
            "operator identity added"
        );

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorDirectoryStatus::Added
    );

    EXPECT_TRUE(result.has_message());
    EXPECT_EQ(result.message(), "operator identity added");

    EXPECT_FALSE(result.has_field());
    EXPECT_FALSE(result.has_value());
}

TEST(OperatorDirectoryResultTests, FailureResultWorks)
{
    const auto result =
        dispatcher::domain::OperatorDirectoryResult::failure(
            dispatcher::domain::OperatorDirectoryStatus::DuplicateUsername,
            "username is already registered",
            "username",
            "operator.one"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorDirectoryStatus::DuplicateUsername
    );

    EXPECT_EQ(result.message(), "username is already registered");
    EXPECT_EQ(result.field(), "username");
    EXPECT_EQ(result.value(), "operator.one");

    EXPECT_TRUE(result.has_message());
    EXPECT_TRUE(result.has_field());
    EXPECT_TRUE(result.has_value());
}

TEST(OperatorDirectoryTests, DefaultDirectoryIsEmpty)
{
    const dispatcher::domain::OperatorDirectory directory;

    EXPECT_TRUE(directory.empty());
    EXPECT_EQ(directory.size(), 0);

    EXPECT_FALSE(
        directory.contains_id(
            dispatcher::domain::OperatorId{ "operator-1" }
        )
    );

    EXPECT_FALSE(directory.contains_username("operator.one"));
    EXPECT_TRUE(directory.identities().empty());
    EXPECT_TRUE(directory.usernames().empty());
}

TEST(OperatorDirectoryTests, AddStoresIdentityByIdAndUsername)
{
    dispatcher::domain::OperatorDirectory directory;

    const auto result =
        directory.add(
            make_directory_identity(
                "operator-1",
                "operator.one"
            )
        );

    EXPECT_TRUE(result.ok());

    EXPECT_FALSE(directory.empty());
    EXPECT_EQ(directory.size(), 1);

    EXPECT_TRUE(
        directory.contains_id(
            dispatcher::domain::OperatorId{ "operator-1" }
        )
    );

    EXPECT_TRUE(directory.contains_username("operator.one"));

    const auto by_id =
        directory.find_by_id(
            dispatcher::domain::OperatorId{ "operator-1" }
        );

    ASSERT_TRUE(by_id.has_value());
    EXPECT_EQ(by_id->username(), "operator.one");

    const auto by_username =
        directory.find_by_username("operator.one");

    ASSERT_TRUE(by_username.has_value());
    EXPECT_EQ(
        by_username->operator_id(),
        dispatcher::domain::OperatorId{ "operator-1" }
    );
}

TEST(OperatorDirectoryTests, AddRejectsEmptyOperatorId)
{
    dispatcher::domain::OperatorDirectory directory;

    const auto result =
        directory.add(
            make_directory_identity(
                "",
                "operator.one"
            )
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorDirectoryStatus::InvalidIdentity
    );

    EXPECT_EQ(result.field(), "operator_id");
    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(directory.empty());
}

TEST(OperatorDirectoryTests, AddRejectsEmptyUsername)
{
    dispatcher::domain::OperatorDirectory directory;

    const auto result =
        directory.add(
            make_directory_identity(
                "operator-1",
                ""
            )
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorDirectoryStatus::InvalidIdentity
    );

    EXPECT_EQ(result.field(), "username");
    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(directory.empty());
}

TEST(OperatorDirectoryTests, AddRejectsDuplicateOperatorId)
{
    dispatcher::domain::OperatorDirectory directory;

    ASSERT_TRUE(
        directory.add(
            make_directory_identity(
                "operator-1",
                "operator.one"
            )
        ).ok()
    );

    const auto result =
        directory.add(
            make_directory_identity(
                "operator-1",
                "operator.duplicate"
            )
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorDirectoryStatus::DuplicateOperatorId
    );

    EXPECT_EQ(result.field(), "operator_id");
    EXPECT_EQ(result.value(), "operator-1");
    EXPECT_EQ(directory.size(), 1);
}

TEST(OperatorDirectoryTests, AddRejectsDuplicateUsername)
{
    dispatcher::domain::OperatorDirectory directory;

    ASSERT_TRUE(
        directory.add(
            make_directory_identity(
                "operator-1",
                "operator.one"
            )
        ).ok()
    );

    const auto result =
        directory.add(
            make_directory_identity(
                "operator-2",
                "operator.one"
            )
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorDirectoryStatus::DuplicateUsername
    );

    EXPECT_EQ(result.field(), "username");
    EXPECT_EQ(result.value(), "operator.one");
    EXPECT_EQ(directory.size(), 1);
}

TEST(OperatorDirectoryTests, FindMissingReturnsNullopt)
{
    const dispatcher::domain::OperatorDirectory directory;

    EXPECT_FALSE(
        directory.find_by_id(
            dispatcher::domain::OperatorId{ "missing" }
        ).has_value()
    );

    EXPECT_FALSE(
        directory.find_by_username("missing").has_value()
    );
}

TEST(OperatorDirectoryTests, IdentitiesAreReturnedSortedByOperatorId)
{
    dispatcher::domain::OperatorDirectory directory;

    ASSERT_TRUE(
        directory.add(
            make_directory_identity(
                "operator-z",
                "zeta"
            )
        ).ok()
    );

    ASSERT_TRUE(
        directory.add(
            make_directory_identity(
                "operator-a",
                "alpha"
            )
        ).ok()
    );

    ASSERT_TRUE(
        directory.add(
            make_directory_identity(
                "operator-m",
                "middle"
            )
        ).ok()
    );

    const auto identities = directory.identities();

    ASSERT_EQ(identities.size(), 3);

    EXPECT_EQ(identities[0].operator_id().value(), "operator-a");
    EXPECT_EQ(identities[1].operator_id().value(), "operator-m");
    EXPECT_EQ(identities[2].operator_id().value(), "operator-z");
}

TEST(OperatorDirectoryTests, UsernamesAreReturnedSorted)
{
    dispatcher::domain::OperatorDirectory directory;

    ASSERT_TRUE(
        directory.add(
            make_directory_identity(
                "operator-z",
                "zeta"
            )
        ).ok()
    );

    ASSERT_TRUE(
        directory.add(
            make_directory_identity(
                "operator-a",
                "alpha"
            )
        ).ok()
    );

    ASSERT_TRUE(
        directory.add(
            make_directory_identity(
                "operator-m",
                "middle"
            )
        ).ok()
    );

    const auto usernames = directory.usernames();

    ASSERT_EQ(usernames.size(), 3);

    EXPECT_EQ(usernames[0], "alpha");
    EXPECT_EQ(usernames[1], "middle");
    EXPECT_EQ(usernames[2], "zeta");
}

TEST(OperatorDirectoryTests, RemoveDeletesIdentityAndUsernameIndex)
{
    dispatcher::domain::OperatorDirectory directory;

    ASSERT_TRUE(
        directory.add(
            make_directory_identity(
                "operator-1",
                "operator.one"
            )
        ).ok()
    );

    ASSERT_TRUE(
        directory.contains_id(
            dispatcher::domain::OperatorId{ "operator-1" }
        )
    );

    ASSERT_TRUE(directory.contains_username("operator.one"));

    const auto result =
        directory.remove(
            dispatcher::domain::OperatorId{ "operator-1" }
        );

    EXPECT_TRUE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorDirectoryStatus::Removed
    );

    EXPECT_FALSE(
        directory.contains_id(
            dispatcher::domain::OperatorId{ "operator-1" }
        )
    );

    EXPECT_FALSE(directory.contains_username("operator.one"));
    EXPECT_TRUE(directory.empty());
}

TEST(OperatorDirectoryTests, RemoveRejectsEmptyOperatorId)
{
    dispatcher::domain::OperatorDirectory directory;

    const auto result =
        directory.remove(
            dispatcher::domain::OperatorId{ "" }
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorDirectoryStatus::InvalidIdentity
    );

    EXPECT_EQ(result.field(), "operator_id");
}

TEST(OperatorDirectoryTests, RemoveRejectsMissingOperatorId)
{
    dispatcher::domain::OperatorDirectory directory;

    const auto result =
        directory.remove(
            dispatcher::domain::OperatorId{ "missing" }
        );

    EXPECT_FALSE(result.ok());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorDirectoryStatus::NotFound
    );

    EXPECT_EQ(result.field(), "operator_id");
    EXPECT_EQ(result.value(), "missing");
}

TEST(OperatorDirectoryTests, ClearRemovesEverything)
{
    dispatcher::domain::OperatorDirectory directory;

    ASSERT_TRUE(
        directory.add(
            make_directory_identity(
                "operator-1",
                "operator.one"
            )
        ).ok()
    );

    ASSERT_TRUE(
        directory.add(
            make_directory_identity(
                "operator-2",
                "operator.two"
            )
        ).ok()
    );

    ASSERT_EQ(directory.size(), 2);

    directory.clear();

    EXPECT_TRUE(directory.empty());
    EXPECT_EQ(directory.size(), 0);
    EXPECT_TRUE(directory.identities().empty());
    EXPECT_TRUE(directory.usernames().empty());

    EXPECT_FALSE(
        directory.contains_id(
            dispatcher::domain::OperatorId{ "operator-1" }
        )
    );

    EXPECT_FALSE(directory.contains_username("operator.one"));
}