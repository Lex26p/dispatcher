using Dispatcher.Contracts.Events;

namespace Dispatcher.Server.Events;

public sealed class EventQueryService
{
    public const int DefaultLimit = 200;
    public const int MaxLimit = 500;
    public const int MaxPage = 100000;
    public const int MaxSourceLength = 200;
    public const int MaxTextLength = 200;

    private readonly IEventJournalStore _store;

    public EventQueryService(
        IEventJournalStore store)
    {
        _store =
            store;
    }

    public async Task<EventQueryResponseDto> QueryAsync(
        DateTimeOffset from,
        DateTimeOffset to,
        EventCategoryDto? category,
        EventSeverityDto? severity,
        string? source,
        string? text,
        int page,
        int limit,
        CancellationToken cancellationToken)
    {
        var normalizedSource =
            NormalizeOptional(
                source);
        var normalizedText =
            NormalizeOptional(
                text);

        Validate(
            from,
            to,
            normalizedSource,
            normalizedText,
            page,
            limit);

        var offset =
            checked(
                (page - 1) * limit);

        var records =
            await _store.QueryEventsAsync(
                from.ToUniversalTime(),
                to.ToUniversalTime(),
                category is null
                    ? null
                    : EventContractMapper.ToInternal(
                        category.Value),
                severity is null
                    ? null
                    : EventContractMapper.ToInternal(
                        severity.Value),
                normalizedSource,
                normalizedText,
                offset,
                limit + 1,
                cancellationToken);

        var hasMore =
            records.Count > limit;

        var items =
            records
                .Take(
                    limit)
                .Select(
                    EventContractMapper.ToDto)
                .ToArray();

        return new EventQueryResponseDto(
            page,
            limit,
            hasMore,
            items);
    }

    private static string? NormalizeOptional(
        string? value)
    {
        var normalized =
            value?.Trim();

        return string.IsNullOrWhiteSpace(
                normalized)
            ? null
            : normalized;
    }

    private static void Validate(
        DateTimeOffset from,
        DateTimeOffset to,
        string? source,
        string? text,
        int page,
        int limit)
    {
        if (from > to)
        {
            throw new ArgumentException(
                "'from' must be less than or equal to 'to'.");
        }

        if (page is < 1 or > MaxPage)
        {
            throw new ArgumentException(
                $"'page' must be between 1 and {MaxPage}.");
        }

        if (limit is < 1 or > MaxLimit)
        {
            throw new ArgumentException(
                $"'limit' must be between 1 and {MaxLimit}.");
        }

        if (source is not null
            && source.Length > MaxSourceLength)
        {
            throw new ArgumentException(
                $"'source' cannot exceed {MaxSourceLength} characters.");
        }

        if (text is not null
            && text.Length > MaxTextLength)
        {
            throw new ArgumentException(
                $"'text' cannot exceed {MaxTextLength} characters.");
        }
    }
}
