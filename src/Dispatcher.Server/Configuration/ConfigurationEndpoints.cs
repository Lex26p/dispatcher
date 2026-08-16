using Dispatcher.Contracts.Configuration;
using Dispatcher.Server.Events;

namespace Dispatcher.Server.Configuration;

public static class ConfigurationEndpoints
{
    public static IEndpointRouteBuilder MapConfigurationEndpoints(
        this IEndpointRouteBuilder endpoints)
    {
        MapModbusEndpoints(
            endpoints);
        MapSnmpEndpoints(
            endpoints);

        return endpoints;
    }

    private static void MapModbusEndpoints(
        IEndpointRouteBuilder endpoints)
    {
        var group =
            endpoints.MapGroup(
                "/api/configuration/modbus");

        group.MapGet(
            "/devices",
            (ConfigurationEditorService editor) =>
            {
                return editor.GetDevices()
                    .Select(
                        ConfigurationContractMapper.ToDto)
                    .ToArray();
            });

        group.MapPost(
            "/devices",
            CreateModbusDeviceAsync);

        group.MapPut(
            "/devices/{deviceId}",
            UpdateModbusDeviceAsync);

        group.MapDelete(
            "/devices/{deviceId}",
            DeleteModbusDeviceAsync);

        group.MapPost(
            "/devices/{deviceId}/tags",
            CreateModbusTagAsync);

        group.MapPut(
            "/devices/{deviceId}/tags/{tagId}",
            UpdateModbusTagAsync);

        group.MapDelete(
            "/devices/{deviceId}/tags/{tagId}",
            DeleteModbusTagAsync);
    }

    private static void MapSnmpEndpoints(
        IEndpointRouteBuilder endpoints)
    {
        var group =
            endpoints.MapGroup(
                "/api/configuration/snmp");

        group.MapGet(
            "/devices",
            (ConfigurationEditorService editor) =>
            {
                return editor.GetSnmpDevices()
                    .Select(
                        ConfigurationContractMapper.ToDto)
                    .ToArray();
            });

        group.MapPost(
            "/devices",
            CreateSnmpDeviceAsync);

        group.MapPut(
            "/devices/{deviceId}",
            UpdateSnmpDeviceAsync);

        group.MapDelete(
            "/devices/{deviceId}",
            DeleteSnmpDeviceAsync);

        group.MapPost(
            "/devices/{deviceId}/tags",
            CreateSnmpTagAsync);

        group.MapPut(
            "/devices/{deviceId}/tags/{tagId}",
            UpdateSnmpTagAsync);

        group.MapDelete(
            "/devices/{deviceId}/tags/{tagId}",
            DeleteSnmpTagAsync);
    }

