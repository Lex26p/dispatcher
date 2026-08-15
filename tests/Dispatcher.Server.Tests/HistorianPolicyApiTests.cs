using System.Net;
using System.Net.Http.Json;
using Dispatcher.Contracts.Historian;
using Dispatcher.Core.Tags;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Historian;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class HistorianPolicyApiTests
{
    [TestMethod]
    public async Task PolicyCrud_PersistsAndAppliesOnChangeWithoutServerRestart()
    {
        var device =
            TestModbusConfiguration.CreateDevice(
                port: 502,
                enabled: false);

        using var database =
            await TestConfigurationDatabase.CreateAsync(
                device);

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var request =
            new HistorianPolicyUpsertRequest(
                Enabled: true,
                Mode: HistorianSamplingModeDto.OnChange,
                PeriodMilliseconds: null,
                RetentionDays: 30);

        var putResponse =
            await client.PutAsJsonAsync(
                "/api/configuration/historian/policies/device01.register100",
                request);

        Assert.AreEqual(
            HttpStatusCode.OK,
            putResponse.StatusCode);

        var policy =
            await putResponse.Content.ReadFromJsonAsync<HistorianPolicyDto>();

        Assert.IsNotNull(
            policy);
        Assert.AreEqual(
            "device01.register100",
            policy.TagId);
        Assert.IsTrue(
            policy.TagExists);
        Assert.AreEqual(
            HistorianSamplingModeDto.OnChange,
            policy.Mode);

        var tags =
            factory.Services.GetRequiredService<TagService>();

        tags.Set(
            "device01.register100",
            123);

        var operationalStore =
            new SqliteOperationalStore(
                TestDispatcherFactory.GetOperationalDatabasePath(
                    database.DatabasePath));

        await operationalStore.InitializeAsync();

        await WaitUntilAsync(
            async () =>
                (await operationalStore.LoadAllAsync()).Count == 1,
            TimeSpan.FromSeconds(2));

        var reopenedConfigurationStore =
            new SqliteConfigurationStore(
                database.DatabasePath);

        await reopenedConfigurationStore.InitializeAsync();

        var persistedPolicies =
            await reopenedConfigurationStore.LoadHistorianPoliciesAsync();

        Assert.AreEqual(
            1,
            persistedPolicies.Count);
        Assert.AreEqual(
            30,
            persistedPolicies[0].RetentionDays);

        var deleteResponse =
            await client.DeleteAsync(
                "/api/configuration/historian/policies/device01.register100");

        Assert.AreEqual(
            HttpStatusCode.NoContent,
            deleteResponse.StatusCode);

        tags.Set(
            "device01.register100",
            456);

        await Task.Delay(150);

        Assert.AreEqual(
            1,
            (await operationalStore.LoadAllAsync()).Count);

        Assert.AreEqual(
            0,
            (await reopenedConfigurationStore.LoadHistorianPoliciesAsync()).Count);
    }

    [TestMethod]
    public async Task PolicyForDeletedTag_RemainsManageableAndReportsStaleBinding()
    {
        var device =
            TestModbusConfiguration.CreateDevice(
                port: 502,
                enabled: false);

        using var database =
            await TestConfigurationDatabase.CreateAsync(
                device);

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var putResponse =
            await client.PutAsJsonAsync(
                "/api/configuration/historian/policies/device01.register100",
                new HistorianPolicyUpsertRequest(
                    true,
                    HistorianSamplingModeDto.OnChange,
                    null,
                    7));

        Assert.AreEqual(
            HttpStatusCode.OK,
            putResponse.StatusCode);

        var deleteTagResponse =
            await client.DeleteAsync(
                "/api/configuration/modbus/devices/device01/tags/device01.register100");

        Assert.AreEqual(
            HttpStatusCode.NoContent,
            deleteTagResponse.StatusCode);

        var policies =
            await client.GetFromJsonAsync<HistorianPolicyDto[]>(
                "/api/configuration/historian/policies");

        Assert.IsNotNull(
            policies);
        Assert.AreEqual(
            1,
            policies.Length);
        Assert.AreEqual(
            "device01.register100",
            policies[0].TagId);
        Assert.IsFalse(
            policies[0].TagExists);

        var updateStaleResponse =
            await client.PutAsJsonAsync(
                "/api/configuration/historian/policies/device01.register100",
                new HistorianPolicyUpsertRequest(
                    false,
                    HistorianSamplingModeDto.OnChange,
                    null,
                    3));

        Assert.AreEqual(
            HttpStatusCode.OK,
            updateStaleResponse.StatusCode);

        var updated =
            await updateStaleResponse.Content.ReadFromJsonAsync<HistorianPolicyDto>();

        Assert.IsNotNull(
            updated);
        Assert.IsFalse(
            updated.Enabled);
        Assert.IsFalse(
            updated.TagExists);
        Assert.AreEqual(
            3,
            updated.RetentionDays);

        var tags =
            factory.Services.GetRequiredService<TagService>();

        tags.Set(
            "device01.register100",
            999);

        var operationalStore =
            new SqliteOperationalStore(
                TestDispatcherFactory.GetOperationalDatabasePath(
                    database.DatabasePath));

        await operationalStore.InitializeAsync();
        await Task.Delay(150);

        Assert.AreEqual(
            0,
            (await operationalStore.LoadAllAsync()).Count);
    }

    [TestMethod]
    public async Task NewPolicyForUnknownTag_ReturnsNotFound()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var response =
            await client.PutAsJsonAsync(
                "/api/configuration/historian/policies/missing.tag",
                new HistorianPolicyUpsertRequest(
                    true,
                    HistorianSamplingModeDto.OnChange,
                    null,
                    30));

        Assert.AreEqual(
            HttpStatusCode.NotFound,
            response.StatusCode);
    }

    [TestMethod]
    public async Task PeriodicPolicyWithoutPeriod_ReturnsBadRequest()
    {
        var device =
            TestModbusConfiguration.CreateDevice(
                port: 502,
                enabled: false);

        using var database =
            await TestConfigurationDatabase.CreateAsync(
                device);

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var response =
            await client.PutAsJsonAsync(
                "/api/configuration/historian/policies/device01.register100",
                new HistorianPolicyUpsertRequest(
                    true,
                    HistorianSamplingModeDto.Periodic,
                    null,
                    30));

        Assert.AreEqual(
            HttpStatusCode.BadRequest,
            response.StatusCode);
    }

    private static async Task WaitUntilAsync(
        Func<Task<bool>> condition,
        TimeSpan timeout)
    {
        var deadline =
            DateTimeOffset.UtcNow + timeout;

        while (!await condition())
        {
            if (DateTimeOffset.UtcNow >= deadline)
            {
                Assert.Fail(
                    "Historian policy did not affect sampling before timeout.");
            }

            await Task.Delay(10);
        }
    }
}
