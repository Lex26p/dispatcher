using Dispatcher.Contracts.Authorization;

namespace Dispatcher.Server.Security;

public sealed class SecurityCatalog
{
    private readonly object _gate = new();
    private IReadOnlyDictionary<string, UserAccess> _users =
        new Dictionary<string, UserAccess>(
            StringComparer.Ordinal);

    public void ReplaceAll(
        IReadOnlyCollection<LocalUserConfiguration> users,
        IReadOnlyCollection<SecurityRoleConfiguration> roles,
        IReadOnlyCollection<UserRoleAssignment> assignments)
    {
        ArgumentNullException.ThrowIfNull(users);
        ArgumentNullException.ThrowIfNull(roles);
        ArgumentNullException.ThrowIfNull(assignments);

        var usersById =
            users.ToDictionary(
                user => user.UserId,
                StringComparer.Ordinal);

        var rolesById =
            roles.ToDictionary(
                role => role.RoleId,
                StringComparer.Ordinal);

        foreach (var role in roles)
        {
            SecurityRoleConfigurationValidator.Validate(
                role);
        }

        var roleIdsByUser =
            users.ToDictionary(
                user => user.UserId,
                _ => new HashSet<string>(
                    StringComparer.Ordinal),
                StringComparer.Ordinal);

        foreach (var assignment in assignments)
        {
            if (!usersById.ContainsKey(
                    assignment.UserId))
            {
                throw new InvalidOperationException(
                    $"Security role assignment references unknown user '{assignment.UserId}'.");
            }

            if (!rolesById.ContainsKey(
                    assignment.RoleId))
            {
                throw new InvalidOperationException(
                    $"Security role assignment references unknown role '{assignment.RoleId}'.");
            }

            roleIdsByUser[
                assignment.UserId]
                .Add(
                    assignment.RoleId);
        }

        var next =
            new Dictionary<string, UserAccess>(
                StringComparer.Ordinal);

        foreach (var user in users)
        {
            var roleIds =
                roleIdsByUser[
                    user.UserId]
                    .OrderBy(
                        roleId => roleId,
                        StringComparer.Ordinal)
                    .ToArray();

            var permissions =
                roleIds
                    .SelectMany(
                        roleId =>
                            rolesById[
                                roleId]
                                .Permissions)
                    .Distinct(
                        StringComparer.Ordinal)
                    .OrderBy(
                        permission => permission,
                        StringComparer.Ordinal)
                    .ToArray();

            next[
                user.UserId] =
                new UserAccess(
                    user.Enabled,
                    roleIds,
                    permissions);
        }

        lock (_gate)
        {
            _users =
                next;
        }
    }

    public bool IsUserEnabled(string userId)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            userId);

        lock (_gate)
        {
            return _users.TryGetValue(
                    userId,
                    out var access)
                && access.Enabled;
        }
    }

    public bool HasPermission(
        string userId,
        string permission)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            userId);
        ArgumentException.ThrowIfNullOrWhiteSpace(
            permission);

        if (!PermissionNames.IsKnown(
                permission))
        {
            throw new ArgumentException(
                $"Unknown permission '{permission}'.",
                nameof(permission));
        }

        lock (_gate)
        {
            return _users.TryGetValue(
                    userId,
                    out var access)
                && access.Enabled
                && access.Permissions.Contains(
                    permission,
                    StringComparer.Ordinal);
        }
    }

    public IReadOnlyList<string> GetEffectivePermissions(
        string userId)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            userId);

        lock (_gate)
        {
            if (!_users.TryGetValue(
                    userId,
                    out var access)
                || !access.Enabled)
            {
                return [];
            }

            return access.Permissions.ToArray();
        }
    }

    public IReadOnlyList<string> GetRoleIds(
        string userId)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            userId);

        lock (_gate)
        {
            if (!_users.TryGetValue(
                    userId,
                    out var access))
            {
                return [];
            }

            return access.RoleIds.ToArray();
        }
    }

    private sealed record UserAccess(
        bool Enabled,
        string[] RoleIds,
        string[] Permissions);
}
