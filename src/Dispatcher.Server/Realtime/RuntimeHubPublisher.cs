using Dispatcher.Contracts.Realtime;
using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Runtime;
using Microsoft.AspNetCore.SignalR;

namespace Dispatcher.Server.Realtime;

public sealed class RuntimeHubPublisher : IHostedService
{
    private readonly TagService _tagService;
    private readonly DeviceStateService _deviceStateService;
    private readonly IHubContext<RuntimeHub> _hubContext;
    private readonly ConfigurationCatalog _configuration;
    private readonly ILogger<RuntimeHubPublisher> _logger;

    public RuntimeHubPublisher(
        TagService tagService,
        DeviceStateService deviceStateService,
        IHubContext<RuntimeHub> hubContext,
        ConfigurationCatalog configuration,
        ILogger<RuntimeHubPublisher> logger)
    {
        _tagService = tagService;
        _deviceStateService = deviceStateService;
        _hubContext = hubContext;
        _configuration = configuration;
        _logger = logger;
    }

    public Task StartAsync(CancellationToken cancellationToken)
    {
        _tagService.Changed += OnTagChanged;
        _deviceStateService.Changed += OnDeviceStateChanged;

        return Task.CompletedTask;
    }

    public Task StopAsync(CancellationToken cancellationToken)
    {
        _tagService.Changed -= OnTagChanged;
        _deviceStateService.Changed -= OnDeviceStateChanged;

        return Task.CompletedTask;
    }

    private async void OnTagChanged(TagValue tag)
    {
        try
        {
            await _hubContext.Clients.All.SendAsync(
                RuntimeHubContract.TagChanged,
                RuntimeContractMapper.ToDto(
                    tag,
                    _configuration.IsTagWritable(tag.TagId)));
        }
        catch (Exception exception)
        {
            _logger.LogWarning(
                exception,
                "Failed to publish tag update for {TagId}.",
                tag.TagId);
        }
    }

    private async void OnDeviceStateChanged(DeviceRuntimeState state)
    {
        try
        {
            await _hubContext.Clients.All.SendAsync(
                RuntimeHubContract.DeviceStateChanged,
                RuntimeContractMapper.ToDto(state));
        }
        catch (Exception exception)
        {
            _logger.LogWarning(
                exception,
                "Failed to publish device state update for {DeviceId}.",
                state.DeviceId);
        }
    }
}
