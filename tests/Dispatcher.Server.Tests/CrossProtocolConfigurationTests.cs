using System.Net;
using System.Net.Http.Json;
using Dispatcher.Contracts.Configuration;
using Dispatcher.Server.Configuration;
using Dispatcher.Tests.Shared;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class CrossProtocolConfigurationTests
{
    [TestMethod]
    public async Task ModbusMutation_PreservesSnmpConfiguration_AndRestartsSnmpPolling()
    {
        using var agent =
            new SnmpV2cTestAgent(
                "public",
                "1.3.6.1.2.1.1.5.0");

        var agentTask =
            agent.ServeOctetStringAsync(
                "dispatcher-switch",
                count: 2);

        var snmpDevice =
            TestSnmpConfiguration.CreateDevice(
                agent.Port);

        using var database =
            await TestConfigurationDatabase.CreateAsync(
                Array.Empty<ModbusDeviceConfiguration>(),
                [snmpDevice]);

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        await WaitForTagAsync(
            client,
            "snmp01.sysName",
            TimeSpan.FromSeconds(2));

        var modbusDevice =
            new ModbusDeviceUpsertRequest(
                DeviceId: "modbus01",
                Name: "Disabled Modbus",
                Enabled: false,
                Host: IPAddress.Loopback.ToString(),
                Port: 502,
                UnitId: 1,
                PollIntervalMilliseconds: 1000,
                RequestTimeoutMilliseconds: 1000);

        var response =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices",
                modbusDevice);

        Assert.AreEqual(
            HttpStatusCode.Created,
            response.StatusCode);

        await agentTask.WaitAsync(
            TimeSpan.FromSeconds(2));

        var store =
            new SqliteConfigurationStore(
                database.DatabasePath);

        await store.InitializeAsync();

        var persistedSnmp =
            await store.LoadSnmpAsync();

        Assert.AreEqual(
            1,
            persistedSnmp.Count);
        Assert.AreEqual(
            "snmp01",
            persistedSnmp[0].DeviceId);

        await WaitForTagAsync(
            client,
            "snmp01.sysName",
            TimeSpan.FromSeconds(2));
    }

    [TestMethod]
    public async Task ModbusApi_RejectsIdsAlreadyOwnedBySnmp()
    {
        var snmpDevice =
            new SnmpDeviceConfiguration(
                DeviceId: "shared-device",
                Name: "SNMP",
                Enabled: false,
                Host: "127.0.0.1",
                Port: 161,
                Community: "public",
                PollIntervalMilliseconds: 1000,
                RequestTimeoutMilliseconds: 1000,
                Tags:
                [
                    new SnmpTagConfiguration(
                        TagId: "shared.tag",
                        Name: "Shared",
                        Oid: "1.3.6.1.2.1.1.5.0")
                ]);

        using var database =
            await TestConfigurationDatabase.CreateAsync(
                Array.Empty<ModbusDeviceConfiguration>(),
                [snmpDevice]);

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var duplicateDevice =
            new ModbusDeviceUpsertRequest(
                DeviceId: "shared-device",
                Name: "Modbus duplicate",
                Enabled: false,
                Host: "127.0.0.1",
                Port: 502,
                UnitId: 1,
                PollIntervalMilliseconds: 1000,
                RequestTimeoutMilliseconds: 1000);

        var deviceResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices",
                duplicateDevice);

        Assert.AreEqual(
            HttpStatusCode.Conflict,
            deviceResponse.StatusCode);

        var validDevice =
            duplicateDevice with
            {
                DeviceId = "modbus01"
            };

        var validResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices",
                validDevice);

        Assert.AreEqual(
            HttpStatusCode.Created,
            validResponse.StatusCode);

        var duplicateTag =
            new ModbusTagUpsertRequest(
                TagId: "shared.tag",
                Name: "Duplicate",
                Address: 0,
                Writable: false);

        var tagResponse =
            await client.PostAsJsonAsync(
                "/api/configuration/modbus/devices/modbus01/tags",
                duplicateTag);

        Assert.AreEqual(
            HttpStatusCode.Conflict,
            tagResponse.StatusCode);
    }

    private static async Task WaitForTagAsync(
        HttpClient client,
        string tagId,
        TimeSpan timeout)
    {
        var deadline =
            DateTimeOffset.UtcNow + timeout;

        while (true)
        {
            var tags =
                await client.GetFromJsonAsync<
                    Dispatcher.Contracts.Tags.TagValueDto[]>(
                    "/api/tags")
                ?? [];

            if (tags.Any(tag =>
                    string.Equals(
                        tag.TagId,
                        tagId,
                        StringComparison.Ordinal)))
            {
                return;
            }

            if (DateTimeOffset.UtcNow
                >= deadline)
            {
                Assert.Fail(
                    $"Tag '{tagId}' was not published before timeout.");
            }

            await Task.Delay(10);
        }
    }
}
