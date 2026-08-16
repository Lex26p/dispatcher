using System.Globalization;
using Dispatcher.Server.Events;

namespace Dispatcher.Server.Alarms;

public static class AlarmRuntimeEndpoints
{
    public static IEndpointRouteBuilder MapAlarmRuntimeEndpoints(
        this IEndpointRouteBuilder endpoints)
    {
        endpoints.MapGet(
            "/api/alarms/current",
            GetCurrent);

        endpoints.MapGet(
            "/api/alarms/history",
            GetHistoryAsync);

        endpoints.MapPost(
            "/api/alarms/{alarmId}/acknowledge",
            Acknowledge);

        return endpoints;
    }

    private static IResult GetCurrent(
        HttpResponse response,
        AlarmRuntimeService runtime)
    {
        SetNoStore(
            response);

        return Results.Ok(
            runtime.GetAll()
                .Where(snapshot =>
                    snapshot.State
                    != AlarmRuntimeState.Normal)
                .Select(
                    AlarmRuntimeContractMapper.ToDto)
                .ToArray());
    }

    private static async Task<IResult> GetHistoryAsync(
        HttpRequest request,
        HttpResponse response,
        AlarmHistoryQueryService service,
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
                    AlarmHistoryQueryService.DefaultLimit);

            var result =
                await service.QueryAsync(
                    from,
                    to,
                    page,
                    limit,
                    cancellationToken);

            SetNoStore(
                response);

            return Results.Ok(
                result);
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
                    "Invalid alarm history query.",
                detail:
                    exception.Message);
        }
    }

    private static IResult Acknowledge(
        string alarmId,
        HttpContext httpContext,
        AlarmRuntimeService runtime)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);

            var snapshot =
                runtime.Acknowledge(
                    alarmId,
                    actor,
                    DateTimeOffset.UtcNow);

            httpContext.Response.Headers[
                "Cache-Control"] =
                "no-store";

            return Results.Ok(
                AlarmRuntimeContractMapper.ToDto(
                    snapshot));
        }
        catch (KeyNotFoundException exception)
        {
            return Results.Problem(
                statusCode:
                    StatusCodes.Status404NotFound,
                title:
                    "Alarm runtime instance not found.",
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

    private static void SetNoStore(
        HttpResponse response)
    {
        response.Headers[
            "Cache-Control"] =
            "no-store";
        response.Headers[
            "Pragma"] =
            "no-cache";
    }
}
