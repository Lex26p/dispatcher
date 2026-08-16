using Dispatcher.Contracts.Alarms;
using Dispatcher.Server.Events;

namespace Dispatcher.Server.Alarms;

public static class AlarmDefinitionEndpoints
{
    public static IEndpointRouteBuilder MapAlarmDefinitionEndpoints(
        this IEndpointRouteBuilder endpoints)
    {
        var group =
            endpoints.MapGroup(
                "/api/configuration/alarms");

        group.MapGet(
            "/definitions",
            GetAllAsync);
        group.MapPost(
            "/definitions",
            CreateAsync);
        group.MapPut(
            "/definitions/{alarmId}",
            UpdateAsync);
        group.MapDelete(
            "/definitions/{alarmId}",
            DeleteAsync);

        return endpoints;
    }

    private static async Task<IResult> GetAllAsync(
        AlarmDefinitionService service,
        CancellationToken cancellationToken)
    {
        var definitions =
            await service.GetAllAsync(
                cancellationToken);

        return Results.Ok(
            definitions
                .Select(
                    AlarmDefinitionContractMapper.ToDto)
                .ToArray());
    }

    private static async Task<IResult> CreateAsync(
        CreateAlarmDefinitionRequest request,
        HttpContext httpContext,
        AlarmDefinitionService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var created =
                await service.CreateAsync(
                    request,
                    cancellationToken);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                operation:
                    "Create",
                created.AlarmId,
                created.TagId);

            return Results.Created(
                $"/api/configuration/alarms/definitions/{Uri.EscapeDataString(created.AlarmId)}",
                AlarmDefinitionContractMapper.ToDto(
                    created));
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
        {
            return ToProblem(
                exception);
        }
    }

    private static async Task<IResult> UpdateAsync(
        string alarmId,
        UpdateAlarmDefinitionRequest request,
        HttpContext httpContext,
        AlarmDefinitionService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var updated =
                await service.UpdateAsync(
                    alarmId,
                    request,
                    cancellationToken);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                operation:
                    "Update",
                updated.AlarmId,
                updated.TagId);

            return Results.Ok(
                AlarmDefinitionContractMapper.ToDto(
                    updated));
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
        {
            return ToProblem(
                exception);
        }
    }

    private static async Task<IResult> DeleteAsync(
        string alarmId,
        HttpContext httpContext,
        AlarmDefinitionService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);

            await service.DeleteAsync(
                alarmId,
                cancellationToken);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                operation:
                    "Delete",
                alarmId,
                tagId:
                    null);

            return Results.NoContent();
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
        {
            return ToProblem(
                exception);
        }
    }

    private static void PublishConfigurationAudit(
        EventJournalService eventJournal,
        EventActor actor,
        string operation,
        string alarmId,
        string? tagId)
    {
        eventJournal.Publish(
            EventCategory.Configuration,
            EventTypes.ConfigurationChanged,
            EventSeverity.Information,
            source:
                "configuration",
            message:
                $"Alarm definition '{alarmId}': {operation}.",
            data:
                new
                {
                    Area =
                        "Alarms",
                    Operation =
                        operation,
                    EntityType =
                        "AlarmDefinition",
                    EntityId =
                        alarmId,
                    TagId =
                        tagId
                },
            actor:
                actor);
    }

    private static IResult ToProblem(
        Exception exception)
    {
        return exception switch
        {
            AlarmDefinitionNotFoundException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status404NotFound,
                    title:
                        "Alarm definition not found.",
                    detail:
                        exception.Message),

            AlarmDefinitionConflictException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status409Conflict,
                    title:
                        "Alarm definition conflict.",
                    detail:
                        exception.Message),

            KeyNotFoundException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status404NotFound,
                    title:
                        "Alarm target not found.",
                    detail:
                        exception.Message),

            ArgumentException or InvalidOperationException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status400BadRequest,
                    title:
                        "Invalid alarm definition.",
                    detail:
                        exception.Message),

            _ =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status500InternalServerError,
                    title:
                        "Alarm definition operation failed.",
                    detail:
                        exception.Message)
        };
    }
}
