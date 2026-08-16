using System.Net;
using System.Net.Http.Json;
using Dispatcher.Contracts.Alarms;
using Dispatcher.Contracts.Authentication;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Events;
using Dispatcher.Server.Security;
using Microsoft.AspNetCore.Identity;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Options;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class AlarmDefinitionApiTests
{
    private const string Password =
        "Alarm-definition-test-password-42";

    [TestMethod]
    public async Task AlarmDefinitions_AdministratorCanCrudAndValidationIsExplicit()
    {
        using var database =
            await CreateConfigurationDatabaseAsync();
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);
        using var client =
            factory.CreateClient();

        var createResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/alarms/definitions",
                CreateHighAlarm(
                    "temperature.high"));

        Assert.AreEqual(
            HttpStatusCode.Created,
            createResponse.StatusCode);

        var created =
            await createResponse.Content.ReadFromJsonAsync<AlarmDefinitionDto>();

        Assert.IsNotNull(
            created);
        Assert.AreEqual(
            "temperature.high",
            created.AlarmId);
        Assert.AreEqual(
            AlarmConditionDto.High,
            created.Condition);
        Assert.IsTrue(
            created.Threshold.HasValue);
        Assert.AreEqual(
            80.25m,
            created.Threshold.Value);
        Assert.IsTrue(
            created.Hysteresis.HasValue);
        Assert.AreEqual(
            2.5m,
            created.Hysteresis.Value);

        var duplicateResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/alarms/definitions",
                CreateHighAlarm(
                    "temperature.high"));

        Assert.AreEqual(
            HttpStatusCode.Conflict,
            duplicateResponse.StatusCode);

        var all =
            await client.GetFromJsonAsync<AlarmDefinitionDto[]>(
                "/api/configuration/alarms/definitions");

        Assert.IsNotNull(
            all);
        Assert.AreEqual(
            1,
            all.Length);

        var updateResponse =
            await client.PutAsJsonAsync(
                "/api/configuration/alarms/definitions/temperature.high",
                new UpdateAlarmDefinitionRequest(
                    Name:
                        "Temperature digital state",
                    Enabled:
                        false,
                    TagId:
                        "device01.register100",
                    Condition:
                        AlarmConditionDto.DigitalTrue,
                    Threshold:
                        null,
                    Severity:
                        AlarmSeverityDto.Information,
                    Message:
                        "Digital state is true.",
                    DelayMilliseconds:
                        0,
                    Hysteresis:
                        null));

        Assert.AreEqual(
            HttpStatusCode.OK,
            updateResponse.StatusCode);

        var invalidDigitalResponse =
            await client.PutAsJsonAsync(
                "/api/configuration/alarms/definitions/temperature.high",
                new UpdateAlarmDefinitionRequest(
                    Name:
                        "Invalid digital alarm",
                    Enabled:
                        true,
                    TagId:
                        "device01.register100",
                    Condition:
                        AlarmConditionDto.DigitalFalse,
                    Threshold:
                        1m,
                    Severity:
                        AlarmSeverityDto.Warning,
                    Message:
                        "Invalid.",
                    DelayMilliseconds:
                        0,
                    Hysteresis:
                        null));

        Assert.AreEqual(
            HttpStatusCode.BadRequest,
            invalidDigitalResponse.StatusCode);

        var missingTagResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/alarms/definitions",
                CreateHighAlarm(
                    "missing.target",
                    tagId:
                        "missing.tag"));

        Assert.AreEqual(
            HttpStatusCode.NotFound,
            missingTagResponse.StatusCode);

        var deleteResponse =
            await client.DeleteAsync(
                "/api/configuration/alarms/definitions/temperature.high");

        Assert.AreEqual(
            HttpStatusCode.NoContent,
            deleteResponse.StatusCode);

        var afterDelete =
            await client.GetFromJsonAsync<AlarmDefinitionDto[]>(
                "/api/configuration/alarms/definitions");

        Assert.IsNotNull(
            afterDelete);
        Assert.AreEqual(
            0,
            afterDelete.Length);
    }

    [TestMethod]
    public async Task AlarmDefinitions_AnonymousAndViewerCannotMutate_EngineerCanConfigure()
    {
        using var database =
            await CreateConfigurationDatabaseAsync();

        using (var anonymousFactory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false))
        using (var anonymousClient =
            CreateCookieClient(
                anonymousFactory))
        {
            await AssertStatusAsync(
                HttpStatusCode.Unauthorized,
                anonymousClient.GetAsync(
                    "/api/configuration/alarms/definitions"));
            await AssertStatusAsync(
                HttpStatusCode.Unauthorized,
                anonymousClient.PostAsJsonAsync(
                    "/api/configuration/alarms/definitions",
                    CreateHighAlarm(
                        "anonymous.high")));
        }

        var viewer =
            await InsertUserWithRoleAsync(
                database.DatabasePath,
                "alarm.viewer",
                BuiltInSecurityRoles.ViewerRoleId);

        using (var viewerFactory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false))
        using (var viewerClient =
            CreateCookieClient(
                viewerFactory))
        {
            await LoginAsync(
                viewerClient,
                viewer.UserName);

            await AssertStatusAsync(
                HttpStatusCode.OK,
                viewerClient.GetAsync(
                    "/api/configuration/alarms/definitions"));
            await AssertStatusAsync(
                HttpStatusCode.Forbidden,
                viewerClient.PostAsJsonAsync(
                    "/api/configuration/alarms/definitions",
                    CreateHighAlarm(
                        "viewer.high")));
        }

        var engineer =
            await InsertUserWithRoleAsync(
                database.DatabasePath,
                "alarm.engineer",
                BuiltInSecurityRoles.EngineerRoleId);

        using var engineerFactory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);
        using var engineerClient =
            CreateCookieClient(
                engineerFactory);

        await LoginAsync(
            engineerClient,
            engineer.UserName);

        var engineerResponse =
            await engineerClient.PostAsJsonAsync(
                "/api/configuration/alarms/definitions",
                CreateHighAlarm(
                    "engineer.high"));

        Assert.AreEqual(
            HttpStatusCode.Created,
            engineerResponse.StatusCode);
    }

    [TestMethod]
    public async Task AlarmDefinitionMutation_PublishesActorAwareConfigurationAudit()
    {
        using var database =
            await CreateConfigurationDatabaseAsync();
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);
        using var client =
            factory.CreateClient();

        var eventJournal =
            factory.Services.GetRequiredService<EventJournalService>();
        var persistedCompletion =
            new TaskCompletionSource<EventRecord>(
                TaskCreationOptions.RunContinuationsAsynchronously);

        void OnPersisted(EventRecord record)
        {
            var dataJson =
                record.DataJson;

            if (record.Type == EventTypes.ConfigurationChanged
                && record.ActorUserId
                    == TestDispatcherFactory.TestAdministratorUserId
                && dataJson is not null
                && dataJson.Contains(
                    "\"Area\":\"Alarms\"",
                    StringComparison.Ordinal)
                && dataJson.Contains(
                    "audit.high",
                    StringComparison.Ordinal))
            {
                persistedCompletion.TrySetResult(
                    record);
            }
        }

        eventJournal.Persisted +=
            OnPersisted;

        try
        {
            var response =
                await client.PostAsJsonAsync(
                    "/api/configuration/alarms/definitions",
                    CreateHighAlarm(
                        "audit.high"));

            Assert.AreEqual(
                HttpStatusCode.Created,
                response.StatusCode);

            var persisted =
                await persistedCompletion.Task.WaitAsync(
                    TimeSpan.FromSeconds(5));

            Assert.AreEqual(
                TestDispatcherFactory.TestAdministratorUserId,
                persisted.ActorUserId);
            Assert.AreEqual(
                "dispatcher.tests.admin",
                persisted.ActorUserName);
            Assert.AreEqual(
                EventCategory.Configuration,
                persisted.Category);
        }
        finally
        {
            eventJournal.Persisted -=
                OnPersisted;
        }
    }

    private static async Task<TestConfigurationDatabase> CreateConfigurationDatabaseAsync()
    {
        return await TestConfigurationDatabase.CreateAsync(
            TestModbusConfiguration.CreateDevice(
                port:
                    502,
                enabled:
                    false));
    }

    private static CreateAlarmDefinitionRequest CreateHighAlarm(
        string alarmId,
        string tagId = "device01.register100")
    {
        return new CreateAlarmDefinitionRequest(
            AlarmId:
                alarmId,
            Name:
                "High temperature",
            Enabled:
                true,
            TagId:
                tagId,
            Condition:
                AlarmConditionDto.High,
            Threshold:
                80.25m,
            Severity:
                AlarmSeverityDto.Warning,
            Message:
                "Temperature is high.",
            DelayMilliseconds:
                1000,
            Hysteresis:
                2.5m);
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
            BuiltInSecurityRoles.All.Single(candidate =>
                candidate.RoleId
                == roleId);
        var user =
            new LocalUserConfiguration(
                UserId:
                    Guid.NewGuid().ToString(
                        "N"),
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
