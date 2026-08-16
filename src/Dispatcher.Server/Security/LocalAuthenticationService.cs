using Dispatcher.Server.Configuration;
using Microsoft.AspNetCore.Identity;
using Microsoft.Extensions.Options;

namespace Dispatcher.Server.Security;

public sealed class LocalAuthenticationService
{
    private readonly SqliteConfigurationStore _store;
    private readonly PasswordHasher<LocalUserConfiguration> _passwordHasher;
    private readonly LocalUserConfiguration _dummyUser;
    private readonly string _dummyPasswordHash;

    public LocalAuthenticationService(
        SqliteConfigurationStore store)
    {
        _store =
            store;

        _passwordHasher =
            new PasswordHasher<LocalUserConfiguration>(
                Options.Create(
                    new PasswordHasherOptions()));

        _dummyUser =
            new LocalUserConfiguration(
                UserId:
                    "authentication-dummy",
                UserName:
                    "dummy",
                NormalizedUserName:
                    "DUMMY",
                DisplayName:
                    "Dummy",
                Enabled:
                    false,
                PasswordHash:
                    "pending");

        _dummyPasswordHash =
            _passwordHasher.HashPassword(
                _dummyUser,
                Guid.NewGuid().ToString("N"));
    }

    public async Task<LocalUserConfiguration?> AuthenticateAsync(
        string? userName,
        string? password,
        CancellationToken cancellationToken = default)
    {
        var providedPassword =
            password
            ?? string.Empty;

        var trimmedUserName =
            userName?.Trim();

        if (string.IsNullOrWhiteSpace(
                trimmedUserName)
            || trimmedUserName.Length
                > LocalUserConfigurationValidator.MaxUserNameLength
            || providedPassword.Length == 0
            || providedPassword.Length
                > LocalUserBootstrapper.MaximumPasswordLength)
        {
            VerifyDummyPassword(
                providedPassword);

            return null;
        }

        var normalizedUserName =
            LocalUserConfiguration.NormalizeUserName(
                trimmedUserName);

        var user =
            await _store.FindLocalUserByNormalizedUserNameAsync(
                normalizedUserName,
                cancellationToken);

        if (user is null)
        {
            VerifyDummyPassword(
                providedPassword);

            return null;
        }

        var verification =
            _passwordHasher.VerifyHashedPassword(
                user,
                user.PasswordHash,
                providedPassword);

        if (verification == PasswordVerificationResult.Failed
            || !user.Enabled)
        {
            return null;
        }

        return user;
    }

    private void VerifyDummyPassword(
        string password)
    {
        _passwordHasher.VerifyHashedPassword(
            _dummyUser,
            _dummyPasswordHash,
            password);
    }
}
