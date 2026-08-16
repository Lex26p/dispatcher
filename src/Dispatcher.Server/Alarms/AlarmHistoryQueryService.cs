using Dispatcher.Contracts.Alarms;
using Dispatcher.Server.Events;
using Dispatcher.Server.Historian;

namespace Dispatcher.Server.Alarms;

public sealed class AlarmHistoryQueryService
{
    public const int DefaultLimit = 200;
    public const int MaximumLimit = 500;
    public const int MaximumPage = 100000;

    private readonly SqliteOperationalStore _store;

    public AlarmHistoryQueryService(
        SqliteOperationalStore store)
    {
        _store =
            store;
    }

    public async Task<AlarmHistoryQueryResponseDto> QueryAsync(
        DateTimeOffset from,
        DateTimeOffset to,
        int page,
        int limit,
        CancellationToken cancellationToken = default)
    {
        if (from > to)
        {
            throw new ArgumentException(
                "'from' must be less than or equal to 'to'.");
        }

        if (page < 1
            || page > MaximumPage)
        {
            throw new ArgumentOutOfRangeException(
                nameof(page),
                $"Page must be between 1 and {MaximumPage}.");
        }

        if (limit < 1
            || limit > MaximumLimit)
        {
            throw new ArgumentOutOfRangeException(
                nameof(limit),
                $"Limit must be between 1 and {MaximumLimit}.");
        }

        var offset =
            checked((page - 1) * limit);

        var records =
            await _store.QueryAlarmEventsAsync(
                from,
                to,
                offset,
                limit + 1,
                cancellationToken);

        var hasMore =
            records.Count > limit;

        return new AlarmHistoryQueryResponseDto(
            records
                .Take(limit)
                .Select(
                    EventContractMapper.ToDto)
                .ToArray(),
            page,
            limit,
            hasMore);
    }
}
