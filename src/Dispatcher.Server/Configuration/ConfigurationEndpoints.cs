using Dispatcher.Contracts.Configuration;

namespace Dispatcher.Server.Configuration;

public static class ConfigurationEndpoints
{
    public static IEndpointRouteBuilder MapConfigurationEndpoints(
        this IEndpointRouteBuilder endpoints)
    {
        var group =
            endpoints.MapGroup(
                "/api/configuration/modbus");

        group.MapGet(
            "/devices",
            (ConfigurationEditorService editor) =>
            {
                return editor.GetDevices()
                    .Select(ConfigurationContractMapper.ToDto)
                    .ToArray();
            });

        group.MapPost(
            "/devices",
            CreateDeviceAsync);

        group.MapPut(
            "/devices/{deviceId}",
            UpdateDeviceAsync);

        group.MapDelete(
            "/devices/{deviceId}",
            DeleteDeviceAsync);

        group.MapPost(
            "/devices/{deviceId}/tags",
            CreateTagAsync);

        group.MapPut(
            "/devices/{deviceId}/tags/{tagId}",
            UpdateTagAsync);

        group.MapDelete(
            "/devices/{deviceId}/tags/{tagId}",
            DeleteTagAsync);

        return endpoints;
    }

    private static async Task<IResult> CreateDeviceAsync(
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

    private static async Task<IResult> UpdateDeviceAsync(
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

    private static async Task<IResult> DeleteDeviceAsync(
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

    private static async Task<IResult> CreateTagAsync(
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

    private static async Task<IResult> UpdateTagAsync(
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

    private static async Task<IResult> DeleteTagAsync(
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
