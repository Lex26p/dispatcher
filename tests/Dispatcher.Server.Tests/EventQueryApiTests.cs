using System.Globalization;
using System.Net;
using System.Net.Http.Json;
using Dispatcher.Contracts.Events;
using Dispatcher.Server.Events;
using Dispatcher.Server.Historian;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class EventQueryApiTests
{
    [TestMethod]
    public async Task Query_FiltersCategorySeveritySourceAndText_WithStableNewestFirstOrder()
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
                1,
                10,
                12,
                0,
                0,
                TimeSpan.Zero);

        var persisted =
            await store.AppendEventsAsync(
                [
                    Event(
                        timestamp,
                        EventCategory.Device,
                        EventSeverity.Warning,
                        "device01",
                        "DeviceOffline",
                        "Offline pump."),
                    Event(
                        timestamp,
                        EventCategory.Device,
                        EventSeverity.Warning,
                        "device01",
                        "DeviceOffline",
                        "Offline valve."),
                    Event(
                        timestamp.AddSeconds(1),
                        EventCategory.Command,
                        EventSeverity.Error,
                        "tag01",
                        "TagWriteFailed",
                        "Offline text but another category.")
                ]);

        var response =
            await client.GetFromJsonAsync<EventQueryResponseDto>(
                BuildUrl(
                    timestamp.AddMinutes(-1),
                    timestamp.AddMinutes(1),
                    category:
                        "Device",
                    severity:
                        "Warning",
                    source:
                        "device01",
                    text:
                        "Offline",
                    page:
                        1,
                    limit:
                        10));

        Assert.IsNotNull(
            response);
        Assert.AreEqual(
            1,
            response.Page);
        Assert.AreEqual(
            10,
            response.Limit);
        Assert.IsFalse(
            response.HasMore);
        Assert.AreEqual(
            2,
            response.Items.Count);

        Assert.AreEqual(
            persisted[1].EventId,
            response.Items[0].EventId);
        Assert.AreEqual(
            persisted[0].EventId,
            response.Items[1].EventId);

        Assert.IsTrue(
            response.Items.All(item =>
                item.Category == EventCategoryDto.Device
                && item.Severity == EventSeverityDto.Warning
                && item.Source == "device01"));
    }

    [TestMethod]
    public async Task Query_PageAndLimit_ReturnHasMoreWithoutCount()
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
                1,
                11,
                10,
                0,
                0,
                TimeSpan.Zero);

        await store.AppendEventsAsync(
            Enumerable.Range(
                    0,
                    5)
                .Select(index =>
                    Event(
                        start.AddSeconds(index),
                        EventCategory.System,
                        EventSeverity.Information,
                        "server",
                        $"Event{index}",
                        $"Event {index}."))
                .ToArray());

        var page1 =
            await client.GetFromJsonAsync<EventQueryResponseDto>(
                BuildUrl(
                    start.AddMinutes(-1),
                    start.AddMinutes(1),
                    page:
                        1,
                    limit:
                        2));

        var page2 =
            await client.GetFromJsonAsync<EventQueryResponseDto>(
                BuildUrl(
                    start.AddMinutes(-1),
                    start.AddMinutes(1),
                    page:
                        2,
                    limit:
                        2));

        var page3 =
            await client.GetFromJsonAsync<EventQueryResponseDto>(
                BuildUrl(
                    start.AddMinutes(-1),
                    start.AddMinutes(1),
                    page:
                        3,
                    limit:
                        2));

        Assert.IsNotNull(
            page1);
        Assert.IsNotNull(
            page2);
        Assert.IsNotNull(
            page3);

        Assert.AreEqual(
            2,
            page1.Items.Count);
        Assert.IsTrue(
            page1.HasMore);

        Assert.AreEqual(
            2,
            page2.Items.Count);
        Assert.IsTrue(
            page2.HasMore);

        Assert.AreEqual(
            1,
            page3.Items.Count);
        Assert.IsFalse(
            page3.HasMore);

        var ids =
            page1.Items
                .Concat(
                    page2.Items)
                .Concat(
                    page3.Items)
                .Select(item =>
                    item.EventId)
                .ToArray();

        Assert.AreEqual(
            5,
            ids.Distinct().Count());

        CollectionAssert.AreEqual(
            ids
                .OrderByDescending(id =>
                    id)
                .ToArray(),
            ids);
    }

    [TestMethod]
    public async Task Query_TimeRange_IsInclusive()
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
                1,
                12,
                8,
                0,
                0,
                TimeSpan.Zero);

        var to =
            from.AddSeconds(
                10);

        await store.AppendEventsAsync(
            [
                Event(
                    from.AddTicks(-1),
                    EventCategory.System,
                    EventSeverity.Information,
                    "server",
                    "Before",
                    "Before."),
                Event(
                    from,
                    EventCategory.System,
                    EventSeverity.Information,
                    "server",
                    "From",
                    "From."),
                Event(
                    to,
                    EventCategory.System,
                    EventSeverity.Information,
                    "server",
                    "To",
                    "To."),
                Event(
                    to.AddTicks(1),
                    EventCategory.System,
                    EventSeverity.Information,
                    "server",
                    "After",
                    "After.")
            ]);

        var response =
            await client.GetFromJsonAsync<EventQueryResponseDto>(
                BuildUrl(
                    from,
                    to,
                    page:
                        1,
                    limit:
                        10));

        Assert.IsNotNull(
            response);

        CollectionAssert.AreEqual(
            new[]
            {
                "To",
                "From"
            },
            response.Items
                .Select(item =>
                    item.Type)
                .ToArray());
    }

    [TestMethod]
    public async Task Query_SourceIsExact_AndTextSearchIncludesUnicodeDataJson()
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
                1,
                13,
                9,
                0,
                0,
                TimeSpan.Zero);

        await store.AppendEventsAsync(
            [
                new EventRecord(
                    0,
                    timestamp,
                    EventCategory.Command,
                    "CommandFailed",
                    EventSeverity.Error,
                    "PLC-01",
                    "Команда завершилась с ошибкой.",
                    """{"detail":"авария насоса"}"""),
                new EventRecord(
                    0,
                    timestamp.AddSeconds(1),
                    EventCategory.Command,
                    "CommandFailed",
                    EventSeverity.Error,
                    "plc-01",
                    "Другая команда.",
                    """{"detail":"авария насоса"}""")
            ]);

        var response =
            await client.GetFromJsonAsync<EventQueryResponseDto>(
                BuildUrl(
                    timestamp.AddMinutes(-1),
                    timestamp.AddMinutes(1),
                    source:
                        "PLC-01",
                    text:
                        "авария",
                    page:
                        1,
                    limit:
                        10));

        Assert.IsNotNull(
            response);
        Assert.AreEqual(
            1,
            response.Items.Count);
        Assert.AreEqual(
            "PLC-01",
            response.Items[0].Source);
        Assert.AreEqual(
            """{"detail":"авария насоса"}""",
            response.Items[0].DataJson);
    }

    [TestMethod]
    public async Task Query_NoMatchingEvents_ReturnsEmptyPage()
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
                2,
                1,
                0,
                0,
                0,
                TimeSpan.Zero);

        var response =
            await client.GetFromJsonAsync<EventQueryResponseDto>(
                BuildUrl(
                    from,
                    from.AddHours(1),
                    category:
                        "Device",
                    page:
                        1,
                    limit:
                        100));

        Assert.IsNotNull(
            response);
        Assert.AreEqual(
            0,
            response.Items.Count);
        Assert.IsFalse(
            response.HasMore);
    }

    [TestMethod]
    public async Task Query_InvalidParameters_ReturnBadRequest()
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
                1,
                15,
                10,
                0,
                0,
                TimeSpan.Zero);

        var to =
            from.AddMinutes(
                1);

        var valid =
            BuildUrl(
                from,
                to,
                page:
                    1,
                limit:
                    100);

        var urls =
            new[]
            {
                "/api/events",
                BuildUrl(
                    to,
                    from,
                    page:
                        1,
                    limit:
                        100),
                BuildUrl(
                    from,
                    to,
                    category:
                        "Unknown",
                    page:
                        1,
                    limit:
                        100),
                BuildUrl(
                    from,
                    to,
                    page:
                        0,
                    limit:
                        100),
                BuildUrl(
                    from,
                    to,
                    page:
                        1,
                    limit:
                        EventQueryService.MaxLimit + 1),
                $"{valid}&source=a&source=b"
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

    private static EventRecord Event(
        DateTimeOffset timestamp,
        EventCategory category,
        EventSeverity severity,
        string source,
        string type,
        string message)
    {
        return new EventRecord(
            EventId:
                0,
            Timestamp:
                timestamp,
            Category:
                category,
            Type:
                type,
            Severity:
                severity,
            Source:
                source,
            Message:
                message,
            DataJson:
                null);
    }

    private static string BuildUrl(
        DateTimeOffset from,
        DateTimeOffset to,
        string? category = null,
        string? severity = null,
        string? source = null,
        string? text = null,
        int page = 1,
        int limit = 200)
    {
        var query =
            new List<string>
            {
                $"from={Escape(from.ToString("O", CultureInfo.InvariantCulture))}",
                $"to={Escape(to.ToString("O", CultureInfo.InvariantCulture))}",
                $"page={page.ToString(CultureInfo.InvariantCulture)}",
                $"limit={limit.ToString(CultureInfo.InvariantCulture)}"
            };

        if (category is not null)
        {
            query.Add(
                $"category={Escape(category)}");
        }

        if (severity is not null)
        {
            query.Add(
                $"severity={Escape(severity)}");
        }

        if (source is not null)
        {
            query.Add(
                $"source={Escape(source)}");
        }

        if (text is not null)
        {
            query.Add(
                $"text={Escape(text)}");
        }

        return $"/api/events?{string.Join("&", query)}";
    }

    private static string Escape(
        string value)
    {
        return Uri.EscapeDataString(
            value);
    }
}
