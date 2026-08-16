using Dispatcher.Contracts.Mimics;
using Dispatcher.Server.Events;
using Dispatcher.Server.Templates;

namespace Dispatcher.Server.Mimics;

public static class MimicTemplateEndpoints
{
    public static IEndpointRouteBuilder MapMimicTemplateEndpoints(
        this IEndpointRouteBuilder endpoints)
    {
        endpoints.MapGet(
            "/api/configuration/mimic-templates",
            GetTemplatesAsync);

        endpoints.MapGet(
            "/api/configuration/mimic-templates/{templateId}",
            GetTemplateAsync);

        endpoints.MapPut(
            "/api/configuration/mimic-templates/{templateId}",
            UpsertTemplateAsync);

        endpoints.MapDelete(
            "/api/configuration/mimic-templates/{templateId}",
            DeleteTemplateAsync);

        endpoints.MapPost(
            "/api/configuration/mimics/{mimicId}/templates/{templateId}/instantiate",
            InstantiateTemplateAsync);

        return endpoints;
    }

    private static async Task<IResult> GetTemplatesAsync(
        MimicTemplateService service,
        CancellationToken cancellationToken)
    {
        var templates =
            await service.GetAllAsync(
                cancellationToken);

        return Results.Ok(
            templates
                .Select(
                    MimicTemplateContractMapper.ToDto)
                .ToArray());
    }

    private static async Task<IResult> GetTemplateAsync(
        string templateId,
        MimicTemplateService service,
        CancellationToken cancellationToken)
    {
        var template =
            await service.GetAsync(
                templateId,
                cancellationToken);

        return template is null
            ? Results.NotFound()
            : Results.Ok(
                MimicTemplateContractMapper.ToDto(
                    template));
    }

    private static async Task<IResult> UpsertTemplateAsync(
        string templateId,
        MimicTemplateDto request,
        HttpContext httpContext,
        MimicTemplateService service,
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
                    templateId,
                    MimicTemplateContractMapper.ToConfiguration(
                        request),
                    cancellationToken);

            PublishTemplateAudit(
                eventJournal,
                actor,
                operation:
                    "Upsert",
                templateId);

            return Results.Ok(
                MimicTemplateContractMapper.ToDto(
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
                exception,
                "Mimic template update failed.");
        }
    }

    private static async Task<IResult> DeleteTemplateAsync(
        string templateId,
        HttpContext httpContext,
        MimicTemplateService service,
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
                    templateId,
                    cancellationToken);

            if (!deleted)
            {
                return Results.NotFound();
            }

            PublishTemplateAudit(
                eventJournal,
                actor,
                operation:
                    "Delete",
                templateId);

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
                exception,
                "Mimic template delete failed.");
        }
    }

    private static async Task<IResult> InstantiateTemplateAsync(
        string mimicId,
        string templateId,
        InstantiateMimicTemplateRequest request,
        HttpContext httpContext,
        MimicTemplateService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var updated =
                await service.InstantiateAsync(
                    mimicId,
                    templateId,
                    request,
                    cancellationToken);

            eventJournal.Publish(
                EventCategory.Configuration,
                EventTypes.ConfigurationChanged,
                EventSeverity.Information,
                source:
                    "configuration",
                message:
                    $"Mimic '{mimicId}': instantiated template '{templateId}'.",
                data:
                    new
                    {
                        Area =
                            "Mimic",
                        Operation =
                            "InstantiateTemplate",
                        EntityType =
                            "Mimic",
                        EntityId =
                            mimicId,
                        TemplateId =
                            templateId
                    },
                actor:
                    actor);

            return Results.Ok(
                MimicContractMapper.ToDto(
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
                exception,
                "Mimic template instantiation failed.");
        }
    }

    private static void PublishTemplateAudit(
        EventJournalService eventJournal,
        EventActor actor,
        string operation,
        string templateId)
    {
        eventJournal.Publish(
            EventCategory.Configuration,
            EventTypes.ConfigurationChanged,
            EventSeverity.Information,
            source:
                "configuration",
            message:
                $"Mimic template '{templateId}': {operation}.",
            data:
                new
                {
                    Area =
                        "MimicTemplate",
                    Operation =
                        operation,
                    EntityType =
                        "MimicTemplate",
                    EntityId =
                        templateId
                },
            actor:
                actor);
    }

    private static IResult ToProblem(
        Exception exception,
        string title)
    {
        return exception switch
        {
            KeyNotFoundException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status404NotFound,
                    title:
                        "Mimic or template was not found.",
                    detail:
                        exception.Message),

            TemplateConflictException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status409Conflict,
                    title:
                        "Template ID conflict.",
                    detail:
                        exception.Message),

            InvalidOperationException or
            ArgumentException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status400BadRequest,
                    title:
                        "Invalid mimic template configuration.",
                    detail:
                        exception.Message),

            _ =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status500InternalServerError,
                    title:
                        title,
                    detail:
                        exception.Message)
        };
    }
}
