using Dispatcher.Contracts.Historian;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Events;

namespace Dispatcher.Server.Historian;

public static class HistorianPolicyEndpoints
{
    public static IEndpointRouteBuilder MapHistorianPolicyEndpoints(
        this IEndpointRouteBuilder endpoints)
    {
        var group =
            endpoints.MapGroup(
                "/api/configuration/historian");

        group.MapGet(
            "/policies",
            (
                HistorianPolicyService service,
                ConfigurationCatalog configuration) =>
            {
                return service.GetAll()
                    .Select(policy =>
                        HistorianContractMapper.ToDto(
                            policy,
                            configuration))
                    .ToArray();
            });

        group.MapPut(
            "/policies/{tagId}",
            UpsertPolicyAsync);

        group.MapDelete(
            "/policies/{tagId}",
            DeletePolicyAsync);

        return endpoints;
    }

    private static async Task<IResult> UpsertPolicyAsync(
        string tagId,
        HistorianPolicyUpsertRequest request,
        HttpContext httpContext,
        HistorianPolicyService service,
        ConfigurationCatalog configuration,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var policy =
                await service.UpsertAsync(
                    tagId,
                    request,
                    cancellationToken);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                operation:
                    "Upsert",
                tagId);

            return Results.Ok(
                HistorianContractMapper.ToDto(
                    policy,
                    configuration));
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

    private static async Task<IResult> DeletePolicyAsync(
        string tagId,
        HttpContext httpContext,
        HistorianPolicyService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);

            await service.DeleteAsync(
                tagId,
                cancellationToken);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                operation:
                    "Delete",
                tagId);

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
        string tagId)
    {
        eventJournal.Publish(
            EventCategory.Configuration,
            EventTypes.ConfigurationChanged,
            EventSeverity.Information,
            source:
                "configuration",
            message:
                $"Historian policy '{tagId}': {operation}.",
            data:
                new
                {
                    Area =
                        "Historian",
                    Operation =
                        operation,
                    EntityType =
                        "Policy",
                    EntityId =
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
            KeyNotFoundException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status404NotFound,
                    title:
                        "Historian policy target not found.",
                    detail:
                        exception.Message),

            InvalidOperationException or
            ArgumentException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status400BadRequest,
                    title:
                        "Invalid historian policy.",
                    detail:
                        exception.Message),

            _ =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status500InternalServerError,
                    title:
                        "Historian policy update failed.",
                    detail:
                        exception.Message)
        };
    }
}
