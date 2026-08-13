using Dispatcher.Server.Configuration;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.Extensions.Configuration;

namespace Dispatcher.Server.Tests;

internal sealed class TestConfigurationDatabase : IDisposable
{
    private readonly string _directory;

    private TestConfigurationDatabase(
        string directory,
        string databasePath)
    {
        _directory = directory;
        DatabasePath = databasePath;
    }

    public string DatabasePath { get; }

    public static async Task<TestConfigurationDatabase> CreateAsync(
        params ModbusDeviceConfiguration[] devices)
    {
        var directory =
            Path.Combine(
                Path.GetTempPath(),
                "dispatcher-tests",
                Guid.NewGuid().ToString("N"));

        Directory.CreateDirectory(directory);

        var databasePath =
            Path.Combine(
                directory,
                "dispatcher.db");

        var database =
            new TestConfigurationDatabase(
                directory,
                databasePath);

        var store =
            new SqliteConfigurationStore(
                databasePath);

        await store.InitializeAsync();
        await store.ReplaceAsync(devices);

        return database;
    }

    public void Dispose()
    {
        if (Directory.Exists(_directory))
        {
            Directory.Delete(
                _directory,
                recursive: true);
        }
    }
}

internal static class TestDispatcherFactory
{
    public static WebApplicationFactory<Program> Create(
        string databasePath)
    {
        return new WebApplicationFactory<Program>()
            .WithWebHostBuilder(builder =>
            {
                builder.ConfigureAppConfiguration(
                    (_, configuration) =>
                    {
                        configuration.AddInMemoryCollection(
                            new Dictionary<string, string?>
                            {
                                ["ConfigurationDatabase:Path"] =
                                    databasePath
                            });
                    });
            });
    }
}

internal static class TestModbusConfiguration
{
    public static ModbusDeviceConfiguration CreateDevice(
        int port,
        bool enabled = true,
        bool writable = false)
    {
        return new ModbusDeviceConfiguration(
            DeviceId: "device01",
            Name: "Test device",
            Enabled: enabled,
            Host: "127.0.0.1",
            Port: port,
            UnitId: 1,
            PollIntervalMilliseconds: 10000,
            RequestTimeoutMilliseconds: 1000,
            Tags:
            [
                new ModbusTagConfiguration(
                    TagId: "device01.register100",
                    Name: "Register 100",
                    Address: 100,
                    Writable: writable)
            ]);
    }
}
