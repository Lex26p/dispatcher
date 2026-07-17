#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/operator_identity.hpp>
#include <dispatcher/domain/operator_role.hpp>
#include <dispatcher/domain/operator_status.hpp>

#include <gtest/gtest.h>

TEST(OperatorRoleTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorRole::Viewer
        ),
        "viewer"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorRole::Operator
        ),
        "operator"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorRole::Supervisor
        ),
        "supervisor"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorRole::Engineer
        ),
        "engineer"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorRole::Administrator
        ),
        "administrator"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorRole::Service
        ),
        "service"
    );
}

TEST(OperatorRoleTests, ViewerCanOnlyViewRuntime)
{
    const auto role = dispatcher::domain::OperatorRole::Viewer;

    EXPECT_TRUE(dispatcher::domain::can_view_runtime(role));
    EXPECT_FALSE(dispatcher::domain::can_acknowledge_alarms(role));
    EXPECT_FALSE(dispatcher::domain::can_shelve_alarms(role));
    EXPECT_FALSE(dispatcher::domain::can_manage_configuration(role));
    EXPECT_FALSE(dispatcher::domain::can_administer_users(role));
    EXPECT_FALSE(dispatcher::domain::is_privileged_role(role));
}

TEST(OperatorRoleTests, OperatorCanAcknowledgeAndShelveAlarms)
{
    const auto role = dispatcher::domain::OperatorRole::Operator;

    EXPECT_TRUE(dispatcher::domain::can_view_runtime(role));
    EXPECT_TRUE(dispatcher::domain::can_acknowledge_alarms(role));
    EXPECT_TRUE(dispatcher::domain::can_shelve_alarms(role));

    EXPECT_FALSE(dispatcher::domain::can_manage_configuration(role));
    EXPECT_FALSE(dispatcher::domain::can_administer_users(role));
    EXPECT_FALSE(dispatcher::domain::is_privileged_role(role));
}

TEST(OperatorRoleTests, SupervisorIsPrivilegedOperator)
{
    const auto role = dispatcher::domain::OperatorRole::Supervisor;

    EXPECT_TRUE(dispatcher::domain::can_view_runtime(role));
    EXPECT_TRUE(dispatcher::domain::can_acknowledge_alarms(role));
    EXPECT_TRUE(dispatcher::domain::can_shelve_alarms(role));

    EXPECT_FALSE(dispatcher::domain::can_manage_configuration(role));
    EXPECT_FALSE(dispatcher::domain::can_administer_users(role));

    EXPECT_TRUE(dispatcher::domain::is_privileged_role(role));
}

TEST(OperatorRoleTests, EngineerCanManageConfiguration)
{
    const auto role = dispatcher::domain::OperatorRole::Engineer;

    EXPECT_TRUE(dispatcher::domain::can_view_runtime(role));
    EXPECT_TRUE(dispatcher::domain::can_acknowledge_alarms(role));
    EXPECT_TRUE(dispatcher::domain::can_shelve_alarms(role));
    EXPECT_TRUE(dispatcher::domain::can_manage_configuration(role));

    EXPECT_FALSE(dispatcher::domain::can_administer_users(role));

    EXPECT_TRUE(dispatcher::domain::is_privileged_role(role));
}

TEST(OperatorRoleTests, AdministratorCanAdministerUsers)
{
    const auto role = dispatcher::domain::OperatorRole::Administrator;

    EXPECT_TRUE(dispatcher::domain::can_view_runtime(role));
    EXPECT_TRUE(dispatcher::domain::can_acknowledge_alarms(role));
    EXPECT_TRUE(dispatcher::domain::can_shelve_alarms(role));
    EXPECT_TRUE(dispatcher::domain::can_manage_configuration(role));
    EXPECT_TRUE(dispatcher::domain::can_administer_users(role));

    EXPECT_TRUE(dispatcher::domain::is_privileged_role(role));
}

TEST(OperatorRoleTests, ServiceCannotOperateByDefault)
{
    const auto role = dispatcher::domain::OperatorRole::Service;

    EXPECT_TRUE(dispatcher::domain::can_view_runtime(role));

    EXPECT_FALSE(dispatcher::domain::can_acknowledge_alarms(role));
    EXPECT_FALSE(dispatcher::domain::can_shelve_alarms(role));
    EXPECT_FALSE(dispatcher::domain::can_manage_configuration(role));
    EXPECT_FALSE(dispatcher::domain::can_administer_users(role));
    EXPECT_FALSE(dispatcher::domain::is_privileged_role(role));
}

