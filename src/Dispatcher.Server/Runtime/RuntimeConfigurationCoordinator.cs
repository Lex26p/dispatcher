using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;
using Dispatcher.Server.Configuration;

namespace Dispatcher.Server.Runtime;

public sealed class RuntimeConfigurationCoordinator
{
    private readonly ConfigurationCatalog _configuration;
    private readonly ModbusRuntimeHostedService _modbusRuntime;
    private readonly SnmpRuntimeHostedService _snmpRuntime;
    private readonly TagService _tagService;
    private readonly DeviceStateService _deviceStateService;
    private readonly SemaphoreSlim _applyLock = new(1, 1);

    public RuntimeConfigurationCoordinator(
        ConfigurationCatalog configuration,
        ModbusRuntimeHostedService modbusRuntime,
        SnmpRuntimeHostedService snmpRuntime,
        TagService tagService,
        DeviceStateService deviceStateService)
    {
        _configuration = configuration;
        _modbusRuntime = modbusRuntime;
        _snmpRuntime = snmpRuntime;
        _tagService = tagService;
        _deviceStateService = deviceStateService;
    }

    public async Task ApplyAsync(
        CancellationToken cancellationToken)
    {
        await _applyLock.WaitAsync(
            cancellationToken);

        try
        {
            await _modbusRuntime.StopPollingAsync(
                cancellationToken);

            await _snmpRuntime.StopPollingAsync(
                cancellationToken);

            _tagService.Clear();
            _deviceStateService.Clear();

            await _modbusRuntime.StartPollingAsync(
                _configuration.ModbusDevices,
                cancellationToken);

            await _snmpRuntime.StartPollingAsync(
                _configuration.SnmpDevices,
                cancellationToken);
        }
        finally
        {
            _applyLock.Release();
        }
    }
}
