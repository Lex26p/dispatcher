using System.Text.Json;
using Dispatcher.Contracts.Realtime;
using Dispatcher.Contracts.Tags;
using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;
using Dispatcher.Modbus;
using Dispatcher.Server.Alarms;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Events;
using Dispatcher.Server.Historian;
using Dispatcher.Server.Mimics;
using Dispatcher.Server.Realtime;
using Dispatcher.Server.Runtime;
using Dispatcher.Server.Security;
using Dispatcher.Server.Templates;
using Dispatcher.Snmp;

var builder =
    WebApplication.CreateBuilder(args);

builder.Services.AddSingleton<TagService>();
builder.Services.AddSingleton<DeviceStateService>();

builder.Services.AddSingleton(
    services =>
        new SqliteConfigurationStore(
            ResolveConfigurationDatabasePath(
                services.GetRequiredService<IConfiguration>(),
                services.GetRequiredService<IHostEnvironment>())));

builder.Services.AddLocalAuthentication();

builder.Services.AddSingleton<ConfigurationCatalog>();
builder.Services.AddSingleton<HistorianPolicyCatalog>();
builder.Services.AddSingleton<AlarmDefinitionCatalog>();
builder.Services.AddSingleton<SecurityCatalog>();
builder.Services.AddSingleton<SecurityManagementService>();
builder.Services.AddHostedService<ConfigurationInitializationHostedService>();

builder.Services.AddSingleton(
    services =>
        new SqliteOperationalStore(
            ResolveOperationalDatabasePath(
                services.GetRequiredService<IConfiguration>(),
                services.GetRequiredService<IHostEnvironment>())));

builder.Services.AddSingleton<IHistorySampleStore>(
    services =>
        services.GetRequiredService<SqliteOperationalStore>());

builder.Services.AddSingleton<IEventJournalStore>(
    services =>
        services.GetRequiredService<SqliteOperationalStore>());

builder.Services.AddSingleton(
    services =>
        HistorianOptions.Create(
            services.GetRequiredService<IConfiguration>()));

builder.Services.AddSingleton<HistorianService>();
builder.Services.AddHostedService<HistorianService>(
    services =>
        services.GetRequiredService<HistorianService>());

builder.Services.AddHostedService<HistorianRetentionHostedService>();
builder.Services.AddSingleton<HistorianPolicyService>();
builder.Services.AddSingleton<HistoryQueryService>();
builder.Services.AddSingleton<AlarmDefinitionService>();
builder.Services.AddSingleton<AlarmHistoryQueryService>();

builder.Services.AddSingleton(
    services =>
        EventJournalOptions.Create(
            services.GetRequiredService<IConfiguration>()));

builder.Services.AddSingleton<EventJournalService>();
builder.Services.AddHostedService<EventJournalService>(
    services =>
        services.GetRequiredService<EventJournalService>());

builder.Services.AddSingleton<EventQueryService>();

builder.Services.AddSingleton<ModbusTcpRegisterReader>();
builder.Services.AddSingleton<ModbusPollingService>();
builder.Services.AddSingleton<ModbusTcpRegisterWriter>();
builder.Services.AddSingleton<ModbusWriteService>();

builder.Services.AddSingleton<SnmpGetClient>();
builder.Services.AddSingleton<SnmpPollingService>();

builder.Services.AddSignalR();
builder.Services.AddHostedService<EventHubPublisher>();

builder.Services.AddSingleton<AlarmRuntimeService>();
builder.Services.AddHostedService<AlarmRuntimeService>(
    services =>
        services.GetRequiredService<AlarmRuntimeService>());

builder.Services.AddHostedService<AlarmHubPublisher>();

builder.Services.AddSingleton<ModbusRuntimeHostedService>();
builder.Services.AddHostedService<ModbusRuntimeHostedService>(
    services =>
        services.GetRequiredService<ModbusRuntimeHostedService>());

builder.Services.AddSingleton<SnmpRuntimeHostedService>();
builder.Services.AddHostedService<SnmpRuntimeHostedService>(
    services =>
        services.GetRequiredService<SnmpRuntimeHostedService>());

builder.Services.AddSingleton<RuntimeConfigurationCoordinator>();
builder.Services.AddSingleton<ConfigurationEditorService>();
builder.Services.AddSingleton<MimicConfigurationService>();
builder.Services.AddSingleton<TemplateMutationGate>();
builder.Services.AddSingleton<TemplateCatalogService>();
builder.Services.AddSingleton<MimicTemplateService>();
builder.Services.AddSingleton<DeviceTemplateService>();
builder.Services.AddHostedService<RuntimeHubPublisher>();

