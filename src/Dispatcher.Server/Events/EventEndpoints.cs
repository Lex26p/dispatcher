using System.Globalization;
using Dispatcher.Contracts.Events;

namespace Dispatcher.Server.Events;

public static class EventEndpoints
{
    public static IEndpointRouteBuilder MapEventEndpoints(
        this IEndpointRouteBuilder endpoints)
    {
        endpoints.MapGet(
            "/api/events",
            QueryAsync);

        return endpoints;
    }

    private static async Task<IResult> QueryAsync(
        HttpRequest request,
        EventQueryService service,
        CancellationToken cancellationToken)
    {
        try
        {
            var from =
                ParseRequiredTimestamp(
                    request,
                    "from");

            var to =
                ParseRequiredTimestamp(
                    request,
                    "to");

            var category =
                ParseOptionalEnum<EventCategoryDto>(
                    GetOptionalSingleQueryValue(
                        request,
                        "category"),
                    "category");

            var severity =
                ParseOptionalEnum<EventSeverityDto>(
                    GetOptionalSingleQueryValue(
                        request,
                        "severity"),
                    "severity");

            var source =
                GetOptionalSingleQueryValue(
                    request,
                    "source");

            var text =
                GetOptionalSingleQueryValue(
                    request,
                    "text");

            var page =
                ParseInt(
                    GetOptionalSingleQueryValue(
                        request,
                        "page"),
                    "page",
                    defaultValue:
                        1);

            var limit =
                ParseInt(
                    GetOptionalSingleQueryValue(
                        request,
                        "limit"),
                    "limit",
                    EventQueryService.DefaultLimit);

            var response =
                await service.QueryAsync(
                    from,
                    to,
                    category,
                    severity,
                    source,
                    text,
                    page,
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
                    "Invalid event query.",
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

        if (!DateTimeOffset.TryParse(
                values[0],
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

    private static TEnum? ParseOptionalEnum<TEnum>(
        string? raw,
        string name)
        where TEnum : struct, Enum
    {
        if (string.IsNullOrWhiteSpace(
                raw))
        {
            return null;
        }

        if (Enum.TryParse<TEnum>(
                raw,
                ignoreCase:
                    true,
                out var value)
            && Enum.IsDefined(
                typeof(TEnum),
                value))
        {
            return value;
        }

        throw new ArgumentException(
            $"'{name}' has an unsupported value '{raw}'.");
    }

    private static int ParseInt(
        string? raw,
        string name,
        int defaultValue)
    {
        if (string.IsNullOrWhiteSpace(
                raw))
        {
            return defaultValue;
        }

        if (!int.TryParse(
                raw,
                NumberStyles.Integer,
                CultureInfo.InvariantCulture,
                out var value))
        {
            throw new ArgumentException(
                $"'{name}' must be an integer.");
        }

        return value;
    }
}
