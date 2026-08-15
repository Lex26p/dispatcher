namespace Dispatcher.Server.Security;

public sealed record LocalUserConfiguration(
    string UserId,
    string UserName,
    string NormalizedUserName,
    string DisplayName,
    bool Enabled,
    string PasswordHash)
{
    public static string NormalizeUserName(
        string userName)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            userName);

        return userName
            .Trim()
            .ToUpperInvariant();
    }
}
