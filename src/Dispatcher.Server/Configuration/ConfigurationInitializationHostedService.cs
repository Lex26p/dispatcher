namespace Dispatcher.Server.Configuration;

public sealed class ConfigurationInitializationHostedService
    : IHostedService
{
    private readonly SqliteConfigurationStore _store;
    private readonly ConfigurationCatalog _catalog;
    private readonly ILogger<ConfigurationInitializationHostedService> _logger;

    public ConfigurationInitializationHostedService(
        SqliteConfigurationStore store,
        ConfigurationCatalog catalog,
        ILogger<ConfigurationInitializationHostedService> logger)
    {
        _store = store;
        _catalog = catalog;
        _logger = logger;
    }

    public async Task StartAsync(CancellationToken cancellationToken)
    {
        await _store.InitializeAsync(cancellationToken);

        var modbusDevices = await _store.LoadAsync(
            cancellationToken);
        var snmpDevices = await _store.LoadSnmpAsync(
            cancellationToken);

        _catalog.ReplaceAll(
            modbusDevices,
            snmpDevices);

        _logger.LogInformation(
            "Loaded {ModbusDeviceCount} Modbus device(s), {ModbusTagCount} Modbus tag(s), {SnmpDeviceCount} SNMP device(s), and {SnmpTagCount} SNMP tag(s) from configuration database {DatabasePath}.",
            modbusDevices.Count,
            modbusDevices.Sum(device => device.Tags.Count),
            snmpDevices.Count,
            snmpDevices.Sum(device => device.Tags.Count),
            _store.DatabasePath);
    }

    public Task StopAsync(CancellationToken cancellationToken)
    {
        return Task.CompletedTask;
    }
}
