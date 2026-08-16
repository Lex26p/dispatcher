using Dispatcher.Contracts.Authorization;
using Dispatcher.Contracts.Security;
using Dispatcher.Server.Configuration;
using Microsoft.AspNetCore.Identity;
using Microsoft.Extensions.Options;

namespace Dispatcher.Server.Security;

public sealed class SecurityManagementService
{
    private readonly SqliteConfigurationStore _store;
    private readonly SecurityCatalog _securityCatalog;
    private readonly PasswordHasher<LocalUserConfiguration> _passwordHasher;
    private readonly SemaphoreSlim _mutationGate =
        new(1, 1);

    public SecurityManagementService(
        SqliteConfigurationStore store,
        SecurityCatalog securityCatalog)
    {
        _store =
            store;
        _securityCatalog =
            securityCatalog;
        _passwordHasher =
            new PasswordHasher<LocalUserConfiguration>(
                Options.Create(
                    new PasswordHasherOptions()));
    }

    public async Task<IReadOnlyList<SecurityUserDto>> GetUsersAsync(
        CancellationToken cancellationToken = default)
    {
        var state =
            await LoadStateAsync(
                cancellationToken);

        return state.Users
            .Select(user =>
                ToUserDto(
                    user,
                    state.Assignments,
                    state.Catalog))
            .ToArray();
    }

    public async Task<IReadOnlyList<SecurityRoleDto>> GetRolesAsync(
        CancellationToken cancellationToken = default)
    {
        var state =
            await LoadStateAsync(
                cancellationToken);

        return state.Roles
            .Select(role =>
                ToRoleDto(
                    role,
                    state.Assignments))
            .ToArray();
    }

    public async Task<SecurityUserDto> CreateUserAsync(
        CreateSecurityUserRequest request,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(
            request);

        await _mutationGate.WaitAsync(
            cancellationToken);

        try
        {
            var state =
                await LoadStateAsync(
                    cancellationToken);
            var userName =
                NormalizeRequiredText(
                    request.UserName,
                    nameof(request.UserName));
            var displayName =
                NormalizeRequiredText(
                    request.DisplayName,
                    nameof(request.DisplayName));

            ValidatePassword(
                request.Password);

            var normalizedUserName =
                LocalUserConfiguration.NormalizeUserName(
                    userName);

            if (state.Users.Any(user =>
                    string.Equals(
                        user.NormalizedUserName,
                        normalizedUserName,
                        StringComparison.Ordinal)))
            {
                throw new SecurityManagementConflictException(
                    $"User name '{userName}' already exists.");
            }

            var user =
                new LocalUserConfiguration(
                    UserId:
                        Guid.NewGuid().ToString("N"),
                    UserName:
                        userName,
                    NormalizedUserName:
                        normalizedUserName,
                    DisplayName:
                        displayName,
                    Enabled:
                        request.Enabled,
                    PasswordHash:
                        "pending");

            user =
                user with
                {
                    PasswordHash =
                        _passwordHasher.HashPassword(
                            user,
                            request.Password)
                };

            LocalUserConfigurationValidator.Validate(
                user);

            await _store.InsertLocalUserAsync(
                user,
                cancellationToken);

            var refreshed =
                await RefreshCatalogAsync(
                    cancellationToken);

            return ToUserDto(
                refreshed.Users.Single(candidate =>
                    string.Equals(
                        candidate.UserId,
                        user.UserId,
                        StringComparison.Ordinal)),
                refreshed.Assignments,
                refreshed.Catalog);
        }
        finally
        {
            _mutationGate.Release();
        }
    }

    public async Task<SecurityUserDto> UpdateUserAsync(
        string userId,
        UpdateSecurityUserRequest request,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            userId);
        ArgumentNullException.ThrowIfNull(
            request);

        await _mutationGate.WaitAsync(
            cancellationToken);

