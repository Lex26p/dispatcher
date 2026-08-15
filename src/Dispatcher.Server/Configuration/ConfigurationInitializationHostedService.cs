using Dispatcher.Server.Historian;
using Dispatcher.Server.Security;

namespace Dispatcher.Server.Configuration;

public sealed class ConfigurationInitializationHostedService
    : IHostedService
{
    private readonly SqliteConfigurationStore _store;
    private readonly ConfigurationCatalog _catalog;
    private readonly HistorianPolicyCatalog _historianPolicies;
    private readonly IConfiguration _configuration;
    private readonly ILogger<ConfigurationInitializationHostedService> _logger;
    private readonly ILogger<LocalUserBootstrapper> _bootstrapLogger;

    public ConfigurationInitializationHostedService(
        SqliteConfigurationStore store,
        ConfigurationCatalog catalog,
        HistorianPolicyCatalog historianPolicies,
        IConfiguration configuration,
        ILogger<ConfigurationInitializationHostedService> logger,
        ILogger<LocalUserBootstrapper> bootstrapLogger)
    {
        _store =
            store;
        _catalog =
            catalog;
        _historianPolicies =
            historianPolicies;
        _configuration =
            configuration;
        _logger =
            logger;
        _bootstrapLogger =
            bootstrapLogger;
    }

    public async Task StartAsync(CancellationToken cancellationToken)
    {
        await _store.InitializeAsync(cancellationToken);

        var bootstrapper =
            new LocalUserBootstrapper(
                _store,
                _configuration,
                _bootstrapLogger);

        await bootstrapper.EnsureBootstrapAdministratorAsync(
            cancellationToken);

        var modbusDevices = await _store.LoadAsync(
            cancellationToken);
        var snmpDevices = await _store.LoadSnmpAsync(
            cancellationToken);
        var historianPolicies = await _store.LoadHistorianPoliciesAsync(
            cancellationToken);
        var localUsers = await _store.LoadLocalUsersAsync(
            cancellationToken);

        _catalog.ReplaceAll(
            modbusDevices,
            snmpDevices);

        _historianPolicies.ReplaceAll(
            historianPolicies);

        _logger.LogInformation(
            "Loaded {ModbusDeviceCount} Modbus device(s), {ModbusTagCount} Modbus tag(s), {SnmpDeviceCount} SNMP device(s), {SnmpTagCount} SNMP tag(s), {HistorianPolicyCount} historian policy/policies, and {LocalUserCount} local user(s) from configuration database {DatabasePath}.",
            modbusDevices.Count,
            modbusDevices.Sum(device => device.Tags.Count),
            snmpDevices.Count,
            snmpDevices.Sum(device => device.Tags.Count),
            historianPolicies.Count,
            localUsers.Count,
            _store.DatabasePath);
    }

    public Task StopAsync(CancellationToken cancellationToken)
    {
        return Task.CompletedTask;
    }
}
