using System.Net;
using System.Net.Http.Json;
using Dispatcher.Contracts.Configuration;
using Dispatcher.Contracts.Tags;
using Dispatcher.Server.Configuration;
using Dispatcher.Tests.Shared;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class SnmpConfigurationApiTests
{
    [TestMethod]
    public async Task SnmpConfigurationCrud_PersistsDeviceAndTagChanges()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var createDevice =
            new SnmpDeviceUpsertRequest(
                DeviceId: "switch01",
                Name: "Switch 01",
                Enabled: false,
                Host: "192.168.1.20",
                Port: 161,
                Community: "public",
                PollIntervalMilliseconds: 5000,
                RequestTimeoutMilliseconds: 1500);

        var createDeviceResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/snmp/devices",
                createDevice);

        Assert.AreEqual(
            HttpStatusCode.Created,
            createDeviceResponse.StatusCode);

        var createTag =
            new SnmpTagUpsertRequest(
                TagId: "switch01.sysName",
                Name: "sysName",
                Oid: "1.3.6.1.2.1.1.5.0");

        var createTagResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/snmp/devices/switch01/tags",
                createTag);

        Assert.AreEqual(
            HttpStatusCode.Created,
            createTagResponse.StatusCode);

        var updateDevice =
            createDevice with
            {
                Name = "Main switch",
                Port = 1161,
                Community = "monitoring"
            };

        var updateDeviceResponse =
            await client.PutAsJsonAsync(
                "/api/configuration/snmp/devices/switch01",
                updateDevice);

        Assert.AreEqual(
            HttpStatusCode.OK,
            updateDeviceResponse.StatusCode);

        var updateTag =
            createTag with
            {
                Name = "System description",
                Oid = "1.3.6.1.2.1.1.1.0"
            };

        var updateTagResponse =
            await client.PutAsJsonAsync(
                "/api/configuration/snmp/devices/switch01/tags/switch01.sysName",
                updateTag);

        Assert.AreEqual(
            HttpStatusCode.OK,
            updateTagResponse.StatusCode);

        var devices =
            await client.GetFromJsonAsync<
                SnmpDeviceConfigurationDto[]>(
                "/api/configuration/snmp/devices");

        Assert.IsNotNull(devices);
        Assert.AreEqual(
            1,
            devices.Length);
        Assert.AreEqual(
            "Main switch",
            devices[0].Name);
        Assert.AreEqual(
            1161,
            devices[0].Port);
        Assert.AreEqual(
            "monitoring",
            devices[0].Community);
        Assert.AreEqual(
            1,
            devices[0].Tags.Count);
        Assert.AreEqual(
            "1.3.6.1.2.1.1.1.0",
            devices[0].Tags[0].Oid);

        var reopenedStore =
            new SqliteConfigurationStore(
                database.DatabasePath);

        await reopenedStore.InitializeAsync();

        var persisted =
            await reopenedStore.LoadSnmpAsync();

        Assert.AreEqual(
            1,
            persisted.Count);
        Assert.AreEqual(
            "Main switch",
            persisted[0].Name);
        Assert.AreEqual(
            "monitoring",
            persisted[0].Community);
        Assert.AreEqual(
            "1.3.6.1.2.1.1.1.0",
            persisted[0].Tags.Single().Oid);

        var deleteTagResponse =
            await client.DeleteAsync(
                "/api/configuration/snmp/devices/switch01/tags/switch01.sysName");

        Assert.AreEqual(
            HttpStatusCode.NoContent,
            deleteTagResponse.StatusCode);

        var deleteDeviceResponse =
            await client.DeleteAsync(
                "/api/configuration/snmp/devices/switch01");

        Assert.AreEqual(
            HttpStatusCode.NoContent,
            deleteDeviceResponse.StatusCode);

        var finalPersisted =
            await reopenedStore.LoadSnmpAsync();

        Assert.AreEqual(
            0,
            finalPersisted.Count);
    }

    [TestMethod]
    public async Task SnmpConfigurationCrud_InvalidOid_ReturnsBadRequest()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var device =
            new SnmpDeviceUpsertRequest(
                "snmp01",
                "SNMP 01",
                false,
                "127.0.0.1",
                161,
                "public",
                1000,
                1000);

        var deviceResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/snmp/devices",
                device);

        Assert.AreEqual(
            HttpStatusCode.Created,
            deviceResponse.StatusCode);

        var invalidTag =
            new SnmpTagUpsertRequest(
                "snmp01.invalid",
                "Invalid",
                "this-is-not-an-oid");

        var response =
            await client.PostAsJsonAsync(
                "/api/configuration/snmp/devices/snmp01/tags",
                invalidTag);

        Assert.AreEqual(
            HttpStatusCode.BadRequest,
            response.StatusCode);
    }

    [TestMethod]
    public async Task SnmpConfigurationCrud_IdsOwnedByModbus_ReturnConflict()
    {
        var modbus =
            TestModbusConfiguration.CreateDevice(
                port: 502,
                enabled: false);

        using var database =
            await TestConfigurationDatabase.CreateAsync(
                [modbus],
                Array.Empty<SnmpDeviceConfiguration>());

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var duplicateDevice =
            new SnmpDeviceUpsertRequest(
                "device01",
                "Duplicate SNMP",
                false,
                "127.0.0.1",
                161,
                "public",
                1000,
                1000);

        var duplicateDeviceResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/snmp/devices",
                duplicateDevice);

        Assert.AreEqual(
            HttpStatusCode.Conflict,
            duplicateDeviceResponse.StatusCode);

        var validDevice =
            duplicateDevice with
            {
                DeviceId = "snmp01"
            };

        var validDeviceResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/snmp/devices",
                validDevice);

        Assert.AreEqual(
            HttpStatusCode.Created,
            validDeviceResponse.StatusCode);

        var duplicateTag =
            new SnmpTagUpsertRequest(
                "device01.register100",
                "Duplicate tag",
                "1.3.6.1.2.1.1.5.0");

        var duplicateTagResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/snmp/devices/snmp01/tags",
                duplicateTag);

        Assert.AreEqual(
            HttpStatusCode.Conflict,
            duplicateTagResponse.StatusCode);
    }

    [TestMethod]
    public async Task CreateSnmpTag_OnRunningServer_AppliesPollingWithoutRestart()
    {
        using var agent =
            new SnmpV2cTestAgent(
                "public",
                "1.3.6.1.2.1.1.5.0");

        var agentTask =
            agent.ServeOctetStringOnceAsync(
                "dispatcher-switch");

        using var database =
            await TestConfigurationDatabase.CreateAsync();

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var device =
            new SnmpDeviceUpsertRequest(
                DeviceId: "snmp01",
                Name: "SNMP 01",
                Enabled: true,
                Host: "127.0.0.1",
                Port: agent.Port,
                Community: "public",
                PollIntervalMilliseconds: 10000,
                RequestTimeoutMilliseconds: 1000);

        var deviceResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/snmp/devices",
                device);

        Assert.AreEqual(
            HttpStatusCode.Created,
            deviceResponse.StatusCode);

        var tag =
            new SnmpTagUpsertRequest(
                TagId: "snmp01.sysName",
                Name: "sysName",
                Oid: "1.3.6.1.2.1.1.5.0");

        var tagResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/snmp/devices/snmp01/tags",
                tag);

        Assert.AreEqual(
            HttpStatusCode.Created,
            tagResponse.StatusCode);

        await agentTask.WaitAsync(
            TimeSpan.FromSeconds(2));

        await WaitUntilAsync(
            async () =>
            {
                var tags =
                    await client.GetFromJsonAsync<TagValueDto[]>(
                        "/api/tags")
                    ?? [];

                return tags.Any(current =>
                    string.Equals(
                        current.TagId,
                        "snmp01.sysName",
                        StringComparison.Ordinal)
                    && string.Equals(
                        current.Value?.ToString(),
                        "dispatcher-switch",
                        StringComparison.Ordinal)
                    && !current.Writable);
            },
            TimeSpan.FromSeconds(2));
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
                    "Expected SNMP runtime value was not published before timeout.");
            }

            await Task.Delay(
                10);
        }
    }
}
