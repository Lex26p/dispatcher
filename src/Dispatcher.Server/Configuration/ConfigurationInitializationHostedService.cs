using Dispatcher.Server.Historian;

namespace Dispatcher.Server.Configuration;

public sealed class ConfigurationInitializationHostedService
    : IHostedService
{
    private readonly SqliteConfigurationStore _store;
    private readonly ConfigurationCatalog _catalog;
    private readonly HistorianPolicyCatalog _historianPolicies;
    private readonly ILogger<ConfigurationInitializationHostedService> _logger;

    public ConfigurationInitializationHostedService(
        SqliteConfigurationStore store,
        ConfigurationCatalog catalog,
        HistorianPolicyCatalog historianPolicies,
        ILogger<ConfigurationInitializationHostedService> logger)
    {
        _store =
            store;
        _catalog =
            catalog;
        _historianPolicies =
            historianPolicies;
        _logger =
            logger;
    }

    public async Task StartAsync(CancellationToken cancellationToken)
    {
        await _store.InitializeAsync(cancellationToken);

        var modbusDevices = await _store.LoadAsync(
            cancellationToken);
        var snmpDevices = await _store.LoadSnmpAsync(
            cancellationToken);
        var historianPolicies = await _store.LoadHistorianPoliciesAsync(
            cancellationToken);

        _catalog.ReplaceAll(
            modbusDevices,
            snmpDevices);

        _historianPolicies.ReplaceAll(
            historianPolicies);

        _logger.LogInformation(
            "Loaded {ModbusDeviceCount} Modbus device(s), {ModbusTagCount} Modbus tag(s), {SnmpDeviceCount} SNMP device(s), {SnmpTagCount} SNMP tag(s), and {HistorianPolicyCount} historian policy/policies from configuration database {DatabasePath}.",
            modbusDevices.Count,
            modbusDevices.Sum(device => device.Tags.Count),
            snmpDevices.Count,
            snmpDevices.Sum(device => device.Tags.Count),
            historianPolicies.Count,
            _store.DatabasePath);
    }

    public Task StopAsync(CancellationToken cancellationToken)
    {
        return Task.CompletedTask;
    }
}
