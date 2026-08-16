namespace Dispatcher.Server.Security;

public sealed record SecurityRoleConfiguration(
    string RoleId,
    string Name,
    string NormalizedName,
    bool BuiltIn,
    IReadOnlyList<string> Permissions)
{
    public static string NormalizeName(string name)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(name);

        return name.Trim().ToUpperInvariant();
    }
}
