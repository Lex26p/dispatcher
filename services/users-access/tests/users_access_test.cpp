#include "dispatcher/users_access/users_access_manager.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ua = dispatcher::users_access;

namespace {

[[noreturn]] void fail(const std::string_view message) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
}

void expect(const bool condition, const std::string_view message) {
    if (!condition) {
        fail(message);
    }
}

[[nodiscard]] bool same_scope(
    const ua::AccessScope& left,
    const ua::AccessScope& right) {
    return left.kind == right.kind && left.project_id == right.project_id;
}

class InMemoryUsersAccessRepository final : public ua::UsersAccessRepository {
public:
    ua::UsersAccessRepositoryStatus insert_user(const ua::User& user) override {
        for (const auto& existing : users_) {
            if (existing.id == user.id || existing.login == user.login) {
                return ua::UsersAccessRepositoryStatus::conflict;
            }
        }
        users_.push_back(user);
        return ua::UsersAccessRepositoryStatus::ok;
    }

    ua::UsersAccessRepositoryStatus update_user(const ua::User& user) override {
        for (auto& existing : users_) {
            if (existing.id == user.id) {
                for (const auto& candidate : users_) {
                    if (candidate.id != user.id && candidate.login == user.login) {
                        return ua::UsersAccessRepositoryStatus::conflict;
                    }
                }
                existing = user;
                return ua::UsersAccessRepositoryStatus::ok;
            }
        }
        return ua::UsersAccessRepositoryStatus::not_found;
    }

    ua::UsersAccessRepositoryStatus find_user_by_id(
        const std::string_view user_id,
        ua::User& user) const override {
        for (const auto& existing : users_) {
            if (existing.id == user_id) {
                user = existing;
                return ua::UsersAccessRepositoryStatus::ok;
            }
        }
        return ua::UsersAccessRepositoryStatus::not_found;
    }

    ua::UsersAccessRepositoryStatus find_user_by_login(
        const std::string_view login,
        ua::User& user) const override {
        for (const auto& existing : users_) {
            if (existing.login == login) {
                user = existing;
                return ua::UsersAccessRepositoryStatus::ok;
            }
        }
        return ua::UsersAccessRepositoryStatus::not_found;
    }

    ua::UsersAccessRepositoryStatus insert_permission_set(
        const ua::PermissionSet& permission_set) override {
        for (const auto& existing : permission_sets_) {
            if (existing.id == permission_set.id) {
                return ua::UsersAccessRepositoryStatus::conflict;
            }
        }
        permission_sets_.push_back(permission_set);
        return ua::UsersAccessRepositoryStatus::ok;
    }

    ua::UsersAccessRepositoryStatus find_permission_set_by_id(
        const std::string_view permission_set_id,
        ua::PermissionSet& permission_set) const override {
        for (const auto& existing : permission_sets_) {
            if (existing.id == permission_set_id) {
                permission_set = existing;
                return ua::UsersAccessRepositoryStatus::ok;
            }
        }
        return ua::UsersAccessRepositoryStatus::not_found;
    }

    ua::UsersAccessRepositoryStatus insert_assignment(
        const ua::AccessAssignment& assignment) override {
        for (const auto& existing : assignments_) {
            if (existing.user_id == assignment.user_id &&
                existing.permission_set_id == assignment.permission_set_id &&
                same_scope(existing.scope, assignment.scope)) {
                return ua::UsersAccessRepositoryStatus::conflict;
            }
        }
        assignments_.push_back(assignment);
        return ua::UsersAccessRepositoryStatus::ok;
    }

    ua::UsersAccessRepositoryStatus list_assignments_for_user(
        const std::string_view user_id,
        std::vector<ua::AccessAssignment>& assignments) const override {
        assignments.clear();
        for (const auto& assignment : assignments_) {
            if (assignment.user_id == user_id) {
                assignments.push_back(assignment);
            }
        }
        return ua::UsersAccessRepositoryStatus::ok;
    }

private:
    std::vector<ua::User> users_;
    std::vector<ua::PermissionSet> permission_sets_;
    std::vector<ua::AccessAssignment> assignments_;
};

class SequentialIdGenerator final {
public:
    std::string operator()() {
        return "test-id-" + std::to_string(next_++);
    }

private:
    int next_{1};
};