TEST(OperatorStatusTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorStatus::Active
        ),
        "active"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorStatus::Disabled
        ),
        "disabled"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorStatus::Locked
        ),
        "locked"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorStatus::Expired
        ),
        "expired"
    );
}

TEST(OperatorStatusTests, OnlyActiveCanSignIn)
{
    EXPECT_TRUE(
        dispatcher::domain::can_sign_in(
            dispatcher::domain::OperatorStatus::Active
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::can_sign_in(
            dispatcher::domain::OperatorStatus::Disabled
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::can_sign_in(
            dispatcher::domain::OperatorStatus::Locked
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::can_sign_in(
            dispatcher::domain::OperatorStatus::Expired
        )
    );
}

TEST(OperatorIdentityTests, CapturesIdentityFields)
{
    const dispatcher::domain::OperatorIdentity identity(
        dispatcher::domain::OperatorId{ "operator-1" },
        "operator.one",
        dispatcher::domain::OperatorRole::Operator,
        dispatcher::domain::OperatorStatus::Active,
        "Operator One",
        "operator.one@example.local"
    );

    EXPECT_EQ(
        identity.operator_id(),
        dispatcher::domain::OperatorId{ "operator-1" }
    );

    EXPECT_EQ(identity.operator_id().value(), "operator-1");
    EXPECT_EQ(identity.username(), "operator.one");
    EXPECT_EQ(identity.role(), dispatcher::domain::OperatorRole::Operator);
    EXPECT_EQ(identity.status(), dispatcher::domain::OperatorStatus::Active);
    EXPECT_EQ(identity.display_name(), "Operator One");
    EXPECT_EQ(identity.email(), "operator.one@example.local");

    EXPECT_TRUE(identity.has_display_name());
    EXPECT_TRUE(identity.has_email());
}

TEST(OperatorIdentityTests, ActiveOperatorCanOperateAlarms)
{
    const dispatcher::domain::OperatorIdentity identity(
        dispatcher::domain::OperatorId{ "operator-1" },
        "operator.one",
        dispatcher::domain::OperatorRole::Operator
    );

    EXPECT_TRUE(identity.active());
    EXPECT_TRUE(identity.enabled());
    EXPECT_TRUE(identity.can_sign_in());
    EXPECT_TRUE(identity.can_view_runtime());
    EXPECT_TRUE(identity.can_acknowledge_alarms());
    EXPECT_TRUE(identity.can_shelve_alarms());

    EXPECT_FALSE(identity.can_manage_configuration());
    EXPECT_FALSE(identity.can_administer_users());
    EXPECT_FALSE(identity.privileged());
}

TEST(OperatorIdentityTests, DisabledAdministratorCannotUsePermissions)
{
    const dispatcher::domain::OperatorIdentity identity(
        dispatcher::domain::OperatorId{ "admin-1" },
        "admin.one",
        dispatcher::domain::OperatorRole::Administrator,
        dispatcher::domain::OperatorStatus::Disabled
    );

    EXPECT_FALSE(identity.active());
    EXPECT_FALSE(identity.enabled());
    EXPECT_FALSE(identity.can_sign_in());

    EXPECT_FALSE(identity.can_view_runtime());
    EXPECT_FALSE(identity.can_acknowledge_alarms());
    EXPECT_FALSE(identity.can_shelve_alarms());
    EXPECT_FALSE(identity.can_manage_configuration());
    EXPECT_FALSE(identity.can_administer_users());
    EXPECT_FALSE(identity.privileged());
}

TEST(OperatorIdentityTests, ActiveEngineerIsPrivilegedAndCanManageConfiguration)
{
    const dispatcher::domain::OperatorIdentity identity(
        dispatcher::domain::OperatorId{ "engineer-1" },
        "engineer.one",
        dispatcher::domain::OperatorRole::Engineer,
        dispatcher::domain::OperatorStatus::Active
    );

    EXPECT_TRUE(identity.can_view_runtime());
    EXPECT_TRUE(identity.can_acknowledge_alarms());
    EXPECT_TRUE(identity.can_shelve_alarms());
    EXPECT_TRUE(identity.can_manage_configuration());

    EXPECT_FALSE(identity.can_administer_users());

    EXPECT_TRUE(identity.privileged());
}

TEST(OperatorIdentityTests, EmptyOptionalFieldsAreDetected)
{
    const dispatcher::domain::OperatorIdentity identity(
        dispatcher::domain::OperatorId{ "viewer-1" },
        "viewer.one",
        dispatcher::domain::OperatorRole::Viewer
    );

    EXPECT_FALSE(identity.has_display_name());
    EXPECT_FALSE(identity.has_email());
}