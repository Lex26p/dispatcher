using System.Globalization;
using Dispatcher.Contracts.Historian;

namespace Dispatcher.Server.Historian;

public static class HistoryEndpoints
{
    public static IEndpointRouteBuilder MapHistoryEndpoints(
        this IEndpointRouteBuilder endpoints)
    {
        endpoints.MapGet(
            "/api/history",
            QueryAsync);

        return endpoints;
    }

    private static async Task<IResult> QueryAsync(
        HttpRequest request,
        HistoryQueryService service,
        CancellationToken cancellationToken)
    {
        try
        {
            var tagIds =
                request.Query["tagId"]
                    .Select(value =>
                        value?.Trim()
                        ?? string.Empty)
                    .ToArray();

            if (tagIds.Length == 0)
            {
                throw new ArgumentException(
                    "At least one tagId query parameter is required.");
            }

            var from =
                ParseRequiredTimestamp(
                    request,
                    "from");

            var to =
                ParseRequiredTimestamp(
                    request,
                    "to");

            var order =
                ParseOrder(
                    GetOptionalSingleQueryValue(
                        request,
                        "order"));

            var limit =
                ParseLimit(
                    GetOptionalSingleQueryValue(
                        request,
                        "limit"));

            var response =
                await service.QueryAsync(
                    tagIds,
                    from,
                    to,
                    order,
                    limit,
                    cancellationToken);

            return Results.Ok(
                response);
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (ArgumentException exception)
        {
            return Results.Problem(
                statusCode:
                    StatusCodes.Status400BadRequest,
                title:
                    "Invalid history query.",
                detail:
                    exception.Message);
        }
    }

    private static DateTimeOffset ParseRequiredTimestamp(
        HttpRequest request,
        string name)
    {
        var values =
            request.Query[name];

        if (values.Count != 1
            || string.IsNullOrWhiteSpace(
                values[0]))
        {
            throw new ArgumentException(
                $"'{name}' query parameter must be specified exactly once.");
        }

        var raw =
            values[0]!;

        if (!DateTimeOffset.TryParse(
                raw,
                CultureInfo.InvariantCulture,
                DateTimeStyles.AllowWhiteSpaces,
                out var value))
        {
            throw new ArgumentException(
                $"'{name}' must be a valid ISO-8601 timestamp.");
        }

        return value;
    }

    private static string? GetOptionalSingleQueryValue(
        HttpRequest request,
        string name)
    {
        var values =
            request.Query[name];

        if (values.Count > 1)
        {
            throw new ArgumentException(
                $"'{name}' query parameter may be specified at most once.");
        }

        return values.Count == 0
            ? null
            : values[0];
    }

    private static HistoryQueryOrderDto ParseOrder(
        string? raw)
    {
        if (string.IsNullOrWhiteSpace(
                raw)
            || string.Equals(
                raw,
                "asc",
                StringComparison.OrdinalIgnoreCase)
            || string.Equals(
                raw,
                "ascending",
                StringComparison.OrdinalIgnoreCase))
        {
            return HistoryQueryOrderDto.Ascending;
        }

        if (string.Equals(
                raw,
                "desc",
                StringComparison.OrdinalIgnoreCase)
            || string.Equals(
                raw,
                "descending",
                StringComparison.OrdinalIgnoreCase))
        {
            return HistoryQueryOrderDto.Descending;
        }

        throw new ArgumentException(
            "'order' must be 'asc' or 'desc'.");
    }

    private static int ParseLimit(
        string? raw)
    {
        if (string.IsNullOrWhiteSpace(
                raw))
        {
            return HistoryQueryService.DefaultLimit;
        }

        if (!int.TryParse(
                raw,
                NumberStyles.Integer,
                CultureInfo.InvariantCulture,
                out var limit))
        {
            throw new ArgumentException(
                "'limit' must be an integer.");
        }

        return limit;
    }
}
