using Dispatcher.Core.Tags;
using Dispatcher.Server.Historian;
using Microsoft.Extensions.Logging.Abstractions;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class HistorianServiceTests
{
    [TestMethod]
    public async Task TagChange_IsPersistedAsynchronously()
    {
        var directory = CreateTempDirectory();

        try
        {
            var store = new SqliteOperationalStore(
                Path.Combine(directory, "operational.db"));
            var tags = new TagService();
            var service = new HistorianService(
                tags,
                store,
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
    public async Task FullBuffer_DropsSamplesWithoutBlockingTagService()
    {
        var tags = new TagService();
        var store = new BlockingHistoryStore();
        var service = new HistorianService(
            tags,
            store,
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
    }
}
