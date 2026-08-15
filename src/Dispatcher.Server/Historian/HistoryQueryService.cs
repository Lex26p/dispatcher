using Dispatcher.Contracts.Historian;

namespace Dispatcher.Server.Historian;

public sealed class HistoryQueryService
{
    public const int DefaultLimit = 1000;
    public const int MaxLimit = 2000;
    public const int MaxTagCount = 16;

    private readonly IHistorySampleStore _store;

    public HistoryQueryService(
        IHistorySampleStore store)
    {
        _store =
            store;
    }

    public async Task<HistoryQueryResponseDto> QueryAsync(
        IReadOnlyList<string> tagIds,
        DateTimeOffset from,
        DateTimeOffset to,
        HistoryQueryOrderDto order,
        int limit,
        CancellationToken cancellationToken)
    {
        Validate(
            tagIds,
            from,
            to,
            limit);

        var normalizedFrom =
            from.ToUniversalTime();
        var normalizedTo =
            to.ToUniversalTime();

        var series =
            new List<HistorySeriesDto>(
                tagIds.Count);

        foreach (var tagId in tagIds)
        {
            var samples =
                await _store.QueryAsync(
                    tagId,
                    normalizedFrom,
                    normalizedTo,
                    ascending:
                        order == HistoryQueryOrderDto.Ascending,
                    limit:
                        limit + 1,
                    cancellationToken:
                        cancellationToken);

            var truncated =
                samples.Count > limit;

            var returned =
                truncated
                    ? samples.Take(limit)
                    : samples;

            series.Add(
                new HistorySeriesDto(
                    tagId,
                    truncated,
                    returned
                        .Select(
                            HistoryContractMapper.ToDto)
                        .ToArray()));
        }

        return new HistoryQueryResponseDto(
            normalizedFrom,
            normalizedTo,
            order,
            limit,
            series);
    }

    private static void Validate(
        IReadOnlyList<string> tagIds,
        DateTimeOffset from,
        DateTimeOffset to,
        int limit)
    {
        ArgumentNullException.ThrowIfNull(
            tagIds);

        if (tagIds.Count == 0)
        {
            throw new ArgumentException(
                "At least one tagId is required.");
        }

        if (tagIds.Count > MaxTagCount)
        {
            throw new ArgumentException(
                $"No more than {MaxTagCount} tagId values may be queried at once.");
        }

        var unique =
            new HashSet<string>(
                StringComparer.Ordinal);

        foreach (var tagId in tagIds)
        {
            if (string.IsNullOrWhiteSpace(
                    tagId))
            {
                throw new ArgumentException(
                    "tagId cannot be empty.");
            }

            if (!unique.Add(
                    tagId))
            {
                throw new ArgumentException(
                    $"Duplicate tagId '{tagId}' is not allowed.");
            }
        }

        if (from > to)
        {
            throw new ArgumentException(
                "'from' must be less than or equal to 'to'.");
        }

        if (limit is < 1 or > MaxLimit)
        {
            throw new ArgumentException(
                $"'limit' must be between 1 and {MaxLimit}.");
        }
    }
}
