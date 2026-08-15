using Dispatcher.Server.Configuration;
using Microsoft.AspNetCore.Identity;
using Microsoft.Extensions.Options;

namespace Dispatcher.Server.Security;

public sealed class LocalUserBootstrapper
{
    public const int MinimumPasswordLength = 12;
    public const int MaximumPasswordLength = 256;

    private readonly SqliteConfigurationStore _store;
    private readonly IConfiguration _configuration;
    private readonly ILogger<LocalUserBootstrapper> _logger;
    private readonly PasswordHasher<LocalUserConfiguration> _passwordHasher;

    public LocalUserBootstrapper(
        SqliteConfigurationStore store,
        IConfiguration configuration,
        ILogger<LocalUserBootstrapper> logger)
    {
        _store =
            store;
        _configuration =
            configuration;
        _logger =
            logger;
        _passwordHasher =
            new PasswordHasher<LocalUserConfiguration>(
                Options.Create(
                    new PasswordHasherOptions()));
    }

    public async Task<bool> EnsureBootstrapAdministratorAsync(
        CancellationToken cancellationToken = default)
    {
        var existingUsers =
            await _store.LoadLocalUsersAsync(
                cancellationToken);

        if (existingUsers.Count > 0)
        {
            return false;
        }

        var section =
            _configuration.GetSection(
                "Authentication:BootstrapAdministrator");

        var password =
            section["Password"];

        if (string.IsNullOrWhiteSpace(
                password))
        {
            _logger.LogWarning(
                "No local users exist. Bootstrap administrator was not created because Authentication:BootstrapAdministrator:Password is empty. Configure the password through a secret or environment variable for the first bootstrap start.");

            return false;
        }

        if (password.Length is < MinimumPasswordLength or > MaximumPasswordLength)
        {
            throw new InvalidOperationException(
                $"Authentication:BootstrapAdministrator:Password must contain between {MinimumPasswordLength} and {MaximumPasswordLength} characters.");
        }

        var userName =
            section["UserName"]?.Trim();

        if (string.IsNullOrWhiteSpace(
                userName))
        {
            userName =
                "admin";
        }

        var displayName =
            section["DisplayName"]?.Trim();

        if (string.IsNullOrWhiteSpace(
                displayName))
        {
            displayName =
                "Administrator";
        }

        var user =
            new LocalUserConfiguration(
                UserId:
                    Guid.NewGuid().ToString("N"),
                UserName:
                    userName,
                NormalizedUserName:
                    LocalUserConfiguration.NormalizeUserName(
                        userName),
                DisplayName:
                    displayName,
                Enabled:
                    true,
                PasswordHash:
                    "pending");

        user =
            user with
            {
                PasswordHash =
                    _passwordHasher.HashPassword(
                        user,
                        password)
            };

        LocalUserConfigurationValidator.Validate(
            user);

        await _store.InsertLocalUserAsync(
            user,
            cancellationToken);

        _logger.LogInformation(
            "Created initial local bootstrap administrator {UserName} ({UserId}). Remove the bootstrap password from configuration after confirming the account exists.",
            user.UserName,
            user.UserId);

        return true;
    }
}
