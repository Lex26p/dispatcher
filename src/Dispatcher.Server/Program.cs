using System.Text.Json;
using Dispatcher.Contracts.Realtime;
using Dispatcher.Contracts.Tags;
using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;
using Dispatcher.Modbus;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Realtime;
using Dispatcher.Server.Runtime;
using Microsoft.Extensions.Options;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddSingleton<TagService>();
builder.Services.AddSingleton<DeviceStateService>();

builder.Services.Configure<ModbusRuntimeOptions>(
    builder.Configuration.GetSection(
        ModbusRuntimeOptions.SectionName));

builder.Services.AddSingleton<ModbusTcpRegisterReader>();
builder.Services.AddSingleton<ModbusPollingService>();
builder.Services.AddSingleton<ModbusTcpRegisterWriter>();
builder.Services.AddSingleton<ModbusWriteService>();
builder.Services.AddHostedService<ModbusRuntimeHostedService>();

builder.Services.AddSignalR();
builder.Services.AddHostedService<RuntimeHubPublisher>();

var app = builder.Build();

app.UseStaticFiles();

app.MapGet("/health", () => Results.Ok(new
{
    status = "ok"
}));

app.MapGet(
    "/api/tags",
    (
        TagService tagService,
        IOptions<ModbusRuntimeOptions> modbusOptions) =>
    {
        return tagService.GetAll()
            .Select(tag => RuntimeContractMapper.ToDto(
                tag,
                modbusOptions.Value.IsTagWritable(tag.TagId)))
            .ToArray();
    });

app.MapGet("/api/devices", (DeviceStateService deviceStateService) =>
{
    return deviceStateService.GetAll()
        .Select(RuntimeContractMapper.ToDto)
        .ToArray();
});

app.MapPost(
    "/api/tags/{tagId}/write",
    (
        string tagId,
        TagWriteRequest request,
        IOptions<ModbusRuntimeOptions> modbusOptions,
        ModbusWriteService writeService,
        ILogger<Program> logger,
        CancellationToken cancellationToken) =>
        WriteTagAsync(
            tagId,
            request,
            modbusOptions,
            writeService,
            logger,
            cancellationToken));

app.MapHub<Dispatcher.Server.Realtime.RuntimeHub>(
    RuntimeHubContract.Path);

app.MapStaticAssets();
app.MapFallbackToFile("index.html");

app.Run();

static async Task<IResult> WriteTagAsync(
    string tagId,
    TagWriteRequest request,
    IOptions<ModbusRuntimeOptions> modbusOptions,
    ModbusWriteService writeService,
    ILogger<Program> logger,
    CancellationToken cancellationToken)
{
    var options = modbusOptions.Value;

    if (!options.Enabled)
    {
        return Results.Problem(
            statusCode: StatusCodes.Status503ServiceUnavailable,
            title: "Modbus runtime is disabled.",
            detail: "Запись недоступна, пока Modbus runtime отключён.");
    }

    var point = options.FindPoint(tagId);

    if (point is null)
    {
        return Results.Problem(
            statusCode: StatusCodes.Status404NotFound,
            title: "Tag not found.",
            detail: $"Тег '{tagId}' отсутствует в текущей Modbus-конфигурации.");
    }

    if (!point.Writable)
    {
        return Results.Problem(
            statusCode: StatusCodes.Status409Conflict,
            title: "Tag is read-only.",
            detail: $"Тег '{tagId}' доступен только для чтения.");
    }

    if (!TryGetUInt16(request.Value, out var value))
    {
        return Results.Problem(
            statusCode: StatusCodes.Status400BadRequest,
            title: "Invalid tag value.",
            detail: "Значение должно быть целым числом от 0 до 65535.");
    }

    try
    {
        var target = options.CreateWriteTarget(point);

        var tagValue = await writeService.WriteHoldingRegisterAsync(
            target.Device,
            target.Point,
            value,
            target.RequestTimeout,
            cancellationToken);

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

        return Results.Problem(
            statusCode: StatusCodes.Status502BadGateway,
            title: "Modbus write failed.",
            detail: exception.Message);
    }
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
            when json.TryGetUInt16(out value):
            return true;

        case ushort direct:
            value = direct;
            return true;

        case int number
            when number is >= ushort.MinValue and <= ushort.MaxValue:
            value = (ushort)number;
            return true;

        default:
            value = default;
            return false;
    }
}

public partial class Program
{
}
