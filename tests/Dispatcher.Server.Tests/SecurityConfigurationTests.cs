using Dispatcher.Contracts.Authorization;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Security;
using Microsoft.Data.Sqlite;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging.Abstractions;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class SecurityConfigurationTests
{
    [TestMethod]
    public async Task InitializeAsync_SeedsBuiltInRoles_AndAssignsBootstrapAdministrator()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        const string password =
            "A-long-bootstrap-password-42";

        var configuration =
            CreateBootstrapConfiguration(
                password);
        var store =
            new SqliteConfigurationStore(
                database.DatabasePath);
        var bootstrapper =
            new LocalUserBootstrapper(
                store,
                configuration,
                NullLogger<LocalUserBootstrapper>.Instance);

        var bootstrapUserCreated =
            await bootstrapper.EnsureBootstrapAdministratorAsync();

        Assert.IsTrue(
            bootstrapUserCreated);
        Assert.AreEqual(
            1,
            (await store.LoadSecurityRolesAsync()).Count);
        Assert.AreEqual(
            1,
            (await store.LoadUserRoleAssignmentsAsync()).Count);

        var initializer =
            new SecurityConfigurationInitializer(
                store,
                NullLogger<SecurityConfigurationInitializer>.Instance);

        await initializer.InitializeAsync();

        var users =
            await store.LoadLocalUsersAsync();
        var roles =
            await store.LoadSecurityRolesAsync();
        var assignments =
            await store.LoadUserRoleAssignmentsAsync();

        Assert.AreEqual(
            1,
            users.Count);
        Assert.AreEqual(
            4,
            roles.Count);
        Assert.IsTrue(
            roles.All(role => role.BuiltIn));
        Assert.AreEqual(
            1,
            assignments.Count);
        Assert.AreEqual(
            users[0].UserId,
            assignments[0].UserId);
        Assert.AreEqual(
            BuiltInSecurityRoles.AdministratorRoleId,
            assignments[0].RoleId);

        var administrator =
            roles.Single(
                role =>
                    role.RoleId
                    == BuiltInSecurityRoles.AdministratorRoleId);

        Assert.IsTrue(
            PermissionNames.All
                .OrderBy(
                    permission => permission,
                    StringComparer.Ordinal)
                .SequenceEqual(
                    administrator.Permissions.OrderBy(
                        permission => permission,
                        StringComparer.Ordinal)));

        var catalog =
            new SecurityCatalog();

        catalog.ReplaceAll(
            users,
            roles,
            assignments);

        foreach (var permission in PermissionNames.All)
        {
            Assert.IsTrue(
                catalog.HasPermission(
                    users[0].UserId,
                    permission),
                permission);
        }
    }

    [TestMethod]
    public async Task InitializeAsync_AssignsAdministrator_WhenBootstrapOccursAfterRolesWereSeeded()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        var store =
            new SqliteConfigurationStore(
                database.DatabasePath);
        var firstInitializer =
            new SecurityConfigurationInitializer(
                store,
                NullLogger<SecurityConfigurationInitializer>.Instance);

        await firstInitializer.InitializeAsync();

        Assert.AreEqual(
            4,
            (await store.LoadSecurityRolesAsync()).Count);
        Assert.AreEqual(
            0,
            (await store.LoadUserRoleAssignmentsAsync()).Count);

        var bootstrapConfiguration =
            CreateBootstrapConfiguration(
                "A-delayed-bootstrap-password-42");
        var bootstrapper =
            new LocalUserBootstrapper(
                store,
                bootstrapConfiguration,
                NullLogger<LocalUserBootstrapper>.Instance);

        var bootstrapUserCreated =
            await bootstrapper.EnsureBootstrapAdministratorAsync();

        Assert.IsTrue(
            bootstrapUserCreated);

        var secondInitializer =
            new SecurityConfigurationInitializer(
                store,
                NullLogger<SecurityConfigurationInitializer>.Instance);

        await secondInitializer.InitializeAsync();

        var assignments =
            await store.LoadUserRoleAssignmentsAsync();

        Assert.AreEqual(
            1,
            assignments.Count);
        Assert.AreEqual(
            BuiltInSecurityRoles.AdministratorRoleId,
            assignments[0].RoleId);
    }

    [TestMethod]
    public async Task InitializeAsync_AssignsAdministrator_ToSingleLegacyUser_OnlyOnFirstRoleInitialization()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        var store =
            new SqliteConfigurationStore(
                database.DatabasePath);
        var user =
            CreateUser(
                "legacy-admin",
                enabled:
                    true);

        await store.InsertLocalUserAsync(
            user);

        var initializer =
            new SecurityConfigurationInitializer(
                store,
                NullLogger<SecurityConfigurationInitializer>.Instance);

        await initializer.InitializeAsync();

        var assignments =
            await store.LoadUserRoleAssignmentsAsync();

        Assert.AreEqual(
            1,
            assignments.Count);
        Assert.AreEqual(
            user.UserId,
            assignments[0].UserId);
        Assert.AreEqual(
            BuiltInSecurityRoles.AdministratorRoleId,
            assignments[0].RoleId);

        await initializer.InitializeAsync();

        Assert.AreEqual(
            1,
            (await store.LoadUserRoleAssignmentsAsync()).Count);
    }

    [TestMethod]
    public void SecurityCatalog_ComputesEffectivePermissions_AndDeniesDisabledUser()
    {
        var enabledUser =
            CreateUser(
                "operator-one",
                enabled:
                    true);
        var disabledUser =
            CreateUser(
                "operator-disabled",
                enabled:
                    false);
        var operatorRole =
            BuiltInSecurityRoles.All.Single(
                role =>
                    role.RoleId
                    == BuiltInSecurityRoles.OperatorRoleId);
        var catalog =
            new SecurityCatalog();

        catalog.ReplaceAll(
            [
                enabledUser,
                disabledUser
            ],
            [
                operatorRole
            ],
            [
                new UserRoleAssignment(
                    enabledUser.UserId,
                    operatorRole.RoleId),
                new UserRoleAssignment(
                    disabledUser.UserId,
                    operatorRole.RoleId)
            ]);

        Assert.IsTrue(
            catalog.IsUserEnabled(
                enabledUser.UserId));
        Assert.IsTrue(
            catalog.HasPermission(
                enabledUser.UserId,
                PermissionNames.RuntimeRead));
        Assert.IsTrue(
            catalog.HasPermission(
                enabledUser.UserId,
                PermissionNames.TagsWrite));
        Assert.IsFalse(
            catalog.HasPermission(
                enabledUser.UserId,
                PermissionNames.DevicesEdit));

        Assert.IsFalse(
            catalog.IsUserEnabled(
                disabledUser.UserId));
        Assert.IsFalse(
            catalog.HasPermission(
                disabledUser.UserId,
                PermissionNames.RuntimeRead));
        Assert.AreEqual(
            0,
            catalog.GetEffectivePermissions(
                disabledUser.UserId).Count);
    }

    [TestMethod]
    public async Task InitializeAsync_MigratesVersion5Database_ToVersion7WithoutLosingLocalUsers()
    {
        var directory =
            Path.Combine(
                Path.GetTempPath(),
                "dispatcher-tests",
                Guid.NewGuid().ToString("N"));

        Directory.CreateDirectory(
            directory);

        var databasePath =
            Path.Combine(
                directory,
                "dispatcher-v5.db");
        var connectionString =
            new SqliteConnectionStringBuilder
            {
                DataSource = databasePath,
                Pooling = false
            }
            .ToString();

        try
        {
            await using (var connection =
                new SqliteConnection(
                    connectionString))
            {
                await connection.OpenAsync();

                await using var command =
                    connection.CreateCommand();

                command.CommandText =
                    """
                    CREATE TABLE local_users (
                        user_id TEXT NOT NULL PRIMARY KEY,
                        user_name TEXT NOT NULL,
                        normalized_user_name TEXT NOT NULL UNIQUE,
                        display_name TEXT NOT NULL,
                        enabled INTEGER NOT NULL CHECK (enabled IN (0, 1)),
                        password_hash TEXT NOT NULL
                    );

                    INSERT INTO local_users (
                        user_id,
                        user_name,
                        normalized_user_name,
                        display_name,
                        enabled,
                        password_hash)
                    VALUES (
                        'legacy-user-id',
                        'admin',
                        'ADMIN',
                        'Administrator',
                        1,
                        'legacy-password-hash');

                    PRAGMA user_version = 5;
                    """;

                await command.ExecuteNonQueryAsync();
            }

            var store =
                new SqliteConfigurationStore(
                    databasePath);

            await store.InitializeAsync();

            var users =
                await store.LoadLocalUsersAsync();
            var roles =
                await store.LoadSecurityRolesAsync();
            var assignments =
                await store.LoadUserRoleAssignmentsAsync();

            Assert.AreEqual(
                1,
                users.Count);
            Assert.AreEqual(
                "legacy-user-id",
                users[0].UserId);
            Assert.AreEqual(
                0,
                roles.Count);
            Assert.AreEqual(
                0,
                assignments.Count);

            await using var verify =
                new SqliteConnection(
                    connectionString);

            await verify.OpenAsync();

            await using var versionCommand =
                verify.CreateCommand();

            versionCommand.CommandText =
                "PRAGMA user_version;";

            var version =
                Convert.ToInt32(
                    await versionCommand.ExecuteScalarAsync());

            Assert.AreEqual(
                7,
                version);
        }
        finally
        {
            if (Directory.Exists(
                    directory))
            {
                Directory.Delete(
                    directory,
                    recursive:
                        true);
            }
        }
    }

    private static LocalUserConfiguration CreateUser(
        string userName,
        bool enabled)
    {
        return new LocalUserConfiguration(
            UserId:
                Guid.NewGuid().ToString("N"),
            UserName:
                userName,
            NormalizedUserName:
                LocalUserConfiguration.NormalizeUserName(
                    userName),
            DisplayName:
                userName,
            Enabled:
                enabled,
            PasswordHash:
                "test-password-hash");
    }

    private static IConfiguration CreateBootstrapConfiguration(
        string password)
    {
        return new ConfigurationBuilder()
            .AddInMemoryCollection(
                new Dictionary<string, string?>
                {
                    ["Authentication:BootstrapAdministrator:UserName"] =
                        "admin",
                    ["Authentication:BootstrapAdministrator:DisplayName"] =
                        "Administrator",
                    ["Authentication:BootstrapAdministrator:Password"] =
                        password
                })
            .Build();
    }
}
