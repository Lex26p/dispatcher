using System.Net;
using System.Net.Http.Json;
using Dispatcher.Contracts.Authentication;
using Dispatcher.Contracts.Configuration;
using Dispatcher.Contracts.Historian;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Security;
using Microsoft.AspNetCore.Identity;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Options;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class AuthorizationApiTests
{
    private const string Password =
        "Authorization-test-password-42";

    [TestMethod]
    public async Task ProtectedServerBoundaries_AnonymousUser_ReturnUnauthorized()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);
        using var client =
            CreateCookieClient(
                factory);

        await AssertStatusAsync(
            HttpStatusCode.Unauthorized,
            client.GetAsync(
                "/api/tags"));
        await AssertStatusAsync(
            HttpStatusCode.Unauthorized,
            client.GetAsync(
                "/api/configuration/modbus/devices"));
        await AssertStatusAsync(
            HttpStatusCode.Unauthorized,
            client.GetAsync(
                CreateHistoryQuery()));
        await AssertStatusAsync(
            HttpStatusCode.Unauthorized,
            client.GetAsync(
                CreateEventsQuery()));
        await AssertStatusAsync(
            HttpStatusCode.Unauthorized,
            client.GetAsync(
                "/api/mimics"));
        await AssertStatusAsync(
            HttpStatusCode.Unauthorized,
            client.PostAsync(
                "/hubs/runtime/negotiate?negotiateVersion=1",
                content:
                    null));

        await AssertStatusAsync(
            HttpStatusCode.OK,
            client.GetAsync(
                "/health"));
        await AssertStatusAsync(
            HttpStatusCode.OK,
            client.GetAsync(
                "/api/auth/current"));
    }

    [TestMethod]
    public async Task Viewer_CanReadRuntimeData_ButMutationPermissionsReturnForbidden()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        var user =
            await InsertUserWithRoleAsync(
                database.DatabasePath,
                "viewer.one",
                BuiltInSecurityRoles.ViewerRoleId);
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);
        using var client =
            CreateCookieClient(
                factory);

        await LoginAsync(
            client,
            user.UserName);

        await AssertStatusAsync(
            HttpStatusCode.OK,
            client.GetAsync(
                "/api/tags"));
        await AssertStatusAsync(
            HttpStatusCode.OK,
            client.GetAsync(
                "/api/configuration/snmp/devices"));
        await AssertStatusAsync(
            HttpStatusCode.OK,
            client.GetAsync(
                CreateHistoryQuery()));
        await AssertStatusAsync(
            HttpStatusCode.OK,
            client.GetAsync(
                CreateEventsQuery()));
        await AssertStatusAsync(
            HttpStatusCode.OK,
            client.GetAsync(
                "/api/mimics"));
        await AssertStatusAsync(
            HttpStatusCode.Forbidden,
            client.PostAsJsonAsync(
                "/api/tags/missing/write",
                new
                {
                    value = 1
                }));
        await AssertStatusAsync(
            HttpStatusCode.Forbidden,
            client.PostAsJsonAsync(
                "/api/configuration/modbus/devices",
                CreateDisabledModbusDeviceRequest(
                    "viewer-device")));
        await AssertStatusAsync(
            HttpStatusCode.Forbidden,
            client.PutAsJsonAsync(
                "/api/configuration/mimics/viewer-mimic",
                CreateMimicDefinition(
                    "viewer-mimic")));
        await AssertStatusAsync(
            HttpStatusCode.Forbidden,
            client.PutAsJsonAsync(
                "/api/configuration/historian/policies/missing-tag",
                CreateHistorianPolicyRequest()));
    }

    [TestMethod]
    public async Task Operator_CanReachTagWrite_ButCannotEditDeviceConfiguration()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        var user =
            await InsertUserWithRoleAsync(
                database.DatabasePath,
                "operator.one",
                BuiltInSecurityRoles.OperatorRoleId);
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);
        using var client =
            CreateCookieClient(
                factory);

        await LoginAsync(
            client,
            user.UserName);

        var writeResponse =
            await client.PostAsJsonAsync(
                "/api/tags/missing/write",
                new
                {
                    value = 1
                });

        Assert.AreEqual(
            HttpStatusCode.NotFound,
            writeResponse.StatusCode);

        await AssertStatusAsync(
            HttpStatusCode.Forbidden,
            client.PostAsJsonAsync(
                "/api/configuration/modbus/devices",
                CreateDisabledModbusDeviceRequest(
                    "operator-device")));
    }

    [TestMethod]
    public async Task Engineer_CanReachEngineeringMutationEndpoints()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        var user =
            await InsertUserWithRoleAsync(
                database.DatabasePath,
                "engineer.one",
                BuiltInSecurityRoles.EngineerRoleId);
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);
        using var client =
            CreateCookieClient(
                factory);

        await LoginAsync(
            client,
            user.UserName);

        var deviceResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices",
                CreateDisabledModbusDeviceRequest(
                    "engineer-device"));

        Assert.AreEqual(
            HttpStatusCode.Created,
            deviceResponse.StatusCode);

        var mimicResponse =
            await client.PutAsJsonAsync(
                "/api/configuration/mimics/engineer-mimic",
                CreateMimicDefinition(
                    "engineer-mimic"));

        Assert.AreEqual(
            HttpStatusCode.OK,
            mimicResponse.StatusCode);

        var historianResponse =
            await client.PutAsJsonAsync(
                "/api/configuration/historian/policies/missing-tag",
                CreateHistorianPolicyRequest());

        Assert.AreNotEqual(
            HttpStatusCode.Forbidden,
            historianResponse.StatusCode);
        Assert.AreNotEqual(
            HttpStatusCode.Unauthorized,
            historianResponse.StatusCode);
    }

    [TestMethod]
    public async Task ExistingCookie_IsForbidden_WhenCurrentSecurityCatalogDisablesUser()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        var user =
            await InsertUserWithRoleAsync(
                database.DatabasePath,
                "viewer.disabled-after-login",
                BuiltInSecurityRoles.ViewerRoleId);
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);
        using var client =
            CreateCookieClient(
                factory);

        await LoginAsync(
            client,
            user.UserName);

        await AssertStatusAsync(
            HttpStatusCode.OK,
            client.GetAsync(
                "/api/tags"));

        var store =
            new SqliteConfigurationStore(
                database.DatabasePath);
        var roles =
            await store.LoadSecurityRolesAsync();
        var assignments =
            await store.LoadUserRoleAssignmentsAsync();
        var catalog =
            factory.Services.GetRequiredService<SecurityCatalog>();

        catalog.ReplaceAll(
            [
                user with
                {
                    Enabled = false
                }
            ],
            roles,
            assignments);

        await AssertStatusAsync(
            HttpStatusCode.Forbidden,
            client.GetAsync(
                "/api/tags"));
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

    private static async Task LoginAsync(
        HttpClient client,
        string userName)
    {
        var response =
            await client.PostAsJsonAsync(
                "/api/auth/login",
                new LoginRequest(
                    UserName:
                        userName,
                    Password:
                        Password));

        Assert.AreEqual(
            HttpStatusCode.OK,
            response.StatusCode);
    }

    private static async Task<LocalUserConfiguration> InsertUserWithRoleAsync(
        string databasePath,
        string userName,
        string roleId)
    {
        var store =
            new SqliteConfigurationStore(
                databasePath);
        var role =
            BuiltInSecurityRoles.All.Single(
                candidate =>
                    candidate.RoleId
                    == roleId);
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
                    userName,
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

    private static ModbusDeviceUpsertRequest CreateDisabledModbusDeviceRequest(
        string deviceId)
    {
        return new ModbusDeviceUpsertRequest(
            DeviceId:
                deviceId,
            Name:
                deviceId,
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
                1000);
    }

    private static object CreateMimicDefinition(
        string mimicId)
    {
        return new
        {
            mimicId,
            name = "Authorization test mimic",
            width = 400,
            height = 250,
            elements = Array.Empty<object>()
        };
    }

    private static HistorianPolicyUpsertRequest CreateHistorianPolicyRequest()
    {
        return new HistorianPolicyUpsertRequest(
            Enabled:
                true,
            Mode:
                HistorianSamplingModeDto.OnChange,
            PeriodMilliseconds:
                null,
            RetentionDays:
                30);
    }

    private static string CreateHistoryQuery()
    {
        return "/api/history?tagId=missing&from=2026-08-16T00%3A00%3A00Z&to=2026-08-17T00%3A00%3A00Z";
    }

    private static string CreateEventsQuery()
    {
        return "/api/events?from=2026-08-16T00%3A00%3A00Z&to=2026-08-17T00%3A00%3A00Z";
    }

    private static async Task AssertStatusAsync(
        HttpStatusCode expected,
        Task<HttpResponseMessage> responseTask)
    {
        using var response =
            await responseTask;

        Assert.AreEqual(
            expected,
            response.StatusCode);
    }
}
