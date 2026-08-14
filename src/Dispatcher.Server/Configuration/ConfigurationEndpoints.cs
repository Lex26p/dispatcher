using Dispatcher.Contracts.Configuration;

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
        ConfigurationEditorService editor,
        CancellationToken cancellationToken)
    {
        try
        {
            var created =
                await editor.CreateDeviceAsync(
                    request,
                    cancellationToken);

            var dto =
                ConfigurationContractMapper.ToDto(
                    created);

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
        ConfigurationEditorService editor,
        CancellationToken cancellationToken)
    {
        try
        {
            var updated =
                await editor.UpdateDeviceAsync(
                    deviceId,
                    request,
                    cancellationToken);

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
        ConfigurationEditorService editor,
        CancellationToken cancellationToken)
    {
        try
        {
            await editor.DeleteDeviceAsync(
                deviceId,
                cancellationToken);

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
        ConfigurationEditorService editor,
        CancellationToken cancellationToken)
    {
        try
        {
            var created =
                await editor.CreateTagAsync(
                    deviceId,
                    request,
                    cancellationToken);

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
        ConfigurationEditorService editor,
        CancellationToken cancellationToken)
    {
        try
        {
            var updated =
                await editor.UpdateTagAsync(
                    deviceId,
                    tagId,
                    request,
                    cancellationToken);

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
        ConfigurationEditorService editor,
        CancellationToken cancellationToken)
    {
        try
        {
            await editor.DeleteTagAsync(
                deviceId,
                tagId,
                cancellationToken);

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
        ConfigurationEditorService editor,
        CancellationToken cancellationToken)
    {
        try
        {
            var created =
                await editor.CreateSnmpDeviceAsync(
                    request,
                    cancellationToken);

            var dto =
                ConfigurationContractMapper.ToDto(
                    created);

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
        ConfigurationEditorService editor,
        CancellationToken cancellationToken)
    {
        try
        {
            var updated =
                await editor.UpdateSnmpDeviceAsync(
                    deviceId,
                    request,
                    cancellationToken);

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
        ConfigurationEditorService editor,
        CancellationToken cancellationToken)
    {
        try
        {
            await editor.DeleteSnmpDeviceAsync(
                deviceId,
                cancellationToken);

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
        ConfigurationEditorService editor,
        CancellationToken cancellationToken)
    {
        try
        {
            var created =
                await editor.CreateSnmpTagAsync(
                    deviceId,
                    request,
                    cancellationToken);

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
        ConfigurationEditorService editor,
        CancellationToken cancellationToken)
    {
        try
        {
            var updated =
                await editor.UpdateSnmpTagAsync(
                    deviceId,
                    tagId,
                    request,
                    cancellationToken);

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
        ConfigurationEditorService editor,
        CancellationToken cancellationToken)
    {
        try
        {
            await editor.DeleteSnmpTagAsync(
                deviceId,
                tagId,
                cancellationToken);

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