void test_identity_and_validation() {
    InMemoryUsersAccessRepository repository;
    SequentialIdGenerator ids;
    ua::UsersAccessManager manager{repository, [&ids] { return ids(); }};

    const auto created = manager.create_user({
        .login = "sansa",
        .display_name = "Sansa",
        .enabled = true,
    });
    expect(created.ok(), "user creation should succeed");
    expect(created.value->id == "test-id-1", "user ID should come from injected generator");
    expect(created.value->login == "sansa", "login should be preserved");
    expect(created.value->display_name == "Sansa", "display name should be preserved");
    expect(created.value->enabled, "enabled state should be preserved");

    const auto duplicate = manager.create_user({
        .login = "sansa",
        .display_name = "Other",
        .enabled = true,
    });
    expect(!duplicate.ok(), "duplicate login should fail");
    expect(
        duplicate.error == ua::UsersAccessManagerError::login_conflict,
        "duplicate login should report login_conflict");

    const auto invalid = manager.create_user({
        .login = "   ",
        .display_name = "Invalid",
        .enabled = true,
    });
    expect(!invalid.ok(), "whitespace-only login should fail");
    expect(
        invalid.error == ua::UsersAccessManagerError::invalid_login,
        "invalid login should have a specific error");
}

void test_permission_sets_are_canonical_and_independent() {
    InMemoryUsersAccessRepository repository;
    SequentialIdGenerator ids;
    ua::UsersAccessManager manager{repository, [&ids] { return ids(); }};

    const auto permission_set = manager.create_permission_set({
        .name = "Project operator",
        .capabilities = {
            ua::Capability::control,
            ua::Capability::view,
            ua::Capability::control,
            ua::Capability::admin,
        },
    });

    expect(permission_set.ok(), "permission set creation should succeed");
    expect(permission_set.value->id == "test-id-1", "permission set should have opaque ID");
    expect(permission_set.value->capabilities.size() == 3, "capabilities should be deduplicated");
    expect(permission_set.value->capabilities[0] == ua::Capability::view, "canonical order starts with view");
    expect(permission_set.value->capabilities[1] == ua::Capability::control, "canonical order keeps control");
    expect(permission_set.value->capabilities[2] == ua::Capability::admin, "canonical order keeps admin");
}

void test_global_and_project_union_semantics() {
    InMemoryUsersAccessRepository repository;
    SequentialIdGenerator ids;
    ua::UsersAccessManager manager{repository, [&ids] { return ids(); }};

    const auto user = manager.create_user({"operator", "Operator", true});
    const auto global_view = manager.create_permission_set({
        "Global viewer",
        {ua::Capability::view},
    });
    const auto project_edit = manager.create_permission_set({
        "Project editor",
        {ua::Capability::edit},
    });
    expect(user.ok() && global_view.ok() && project_edit.ok(), "test fixtures should be created");

    expect(
        manager.assign({user.value->id, global_view.value->id, ua::AccessScope::global()}).ok(),
        "global assignment should succeed");
    expect(
        manager.assign({user.value->id, project_edit.value->id, ua::AccessScope::project("project-a")}).ok(),
        "project assignment should succeed");

    const auto global_view_result = manager.evaluate(
        user.value->id,
        ua::AccessScope::global(),
        ua::Capability::view);
    expect(global_view_result.ok() && global_view_result.allowed, "global view should be allowed");
    expect(!global_view_result.has(ua::Capability::edit), "project edit must not leak into global scope");

    const auto project_a_edit = manager.evaluate(
        user.value->id,
        ua::AccessScope::project("project-a"),
        ua::Capability::edit);
    expect(project_a_edit.ok() && project_a_edit.allowed, "project-a edit should be allowed");
    expect(project_a_edit.has(ua::Capability::view), "global view should participate in project evaluation");
    expect(project_a_edit.has(ua::Capability::edit), "matching project capability should be effective");

    const auto project_b_edit = manager.evaluate(
        user.value->id,
        ua::AccessScope::project("project-b"),
        ua::Capability::edit);
    expect(project_b_edit.ok() && !project_b_edit.allowed, "project-a edit must not apply to project-b");
    expect(project_b_edit.has(ua::Capability::view), "global view should still apply to project-b");
}

void test_capabilities_do_not_imply_each_other() {
    InMemoryUsersAccessRepository repository;
    SequentialIdGenerator ids;
    ua::UsersAccessManager manager{repository, [&ids] { return ids(); }};

    const auto user = manager.create_user({"admin-only", "Admin only", true});
    const auto admin = manager.create_permission_set({"Admin only", {ua::Capability::admin}});
    expect(user.ok() && admin.ok(), "admin fixture should be created");
    expect(
        manager.assign({user.value->id, admin.value->id, ua::AccessScope::global()}).ok(),
        "admin assignment should succeed");

    const auto admin_result = manager.evaluate(
        user.value->id,
        ua::AccessScope::global(),
        ua::Capability::admin);
    const auto edit_result = manager.evaluate(
        user.value->id,
        ua::AccessScope::global(),
        ua::Capability::edit);

    expect(admin_result.allowed, "explicit admin capability should be allowed");
    expect(!edit_result.allowed, "admin must not implicitly grant edit in Step 1 model");
}

