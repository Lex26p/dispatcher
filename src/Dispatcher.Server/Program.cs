using Dispatcher.Contracts.Devices;
using Dispatcher.Contracts.Tags;
using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddSingleton<TagService>();
builder.Services.AddSingleton<DeviceStateService>();

var app = builder.Build();

app.MapGet("/health", () => Results.Ok(new
{
    status = "ok"
}));

app.MapGet("/api/tags", (TagService tagService) =>
{
    return tagService.GetAll()
        .Select(tag => new TagValueDto(
            tag.TagId,
            tag.Value,
            tag.Timestamp))
        .ToArray();
});

app.MapGet("/api/devices", (DeviceStateService deviceStateService) =>
{
    return deviceStateService.GetAll()
        .Select(state => new DeviceStateDto(
            state.DeviceId,
            MapStatus(state.Status),
            state.UpdatedAt,
            state.LastSuccessfulPollAt,
            state.Error))
        .ToArray();
});

app.Run();

static DeviceConnectionStatusDto MapStatus(DeviceConnectionStatus status)
{
    return status switch
    {
        DeviceConnectionStatus.Unknown => DeviceConnectionStatusDto.Unknown,
        DeviceConnectionStatus.Online => DeviceConnectionStatusDto.Online,
        DeviceConnectionStatus.Offline => DeviceConnectionStatusDto.Offline,
        _ => throw new ArgumentOutOfRangeException(
            nameof(status),
            status,
            "Unknown device connection status.")
    };
}

public partial class Program
{
}
