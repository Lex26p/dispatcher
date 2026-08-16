using System.Security.Claims;
using System.Text.Encodings.Web;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Security;
using Microsoft.AspNetCore.Authentication;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.AspNetCore.TestHost;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Options;

namespace Dispatcher.Server.Tests;

internal sealed class TestConfigurationDatabase : IDisposable
{
    private readonly string _directory;

    private TestConfigurationDatabase(
        string directory,
        string databasePath)
    {
        _directory =
            directory;
        DatabasePath =
            databasePath;
    }

    public string DatabasePath { get; }

    public static Task<TestConfigurationDatabase> CreateAsync(
        params ModbusDeviceConfiguration[] devices)
    {
        return CreateAsync(
            devices,
            Array.Empty<SnmpDeviceConfiguration>());
    }

    public static async Task<TestConfigurationDatabase> CreateAsync(
        IReadOnlyCollection<ModbusDeviceConfiguration> modbusDevices,
        IReadOnlyCollection<SnmpDeviceConfiguration> snmpDevices)
    {
        var directory =
            Path.Combine(
                Path.GetTempPath(),
                "dispatcher-tests",
                Guid.NewGuid().ToString(
                    "N"));

        Directory.CreateDirectory(
            directory);

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
        await store.ReplaceAsync(
            modbusDevices);
        await store.ReplaceSnmpAsync(
            snmpDevices);

        return database;
    }

    public void Dispose()
    {
        if (Directory.Exists(
                _directory))
        {
            Directory.Delete(
                _directory,
                recursive: true);
        }
    }
}

internal static class TestDispatcherFactory
{
    public const string TestAdministratorUserId =
        "dispatcher-tests-administrator";

    private const string TestAuthenticationScheme =
        "Dispatcher.Tests";

    public static WebApplicationFactory<Program> Create(
        string databasePath,
        bool authenticateAsAdministrator = true)
    {
        if (authenticateAsAdministrator)
        {
            EnsureTestAdministrator(
                databasePath);
        }

        var operationalDatabasePath =
            GetOperationalDatabasePath(
                databasePath);

        return new WebApplicationFactory<Program>()
            .WithWebHostBuilder(
                builder =>
                {
                    builder.ConfigureAppConfiguration(
                        (_, configuration) =>
                        {
                            configuration.AddInMemoryCollection(
                                new Dictionary<string, string?>
                                {
                                    ["ConfigurationDatabase:Path"] =
                                        databasePath,
                                    ["OperationalDatabase:Path"] =
                                        operationalDatabasePath
                                });
                        });

                    if (authenticateAsAdministrator)
                    {
                        builder.ConfigureTestServices(
                            services =>
                            {
                                services
                                    .AddAuthentication(
                                        options =>
                                        {
                                            options.DefaultAuthenticateScheme =
                                                TestAuthenticationScheme;
                                            options.DefaultChallengeScheme =
                                                TestAuthenticationScheme;
                                            options.DefaultForbidScheme =
                                                TestAuthenticationScheme;
                                        })
                                    .AddScheme<AuthenticationSchemeOptions, TestAdministratorAuthenticationHandler>(
                                        TestAuthenticationScheme,
                                        _ =>
                                        {
                                        });
                            });
                    }
                });
    }

    public static string GetOperationalDatabasePath(
        string configurationDatabasePath)
    {
        var directory =
            Path.GetDirectoryName(
                Path.GetFullPath(
                    configurationDatabasePath))
            ?? throw new InvalidOperationException(
                "Configuration database directory could not be resolved.");

        return Path.Combine(
            directory,
            "dispatcher-operational.db");
    }

    private static void EnsureTestAdministrator(
        string databasePath)
    {
        var store =
            new SqliteConfigurationStore(
                databasePath);

        var users =
            store.LoadLocalUsersAsync()
                .GetAwaiter()
                .GetResult();

        if (users.Any(
                user =>
                    string.Equals(
                        user.UserId,
                        TestAdministratorUserId,
                        StringComparison.Ordinal)))
        {
            return;
        }

        var user =
            new LocalUserConfiguration(
                UserId:
                    TestAdministratorUserId,
                UserName:
                    "dispatcher.tests.admin",
                NormalizedUserName:
                    LocalUserConfiguration.NormalizeUserName(
                        "dispatcher.tests.admin"),
                DisplayName:
                    "Dispatcher Tests Administrator",
                Enabled:
                    true,
                PasswordHash:
                    "test-host-authentication-does-not-use-password");

        store.InsertLocalUserWithRoleAsync(
                user,
                BuiltInSecurityRoles.All.Single(
                    role =>
                        role.RoleId
                        == BuiltInSecurityRoles.AdministratorRoleId))
            .GetAwaiter()
            .GetResult();
    }

    private sealed class TestAdministratorAuthenticationHandler
        : AuthenticationHandler<AuthenticationSchemeOptions>
    {
        public TestAdministratorAuthenticationHandler(
            IOptionsMonitor<AuthenticationSchemeOptions> options,
            ILoggerFactory logger,
            UrlEncoder encoder)
            : base(
                options,
                logger,
                encoder)
        {
        }

        protected override Task<AuthenticateResult> HandleAuthenticateAsync()
        {
            var identity =
                new ClaimsIdentity(
                    [
                        new Claim(
                            ClaimTypes.NameIdentifier,
                            TestAdministratorUserId),
                        new Claim(
                            ClaimTypes.Name,
                            "dispatcher.tests.admin")
                    ],
                    TestAuthenticationScheme);

            var principal =
                new ClaimsPrincipal(
                    identity);
            var ticket =
                new AuthenticationTicket(
                    principal,
                    TestAuthenticationScheme);

            return Task.FromResult(
                AuthenticateResult.Success(
                    ticket));
        }
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
            DeviceId:
                "device01",
            Name:
                "Test device",
            Enabled:
                enabled,
            Host:
                "127.0.0.1",
            Port:
                port,
            UnitId:
                1,
            PollIntervalMilliseconds:
                10000,
            RequestTimeoutMilliseconds:
                1000,
            Tags:
            [
                new ModbusTagConfiguration(
                    TagId:
                        "device01.register100",
                    Name:
                        "Register 100",
                    Address:
                        100,
                    Writable:
                        writable)
            ]);
    }
}

internal static class TestSnmpConfiguration
{
    public static SnmpDeviceConfiguration CreateDevice(
        int port,
        bool enabled = true)
    {
        return new SnmpDeviceConfiguration(
            DeviceId:
                "snmp01",
            Name:
                "Test SNMP device",
            Enabled:
                enabled,
            Host:
                "127.0.0.1",
            Port:
                port,
            Community:
                "public",
            PollIntervalMilliseconds:
                10000,
            RequestTimeoutMilliseconds:
                1000,
            Tags:
            [
                new SnmpTagConfiguration(
                    TagId:
                        "snmp01.sysName",
                    Name:
                        "sysName",
                    Oid:
                        "1.3.6.1.2.1.1.5.0")
            ]);
    }
}
