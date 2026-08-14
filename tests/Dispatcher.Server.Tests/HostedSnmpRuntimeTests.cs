using System.Net.Http.Json;
using Dispatcher.Contracts.Devices;
using Dispatcher.Contracts.Tags;
using Dispatcher.Server.Configuration;
using Dispatcher.Tests.Shared;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class HostedSnmpRuntimeTests
{
    [TestMethod]
    public async Task PersistedSnmpDevice_PollsOid_AndPublishesRuntimeState()
    {
        using var agent =
            new SnmpV2cTestAgent(
                expectedCommunity:
                    "public",
                expectedOid:
                    "1.3.6.1.2.1.1.5.0");

        var serverTask =
            agent.ServeOctetStringOnceAsync(
                "dispatcher-switch");

        using var database =
            await TestConfigurationDatabase.CreateAsync(
                Array.Empty<ModbusDeviceConfiguration>(),
                [
                    TestSnmpConfiguration.CreateDevice(
                        agent.Port)
                ]);

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        await serverTask.WaitAsync(
            TimeSpan.FromSeconds(2));

        await WaitUntilAsync(
            async () =>
            {
                var tags =
                    await client.GetFromJsonAsync<TagValueDto[]>(
                        "/api/tags");

                var devices =
                    await client.GetFromJsonAsync<DeviceStateDto[]>(
                        "/api/devices");

                return tags is
                    [
                        {
                            TagId: "snmp01.sysName"
                        }
                    ]
                    && devices is
                    [
                        {
                            DeviceId: "snmp01",
                            Status:
                                DeviceConnectionStatusDto.Online
                        }
                    ];
            },
            TimeSpan.FromSeconds(2));

        var finalTags =
            await client.GetFromJsonAsync<TagValueDto[]>(
                "/api/tags");

        Assert.IsNotNull(
            finalTags);
        Assert.AreEqual(
            1,
            finalTags.Length);
        Assert.AreEqual(
            "dispatcher-switch",
            finalTags[0].Value?.ToString());
        Assert.IsFalse(
            finalTags[0].Writable);
    }

    private static async Task WaitUntilAsync(
        Func<Task<bool>> condition,
        TimeSpan timeout)
    {
        var deadline =
            DateTimeOffset.UtcNow
            + timeout;

        while (!await condition())
        {
            if (DateTimeOffset.UtcNow
                >= deadline)
            {
                Assert.Fail(
                    "Hosted SNMP runtime did not publish the expected state before timeout.");
            }

            await Task.Delay(
                10);
        }
    }
}
