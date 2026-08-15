using Dispatcher.Core.Tags;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Historian;
using Microsoft.Extensions.Logging.Abstractions;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class HistorianServiceTests
{
    [TestMethod]
    public async Task OnChangePolicy_TagChange_IsPersistedAsynchronously()
    {
        var directory = CreateTempDirectory();

        try
        {
            var store = new SqliteOperationalStore(
                Path.Combine(directory, "operational.db"));
            var tags = new TagService();
            var configuration = CreateConfiguration(
                "plc01.temperature");
            var policies = CreatePolicies(
                new HistorianPolicyConfiguration(
                    "plc01.temperature",
                    true,
                    HistorianSamplingMode.OnChange,
                    null,
                    30));

            var service = new HistorianService(
                tags,
                store,
                configuration,
                policies,
                new HistorianOptions(32, 8),
                NullLogger<HistorianService>.Instance);

            await service.StartAsync(CancellationToken.None);

            var timestamp = new DateTimeOffset(
                2026, 8, 15, 15, 30, 0, TimeSpan.FromHours(3));

            tags.Set(
                "plc01.temperature",
                (ushort)321,
                timestamp);

            await WaitUntilAsync(
                async () => (await store.LoadAllAsync()).Count == 1,
                TimeSpan.FromSeconds(2));

            await service.StopAsync(CancellationToken.None);

            var samples = await store.LoadAllAsync();

            Assert.AreEqual(1, samples.Count);
            Assert.AreEqual("plc01.temperature", samples[0].TagId);
            Assert.AreEqual(HistoryValueType.Int64, samples[0].ValueType);
            Assert.AreEqual("321", samples[0].ValueText);
            Assert.AreEqual(timestamp.ToUniversalTime(), samples[0].Timestamp);
        }
        finally
        {
            Directory.Delete(directory, recursive: true);
        }
    }

    [TestMethod]
    public async Task NoEnabledPolicy_TagChange_IsNotPersisted()
    {
        var directory = CreateTempDirectory();

        try
        {
            var store = new SqliteOperationalStore(
                Path.Combine(directory, "operational.db"));
            var tags = new TagService();
            var configuration = CreateConfiguration(
                "plc01.temperature");
            var policies = CreatePolicies(
                new HistorianPolicyConfiguration(
                    "plc01.temperature",
                    false,
                    HistorianSamplingMode.OnChange,
                    null,
                    30));

            var service = new HistorianService(
                tags,
                store,
                configuration,
                policies,
                new HistorianOptions(32, 8),
                NullLogger<HistorianService>.Instance);

            await service.StartAsync(CancellationToken.None);

            tags.Set(
                "plc01.temperature",
                123);

            await Task.Delay(100);

            await service.StopAsync(CancellationToken.None);

            Assert.AreEqual(
                0,
                (await store.LoadAllAsync()).Count);
        }
        finally
        {
            Directory.Delete(directory, recursive: true);
        }
    }

    [TestMethod]
    public async Task PeriodicPolicy_PersistsCurrentValueWithoutNewChanges()
    {
        var directory = CreateTempDirectory();

        try
        {
            var store = new SqliteOperationalStore(
                Path.Combine(directory, "operational.db"));
            var tags = new TagService();
            var configuration = CreateConfiguration(
                "plc01.temperature");
            var policies = CreatePolicies(
                new HistorianPolicyConfiguration(
                    "plc01.temperature",
                    true,
                    HistorianSamplingMode.Periodic,
                    100,
                    30));

            tags.Set(
                "plc01.temperature",
                456);

            var service = new HistorianService(
                tags,
                store,
                configuration,
                policies,
                new HistorianOptions(
                    32,
                    8,
                    PeriodicScanMilliseconds: 10),
                NullLogger<HistorianService>.Instance);

            await service.StartAsync(CancellationToken.None);

            await WaitUntilAsync(
                async () => (await store.LoadAllAsync()).Count >= 2,
                TimeSpan.FromSeconds(2));

            await service.StopAsync(CancellationToken.None);

            var samples = await store.LoadAllAsync();

            Assert.IsTrue(samples.Count >= 2);
            Assert.IsTrue(samples.All(sample =>
                sample.TagId == "plc01.temperature"
                && sample.ValueText == "456"));
            Assert.IsTrue(
                samples[1].Timestamp > samples[0].Timestamp);
        }
        finally
        {
            Directory.Delete(directory, recursive: true);
        }
    }

    [TestMethod]
    public async Task FullBuffer_DropsSamplesWithoutBlockingTagService()
    {
        var tags = new TagService();
        var store = new BlockingHistoryStore();
        var configuration = CreateConfiguration(
            "tag.1",
            "tag.2",
            "tag.3");
        var policies = CreatePolicies(
            "tag.1",
            "tag.2",
            "tag.3");

        var service = new HistorianService(
            tags,
            store,
            configuration,
            policies,
            new HistorianOptions(1, 1),
            NullLogger<HistorianService>.Instance);

        await service.StartAsync(CancellationToken.None);

        tags.Set("tag.1", 1);

        await store.AppendStarted.Task.WaitAsync(
            TimeSpan.FromSeconds(2));

        tags.Set("tag.2", 2);
        tags.Set("tag.3", 3);

        Assert.IsTrue(
            service.DroppedSampleCount >= 1);

        store.Release.TrySetResult(true);

        await service.StopAsync(CancellationToken.None);
    }

    private static ConfigurationCatalog CreateConfiguration(
        params string[] tagIds)
    {
        var configuration = new ConfigurationCatalog();

        configuration.ReplaceAll(
            [
                new ModbusDeviceConfiguration(
                    "historian-test-device",
                    "Historian test device",
                    false,
                    "127.0.0.1",
                    502,
                    1,
                    1000,
                    1000,
                    tagIds.Select((tagId, index) =>
                        new ModbusTagConfiguration(
                            tagId,
                            tagId,
                            (ushort)index,
                            false))
                        .ToArray())
            ],
            Array.Empty<SnmpDeviceConfiguration>());

        return configuration;
    }

    private static HistorianPolicyCatalog CreatePolicies(
        params HistorianPolicyConfiguration[] policies)
    {
        var catalog = new HistorianPolicyCatalog();
        catalog.ReplaceAll(
            policies);
        return catalog;
    }

    private static HistorianPolicyCatalog CreatePolicies(
        params string[] tagIds)
    {
        return CreatePolicies(
            tagIds.Select(tagId =>
                new HistorianPolicyConfiguration(
                    tagId,
                    true,
                    HistorianSamplingMode.OnChange,
                    null,
                    30))
                .ToArray());
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
                Assert.Fail("Condition was not reached before timeout.");
            }

            await Task.Delay(10);
        }
    }

    private static string CreateTempDirectory()
    {
        var directory = Path.Combine(
            Path.GetTempPath(),
            "dispatcher-historian-tests",
            Guid.NewGuid().ToString("N"));

        Directory.CreateDirectory(directory);
        return directory;
    }

    private sealed class BlockingHistoryStore : IHistorySampleStore
    {
        public TaskCompletionSource<bool> AppendStarted { get; } =
            new(TaskCreationOptions.RunContinuationsAsynchronously);

        public TaskCompletionSource<bool> Release { get; } =
            new(TaskCreationOptions.RunContinuationsAsynchronously);

        public Task InitializeAsync(
            CancellationToken cancellationToken = default)
        {
            return Task.CompletedTask;
        }

        public async Task AppendAsync(
            IReadOnlyList<HistorySample> samples,
            CancellationToken cancellationToken = default)
        {
            AppendStarted.TrySetResult(true);

            await Release.Task.WaitAsync(
                cancellationToken);
        }

        public Task<IReadOnlyList<HistorySample>> QueryAsync(
            string tagId,
            DateTimeOffset from,
            DateTimeOffset to,
            bool ascending,
            int limit,
            CancellationToken cancellationToken = default)
        {
            return Task.FromResult<IReadOnlyList<HistorySample>>(
                Array.Empty<HistorySample>());
        }

        public Task<int> DeleteBeforeAsync(
            string tagId,
            DateTimeOffset cutoff,
            CancellationToken cancellationToken = default)
        {
            return Task.FromResult(0);
        }
    }
}
