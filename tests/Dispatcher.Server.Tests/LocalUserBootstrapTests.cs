using Dispatcher.Server.Configuration;
using Dispatcher.Server.Security;
using Microsoft.AspNetCore.Identity;
using Microsoft.Data.Sqlite;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging.Abstractions;
using Microsoft.Extensions.Options;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class LocalUserBootstrapTests
{
    [TestMethod]
    public async Task EnsureBootstrapAdministratorAsync_CreatesHashedEnabledUser_OnlyOnce()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        const string password =
            "A-long-bootstrap-password-42";

        var configuration =
            CreateBootstrapConfiguration(
                password);

        var bootstrapper =
            new LocalUserBootstrapper(
                new SqliteConfigurationStore(
                    database.DatabasePath),
                configuration,
                NullLogger<LocalUserBootstrapper>.Instance);

        var created =
            await bootstrapper.EnsureBootstrapAdministratorAsync();
        var createdAgain =
            await bootstrapper.EnsureBootstrapAdministratorAsync();

        Assert.IsTrue(created);
        Assert.IsFalse(createdAgain);

        var store =
            new SqliteConfigurationStore(
                database.DatabasePath);
        var users =
            await store.LoadLocalUsersAsync();
        var roles =
            await store.LoadSecurityRolesAsync();
        var assignments =
            await store.LoadUserRoleAssignmentsAsync();

        Assert.AreEqual(1, users.Count);
        Assert.AreEqual(1, roles.Count);
        Assert.AreEqual(
            BuiltInSecurityRoles.AdministratorRoleId,
            roles[0].RoleId);
        Assert.AreEqual(1, assignments.Count);
        Assert.AreEqual(
            users[0].UserId,
            assignments[0].UserId);
        Assert.AreEqual(
            BuiltInSecurityRoles.AdministratorRoleId,
            assignments[0].RoleId);

        var user =
            users[0];

        Assert.AreEqual("admin", user.UserName);
        Assert.AreEqual("ADMIN", user.NormalizedUserName);
        Assert.AreEqual("Administrator", user.DisplayName);
        Assert.IsTrue(user.Enabled);
        Assert.AreNotEqual(password, user.PasswordHash);

        var passwordHasher =
            new PasswordHasher<LocalUserConfiguration>(
                Options.Create(
                    new PasswordHasherOptions()));

        var verification =
            passwordHasher.VerifyHashedPassword(
                user,
                user.PasswordHash,
                password);

        Assert.AreNotEqual(
            PasswordVerificationResult.Failed,
            verification);
    }

    [TestMethod]
    public async Task EnsureBootstrapAdministratorAsync_DoesNotCreateUser_WhenPasswordIsMissing()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        var bootstrapper =
            new LocalUserBootstrapper(
                new SqliteConfigurationStore(
                    database.DatabasePath),
                new ConfigurationBuilder().Build(),
                NullLogger<LocalUserBootstrapper>.Instance);

        var created =
            await bootstrapper.EnsureBootstrapAdministratorAsync();

        Assert.IsFalse(created);

        var store =
            new SqliteConfigurationStore(
                database.DatabasePath);
        var users =
            await store.LoadLocalUsersAsync();
        var roles =
            await store.LoadSecurityRolesAsync();
        var assignments =
            await store.LoadUserRoleAssignmentsAsync();

        Assert.AreEqual(0, users.Count);
        Assert.AreEqual(0, roles.Count);
        Assert.AreEqual(0, assignments.Count);
    }

    [TestMethod]
    public async Task InsertLocalUserAsync_PersistsDisabledState_AndSupportsNormalizedLookup()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        var store =
            new SqliteConfigurationStore(
                database.DatabasePath);

        var user =
            new LocalUserConfiguration(
                UserId:
                    Guid.NewGuid().ToString("N"),
                UserName:
                    "operator.one",
                NormalizedUserName:
                    LocalUserConfiguration.NormalizeUserName(
                        "operator.one"),
                DisplayName:
                    "Operator One",
                Enabled:
                    false,
                PasswordHash:
                    "test-password-hash");

        await store.InsertLocalUserAsync(
            user);

        var reopenedStore =
            new SqliteConfigurationStore(
                database.DatabasePath);

        await reopenedStore.InitializeAsync();

        var loaded =
            await reopenedStore.FindLocalUserByNormalizedUserNameAsync(
                LocalUserConfiguration.NormalizeUserName(
                    "Operator.One"));

        Assert.IsNotNull(loaded);
        Assert.AreEqual(user.UserId, loaded.UserId);
        Assert.AreEqual("operator.one", loaded.UserName);
        Assert.AreEqual("Operator One", loaded.DisplayName);
        Assert.IsFalse(loaded.Enabled);
    }

    [TestMethod]
    public async Task InitializeAsync_MigratesVersion4Database_ToVersion7WithoutLosingConfigurationTable()
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
                "dispatcher-v4.db");

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
                    CREATE TABLE historian_policies (
                        tag_id TEXT NOT NULL PRIMARY KEY,
                        enabled INTEGER NOT NULL,
                        mode INTEGER NOT NULL,
                        period_ms INTEGER NULL,
                        retention_days INTEGER NOT NULL
                    );

                    INSERT INTO historian_policies (
                        tag_id,
                        enabled,
                        mode,
                        period_ms,
                        retention_days)
                    VALUES (
                        'plc01.temperature',
                        1,
                        0,
                        NULL,
                        30);

                    PRAGMA user_version = 4;
                    """;

                await command.ExecuteNonQueryAsync();
            }

            var store =
                new SqliteConfigurationStore(
                    databasePath);

            await store.InitializeAsync();

            var policies =
                await store.LoadHistorianPoliciesAsync();
            var users =
                await store.LoadLocalUsersAsync();

            Assert.AreEqual(1, policies.Count);
            Assert.AreEqual(
                "plc01.temperature",
                policies[0].TagId);
            Assert.AreEqual(0, users.Count);

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

            Assert.AreEqual(7, version);
        }
        finally
        {
            if (Directory.Exists(
                    directory))
            {
                Directory.Delete(
                    directory,
                    recursive: true);
            }
        }
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
