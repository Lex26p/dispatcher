using System.Net;
using System.Net.Http.Json;
using Dispatcher.Contracts.Authentication;
using Dispatcher.Contracts.Authorization;
using Dispatcher.Contracts.Configuration;
using Dispatcher.Contracts.Events;
using Dispatcher.Contracts.Security;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Events;
using Dispatcher.Server.Historian;
using Dispatcher.Server.Security;
using Microsoft.AspNetCore.Identity;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.Extensions.Options;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class ActorAwareAuditTests
{
    private const string Password =
        "Actor-aware-audit-password-42";

    [TestMethod]
    public async Task LoginAudit_VerifiedActorOnlyOnSuccessfulAuthentication()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        var user =
            await InsertLocalAdministratorAsync(
                database.DatabasePath,
                "audit.login");
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);
        using var client =
            CreateCookieClient(
                factory);

        using var failed =
            await client.PostAsJsonAsync(
                "/api/auth/login",
                new LoginRequest(
                    user.UserName,
                    "wrong-password"));

        Assert.AreEqual(
            HttpStatusCode.Unauthorized,
            failed.StatusCode);

        using var succeeded =
            await client.PostAsJsonAsync(
                "/api/auth/login",
                new LoginRequest(
                    user.UserName,
                    Password));

        Assert.AreEqual(
            HttpStatusCode.OK,
            succeeded.StatusCode);

        var store =
            CreateOperationalStore(
                database.DatabasePath);
        var failedEvent =
            await WaitForEventAsync(
                store,
                EventTypes.LoginFailed);
        var succeededEvent =
            await WaitForEventAsync(
                store,
                EventTypes.LoginSucceeded);

        Assert.IsNull(
            failedEvent.ActorUserId);
        Assert.IsNull(
            failedEvent.ActorUserName);
        StringAssert.Contains(
            failedEvent.DataJson!,
            user.UserName);
        Assert.IsFalse(
            failedEvent.DataJson!.Contains(
                "wrong-password",
                StringComparison.Ordinal));

        Assert.AreEqual(
            user.UserId,
            succeededEvent.ActorUserId);
        Assert.AreEqual(
            user.UserName,
            succeededEvent.ActorUserName);
    }

    [TestMethod]
    public async Task AuthenticatedMutations_PersistActorIdentityAcrossAuditProducers()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);
        using var client =
            factory.CreateClient();

        var createdUserResponse =
            await client.PostAsJsonAsync(
                "/api/security/users",
                new CreateSecurityUserRequest(
                    UserName:
                        "audit.target",
                    DisplayName:
                        "Audit Target",
                    Password:
                        Password,
                    Enabled:
                        true));

        Assert.AreEqual(
            HttpStatusCode.Created,
            createdUserResponse.StatusCode);

        var createdUser =
            await createdUserResponse.Content.ReadFromJsonAsync<SecurityUserDto>()
            ?? throw new InvalidOperationException(
                "Security user response was empty.");

        var createdRoleResponse =
            await client.PostAsJsonAsync(
                "/api/security/roles",
                new SecurityRoleUpsertRequest(
                    Name:
                        "Audit Viewer",
                    Permissions:
                    [
                        PermissionNames.RuntimeRead
                    ]));

        Assert.AreEqual(
            HttpStatusCode.Created,
            createdRoleResponse.StatusCode);

        var createdRole =
            await createdRoleResponse.Content.ReadFromJsonAsync<SecurityRoleDto>()
            ?? throw new InvalidOperationException(
                "Security role response was empty.");

        using var assignmentResponse =
            await client.PutAsJsonAsync(
                $"/api/security/users/{createdUser.UserId}/roles",
                new ReplaceSecurityUserRolesRequest(
                    [
                        createdRole.RoleId
                    ]));

        Assert.AreEqual(
            HttpStatusCode.OK,
            assignmentResponse.StatusCode);

        using var passwordResetResponse =
            await client.PutAsJsonAsync(
                $"/api/security/users/{createdUser.UserId}/password",
                new ResetSecurityUserPasswordRequest(
                    "Actor-aware-audit-new-password-84"));

        Assert.AreEqual(
            HttpStatusCode.NoContent,
            passwordResetResponse.StatusCode);

        using var tagWriteResponse =
            await client.PostAsJsonAsync(
                "/api/tags/missing/write",
                new
                {
                    value = 1
                });

        Assert.AreEqual(
            HttpStatusCode.NotFound,
            tagWriteResponse.StatusCode);

        using var configurationResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices",
                new ModbusDeviceUpsertRequest(
                    DeviceId:
                        "audit-device",
                    Name:
                        "Audit device",
                    Enabled:
                        false,
                    Host:
                        "127.0.0.1",
                    Port:
                        502,
                    UnitId:
                        1,
                    PollIntervalMilliseconds:
                        1000,
                    RequestTimeoutMilliseconds:
                        1000));

        Assert.AreEqual(
            HttpStatusCode.Created,
            configurationResponse.StatusCode);

        var store =
            CreateOperationalStore(
                database.DatabasePath);

        var userCreated =
            await WaitForEventAsync(
                store,
                EventTypes.SecurityUserCreated);
        var roleCreated =
            await WaitForEventAsync(
                store,
                EventTypes.SecurityRoleCreated);
        var rolesChanged =
            await WaitForEventAsync(
                store,
                EventTypes.SecurityUserRolesChanged);
        var passwordReset =
            await WaitForEventAsync(
                store,
                EventTypes.SecurityUserPasswordReset);
        var tagWrite =
            await WaitForEventAsync(
                store,
                EventTypes.TagWriteFailed);
        var configurationChanged =
            await WaitForEventAsync(
                store,
                EventTypes.ConfigurationChanged,
                record =>
                    record.DataJson?.Contains(
                        "audit-device",
                        StringComparison.Ordinal)
                    == true);

        foreach (var record in new[]
                 {
                     userCreated,
                     roleCreated,
                     rolesChanged,
                     passwordReset,
                     tagWrite,
                     configurationChanged
                 })
        {
            Assert.AreEqual(
                TestDispatcherFactory.TestAdministratorUserId,
                record.ActorUserId);
            Assert.AreEqual(
                "dispatcher.tests.admin",
                record.ActorUserName);
        }

        Assert.IsFalse(
            passwordReset.DataJson?.Contains(
                "Actor-aware-audit-new-password-84",
                StringComparison.Ordinal)
            ?? false);

        var from =
            DateTimeOffset.UtcNow.AddMinutes(-5);
        var to =
            DateTimeOffset.UtcNow.AddMinutes(5);
        var queryUri =
            $"/api/events?from={Uri.EscapeDataString(from.ToString("O"))}&to={Uri.EscapeDataString(to.ToString("O"))}&page=1&limit=200";
        var query =
            await client.GetFromJsonAsync<EventQueryResponseDto>(
                queryUri)
            ?? throw new InvalidOperationException(
                "Events query response was empty.");
        var projected =
            query.Items.Single(record =>
                record.EventId
                == configurationChanged.EventId);

        Assert.AreEqual(
            TestDispatcherFactory.TestAdministratorUserId,
            projected.ActorUserId);
        Assert.AreEqual(
            "dispatcher.tests.admin",
            projected.ActorUserName);
    }

    private static HttpClient CreateCookieClient(
        WebApplicationFactory<Program> factory)
    {
        return factory.CreateClient(
            new WebApplicationFactoryClientOptions
            {
                AllowAutoRedirect =
                    false,
                HandleCookies =
                    true
            });
    }

    private static SqliteOperationalStore CreateOperationalStore(
        string configurationDatabasePath)
    {
        return new SqliteOperationalStore(
            TestDispatcherFactory.GetOperationalDatabasePath(
                configurationDatabasePath));
    }

    private static async Task<EventRecord> WaitForEventAsync(
        SqliteOperationalStore store,
        string type,
        Func<EventRecord, bool>? predicate = null)
    {
        for (var attempt = 0;
             attempt < 100;
             attempt++)
        {
            var events =
                await store.LoadAllEventsAsync();
            var found =
                events.LastOrDefault(record =>
                    string.Equals(
                        record.Type,
                        type,
                        StringComparison.Ordinal)
                    && (predicate?.Invoke(record)
                        ?? true));

            if (found is not null)
            {
                return found;
            }

            await Task.Delay(
                20);
        }

        throw new TimeoutException(
            $"Audit event '{type}' was not persisted.");
    }

    private static async Task<LocalUserConfiguration> InsertLocalAdministratorAsync(
        string databasePath,
        string userName)
    {
        var store =
            new SqliteConfigurationStore(
                databasePath);
        var role =
            BuiltInSecurityRoles.All.Single(candidate =>
                candidate.RoleId
                == BuiltInSecurityRoles.AdministratorRoleId);
        var user =
            new LocalUserConfiguration(
                UserId:
                    Guid.NewGuid().ToString("N"),
                UserName:
                    userName,
                NormalizedUserName:
                    LocalUserConfiguration.NormalizeUserName(
                        userName),
                DisplayName:
                    "Audit Login User",
                Enabled:
                    true,
                PasswordHash:
                    "pending");
        var hasher =
            new PasswordHasher<LocalUserConfiguration>(
                Options.Create(
                    new PasswordHasherOptions()));

        user =
            user with
            {
                PasswordHash =
                    hasher.HashPassword(
                        user,
                        Password)
            };

        await store.InsertLocalUserWithRoleAsync(
            user,
            role);

        return user;
    }
}