var app =
    builder.Build();

app.UseStaticFiles();
app.UseAuthentication();
app.UseAuthorization();
app.UseMiddleware<PermissionEndpointAuthorizationMiddleware>();

app.MapGet(
    "/health",
    () => Results.Ok(new
    {
        status = "ok"
    }));

app.MapGet(
    "/api/tags",
    (
        TagService tagService,
        ConfigurationCatalog configuration) =>
    {
        return tagService.GetAll()
            .Select(tag =>
                RuntimeContractMapper.ToDto(
                    tag,
                    configuration.IsTagWritable(
                        tag.TagId)))
            .ToArray();
    });

app.MapGet(
    "/api/devices",
    (DeviceStateService deviceStateService) =>
    {
        return deviceStateService.GetAll()
            .Select(
                RuntimeContractMapper.ToDto)
            .ToArray();
    });

app.MapPost(
    "/api/tags/{tagId}/write",
    (
        string tagId,
        TagWriteRequest request,
        HttpContext httpContext,
        ConfigurationCatalog configuration,
        ModbusWriteService writeService,
        EventJournalService eventJournal,
        ILogger<Program> logger,
        CancellationToken cancellationToken) =>
        WriteTagAsync(
            tagId,
            request,
            EventActor.FromAuthenticatedPrincipal(
                httpContext.User),
            configuration,
            writeService,
            eventJournal,
            logger,
            cancellationToken));

app.MapConfigurationEndpoints();
app.MapAlarmDefinitionEndpoints();
app.MapAlarmRuntimeEndpoints();
app.MapHistorianPolicyEndpoints();
app.MapHistoryEndpoints();
app.MapEventEndpoints();
app.MapMimicEndpoints();
app.MapMimicTemplateEndpoints();
app.MapTemplateEndpoints();
app.MapAuthenticationEndpoints();
app.MapSecurityManagementEndpoints();

app.MapHub<Dispatcher.Server.Realtime.RuntimeHub>(
    RuntimeHubContract.Path);

app.MapStaticAssets();
app.MapFallbackToFile(
    "index.html");

app.Run();

static async Task<IResult> WriteTagAsync(
    string tagId,
    TagWriteRequest request,
    EventActor actor,
    ConfigurationCatalog configuration,
    ModbusWriteService writeService,
    EventJournalService eventJournal,
    ILogger<Program> logger,
    CancellationToken cancellationToken)
{
    var binding =
        configuration.FindTag(
            tagId);

    if (binding is null)
    {
        if (configuration.ContainsTagId(
                tagId))
        {
            eventJournal.Publish(
                EventCategory.Command,
                EventTypes.TagWriteFailed,
                EventSeverity.Warning,
                tagId,
                $"Запись тега '{tagId}' отклонена: текущий протокол read-only.",
                new
                {
                    Reason =
                        "ProtocolReadOnly"
                },
                actor:
                    actor);

            return Results.Problem(
                statusCode:
                    StatusCodes.Status409Conflict,
                title:
                    "Tag is read-only.",
                detail:
                    $"Тег '{tagId}' не поддерживает запись текущим протоколом.");
        }

        eventJournal.Publish(
            EventCategory.Command,
            EventTypes.TagWriteFailed,
            EventSeverity.Warning,
            tagId,
            $"Запись тега '{tagId}' отклонена: тег не найден.",
            new
            {
                Reason =
                    "TagNotFound"
            },
            actor:
                actor);

        return Results.Problem(
            statusCode:
                StatusCodes.Status404NotFound,
            title:
                "Tag not found.",
            detail:
                $"Тег '{tagId}' отсутствует в текущей конфигурации.");
    }

    if (!binding.Device.Enabled)
    {
        eventJournal.Publish(
            EventCategory.Command,
            EventTypes.TagWriteFailed,
            EventSeverity.Warning,
            tagId,
            $"Запись тега '{tagId}' отклонена: устройство отключено.",
            new
            {
                Reason =
                    "DeviceDisabled",
                binding.Device.DeviceId
            },
            actor:
                actor);

        return Results.Problem(
            statusCode:
                StatusCodes.Status503ServiceUnavailable,
            title:
                "Device is disabled.",
            detail:
                $"Устройство '{binding.Device.DeviceId}' отключено в конфигурации.");
    }

    if (!binding.Tag.Writable)
    {
        eventJournal.Publish(
            EventCategory.Command,
            EventTypes.TagWriteFailed,
            EventSeverity.Warning,
            tagId,
            $"Запись тега '{tagId}' отклонена: тег read-only.",
            new
            {
                Reason =
                    "TagReadOnly",
                binding.Device.DeviceId
            },
            actor:
                actor);

        return Results.Problem(
            statusCode:
                StatusCodes.Status409Conflict,
            title:
                "Tag is read-only.",
            detail:
                $"Тег '{tagId}' доступен только для чтения.");
    }

    if (!TryGetUInt16(
            request.Value,
            out var value))
    {
        eventJournal.Publish(
            EventCategory.Command,
            EventTypes.TagWriteFailed,
            EventSeverity.Warning,
            tagId,
            $"Запись тега '{tagId}' отклонена: значение вне UInt16.",
            new
            {
                Reason =
                    "InvalidUInt16Value",
                binding.Device.DeviceId,
                request.Value
            },
            actor:
                actor);

        return Results.Problem(
            statusCode:
                StatusCodes.Status400BadRequest,
            title:
                "Invalid tag value.",
            detail:
                "Значение должно быть целым числом от 0 до 65535.");
    }

    try
    {
        var target =
            ModbusConfigurationMapper.CreateWriteTarget(
                binding);

        var tagValue =
            await writeService.WriteHoldingRegisterAsync(
                target.Device,
                target.Point,
                value,
                target.RequestTimeout,
                cancellationToken);

        eventJournal.Publish(
            EventCategory.Command,
            EventTypes.TagWriteSucceeded,
            EventSeverity.Information,
            tagId,
            $"Тег '{tagId}' записан.",
            new
            {
                binding.Device.DeviceId,
                Value =
                    value
            },
            timestamp:
                tagValue.Timestamp,
            actor:
                actor);

        return Results.Ok(
            RuntimeContractMapper.ToDto(
                tagValue,
                writable: true));
    }
    catch (OperationCanceledException)
        when (cancellationToken.IsCancellationRequested)
    {
        throw;
    }
    catch (Exception exception)
    {
        logger.LogWarning(
            exception,
            "Failed to write tag {TagId}.",
            tagId);

        eventJournal.Publish(
            EventCategory.Command,
            EventTypes.TagWriteFailed,
            EventSeverity.Error,
            tagId,
            $"Ошибка записи тега '{tagId}'.",
            new
            {
                binding.Device.DeviceId,
                Error =
                    exception.Message
            },
            actor:
                actor);

        return Results.Problem(
            statusCode:
                StatusCodes.Status502BadGateway,
            title:
                "Modbus write failed.",
            detail:
                exception.Message);
    }
}