        try
        {
            var state =
                await LoadStateAsync(
                    cancellationToken);
            var current =
                FindUser(
                    state.Users,
                    userId);
            var updated =
                current with
                {
                    DisplayName =
                        NormalizeRequiredText(
                            request.DisplayName,
                            nameof(request.DisplayName)),
                    Enabled =
                        request.Enabled
                };

            LocalUserConfigurationValidator.Validate(
                updated);

            var proposedUsers =
                state.Users
                    .Select(user =>
                        string.Equals(
                            user.UserId,
                            updated.UserId,
                            StringComparison.Ordinal)
                            ? updated
                            : user)
                    .ToArray();

            if (current.Enabled
                && !updated.Enabled)
            {
                EnsureManagementAuthorityRemains(
                    proposedUsers,
                    state.Roles,
                    state.Assignments);
            }

            if (!await _store.UpdateLocalUserAsync(
                    updated,
                    cancellationToken))
            {
                throw new SecurityManagementNotFoundException(
                    $"User '{userId}' was not found.");
            }

            var refreshed =
                await RefreshCatalogAsync(
                    cancellationToken);

            return ToUserDto(
                refreshed.Users.Single(user =>
                    string.Equals(
                        user.UserId,
                        userId,
                        StringComparison.Ordinal)),
                refreshed.Assignments,
                refreshed.Catalog);
        }
        finally
        {
            _mutationGate.Release();
        }
    }

    public async Task ResetUserPasswordAsync(
        string userId,
        ResetSecurityUserPasswordRequest request,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            userId);
        ArgumentNullException.ThrowIfNull(
            request);

        ValidatePassword(
            request.Password);

        await _mutationGate.WaitAsync(
            cancellationToken);

        try
        {
            var state =
                await LoadStateAsync(
                    cancellationToken);
            var current =
                FindUser(
                    state.Users,
                    userId);
            var updated =
                current with
                {
                    PasswordHash =
                        _passwordHasher.HashPassword(
                            current,
                            request.Password)
                };

            if (!await _store.UpdateLocalUserAsync(
                    updated,
                    cancellationToken))
            {
                throw new SecurityManagementNotFoundException(
                    $"User '{userId}' was not found.");
            }
        }
        finally
        {
            _mutationGate.Release();
        }
    }

    public async Task<SecurityUserDto> ReplaceUserRolesAsync(
        string userId,
        ReplaceSecurityUserRolesRequest request,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            userId);
        ArgumentNullException.ThrowIfNull(
            request);

        await _mutationGate.WaitAsync(
            cancellationToken);

        try
        {
            var state =
                await LoadStateAsync(
                    cancellationToken);
            var user =
                FindUser(
                    state.Users,
                    userId);
            var roleIds =
                ValidateRoleIds(
                    request.RoleIds,
                    state.Roles);
            var proposedAssignments =
                state.Assignments
                    .Where(assignment =>
                        !string.Equals(
                            assignment.UserId,
                            userId,
                            StringComparison.Ordinal))
                    .Concat(roleIds.Select(roleId =>
                        new UserRoleAssignment(
                            UserId:
                                userId,
                            RoleId:
                                roleId)))
                    .ToArray();

            var currentRoleIds =
                state.Assignments
                    .Where(assignment =>
                        string.Equals(
                            assignment.UserId,
                            userId,
                            StringComparison.Ordinal))
                    .Select(assignment =>
                        assignment.RoleId)
                    .ToHashSet(
                        StringComparer.Ordinal);

            if (currentRoleIds.Except(
                    roleIds,
                    StringComparer.Ordinal).Any())
            {
                EnsureManagementAuthorityRemains(
                    state.Users,
                    state.Roles,
                    proposedAssignments);
            }

            await _store.ReplaceUserRoleAssignmentsAsync(
                userId,
                roleIds,
                cancellationToken);

            var refreshed =
                await RefreshCatalogAsync(
                    cancellationToken);

            return ToUserDto(
                refreshed.Users.Single(candidate =>
                    string.Equals(
                        candidate.UserId,
                        user.UserId,
                        StringComparison.Ordinal)),
                refreshed.Assignments,
                refreshed.Catalog);
        }
        finally
        {
            _mutationGate.Release();
        }
    }

    public async Task<SecurityRoleDto> CreateRoleAsync(
        SecurityRoleUpsertRequest request,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(
            request);

        await _mutationGate.WaitAsync(
            cancellationToken);

        try
        {
            var state =
                await LoadStateAsync(
                    cancellationToken);
            var role =
                CreateCustomRole(
                    Guid.NewGuid().ToString("N"),
                    request);

            EnsureRoleNameAvailable(
                state.Roles,
                role.Name,
                exceptRoleId:
                    null);

            await _store.UpsertSecurityRoleAsync(
                role,
                cancellationToken);

            var refreshed =
                await RefreshCatalogAsync(
                    cancellationToken);

            return ToRoleDto(
                refreshed.Roles.Single(candidate =>
                    string.Equals(
                        candidate.RoleId,
                        role.RoleId,
                        StringComparison.Ordinal)),
                refreshed.Assignments);
        }
        finally
        {
            _mutationGate.Release();
        }
    }

    public async Task<SecurityRoleDto> UpdateRoleAsync(
        string roleId,
        SecurityRoleUpsertRequest request,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            roleId);
        ArgumentNullException.ThrowIfNull(
            request);

        await _mutationGate.WaitAsync(
            cancellationToken);

        try
        {
            var state =
                await LoadStateAsync(
                    cancellationToken);
            var current =
                FindRole(
                    state.Roles,
                    roleId);

            EnsureCustomRole(
                current);

            var updated =
                CreateCustomRole(
                    current.RoleId,
                    request);

            EnsureRoleNameAvailable(
                state.Roles,
                updated.Name,
                current.RoleId);

            var proposedRoles =
                state.Roles
                    .Select(role =>
                        string.Equals(
                            role.RoleId,
                            updated.RoleId,
                            StringComparison.Ordinal)
                            ? updated
                            : role)
                    .ToArray();

            if (current.Permissions.Except(
                    updated.Permissions,
                    StringComparer.Ordinal).Any())
            {
                EnsureManagementAuthorityRemains(
                    state.Users,
                    proposedRoles,
                    state.Assignments);
            }

            await _store.UpsertSecurityRoleAsync(
                updated,
                cancellationToken);

            var refreshed =
                await RefreshCatalogAsync(
                    cancellationToken);

            return ToRoleDto(
                refreshed.Roles.Single(role =>
                    string.Equals(
                        role.RoleId,
                        roleId,
                        StringComparison.Ordinal)),
                refreshed.Assignments);
        }
        finally
        {
            _mutationGate.Release();
        }
    }

    public async Task DeleteRoleAsync(
        string roleId,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            roleId);

        await _mutationGate.WaitAsync(
            cancellationToken);

        try
        {
            var state =
                await LoadStateAsync(
                    cancellationToken);
            var role =
                FindRole(
                    state.Roles,
                    roleId);

            EnsureCustomRole(
                role);

            if (state.Assignments.Any(assignment =>
                    string.Equals(
                        assignment.RoleId,
                        roleId,
                        StringComparison.Ordinal)))
            {
                throw new SecurityManagementConflictException(
                    $"Role '{role.Name}' is assigned to one or more users and cannot be deleted.");
            }

            if (!await _store.DeleteSecurityRoleAsync(
                    roleId,
                    cancellationToken))
            {
                throw new SecurityManagementNotFoundException(
                    $"Role '{roleId}' was not found.");
            }

            await RefreshCatalogAsync(
                cancellationToken);
        }
        finally
        {
            _mutationGate.Release();
        }
    }

    private async Task<SecurityState> RefreshCatalogAsync(
        CancellationToken cancellationToken)
    {
        var state =
            await LoadStateAsync(
                cancellationToken);

        _securityCatalog.ReplaceAll(
            state.Users,
            state.Roles,
            state.Assignments);

        return state with
        {
            Catalog =
                _securityCatalog
        };
    }

    private async Task<SecurityState> LoadStateAsync(
        CancellationToken cancellationToken)
    {
        var users =
            await _store.LoadLocalUsersAsync(
                cancellationToken);
        var roles =
            await _store.LoadSecurityRolesAsync(
                cancellationToken);
        var assignments =
            await _store.LoadUserRoleAssignmentsAsync(
                cancellationToken);
        var catalog =
            new SecurityCatalog();

        catalog.ReplaceAll(
            users,
            roles,
            assignments);

        return new SecurityState(
            users,
            roles,
            assignments,
            catalog);
    }

    private static SecurityUserDto ToUserDto(
        LocalUserConfiguration user,
        IReadOnlyCollection<UserRoleAssignment> assignments,
        SecurityCatalog catalog)
    {
        return new SecurityUserDto(
            UserId:
                user.UserId,
            UserName:
                user.UserName,
            DisplayName:
                user.DisplayName,
            Enabled:
                user.Enabled,
            RoleIds:
                assignments
                    .Where(assignment =>
                        string.Equals(
                            assignment.UserId,
                            user.UserId,
                            StringComparison.Ordinal))
                    .Select(assignment =>
                        assignment.RoleId)
                    .OrderBy(
                        roleId => roleId,
                        StringComparer.Ordinal)
                    .ToArray(),
            EffectivePermissions:
                catalog.GetEffectivePermissions(
                    user.UserId));
    }

    private static SecurityRoleDto ToRoleDto(
        SecurityRoleConfiguration role,
        IReadOnlyCollection<UserRoleAssignment> assignments)
    {
        return new SecurityRoleDto(
            RoleId:
                role.RoleId,
            Name:
                role.Name,
            BuiltIn:
                role.BuiltIn,
            Permissions:
                role.Permissions
                    .OrderBy(
                        permission => permission,
                        StringComparer.Ordinal)
                    .ToArray(),
            AssignedUserCount:
                assignments.Count(assignment =>
                    string.Equals(
                        assignment.RoleId,
                        role.RoleId,
                        StringComparison.Ordinal)));
    }

    private static LocalUserConfiguration FindUser(
        IReadOnlyCollection<LocalUserConfiguration> users,
        string userId)
    {
        return users.SingleOrDefault(user =>
            string.Equals(
                user.UserId,
                userId,
                StringComparison.Ordinal))
            ?? throw new SecurityManagementNotFoundException(
                $"User '{userId}' was not found.");
    }

    private static SecurityRoleConfiguration FindRole(
        IReadOnlyCollection<SecurityRoleConfiguration> roles,
        string roleId)
    {
        return roles.SingleOrDefault(role =>
            string.Equals(
                role.RoleId,
                roleId,
                StringComparison.Ordinal))
            ?? throw new SecurityManagementNotFoundException(
                $"Role '{roleId}' was not found.");
    }

    private static IReadOnlyList<string> ValidateRoleIds(
        IReadOnlyList<string>? roleIds,
        IReadOnlyCollection<SecurityRoleConfiguration> roles)
    {
        if (roleIds is null)
        {
            throw new ArgumentException(
                "RoleIds are required.",
                nameof(roleIds));
        }

        var distinct =
            new HashSet<string>(
                StringComparer.Ordinal);
        var result =
            new List<string>();

        foreach (var roleId in roleIds)
        {
            if (string.IsNullOrWhiteSpace(
                    roleId))
            {
                throw new ArgumentException(
                    "RoleIds cannot contain an empty value.",
                    nameof(roleIds));
            }

            if (!distinct.Add(
                    roleId))
            {
                throw new ArgumentException(
                    $"RoleIds contains duplicate value '{roleId}'.",
                    nameof(roleIds));
            }

            if (!roles.Any(role =>
                    string.Equals(
                        role.RoleId,
                        roleId,
                        StringComparison.Ordinal)))
            {
                throw new SecurityManagementNotFoundException(
                    $"Role '{roleId}' was not found.");
            }

            result.Add(
                roleId);
        }

        return result
            .OrderBy(
                roleId => roleId,
                StringComparer.Ordinal)
            .ToArray();
    }

    private static SecurityRoleConfiguration CreateCustomRole(
        string roleId,
        SecurityRoleUpsertRequest request)
    {
        var name =
            NormalizeRequiredText(
                request.Name,
                nameof(request.Name));

        if (request.Permissions is null)
        {
            throw new ArgumentException(
                "Permissions are required.",
                nameof(request.Permissions));
        }

        var permissions =
            request.Permissions
                .OrderBy(
                    permission => permission,
                    StringComparer.Ordinal)
                .ToArray();
        var role =
            new SecurityRoleConfiguration(
                RoleId:
                    roleId,
                Name:
                    name,
                NormalizedName:
                    SecurityRoleConfiguration.NormalizeName(
                        name),
                BuiltIn:
                    false,
                Permissions:
                    permissions);

        SecurityRoleConfigurationValidator.Validate(
            role);

        return role;
    }

    private static void EnsureRoleNameAvailable(
        IReadOnlyCollection<SecurityRoleConfiguration> roles,
        string name,
        string? exceptRoleId)
    {
        var normalizedName =
            SecurityRoleConfiguration.NormalizeName(
                name);

        if (roles.Any(role =>
                !string.Equals(
                    role.RoleId,
                    exceptRoleId,
                    StringComparison.Ordinal)
                && string.Equals(
                    role.NormalizedName,
                    normalizedName,
                    StringComparison.Ordinal)))
        {
            throw new SecurityManagementConflictException(
                $"Role name '{name}' already exists.");
        }
    }

    private static void EnsureCustomRole(
        SecurityRoleConfiguration role)
    {
        if (role.BuiltIn)
        {
            throw new SecurityManagementConflictException(
                $"Built-in role '{role.Name}' is system-managed and cannot be changed through the management API.");
        }
    }

    private static void EnsureManagementAuthorityRemains(
        IReadOnlyCollection<LocalUserConfiguration> users,
        IReadOnlyCollection<SecurityRoleConfiguration> roles,
        IReadOnlyCollection<UserRoleAssignment> assignments)
    {
        var proposedCatalog =
            new SecurityCatalog();

        proposedCatalog.ReplaceAll(
            users,
            roles,
            assignments);

        var hasSecurityManager =
            users.Any(user =>
                user.Enabled
                && proposedCatalog.HasPermission(
                    user.UserId,
                    PermissionNames.UsersManage)
                && proposedCatalog.HasPermission(
                    user.UserId,
                    PermissionNames.RolesManage));

        if (!hasSecurityManager)
        {
            throw new SecurityManagementConflictException(
                $"Security change would leave no enabled user with both '{PermissionNames.UsersManage}' and '{PermissionNames.RolesManage}'.");
        }
    }

    private static string NormalizeRequiredText(
        string? value,
        string parameterName)
    {
        var normalized =
            value?.Trim();

        if (string.IsNullOrWhiteSpace(
                normalized))
        {
            throw new ArgumentException(
                $"{parameterName} is required.",
                parameterName);
        }

        return normalized;
    }

    private static void ValidatePassword(
        string? password)
    {
        if (string.IsNullOrWhiteSpace(
                password)
            || password.Length
                is < LocalUserBootstrapper.MinimumPasswordLength
                or > LocalUserBootstrapper.MaximumPasswordLength)
        {
            throw new ArgumentException(
                $"Password must contain between {LocalUserBootstrapper.MinimumPasswordLength} and {LocalUserBootstrapper.MaximumPasswordLength} characters.",
                nameof(password));
        }
    }

    private sealed record SecurityState(
        IReadOnlyList<LocalUserConfiguration> Users,
        IReadOnlyList<SecurityRoleConfiguration> Roles,
        IReadOnlyList<UserRoleAssignment> Assignments,
        SecurityCatalog Catalog);
}
