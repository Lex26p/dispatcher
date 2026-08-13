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

    public async Task StartAsync(
        CancellationToken cancellationToken)
    {
        await _store.InitializeAsync(
            cancellationToken);

        var devices =
            await _store.LoadAsync(
                cancellationToken);

        _catalog.Replace(devices);

        _logger.LogInformation(
            "Loaded {DeviceCount} device(s) and {TagCount} tag(s) from configuration database {DatabasePath}.",
            devices.Count,
            devices.Sum(device => device.Tags.Count),
            _store.DatabasePath);
    }

    public Task StopAsync(
        CancellationToken cancellationToken)
    {
        return Task.CompletedTask;
    }
}