    private static async Task<IResult> CreateModbusDeviceAsync(
        ModbusDeviceUpsertRequest request,
        HttpContext httpContext,
        ConfigurationEditorService editor,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var created =
                await editor.CreateDeviceAsync(
                    request,
                    cancellationToken);

            var dto =
                ConfigurationContractMapper.ToDto(
                    created);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                area: "Modbus",
                operation: "Create",
                entityType: "Device",
                entityId: dto.DeviceId);

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
            return ToProblem(
                exception);
        }
    }

    private static async Task<IResult> UpdateModbusDeviceAsync(
        string deviceId,
        ModbusDeviceUpsertRequest request,
        HttpContext httpContext,
        ConfigurationEditorService editor,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var updated =
                await editor.UpdateDeviceAsync(
                    deviceId,
                    request,
                    cancellationToken);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                area: "Modbus",
                operation: "Update",
                entityType: "Device",
                entityId: updated.DeviceId);

            return Results.Ok(
                ConfigurationContractMapper.ToDto(
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

    private static async Task<IResult> DeleteModbusDeviceAsync(
        string deviceId,
        HttpContext httpContext,
        ConfigurationEditorService editor,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);

            await editor.DeleteDeviceAsync(
                deviceId,
                cancellationToken);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                area: "Modbus",
                operation: "Delete",
                entityType: "Device",
                entityId: deviceId);

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

    private static async Task<IResult> CreateModbusTagAsync(
        string deviceId,
        ModbusTagUpsertRequest request,
        HttpContext httpContext,
        ConfigurationEditorService editor,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var created =
                await editor.CreateTagAsync(
                    deviceId,
                    request,
                    cancellationToken);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                area: "Modbus",
                operation: "Create",
                entityType: "Tag",
                entityId: created.TagId,
                parentId: deviceId);

            return Results.Created(
                $"/api/configuration/modbus/devices/{Uri.EscapeDataString(deviceId)}/tags/{Uri.EscapeDataString(created.TagId)}",
                ConfigurationContractMapper.ToDto(
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

    private static async Task<IResult> UpdateModbusTagAsync(
        string deviceId,
        string tagId,
        ModbusTagUpsertRequest request,
        HttpContext httpContext,
        ConfigurationEditorService editor,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var updated =
                await editor.UpdateTagAsync(
                    deviceId,
                    tagId,
                    request,
                    cancellationToken);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                area: "Modbus",
                operation: "Update",
                entityType: "Tag",
                entityId: updated.TagId,
                parentId: deviceId);

            return Results.Ok(
                ConfigurationContractMapper.ToDto(
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

    private static async Task<IResult> DeleteModbusTagAsync(
        string deviceId,
        string tagId,
        HttpContext httpContext,
        ConfigurationEditorService editor,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);

            await editor.DeleteTagAsync(
                deviceId,
                tagId,
                cancellationToken);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                area: "Modbus",
                operation: "Delete",
                entityType: "Tag",
                entityId: tagId,
                parentId: deviceId);

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

    private static async Task<IResult> CreateSnmpDeviceAsync(
        SnmpDeviceUpsertRequest request,
        HttpContext httpContext,
        ConfigurationEditorService editor,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var created =
                await editor.CreateSnmpDeviceAsync(
                    request,
                    cancellationToken);

            var dto =
                ConfigurationContractMapper.ToDto(
                    created);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                area: "SNMP",
                operation: "Create",
                entityType: "Device",
                entityId: dto.DeviceId);

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
            return ToProblem(
                exception);
        }
    }

    private static async Task<IResult> UpdateSnmpDeviceAsync(
        string deviceId,
        SnmpDeviceUpsertRequest request,
        HttpContext httpContext,
        ConfigurationEditorService editor,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var updated =
                await editor.UpdateSnmpDeviceAsync(
                    deviceId,
                    request,
                    cancellationToken);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                area: "SNMP",
                operation: "Update",
                entityType: "Device",
                entityId: updated.DeviceId);

            return Results.Ok(
                ConfigurationContractMapper.ToDto(
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

    private static async Task<IResult> DeleteSnmpDeviceAsync(
        string deviceId,
        HttpContext httpContext,
        ConfigurationEditorService editor,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);

            await editor.DeleteSnmpDeviceAsync(
                deviceId,
                cancellationToken);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                area: "SNMP",
                operation: "Delete",
                entityType: "Device",
                entityId: deviceId);

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

    private static async Task<IResult> CreateSnmpTagAsync(
        string deviceId,
        SnmpTagUpsertRequest request,
        HttpContext httpContext,
        ConfigurationEditorService editor,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var created =
                await editor.CreateSnmpTagAsync(
                    deviceId,
                    request,
                    cancellationToken);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                area: "SNMP",
                operation: "Create",
                entityType: "Tag",
                entityId: created.TagId,
                parentId: deviceId);

            return Results.Created(
                $"/api/configuration/snmp/devices/{Uri.EscapeDataString(deviceId)}/tags/{Uri.EscapeDataString(created.TagId)}",
                ConfigurationContractMapper.ToDto(
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

    private static async Task<IResult> UpdateSnmpTagAsync(
        string deviceId,
        string tagId,
        SnmpTagUpsertRequest request,
        HttpContext httpContext,
        ConfigurationEditorService editor,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);
            var updated =
                await editor.UpdateSnmpTagAsync(
                    deviceId,
                    tagId,
                    request,
                    cancellationToken);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                area: "SNMP",
                operation: "Update",
                entityType: "Tag",
                entityId: updated.TagId,
                parentId: deviceId);

            return Results.Ok(
                ConfigurationContractMapper.ToDto(
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

    private static async Task<IResult> DeleteSnmpTagAsync(
        string deviceId,
        string tagId,
        HttpContext httpContext,
        ConfigurationEditorService editor,
        EventJournalService eventJournal,
        CancellationToken cancellationToken)
    {
        try
        {
            var actor =
                EventActor.FromAuthenticatedPrincipal(
                    httpContext.User);

            await editor.DeleteSnmpTagAsync(
                deviceId,
                tagId,
                cancellationToken);

            PublishConfigurationAudit(
                eventJournal,
                actor,
                area: "SNMP",
                operation: "Delete",
                entityType: "Tag",
                entityId: tagId,
                parentId: deviceId);

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
        string area,
        string operation,
        string entityType,
        string entityId,
        string? parentId = null)
    {
        eventJournal.Publish(
            EventCategory.Configuration,
            EventTypes.ConfigurationChanged,
            EventSeverity.Information,
            source:
                "configuration",
            message:
                $"{area} {entityType} '{entityId}': {operation}.",
            data:
                new
                {
                    Area =
                        area,
                    Operation =
                        operation,
                    EntityType =
                        entityType,
                    EntityId =
                        entityId,
                    ParentId =
                        parentId
                },
            actor:
                actor);
    }

    private static IResult ToProblem(
        Exception exception)
    {
        return exception switch
        {
            ConfigurationNotFoundException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status404NotFound,
                    title:
                        "Configuration object not found.",
                    detail:
                        exception.Message),

            ConfigurationConflictException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status409Conflict,
                    title:
                        "Configuration conflict.",
                    detail:
                        exception.Message),

            InvalidOperationException or
            ArgumentException =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status400BadRequest,
                    title:
                        "Invalid configuration.",
                    detail:
                        exception.Message),

            _ =>
                Results.Problem(
                    statusCode:
                        StatusCodes.Status500InternalServerError,
                    title:
                        "Configuration update failed.",
                    detail:
                        exception.Message)
        };
    }
}
