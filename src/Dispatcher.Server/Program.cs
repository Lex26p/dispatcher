using Dispatcher.Contracts.Realtime;
using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;
using Dispatcher.Modbus;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Realtime;
using Dispatcher.Server.Runtime;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddSingleton<TagService>();
builder.Services.AddSingleton<DeviceStateService>();

builder.Services.Configure<ModbusRuntimeOptions>(
    builder.Configuration.GetSection(
        ModbusRuntimeOptions.SectionName));

builder.Services.AddSingleton<ModbusTcpRegisterReader>();
builder.Services.AddSingleton<ModbusPollingService>();
builder.Services.AddHostedService<ModbusRuntimeHostedService>();

builder.Services.AddSignalR();
builder.Services.AddHostedService<RuntimeHubPublisher>();

var app = builder.Build();

app.UseStaticFiles();

app.MapGet("/health", () => Results.Ok(new
{
    status = "ok"
}));

app.MapGet("/api/tags", (TagService tagService) =>
{
    return tagService.GetAll()
        .Select(RuntimeContractMapper.ToDto)
        .ToArray();
});

app.MapGet("/api/devices", (DeviceStateService deviceStateService) =>
{
    return deviceStateService.GetAll()
        .Select(RuntimeContractMapper.ToDto)
        .ToArray();
});

app.MapHub<Dispatcher.Server.Realtime.RuntimeHub>(
    RuntimeHubContract.Path);

app.MapStaticAssets();
app.MapFallbackToFile("index.html");

app.Run();

public partial class Program
{
}
