using Dispatcher.Contracts.Templates;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Events;

namespace Dispatcher.Server.Templates;

public static class TemplateEndpoints
{
    public static IEndpointRouteBuilder MapTemplateEndpoints(
        this IEndpointRouteBuilder endpoints)
    {
        endpoints.MapGet(
            "/api/configuration/templates",
            GetCatalogAsync);

        endpoints.MapGet(
            "/api/configuration/templates/modbus-devices",
            GetModbusTemplatesAsync);
        endpoints.MapGet(
            "/api/configuration/templates/modbus-devices/{templateId}",
            GetModbusTemplateAsync);
        endpoints.MapPut(
            "/api/configuration/templates/modbus-devices/{templateId}",
            UpsertModbusAsync);
        endpoints.MapDelete(
            "/api/configuration/templates/modbus-devices/{templateId}",
            DeleteModbusAsync);
        endpoints.MapPost(
            "/api/configuration/templates/modbus-devices/{templateId}/instantiate",
            InstantiateModbusAsync);

        endpoints.MapGet(
            "/api/configuration/templates/snmp-devices",
            GetSnmpTemplatesAsync);
        endpoints.MapGet(
            "/api/configuration/templates/snmp-devices/{templateId}",
            GetSnmpTemplateAsync);
        endpoints.MapPut(
            "/api/configuration/templates/snmp-devices/{templateId}",
            UpsertSnmpAsync);
        endpoints.MapDelete(
            "/api/configuration/templates/snmp-devices/{templateId}",
            DeleteSnmpAsync);
        endpoints.MapPost(
            "/api/configuration/templates/snmp-devices/{templateId}/instantiate",
            InstantiateSnmpAsync);

        return endpoints;
    }

    private static async Task<IResult> GetCatalogAsync(
        TemplateCatalogService service,
        CancellationToken cancellationToken)
    {
        var entries =
            await service.GetAllAsync(
                cancellationToken);

        return Results.Ok(
            entries.Select(TemplateContractMapper.ToDto).ToArray());
    }

    private static async Task<IResult> GetModbusTemplatesAsync(
        DeviceTemplateService service,
        CancellationToken cancellationToken)
    {
        var templates =
            await service.GetModbusTemplatesAsync(
                cancellationToken);

        return Results.Ok(
            templates.Select(TemplateContractMapper.ToDto).ToArray());
    }

    private static async Task<IResult> GetModbusTemplateAsync(
        string templateId,
        DeviceTemplateService service,
        CancellationToken cancellationToken)
    {
        var template =
            await service.GetModbusTemplateAsync(
                templateId,
                cancellationToken);

        return template is null
            ? Results.NotFound()
            : Results.Ok(TemplateContractMapper.ToDto(template));
    }

    private static async Task<IResult> GetSnmpTemplatesAsync(
        DeviceTemplateService service,
        CancellationToken cancellationToken)
    {
        var templates =
            await service.GetSnmpTemplatesAsync(
                cancellationToken);

        return Results.Ok(
            templates.Select(TemplateContractMapper.ToDto).ToArray());
    }

    private static async Task<IResult> GetSnmpTemplateAsync(
        string templateId,
        DeviceTemplateService service,
        CancellationToken cancellationToken)
    {
        var template =
            await service.GetSnmpTemplateAsync(
                templateId,
                cancellationToken);

        return template is null
            ? Results.NotFound()
            : Results.Ok(TemplateContractMapper.ToDto(template));
    }

    private static async Task<IResult> UpsertModbusAsync(
        string templateId,
        ModbusDeviceTemplateUpsertRequest request,
        HttpContext httpContext,
        DeviceTemplateService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var saved =
                await service.UpsertModbusAsync(
                    templateId,
                    TemplateContractMapper.ToConfiguration(request),
                    cancellationToken);
            PublishTemplateAudit(
                eventJournal,
                EventActor.FromAuthenticatedPrincipal(httpContext.User),
                "Upsert",
                saved.TemplateId,
                TemplateKind.ModbusDevice,
                saved.Version);

            return Results.Ok(TemplateContractMapper.ToDto(saved));
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
        {
            return ToProblem(exception, "Modbus device template update failed.");
        }
    }

    private static async Task<IResult> UpsertSnmpAsync(
        string templateId,
        SnmpDeviceTemplateUpsertRequest request,
        HttpContext httpContext,
        DeviceTemplateService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var saved =
                await service.UpsertSnmpAsync(
                    templateId,
                    TemplateContractMapper.ToConfiguration(request),
                    cancellationToken);
            PublishTemplateAudit(
                eventJournal,
                EventActor.FromAuthenticatedPrincipal(httpContext.User),
                "Upsert",
                saved.TemplateId,
                TemplateKind.SnmpDevice,
                saved.Version);

            return Results.Ok(TemplateContractMapper.ToDto(saved));
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
        {
            return ToProblem(exception, "SNMP device template update failed.");
        }
    }

