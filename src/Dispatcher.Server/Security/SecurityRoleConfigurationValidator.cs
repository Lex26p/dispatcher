using Dispatcher.Contracts.Authorization;

namespace Dispatcher.Server.Security;

public static class SecurityRoleConfigurationValidator
{
    public const int MaximumRoleIdLength = 64;
    public const int MaximumNameLength = 128;

    public static void Validate(SecurityRoleConfiguration role)
    {
        ArgumentNullException.ThrowIfNull(role);

        ValidateRequired(
            role.RoleId,
            nameof(role.RoleId),
            MaximumRoleIdLength);

        ValidateRequired(
            role.Name,
            nameof(role.Name),
            MaximumNameLength);

        if (!string.Equals(
                role.RoleId,
                role.RoleId.Trim(),
                StringComparison.Ordinal))
        {
            throw new InvalidOperationException(
                "RoleId cannot contain leading or trailing whitespace.");
        }

        if (!string.Equals(
                role.Name,
                role.Name.Trim(),
                StringComparison.Ordinal))
        {
            throw new InvalidOperationException(
                "Role name cannot contain leading or trailing whitespace.");
        }

        var expectedNormalizedName =
            SecurityRoleConfiguration.NormalizeName(
                role.Name);

        if (!string.Equals(
                role.NormalizedName,
                expectedNormalizedName,
                StringComparison.Ordinal))
        {
            throw new InvalidOperationException(
                $"Role '{role.RoleId}' has invalid normalized name.");
        }

        if (role.Permissions is null)
        {
            throw new InvalidOperationException(
                $"Role '{role.RoleId}' permissions are required.");
        }

        var permissions =
            new HashSet<string>(
                StringComparer.Ordinal);

        foreach (var permission in role.Permissions)
        {
            if (string.IsNullOrWhiteSpace(permission)
                || !PermissionNames.IsKnown(permission))
            {
                throw new InvalidOperationException(
                    $"Role '{role.RoleId}' contains unknown permission '{permission}'.");
            }

            if (!permissions.Add(permission))
            {
                throw new InvalidOperationException(
                    $"Role '{role.RoleId}' contains duplicate permission '{permission}'.");
            }
        }
    }

    private static void ValidateRequired(
        string value,
        string name,
        int maximumLength)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            throw new InvalidOperationException(
                $"{name} is required.");
        }

        if (value.Length > maximumLength)
        {
            throw new InvalidOperationException(
                $"{name} cannot exceed {maximumLength} characters.");
        }
    }
}
