#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/operator_authorization_result.hpp>
#include <dispatcher/domain/operator_authorization_status.hpp>
#include <dispatcher/domain/operator_authorizer.hpp>
#include <dispatcher/domain/operator_identity.hpp>
#include <dispatcher/domain/operator_permission.hpp>
#include <dispatcher/domain/operator_role.hpp>
#include <dispatcher/domain/operator_status.hpp>

#include <gtest/gtest.h>

namespace
{
    dispatcher::domain::OperatorIdentity make_authorization_identity(
        std::string operator_id,
        std::string username,
        dispatcher::domain::OperatorRole role,
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

TEST(OperatorPermissionTests, ToStringReturnsStableNames)
{
    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorPermission::ViewRuntime
        ),
        "view_runtime"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorPermission::AcknowledgeAlarms
        ),
        "acknowledge_alarms"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorPermission::ShelveAlarms
        ),
        "shelve_alarms"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorPermission::ManageConfiguration
        ),
        "manage_configuration"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorPermission::ImportConfiguration
        ),
        "import_configuration"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorPermission::ExportConfiguration
        ),
        "export_configuration"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorPermission::ManageOperators
        ),
        "manage_operators"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorPermission::ViewAuditLog
        ),
        "view_audit_log"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorPermission::ServiceAccess
        ),
        "service_access"
    );
}

TEST(OperatorPermissionTests, PermissionCategoriesWork)
{
    EXPECT_TRUE(
        dispatcher::domain::is_alarm_permission(
            dispatcher::domain::OperatorPermission::AcknowledgeAlarms
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_alarm_permission(
            dispatcher::domain::OperatorPermission::ShelveAlarms
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_configuration_permission(
            dispatcher::domain::OperatorPermission::ManageConfiguration
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_configuration_permission(
            dispatcher::domain::OperatorPermission::ImportConfiguration
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_configuration_permission(
            dispatcher::domain::OperatorPermission::ExportConfiguration
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_administrative_permission(
            dispatcher::domain::OperatorPermission::ManageOperators
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_administrative_permission(
            dispatcher::domain::OperatorPermission::ViewAuditLog
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_administrative_permission(
            dispatcher::domain::OperatorPermission::ServiceAccess
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_alarm_permission(
            dispatcher::domain::OperatorPermission::ViewRuntime
        )
    );
}

TEST(OperatorPermissionTests, ViewerCanOnlyViewRuntime)
{
    const auto role = dispatcher::domain::OperatorRole::Viewer;

    EXPECT_TRUE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::ViewRuntime
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::AcknowledgeAlarms
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::ExportConfiguration
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::ManageOperators
        )
    );
}

TEST(OperatorPermissionTests, OperatorCanOperateAlarms)
{
    const auto role = dispatcher::domain::OperatorRole::Operator;

    EXPECT_TRUE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::ViewRuntime
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::AcknowledgeAlarms
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::ShelveAlarms
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::ManageConfiguration
        )
    );
}

TEST(OperatorPermissionTests, EngineerCanImportAndManageConfiguration)
{
    const auto role = dispatcher::domain::OperatorRole::Engineer;

    EXPECT_TRUE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::ManageConfiguration
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::ImportConfiguration
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::ExportConfiguration
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::ManageOperators
        )
    );
}

TEST(OperatorPermissionTests, AdministratorCanManageOperators)
{
    const auto role = dispatcher::domain::OperatorRole::Administrator;

    EXPECT_TRUE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::ManageOperators
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::ServiceAccess
        )
    );
}

TEST(OperatorPermissionTests, ServiceRoleOnlyHasServiceAccess)
{
    const auto role = dispatcher::domain::OperatorRole::Service;

    EXPECT_TRUE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::ViewRuntime
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::ServiceAccess
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::AcknowledgeAlarms
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_permission_allowed_for_role(
            role,
            dispatcher::domain::OperatorPermission::ManageConfiguration
        )
    );
}

TEST(OperatorAuthorizationStatusTests, ToStringAndPredicatesWork)
{
    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorAuthorizationStatus::Allowed
        ),
        "allowed"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorAuthorizationStatus::
            DeniedInactiveOperator
        ),
        "denied_inactive_operator"
    );

    EXPECT_STREQ(
        dispatcher::domain::to_string(
            dispatcher::domain::OperatorAuthorizationStatus::
            DeniedInsufficientRole
        ),
        "denied_insufficient_role"
    );

    EXPECT_TRUE(
        dispatcher::domain::is_authorized(
            dispatcher::domain::OperatorAuthorizationStatus::Allowed
        )
    );

    EXPECT_FALSE(
        dispatcher::domain::is_authorized(
            dispatcher::domain::OperatorAuthorizationStatus::
            DeniedInsufficientRole
        )
    );

    EXPECT_TRUE(
        dispatcher::domain::is_denied(
            dispatcher::domain::OperatorAuthorizationStatus::
            DeniedInsufficientRole
        )
    );
}

TEST(OperatorAuthorizationResultTests, AllowedResultCapturesContext)
{
    const auto result =
        dispatcher::domain::OperatorAuthorizationResult::allowed(
            dispatcher::domain::OperatorId{ "operator-1" },
            "operator.one",
            dispatcher::domain::OperatorRole::Operator,
            dispatcher::domain::OperatorPermission::AcknowledgeAlarms
        );

    EXPECT_TRUE(result.authorized());
    EXPECT_FALSE(result.denied());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorAuthorizationStatus::Allowed
    );

    EXPECT_EQ(
        result.operator_id(),
        dispatcher::domain::OperatorId{ "operator-1" }
    );

    EXPECT_EQ(result.username(), "operator.one");
    EXPECT_EQ(result.role(), dispatcher::domain::OperatorRole::Operator);
    EXPECT_EQ(
        result.operator_status(),
        dispatcher::domain::OperatorStatus::Active
    );

    EXPECT_EQ(
        result.permission(),
        dispatcher::domain::OperatorPermission::AcknowledgeAlarms
    );

    EXPECT_FALSE(result.has_reason());
}

