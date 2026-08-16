using Dispatcher.Contracts.Mimics;
using Dispatcher.Server.Events;

namespace Dispatcher.Server.Mimics;

public static class MimicEndpoints
{
    public static IEndpointRouteBuilder MapMimicEndpoints(
        this IEndpointRouteBuilder endpoints)
    {
        endpoints.MapGet(
            "/api/mimics",
            async (
                MimicConfigurationService service,
                CancellationToken cancellationToken) =>
            {
                var mimics =
                    await service.GetAllAsync(
                        cancellationToken);

                return mimics
                    .Select(
                        MimicContractMapper.ToSummaryDto)
                    .ToArray();
            });

        endpoints.MapGet(
            "/api/mimics/{mimicId}",
            GetMimicAsync);

        var configuration =
            endpoints.MapGroup(
                "/api/configuration/mimics");

        configuration.MapPut(
            "/{mimicId}",
            UpsertMimicAsync);

        configuration.MapDelete(
            "/{mimicId}",
            DeleteMimicAsync);

        return endpoints;
    }

    private static async Task<IResult> GetMimicAsync(
        string mimicId,
        MimicConfigurationService service,
        CancellationToken cancellationToken)
    {
        var mimic =
            await service.GetAsync(
                mimicId,
                cancellationToken);

        return mimic is null
            ? Results.NotFound()
            : Results.Ok(
                MimicContractMapper.ToDto(
                    mimic));
    }

    private static async Task<IResult> UpsertMimicAsync(
        string mimicId,
        MimicDefinitionDto request,
        HttpContext httpContext,
        MimicConfigurationService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var saved =
                await service.UpsertAsync(
                    mimicId,
                    MimicContractMapper.ToConfiguration(
                        request),
                    cancellationToken);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                operation:
                    "Upsert",
                mimicId);

            return Results.Ok(
                MimicContractMapper.ToDto(
                    saved));
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

    private static async Task<IResult> DeleteMimicAsync(
        string mimicId,
        HttpContext httpContext,
        MimicConfigurationService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var deleted =
                await service.DeleteAsync(
                    mimicId,
                    cancellationToken);

            if (deleted)
            {
                PublishConfigurationAudit(
                    eventJournal,
                    actor,
                    operation:
                        "Delete",
                    mimicId);
            }

            return deleted
                ? Results.NoContent()
                : Results.NotFound();
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
        string mimicId)
    {
        eventJournal.Publish(
            EventCategory.Configuration,
            EventTypes.ConfigurationChanged,
            EventSeverity.Information,
            source:
                "configuration",
            message:
                $"Mimic '{mimicId}': {operation}.",
            data:
                new
                {
                    Area =
                        "Mimic",
                    Operation =
                        operation,
                    EntityType =
                        "Mimic",
                    EntityId =
                        mimicId
                },
            actor:
                actor);
    }

    private static IResult ToProblem(
        Exception exception)
    {
        return exception switch
        {
            InvalidOperationException or
            ArgumentException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status400BadRequest,
                    title:
                        "Invalid mimic configuration.",
                    detail:
                        exception.Message),

            _ =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status500InternalServerError,
                    title:
                        "Mimic configuration update failed.",
                    detail:
                        exception.Message)
        };
    }
}
