using Dispatcher.Server.Configuration;

namespace Dispatcher.Server.Security;

public sealed class SecurityConfigurationInitializer
{
    private readonly SqliteConfigurationStore _store;
    private readonly ILogger<SecurityConfigurationInitializer> _logger;

    public SecurityConfigurationInitializer(
        SqliteConfigurationStore store,
        ILogger<SecurityConfigurationInitializer> logger)
    {
        _store =
            store;
        _logger =
            logger;
    }

    public async Task InitializeAsync(
        CancellationToken cancellationToken = default)
    {
        var rolesBeforeInitialization =
            await _store.LoadSecurityRolesAsync(
                cancellationToken);
        var users =
            await _store.LoadLocalUsersAsync(
                cancellationToken);
        var assignments =
            await _store.LoadUserRoleAssignmentsAsync(
                cancellationToken);

        var builtInRolesInitialized =
            BuiltInSecurityRoles.All.All(
                expectedRole =>
                    rolesBeforeInitialization.Any(
                        existingRole =>
                            string.Equals(
                                existingRole.RoleId,
                                expectedRole.RoleId,
                                StringComparison.Ordinal)));

        UserRoleAssignment? initialAdministratorAssignment =
            null;

        if (!builtInRolesInitialized
            && assignments.Count == 0)
        {
            if (users.Count == 1)
            {
                initialAdministratorAssignment =
                    new UserRoleAssignment(
                        UserId:
                            users[0].UserId,
                        RoleId:
                            BuiltInSecurityRoles.AdministratorRoleId);
            }
            else if (users.Count > 1)
            {
                _logger.LogWarning(
                    "Security roles were initialized with {UserCount} existing local users and no role assignments. No Administrator role was granted automatically because the legacy owner identity is ambiguous.",
                    users.Count);
            }
        }

        await _store.InitializeBuiltInSecurityRolesAsync(
            BuiltInSecurityRoles.All,
            initialAdministratorAssignment,
            cancellationToken);

        if (initialAdministratorAssignment is not null)
        {
            var user =
                users.Single(
                    candidate =>
                        string.Equals(
                            candidate.UserId,
                            initialAdministratorAssignment.UserId,
                            StringComparison.Ordinal));

            _logger.LogInformation(
                "Assigned built-in Administrator role to local user {UserName} ({UserId}) during first security-role initialization.",
                user.UserName,
                user.UserId);
        }
    }
}
