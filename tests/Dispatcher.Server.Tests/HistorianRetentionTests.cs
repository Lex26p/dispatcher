using Dispatcher.Server.Historian;
using Microsoft.Extensions.Logging.Abstractions;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class HistorianRetentionTests
{
    [TestMethod]
    public async Task CleanupOnce_UsesPerTagRetentionAndKeepsUnconfiguredHistory()
    {
        var directory =
            Path.Combine(
                Path.GetTempPath(),
                "dispatcher-retention-tests",
                Guid.NewGuid().ToString("N"));

        Directory.CreateDirectory(
            directory);

        try
        {
            var store =
                new SqliteOperationalStore(
                    Path.Combine(
                        directory,
                        "operational.db"));

            await store.InitializeAsync();

            var now =
                new DateTimeOffset(
                    2026,
                    8,
                    15,
                    12,
                    0,
                    0,
                    TimeSpan.Zero);

            await store.AppendAsync(
                [
                    CreateSample(
                        "tag.short",
                        now.AddDays(-2),
                        "old-short"),
                    CreateSample(
                        "tag.short",
                        now.AddHours(-12),
                        "new-short"),
                    CreateSample(
                        "tag.long",
                        now.AddDays(-12),
                        "old-long"),
                    CreateSample(
                        "tag.long",
                        now.AddDays(-5),
                        "new-long"),
                    CreateSample(
                        "tag.no-policy",
                        now.AddDays(-100),
                        "must-remain")
                ]);

            var policies =
                new HistorianPolicyCatalog();

            policies.ReplaceAll(
                [
                    new HistorianPolicyConfiguration(
                        "tag.short",
                        Enabled: false,
                        HistorianSamplingMode.OnChange,
                        PeriodMilliseconds: null,
                        RetentionDays: 1),
                    new HistorianPolicyConfiguration(
                        "tag.long",
                        Enabled: true,
                        HistorianSamplingMode.OnChange,
                        PeriodMilliseconds: null,
                        RetentionDays: 10)
                ]);

            var service =
                new HistorianRetentionHostedService(
                    store,
                    policies,
                    new HistorianOptions(
                        32,
                        8),
                    NullLogger<HistorianRetentionHostedService>.Instance);

            var deleted =
                await service.CleanupOnceAsync(
                    now);

            Assert.AreEqual(
                2,
                deleted);
            Assert.AreEqual(
                2L,
                service.DeletedSampleCount);
            Assert.AreEqual(
                1L,
                service.CleanupRunCount);

            var remaining =
                await store.LoadAllAsync();

            Assert.AreEqual(
                3,
                remaining.Count);

            CollectionAssert.AreEquivalent(
                new[]
                {
                    "new-short",
                    "new-long",
                    "must-remain"
                },
                remaining
                    .Select(sample => sample.ValueText)
                    .ToArray());
        }
        finally
        {
            Directory.Delete(
                directory,
                recursive: true);
        }
    }

    private static HistorySample CreateSample(
        string tagId,
        DateTimeOffset timestamp,
        string value)
    {
        return new HistorySample(
            0,
            tagId,
            timestamp,
            HistoryValueType.String,
            value);
    }
}