static string ResolveConfigurationDatabasePath(
    IConfiguration configuration,
    IHostEnvironment environment)
{
    var configuredPath =
        configuration[
            "ConfigurationDatabase:Path"];

    if (!string.IsNullOrWhiteSpace(
            configuredPath))
    {
        return Path.IsPathRooted(
                configuredPath)
            ? configuredPath
            : Path.Combine(
                environment.ContentRootPath,
                configuredPath);
    }

    var localApplicationData =
        Environment.GetFolderPath(
            Environment.SpecialFolder.LocalApplicationData);

    if (!string.IsNullOrWhiteSpace(
            localApplicationData))
    {
        return Path.Combine(
            localApplicationData,
            "Dispatcher",
            "dispatcher.db");
    }

    return Path.Combine(
        AppContext.BaseDirectory,
        "data",
        "dispatcher.db");
}

static string ResolveOperationalDatabasePath(
    IConfiguration configuration,
    IHostEnvironment environment)
{
    var configuredPath =
        configuration[
            "OperationalDatabase:Path"];

    if (!string.IsNullOrWhiteSpace(
            configuredPath))
    {
        return Path.IsPathRooted(
                configuredPath)
            ? configuredPath
            : Path.Combine(
                environment.ContentRootPath,
                configuredPath);
    }

    var localApplicationData =
        Environment.GetFolderPath(
            Environment.SpecialFolder.LocalApplicationData);

    if (!string.IsNullOrWhiteSpace(
            localApplicationData))
    {
        return Path.Combine(
            localApplicationData,
            "Dispatcher",
            "dispatcher-operational.db");
    }

    return Path.Combine(
        AppContext.BaseDirectory,
        "data",
        "dispatcher-operational.db");
}

static bool TryGetUInt16(
    object? rawValue,
    out ushort value)
{
    switch (rawValue)
    {
        case JsonElement
        {
            ValueKind: JsonValueKind.Number
        } json
            when json.TryGetUInt16(
                out value):
            return true;

        case ushort direct:
            value =
                direct;
            return true;

        case int number
            when number is
                >= ushort.MinValue
                and <= ushort.MaxValue:
            value =
                (ushort)number;
            return true;

        default:
            value =
                default;
            return false;
    }
}

public partial class Program
{
}