    private static async Task<IResult> DeleteModbusAsync(
        string templateId,
        HttpContext httpContext,
        DeviceTemplateService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        return await DeleteAsync(
            templateId,
            TemplateKind.ModbusDevice,
            httpContext,
            service.DeleteModbusAsync,
            eventJournal,
            cancellationToken);
    }

    private static async Task<IResult> DeleteSnmpAsync(
        string templateId,
        HttpContext httpContext,
        DeviceTemplateService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        return await DeleteAsync(
            templateId,
            TemplateKind.SnmpDevice,
            httpContext,
            service.DeleteSnmpAsync,
            eventJournal,
            cancellationToken);
    }

    private static async Task<IResult> DeleteAsync(
        string templateId,
        TemplateKind kind,
        HttpContext httpContext,
        Func<string, CancellationToken, Task<bool>> delete,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            if (!await delete(templateId, cancellationToken))
            {
                return Results.NotFound();
            }

            PublishTemplateAudit(
                eventJournal,
                EventActor.FromAuthenticatedPrincipal(httpContext.User),
                "Delete",
                templateId,
                kind,
                version: null);
            return Results.NoContent();
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
        {
            return ToProblem(exception, "Device template delete failed.");
        }
    }

    private static async Task<IResult> InstantiateModbusAsync(
        string templateId,
        InstantiateDeviceTemplateRequest request,
        HttpContext httpContext,
        DeviceTemplateService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var result =
                await service.InstantiateModbusAsync(
                    templateId,
                    request,
                    cancellationToken);
            PublishInstantiationAudit(
                eventJournal,
                EventActor.FromAuthenticatedPrincipal(httpContext.User),
                templateId,
                TemplateKind.ModbusDevice,
                result.TemplateVersion,
                result.Device.DeviceId);

            var dto =
                TemplateContractMapper.ToDto(result.Device);
            return Results.Created(
                $"/api/configuration/modbus/devices/{Uri.EscapeDataString(dto.DeviceId)}",
                dto);
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
        {
            return ToProblem(exception, "Modbus device template instantiation failed.");
        }
    }

    private static async Task<IResult> InstantiateSnmpAsync(
        string templateId,
        InstantiateDeviceTemplateRequest request,
        HttpContext httpContext,
        DeviceTemplateService service,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var result =
                await service.InstantiateSnmpAsync(
                    templateId,
                    request,
                    cancellationToken);
            PublishInstantiationAudit(
                eventJournal,
                EventActor.FromAuthenticatedPrincipal(httpContext.User),
                templateId,
                TemplateKind.SnmpDevice,
                result.TemplateVersion,
                result.Device.DeviceId);

            var dto =
                TemplateContractMapper.ToDto(result.Device);
            return Results.Created(
                $"/api/configuration/snmp/devices/{Uri.EscapeDataString(dto.DeviceId)}",
                dto);
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
        {
            return ToProblem(exception, "SNMP device template instantiation failed.");
        }
    }

    private static void PublishTemplateAudit(
        EventJournalService eventJournal,
        EventActor actor,
        string operation,
        string templateId,
        TemplateKind kind,
        int? version)
    {
        eventJournal.Publish(
            EventCategory.Configuration,
            EventTypes.ConfigurationChanged,
            EventSeverity.Information,
            source: "configuration",
            message: $"Template '{templateId}' ({kind}): {operation}.",
            data: new
            {
                Area = "Templates",
                Operation = operation,
                EntityType = "Template",
                EntityId = templateId,
                TemplateKind = kind.ToString(),
                Version = version
            },
            actor: actor);
    }

    private static void PublishInstantiationAudit(
        EventJournalService eventJournal,
        EventActor actor,
        string templateId,
        TemplateKind kind,
        int version,
        string deviceId)
    {
        eventJournal.Publish(
            EventCategory.Configuration,
            EventTypes.ConfigurationChanged,
            EventSeverity.Information,
            source: "configuration",
            message: $"Device '{deviceId}': instantiated template '{templateId}'.",
            data: new
            {
                Area = "Devices",
                Operation = "InstantiateTemplate",
                EntityType = "Device",
                EntityId = deviceId,
                TemplateId = templateId,
                TemplateKind = kind.ToString(),
                TemplateVersion = version
            },
            actor: actor);
    }

    private static IResult ToProblem(
        Exception exception,
        string title)
    {
        return exception switch
        {
            KeyNotFoundException =>
                Results.Problem(
                    statusCode: StatusCodes.Status404NotFound,
                    title: "Template was not found.",
                    detail: exception.Message),

            TemplateConflictException or ConfigurationConflictException =>
                Results.Problem(
                    statusCode: StatusCodes.Status409Conflict,
                    title: "Template or configuration conflict.",
                    detail: exception.Message),

            InvalidOperationException or ArgumentException =>
                Results.Problem(
                    statusCode: StatusCodes.Status400BadRequest,
                    title: "Invalid device template configuration.",
                    detail: exception.Message),

            _ =>
                Results.Problem(
                    statusCode: StatusCodes.Status500InternalServerError,
                    title: title,
                    detail: exception.Message)
        };
    }
}
