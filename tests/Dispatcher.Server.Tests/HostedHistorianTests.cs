using Dispatcher.Core.Tags;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Historian;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class HostedHistorianTests
{
    [TestMethod]
    public async Task ServerStartup_LoadsPolicyBeforeRuntimeTagChanges()
    {
        var device =
            TestModbusConfiguration.CreateDevice(
                port: 502,
                enabled: false);

        using var database =
            await TestConfigurationDatabase.CreateAsync(
                device);

        var configurationStore =
            new SqliteConfigurationStore(
                database.DatabasePath);

        await configurationStore.UpsertHistorianPolicyAsync(
            new HistorianPolicyConfiguration(
                "device01.register100",
                true,
                HistorianSamplingMode.OnChange,
                null,
                30));

        var operationalPath =
            TestDispatcherFactory.GetOperationalDatabasePath(
                database.DatabasePath);

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var tags =
            factory.Services.GetRequiredService<TagService>();

        var timestamp =
            new DateTimeOffset(
                2026, 8, 15, 16, 0, 0, TimeSpan.Zero);

        tags.Set(
            "device01.register100",
            "online",
            timestamp);

        var store =
            new SqliteOperationalStore(
                operationalPath);

        await store.InitializeAsync();

        await WaitUntilAsync(
            async () =>
            {
                var samples =
                    await store.LoadAllAsync();

                return samples.Any(sample =>
                    sample.TagId == "device01.register100");
            },
            TimeSpan.FromSeconds(2));

        var persisted =
            (await store.LoadAllAsync())
                .Single(sample =>
                    sample.TagId == "device01.register100");

        Assert.AreEqual(
            HistoryValueType.String,
            persisted.ValueType);
        Assert.AreEqual(
            "online",
            persisted.ValueText);
    }

    private static async Task WaitUntilAsync(
        Func<Task<bool>> condition,
        TimeSpan timeout)
    {
        var deadline = DateTimeOffset.UtcNow + timeout;

        while (!await condition())
        {
            if (DateTimeOffset.UtcNow >= deadline)
            {
                Assert.Fail(
                    "Historian sample was not persisted before timeout.");
            }

            await Task.Delay(10);
        }
    }
}
