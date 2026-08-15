using System.Globalization;
using System.Net;
using System.Net.Http.Json;
using Dispatcher.Contracts.Historian;
using Dispatcher.Server.Historian;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class HistoryQueryApiTests
{
    [TestMethod]
    public async Task Query_MultipleTags_ReturnsSeriesInRequestOrderAndLosslessValues()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var store =
            await CreateOperationalStoreAsync(
                database.DatabasePath);

        var start =
            new DateTimeOffset(
                2026,
                8,
                15,
                10,
                0,
                0,
                TimeSpan.Zero);

        await store.AppendAsync(
            [
                Sample(
                    "tag.a",
                    start,
                    HistoryValueType.Boolean,
                    "1"),
                Sample(
                    "tag.b",
                    start.AddSeconds(1),
                    HistoryValueType.String,
                    "alpha"),
                Sample(
                    "tag.a",
                    start.AddSeconds(2),
                    HistoryValueType.UInt64,
                    ulong.MaxValue.ToString(
                        CultureInfo.InvariantCulture)),
                Sample(
                    "tag.b",
                    start.AddSeconds(3),
                    HistoryValueType.Json,
                    """{"x":1}""")
            ]);

        var response =
            await client.GetFromJsonAsync<HistoryQueryResponseDto>(
                BuildUrl(
                    ["tag.b", "tag.a"],
                    start,
                    start.AddSeconds(3),
                    order: "asc",
                    limit: 100));

        Assert.IsNotNull(
            response);

        Assert.AreEqual(
            start,
            response.From);
        Assert.AreEqual(
            start.AddSeconds(3),
            response.To);
        Assert.AreEqual(
            HistoryQueryOrderDto.Ascending,
            response.Order);
        Assert.AreEqual(
            100,
            response.Limit);
        Assert.AreEqual(
            2,
            response.Series.Count);

        var tagB =
            response.Series[0];

        Assert.AreEqual(
            "tag.b",
            tagB.TagId);
        Assert.IsFalse(
            tagB.Truncated);
        Assert.AreEqual(
            2,
            tagB.Samples.Count);
        Assert.AreEqual(
            HistoryValueTypeDto.String,
            tagB.Samples[0].ValueType);
        Assert.AreEqual(
            "alpha",
            tagB.Samples[0].ValueText);
        Assert.AreEqual(
            HistoryValueTypeDto.Json,
            tagB.Samples[1].ValueType);
        Assert.AreEqual(
            """{"x":1}""",
            tagB.Samples[1].ValueText);

        var tagA =
            response.Series[1];

        Assert.AreEqual(
            "tag.a",
            tagA.TagId);
        Assert.AreEqual(
            HistoryValueTypeDto.Boolean,
            tagA.Samples[0].ValueType);
        Assert.AreEqual(
            "1",
            tagA.Samples[0].ValueText);
        Assert.AreEqual(
            HistoryValueTypeDto.UInt64,
            tagA.Samples[1].ValueType);
        Assert.AreEqual(
            ulong.MaxValue.ToString(
                CultureInfo.InvariantCulture),
            tagA.Samples[1].ValueText);
    }

    [TestMethod]
    public async Task Query_LimitIsPerSeries_AndReportsTruncated()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var store =
            await CreateOperationalStoreAsync(
                database.DatabasePath);

        var start =
            new DateTimeOffset(
                2026,
                8,
                15,
                11,
                0,
                0,
                TimeSpan.Zero);

        await store.AppendAsync(
            [
                StringSample("tag.a", start, "a1"),
                StringSample("tag.a", start.AddSeconds(1), "a2"),
                StringSample("tag.a", start.AddSeconds(2), "a3"),
                StringSample("tag.b", start, "b1")
            ]);

        var response =
            await client.GetFromJsonAsync<HistoryQueryResponseDto>(
                BuildUrl(
                    ["tag.a", "tag.b"],
                    start,
                    start.AddMinutes(1),
                    order: "asc",
                    limit: 2));

        Assert.IsNotNull(
            response);
        Assert.AreEqual(
            2,
            response.Limit);

        var tagA =
            response.Series[0];
        var tagB =
            response.Series[1];

        Assert.IsTrue(
            tagA.Truncated);
        Assert.AreEqual(
            2,
            tagA.Samples.Count);
        Assert.AreEqual(
            "a1",
            tagA.Samples[0].ValueText);
        Assert.AreEqual(
            "a2",
            tagA.Samples[1].ValueText);

        Assert.IsFalse(
            tagB.Truncated);
        Assert.AreEqual(
            1,
            tagB.Samples.Count);
    }

    [TestMethod]
    public async Task Query_Descending_UsesSampleIdAsStableTieBreaker()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var store =
            await CreateOperationalStoreAsync(
                database.DatabasePath);

        var timestamp =
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
                StringSample(
                    "tag.same-time",
                    timestamp.AddSeconds(-1),
                    "older"),
                StringSample(
                    "tag.same-time",
                    timestamp,
                    "first"),
                StringSample(
                    "tag.same-time",
                    timestamp,
                    "second")
            ]);

        var response =
            await client.GetFromJsonAsync<HistoryQueryResponseDto>(
                BuildUrl(
                    ["tag.same-time"],
                    timestamp.AddSeconds(-1),
                    timestamp,
                    order: "desc",
                    limit: 10));

        Assert.IsNotNull(
            response);
        Assert.AreEqual(
            HistoryQueryOrderDto.Descending,
            response.Order);

        CollectionAssert.AreEqual(
            new[]
            {
                "second",
                "first",
                "older"
            },
            response.Series[0]
                .Samples
                .Select(sample => sample.ValueText)
                .ToArray());
    }

    [TestMethod]
    public async Task Query_TimeBoundsAreInclusive()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var store =
            await CreateOperationalStoreAsync(
                database.DatabasePath);

        var from =
            new DateTimeOffset(
                2026,
                8,
                15,
                13,
                0,
                0,
                TimeSpan.Zero);

        var to =
            from.AddSeconds(10);

        await store.AppendAsync(
            [
                StringSample(
                    "tag.bounds",
                    from.AddTicks(-1),
                    "before"),
                StringSample(
                    "tag.bounds",
                    from,
                    "from"),
                StringSample(
                    "tag.bounds",
                    to,
                    "to"),
                StringSample(
                    "tag.bounds",
                    to.AddTicks(1),
                    "after")
            ]);

        var response =
            await client.GetFromJsonAsync<HistoryQueryResponseDto>(
                BuildUrl(
                    ["tag.bounds"],
                    from,
                    to,
                    order: "asc",
                    limit: 10));

        Assert.IsNotNull(
            response);

        CollectionAssert.AreEqual(
            new[]
            {
                "from",
                "to"
            },
            response.Series[0]
                .Samples
                .Select(sample => sample.ValueText)
                .ToArray());
    }

    [TestMethod]
    public async Task Query_DoesNotRequireCurrentConfigurationOrPolicy()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var store =
            await CreateOperationalStoreAsync(
                database.DatabasePath);

        var timestamp =
            new DateTimeOffset(
                2026,
                8,
                15,
                14,
                0,
                0,
                TimeSpan.Zero);

        await store.AppendAsync(
            [
                StringSample(
                    "deleted.tag",
                    timestamp,
                    "historical")
            ]);

        var response =
            await client.GetFromJsonAsync<HistoryQueryResponseDto>(
                BuildUrl(
                    ["deleted.tag", "never.existed"],
                    timestamp.AddMinutes(-1),
                    timestamp.AddMinutes(1),
                    order: "asc",
                    limit: 100));

        Assert.IsNotNull(
            response);
        Assert.AreEqual(
            2,
            response.Series.Count);

        Assert.AreEqual(
            "deleted.tag",
            response.Series[0].TagId);
        Assert.AreEqual(
            "historical",
            response.Series[0].Samples.Single().ValueText);

        Assert.AreEqual(
            "never.existed",
            response.Series[1].TagId);
        Assert.AreEqual(
            0,
            response.Series[1].Samples.Count);
        Assert.IsFalse(
            response.Series[1].Truncated);
    }

    [TestMethod]
    public async Task Query_InvalidScalarParameters_ReturnBadRequest()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var from =
            new DateTimeOffset(
                2026,
                8,
                15,
                15,
                0,
                0,
                TimeSpan.Zero);

        var to =
            from.AddMinutes(1);

        var validUrl =
            BuildUrl(
                ["tag.a"],
                from,
                to,
                "asc",
                10);

        var urls =
            new[]
            {
                "/api/history?tagId=tag.a",
                BuildUrl(
                    ["tag.a"],
                    to,
                    from,
                    "asc",
                    10),
                BuildUrl(
                    ["tag.a"],
                    from,
                    to,
                    "invalid",
                    10),
                BuildUrl(
                    ["tag.a"],
                    from,
                    to,
                    "asc",
                    HistoryQueryService.MaxLimit + 1),
                $"{validUrl}&limit=11"
            };

        foreach (var url in urls)
        {
            using var response =
                await client.GetAsync(
                    url);

            Assert.AreEqual(
                HttpStatusCode.BadRequest,
                response.StatusCode,
                url);
        }
    }

    [TestMethod]
    public async Task Query_DuplicateOrTooManyTags_ReturnBadRequest()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);

        using var client =
            factory.CreateClient();

        var from =
            new DateTimeOffset(
                2026,
                8,
                15,
                16,
                0,
                0,
                TimeSpan.Zero);

        var to =
            from.AddMinutes(1);

        var duplicate =
            BuildUrl(
                ["tag.a", "tag.a"],
                from,
                to,
                "asc",
                10);

        var tooMany =
            BuildUrl(
                Enumerable.Range(
                        0,
                        HistoryQueryService.MaxTagCount + 1)
                    .Select(index =>
                        $"tag.{index}")
                    .ToArray(),
                from,
                to,
                "asc",
                10);

        foreach (var url in new[]
        {
            duplicate,
            tooMany
        })
        {
            using var response =
                await client.GetAsync(
                    url);

            Assert.AreEqual(
                HttpStatusCode.BadRequest,
                response.StatusCode,
                url);
        }
    }

    private static async Task<SqliteOperationalStore> CreateOperationalStoreAsync(
        string configurationDatabasePath)
    {
        var store =
            new SqliteOperationalStore(
                TestDispatcherFactory.GetOperationalDatabasePath(
                    configurationDatabasePath));

        await store.InitializeAsync();

        return store;
    }

    private static HistorySample StringSample(
        string tagId,
        DateTimeOffset timestamp,
        string value)
    {
        return Sample(
            tagId,
            timestamp,
            HistoryValueType.String,
            value);
    }

    private static HistorySample Sample(
        string tagId,
        DateTimeOffset timestamp,
        HistoryValueType valueType,
        string? valueText)
    {
        return new HistorySample(
            0,
            tagId,
            timestamp,
            valueType,
            valueText);
    }

    private static string BuildUrl(
        IReadOnlyList<string> tagIds,
        DateTimeOffset from,
        DateTimeOffset to,
        string order,
        int limit)
    {
        var query =
            string.Join(
                "&",
                tagIds.Select(tagId =>
                    $"tagId={Uri.EscapeDataString(tagId)}"));

        return $"/api/history?{query}" +
               $"&from={Uri.EscapeDataString(from.ToString("O", CultureInfo.InvariantCulture))}" +
               $"&to={Uri.EscapeDataString(to.ToString("O", CultureInfo.InvariantCulture))}" +
               $"&order={Uri.EscapeDataString(order)}" +
               $"&limit={limit.ToString(CultureInfo.InvariantCulture)}";
    }
}