void test_disabled_and_invalid_subjects_fail_closed() {
    InMemoryUsersAccessRepository repository;
    SequentialIdGenerator ids;
    ua::UsersAccessManager manager{repository, [&ids] { return ids(); }};

    const auto user = manager.create_user({"disabled", "Disabled", false});
    const auto viewer = manager.create_permission_set({"Viewer", {ua::Capability::view}});
    expect(user.ok() && viewer.ok(), "disabled fixture should be created");
    expect(
        manager.assign({user.value->id, viewer.value->id, ua::AccessScope::global()}).ok(),
        "disabled user can have durable configuration assignments");

    const auto disabled = manager.evaluate(
        user.value->id,
        ua::AccessScope::project("project-a"),
        ua::Capability::view);
    expect(disabled.ok(), "disabled user is a valid subject, not a lookup error");
    expect(!disabled.allowed, "disabled user should be denied");
    expect(disabled.effective_capabilities.empty(), "disabled user should expose no effective capabilities");

    const auto enabled = manager.set_user_enabled(user.value->id, true);
    expect(enabled.ok() && enabled.value->enabled, "user should be enableable");
    const auto enabled_result = manager.evaluate(
        user.value->id,
        ua::AccessScope::project("project-a"),
        ua::Capability::view);
    expect(enabled_result.ok() && enabled_result.allowed, "enabled user should regain configured access");

    const auto disabled_again = manager.set_user_enabled(user.value->id, false);
    expect(disabled_again.ok() && !disabled_again.value->enabled, "user should be disableable");

    const auto missing = manager.evaluate(
        "missing-user",
        ua::AccessScope::global(),
        ua::Capability::view);
    expect(!missing.ok(), "missing user should produce evaluation error");
    expect(
        missing.error == ua::AccessEvaluationError::user_not_found,
        "missing user should report user_not_found");

    const auto invalid_scope = manager.evaluate(
        user.value->id,
        ua::AccessScope::project("   "),
        ua::Capability::view);
    expect(!invalid_scope.ok(), "invalid scope should produce evaluation error");
    expect(
        invalid_scope.error == ua::AccessEvaluationError::invalid_scope,
        "invalid scope should report invalid_scope");
}

void test_assignment_validation_and_conflict() {
    InMemoryUsersAccessRepository repository;
    SequentialIdGenerator ids;
    ua::UsersAccessManager manager{repository, [&ids] { return ids(); }};

    const auto user = manager.create_user({"engineer", "Engineer", true});
    const auto editor = manager.create_permission_set({"Editor", {ua::Capability::edit}});
    expect(user.ok() && editor.ok(), "assignment fixtures should be created");

    const ua::CreateAccessAssignmentInput assignment{
        user.value->id,
        editor.value->id,
        ua::AccessScope::project("project-a"),
    };
    expect(manager.assign(assignment).ok(), "first assignment should succeed");

    const auto duplicate = manager.assign(assignment);
    expect(!duplicate.ok(), "duplicate assignment should fail");
    expect(
        duplicate.error == ua::UsersAccessManagerError::assignment_conflict,
        "duplicate assignment should report assignment_conflict");

    const auto missing_user = manager.assign({
        "missing-user",
        editor.value->id,
        ua::AccessScope::global(),
    });
    expect(
        missing_user.error == ua::UsersAccessManagerError::user_not_found,
        "assignment should validate user existence");

    const auto missing_set = manager.assign({
        user.value->id,
        "missing-set",
        ua::AccessScope::global(),
    });
    expect(
        missing_set.error == ua::UsersAccessManagerError::permission_set_not_found,
        "assignment should validate permission-set existence");
}

}  // namespace

int main() {
    test_identity_and_validation();
    test_permission_sets_are_canonical_and_independent();
    test_global_and_project_union_semantics();
    test_capabilities_do_not_imply_each_other();
    test_disabled_and_invalid_subjects_fail_closed();
    test_assignment_validation_and_conflict();

    std::cout << "Users & Access domain/application tests passed\n";
    return 0;
}