TEST(OperatorAuthorizationResultTests, DeniedResultCapturesReason)
{
    const auto result =
        dispatcher::domain::OperatorAuthorizationResult::denied(
            dispatcher::domain::OperatorId{ "viewer-1" },
            "viewer.one",
            dispatcher::domain::OperatorRole::Viewer,
            dispatcher::domain::OperatorStatus::Active,
            dispatcher::domain::OperatorPermission::AcknowledgeAlarms,
            dispatcher::domain::OperatorAuthorizationStatus::
            DeniedInsufficientRole,
            "operator role does not grant requested permission"
        );

    EXPECT_FALSE(result.authorized());
    EXPECT_TRUE(result.denied());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorAuthorizationStatus::
        DeniedInsufficientRole
    );

    EXPECT_EQ(result.username(), "viewer.one");
    EXPECT_TRUE(result.has_reason());
    EXPECT_EQ(
        result.reason(),
        "operator role does not grant requested permission"
    );
}

TEST(OperatorAuthorizationResultTests, DeniedRejectsAllowedStatus)
{
    const auto result =
        dispatcher::domain::OperatorAuthorizationResult::denied(
            dispatcher::domain::OperatorId{ "viewer-1" },
            "viewer.one",
            dispatcher::domain::OperatorRole::Viewer,
            dispatcher::domain::OperatorStatus::Active,
            dispatcher::domain::OperatorPermission::AcknowledgeAlarms,
            dispatcher::domain::OperatorAuthorizationStatus::Allowed,
            "bad caller status"
        );

    EXPECT_TRUE(result.denied());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorAuthorizationStatus::
        DeniedInsufficientRole
    );
}

TEST(OperatorAuthorizerTests, ActiveOperatorCanAcknowledgeAlarms)
{
    const auto identity = make_authorization_identity(
        "operator-1",
        "operator.one",
        dispatcher::domain::OperatorRole::Operator
    );

    const dispatcher::domain::OperatorAuthorizer authorizer;

    const auto result = authorizer.authorize(
        identity,
        dispatcher::domain::OperatorPermission::AcknowledgeAlarms
    );

    EXPECT_TRUE(result.authorized());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorAuthorizationStatus::Allowed
    );

    EXPECT_EQ(result.operator_id(), identity.operator_id());
    EXPECT_EQ(result.username(), identity.username());

    EXPECT_TRUE(
        authorizer.is_authorized(
            identity,
            dispatcher::domain::OperatorPermission::AcknowledgeAlarms
        )
    );
}

TEST(OperatorAuthorizerTests, ViewerCannotAcknowledgeAlarms)
{
    const auto identity = make_authorization_identity(
        "viewer-1",
        "viewer.one",
        dispatcher::domain::OperatorRole::Viewer
    );

    const dispatcher::domain::OperatorAuthorizer authorizer;

    const auto result = authorizer.authorize(
        identity,
        dispatcher::domain::OperatorPermission::AcknowledgeAlarms
    );

    EXPECT_TRUE(result.denied());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorAuthorizationStatus::
        DeniedInsufficientRole
    );

    EXPECT_EQ(
        result.reason(),
        "operator role does not grant requested permission"
    );

    EXPECT_FALSE(
        authorizer.is_authorized(
            identity,
            dispatcher::domain::OperatorPermission::AcknowledgeAlarms
        )
    );
}

TEST(OperatorAuthorizerTests, DisabledAdministratorIsDenied)
{
    const auto identity = make_authorization_identity(
        "admin-1",
        "admin.one",
        dispatcher::domain::OperatorRole::Administrator,
        dispatcher::domain::OperatorStatus::Disabled
    );

    const dispatcher::domain::OperatorAuthorizer authorizer;

    const auto result = authorizer.authorize(
        identity,
        dispatcher::domain::OperatorPermission::ManageOperators
    );

    EXPECT_TRUE(result.denied());

    EXPECT_EQ(
        result.status(),
        dispatcher::domain::OperatorAuthorizationStatus::
        DeniedInactiveOperator
    );

    EXPECT_EQ(result.reason(), "operator is not active");

    EXPECT_FALSE(
        authorizer.is_authorized(
            identity,
            dispatcher::domain::OperatorPermission::ManageOperators
        )
    );
}

TEST(OperatorAuthorizerTests, EngineerCanImportConfiguration)
{
    const auto identity = make_authorization_identity(
        "engineer-1",
        "engineer.one",
        dispatcher::domain::OperatorRole::Engineer
    );

    const dispatcher::domain::OperatorAuthorizer authorizer;

    const auto result = authorizer.authorize(
        identity,
        dispatcher::domain::OperatorPermission::ImportConfiguration
    );

    EXPECT_TRUE(result.authorized());
}

TEST(OperatorAuthorizerTests, ServiceCanUseServiceAccessButNotShelveAlarms)
{
    const auto identity = make_authorization_identity(
        "service-1",
        "service.account",
        dispatcher::domain::OperatorRole::Service
    );

    const dispatcher::domain::OperatorAuthorizer authorizer;

    EXPECT_TRUE(
        authorizer.is_authorized(
            identity,
            dispatcher::domain::OperatorPermission::ServiceAccess
        )
    );

    EXPECT_FALSE(
        authorizer.is_authorized(
            identity,
            dispatcher::domain::OperatorPermission::ShelveAlarms
        )
    );
}