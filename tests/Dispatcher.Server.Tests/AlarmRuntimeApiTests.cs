using System.Net;
using System.Net.Http.Json;
using Dispatcher.Contracts.Alarms;
using Dispatcher.Contracts.Authentication;
using Dispatcher.Core.Tags;
using Dispatcher.Server.Alarms;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Events;
using Dispatcher.Server.Historian;
using Dispatcher.Server.Security;
using Microsoft.AspNetCore.Identity;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Options;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class AlarmRuntimeApiTests
{
    private const string Password =
        "Alarm-runtime-api-password-42";

    [TestMethod]
    public async Task CurrentHistoryAndAcknowledge_UseRuntimeAndPermissionBoundaries()
    {
        using var database =
            await CreateDatabaseWithAlarmAsync();
        var viewer =
            await InsertUserWithRoleAsync(
                database.DatabasePath,
                "alarm.viewer",
                BuiltInSecurityRoles.ViewerRoleId);
        var operatorUser =
            await InsertUserWithRoleAsync(
                database.DatabasePath,
                "alarm.operator",
                BuiltInSecurityRoles.OperatorRoleId);

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);

        using var anonymous =
            CreateCookieClient(
                factory);

        await AssertStatusAsync(
            HttpStatusCode.Unauthorized,
            anonymous.GetAsync(
                "/api/alarms/current"));
        await AssertStatusAsync(
            HttpStatusCode.Unauthorized,
            anonymous.PostAsync(
                "/api/alarms/test.high/acknowledge",
                content:
                    null));

        using var viewerClient =
            CreateCookieClient(
                factory);
        await LoginAsync(
            viewerClient,
            viewer.UserName);

        var tagService =
            factory.Services.GetRequiredService<TagService>();
        var raisedAt =
            DateTimeOffset.UtcNow.AddSeconds(-2);

        tagService.Set(
            "device01.register100",
            15,
            raisedAt);

        var current =
            await viewerClient.GetFromJsonAsync<AlarmRuntimeSnapshotDto[]>(
                "/api/alarms/current");

        Assert.IsNotNull(
            current);
        Assert.AreEqual(
            1,
            current.Length);

        var active =
            current[0];

        Assert.AreEqual(
            "test.high",
            active.AlarmId);
        Assert.AreEqual(
            AlarmRuntimeStateDto.ActiveUnacknowledged,
            active.State);
        Assert.AreEqual(
            raisedAt,
            active.RaisedAt);
        Assert.IsNotNull(
            active.CurrentValue);

        using var viewerAck =
            await viewerClient.PostAsync(
                "/api/alarms/test.high/acknowledge",
                content:
                    null);

        Assert.AreEqual(
            HttpStatusCode.Forbidden,
            viewerAck.StatusCode);

        using var operatorClient =
            CreateCookieClient(
                factory);
        await LoginAsync(
            operatorClient,
            operatorUser.UserName);

        using var ackResponse =
            await operatorClient.PostAsync(
                "/api/alarms/test.high/acknowledge",
                content:
                    null);

        Assert.AreEqual(
            HttpStatusCode.OK,
            ackResponse.StatusCode);

        var acknowledged =
            await ackResponse.Content.ReadFromJsonAsync<AlarmRuntimeSnapshotDto>();

        Assert.IsNotNull(
            acknowledged);
        Assert.AreEqual(
            AlarmRuntimeStateDto.ActiveAcknowledged,
            acknowledged.State);
        Assert.AreEqual(
            operatorUser.UserId,
            acknowledged.AcknowledgedByUserId);
        Assert.AreEqual(
            operatorUser.UserName,
            acknowledged.AcknowledgedByUserName);
        Assert.IsNotNull(
            acknowledged.AcknowledgedAt);

        var store =
            new SqliteOperationalStore(
                TestDispatcherFactory.GetOperationalDatabasePath(
                    database.DatabasePath));

        var persistedAck =
            await WaitForEventAsync(
                store,
                EventTypes.AlarmAcknowledged);

        Assert.AreEqual(
            operatorUser.UserId,
            persistedAck.ActorUserId);
        Assert.AreEqual(
            operatorUser.UserName,
            persistedAck.ActorUserName);

        var from =
            Uri.EscapeDataString(
                raisedAt.AddMinutes(-1).ToString("O"));
        var to =
            Uri.EscapeDataString(
                DateTimeOffset.UtcNow.AddMinutes(1).ToString("O"));

        var history =
            await operatorClient.GetFromJsonAsync<AlarmHistoryQueryResponseDto>(
                $"/api/alarms/history?from={from}&to={to}&page=1&limit=100");

        Assert.IsNotNull(
            history);
        Assert.IsTrue(
            history.Items.Any(item =>
                item.Type == EventTypes.AlarmRaised));
        Assert.IsTrue(
            history.Items.Any(item =>
                item.Type == EventTypes.AlarmAcknowledged));
        Assert.IsTrue(
            history.Items.All(item =>
                item.Type == EventTypes.AlarmRaised
                || item.Type == EventTypes.AlarmAcknowledged
                || item.Type == EventTypes.AlarmReturned));
    }

    [TestMethod]
    public async Task AcknowledgeAfterReturn_CompletesLifecycleAndRemovesCurrentAlarm()
    {
        using var database =
            await CreateDatabaseWithAlarmAsync();
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);
        using var client =
            factory.CreateClient();

        var tagService =
            factory.Services.GetRequiredService<TagService>();

        tagService.Set(
            "device01.register100",
            15,
            DateTimeOffset.UtcNow.AddSeconds(-2));
        tagService.Set(
            "device01.register100",
            7,
            DateTimeOffset.UtcNow.AddSeconds(-1));

        var before =
            await client.GetFromJsonAsync<AlarmRuntimeSnapshotDto[]>(
                "/api/alarms/current");

        Assert.IsNotNull(
            before);
        Assert.AreEqual(
            1,
            before.Length);
        Assert.AreEqual(
            AlarmRuntimeStateDto.ReturnedUnacknowledged,
            before[0].State);

        using var response =
            await client.PostAsync(
                "/api/alarms/test.high/acknowledge",
                content:
                    null);

        Assert.AreEqual(
            HttpStatusCode.OK,
            response.StatusCode);

        var result =
            await response.Content.ReadFromJsonAsync<AlarmRuntimeSnapshotDto>();

        Assert.IsNotNull(
            result);
        Assert.AreEqual(
            AlarmRuntimeStateDto.Normal,
            result.State);
        Assert.AreEqual(
            TestDispatcherFactory.TestAdministratorUserId,
            result.AcknowledgedByUserId);

        var after =
            await client.GetFromJsonAsync<AlarmRuntimeSnapshotDto[]>(
                "/api/alarms/current");

        Assert.IsNotNull(
            after);
        Assert.AreEqual(
            0,
            after.Length);
    }

    private static async Task<TestConfigurationDatabase> CreateDatabaseWithAlarmAsync()
    {
        var database =
            await TestConfigurationDatabase.CreateAsync(
                TestModbusConfiguration.CreateDevice(
                    port:
                        502,
                    enabled:
                        false));

        var store =
            new SqliteConfigurationStore(
                database.DatabasePath);

        await store.InsertAlarmDefinitionAsync(
            new AlarmDefinitionConfiguration(
                AlarmId:
                    "test.high",
                Name:
                    "Test high alarm",
                Enabled:
                    true,
                TagId:
                    "device01.register100",
                Condition:
                    AlarmCondition.High,
                Threshold:
                    10m,
                Severity:
                    AlarmSeverity.Warning,
                Message:
                    "Test value is high.",
                DelayMilliseconds:
                    0,
                Hysteresis:
                    2m));

        return database;
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
        using var response =
            await client.PostAsJsonAsync(
                "/api/auth/login",
                new LoginRequest(
                    userName,
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
            BuiltInSecurityRoles.All.Single(candidate =>
                candidate.RoleId == roleId);
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

    private static async Task<EventRecord> WaitForEventAsync(
        SqliteOperationalStore store,
        string eventType)
    {
        for (var attempt = 0;
             attempt < 100;
             attempt++)
        {
            var record =
                (await store.LoadAllEventsAsync())
                    .LastOrDefault(candidate =>
                        candidate.Type == eventType);

            if (record is not null)
            {
                return record;
            }

            await Task.Delay(
                20);
        }

        throw new TimeoutException(
            $"Event '{eventType}' was not persisted.");
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
