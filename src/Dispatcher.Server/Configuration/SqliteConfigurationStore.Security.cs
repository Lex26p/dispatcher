using Dispatcher.Server.Security;
using Microsoft.Data.Sqlite;

namespace Dispatcher.Server.Configuration;

public sealed partial class SqliteConfigurationStore
{
    public async Task<IReadOnlyList<SecurityRoleConfiguration>> LoadSecurityRolesAsync(
        CancellationToken cancellationToken = default)
    {
        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        var rows =
            new List<SecurityRoleRow>();

        await using (var command =
            connection.CreateCommand())
        {
            command.CommandText =
                """
                SELECT
                    role_id,
                    name,
                    normalized_name,
                    built_in
                FROM security_roles
                ORDER BY normalized_name;
                """;

            await using var reader =
                await command.ExecuteReaderAsync(
                    cancellationToken);

            while (await reader.ReadAsync(
                cancellationToken))
            {
                rows.Add(
                    new SecurityRoleRow(
                        RoleId:
                            reader.GetString(0),
                        Name:
                            reader.GetString(1),
                        NormalizedName:
                            reader.GetString(2),
                        BuiltIn:
                            reader.GetInt64(3) != 0));
            }
        }

        var permissionsByRole =
            rows.ToDictionary(
                row => row.RoleId,
                _ => new List<string>(),
                StringComparer.Ordinal);

        await using (var command =
            connection.CreateCommand())
        {
            command.CommandText =
                """
                SELECT
                    role_id,
                    permission
                FROM security_role_permissions
                ORDER BY role_id, permission;
                """;

            await using var reader =
                await command.ExecuteReaderAsync(
                    cancellationToken);

            while (await reader.ReadAsync(
                cancellationToken))
            {
                var roleId =
                    reader.GetString(0);

                if (!permissionsByRole.TryGetValue(
                        roleId,
                        out var permissions))
                {
                    throw new InvalidOperationException(
                        $"Permission references unknown security role '{roleId}'.");
                }

                permissions.Add(
                    reader.GetString(1));
            }
        }

        var roles =
            rows
                .Select(row =>
                    new SecurityRoleConfiguration(
                        RoleId:
                            row.RoleId,
                        Name:
                            row.Name,
                        NormalizedName:
                            row.NormalizedName,
                        BuiltIn:
                            row.BuiltIn,
                        Permissions:
                            permissionsByRole[
                                row.RoleId]
                                .ToArray()))
                .ToArray();

        foreach (var role in roles)
        {
            SecurityRoleConfigurationValidator.Validate(
                role);
        }

        return roles;
    }

    public async Task UpsertSecurityRoleAsync(
        SecurityRoleConfiguration role,
        CancellationToken cancellationToken = default)
    {
        SecurityRoleConfigurationValidator.Validate(
            role);

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        using var transaction =
            connection.BeginTransaction();

        await UpsertSecurityRoleAsync(
            connection,
            transaction,
            role,
            cancellationToken);

        transaction.Commit();
    }

    public async Task InitializeBuiltInSecurityRolesAsync(
        IReadOnlyCollection<SecurityRoleConfiguration> roles,
        UserRoleAssignment? initialAdministratorAssignment,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(
            roles);

        foreach (var role in roles)
        {
            SecurityRoleConfigurationValidator.Validate(
                role);

            if (!role.BuiltIn)
            {
                throw new InvalidOperationException(
                    $"Role '{role.RoleId}' is not a built-in security role.");
            }
        }

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        using var transaction =
            connection.BeginTransaction();

        foreach (var role in roles)
        {
            await UpsertSecurityRoleAsync(
                connection,
                transaction,
                role,
                cancellationToken);
        }

        if (initialAdministratorAssignment is not null)
        {
            await AssignUserRoleAsync(
                connection,
                transaction,
                initialAdministratorAssignment,
                cancellationToken);
        }

        transaction.Commit();
    }

    public async Task InsertLocalUserWithRoleAsync(
        LocalUserConfiguration user,
        SecurityRoleConfiguration role,
        CancellationToken cancellationToken = default)
    {
        LocalUserConfigurationValidator.Validate(
            user);
        SecurityRoleConfigurationValidator.Validate(
            role);

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        using var transaction =
            connection.BeginTransaction();

        await UpsertSecurityRoleAsync(
            connection,
            transaction,
            role,
            cancellationToken);

        await using (var command =
            connection.CreateCommand())
        {
            command.Transaction =
                transaction;
            command.CommandText =
                """
                INSERT INTO local_users (
                    user_id,
                    user_name,
                    normalized_user_name,
                    display_name,
                    enabled,
                    password_hash)
                VALUES (
                    $userId,
                    $userName,
                    $normalizedUserName,
                    $displayName,
                    $enabled,
                    $passwordHash);
                """;

            command.Parameters.AddWithValue(
                "$userId",
                user.UserId);
            command.Parameters.AddWithValue(
                "$userName",
                user.UserName);
            command.Parameters.AddWithValue(
                "$normalizedUserName",
                user.NormalizedUserName);
            command.Parameters.AddWithValue(
                "$displayName",
                user.DisplayName);
            command.Parameters.AddWithValue(
                "$enabled",
                user.Enabled ? 1 : 0);
            command.Parameters.AddWithValue(
                "$passwordHash",
                user.PasswordHash);

            await command.ExecuteNonQueryAsync(
                cancellationToken);
        }

        await AssignUserRoleAsync(
            connection,
            transaction,
            new UserRoleAssignment(
                UserId:
                    user.UserId,
                RoleId:
                    role.RoleId),
            cancellationToken);

        transaction.Commit();
    }

    public async Task<IReadOnlyList<UserRoleAssignment>> LoadUserRoleAssignmentsAsync(
        CancellationToken cancellationToken = default)
    {
        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        await using var command =
            connection.CreateCommand();

        command.CommandText =
            """
            SELECT
                user_id,
                role_id
            FROM security_user_roles
            ORDER BY user_id, role_id;
            """;

        await using var reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        var assignments =
            new List<UserRoleAssignment>();

        while (await reader.ReadAsync(
            cancellationToken))
        {
            assignments.Add(
                new UserRoleAssignment(
                    UserId:
                        reader.GetString(0),
                    RoleId:
                        reader.GetString(1)));
        }

        return assignments;
    }

    public async Task AssignUserRoleAsync(
        UserRoleAssignment assignment,
        CancellationToken cancellationToken = default)
    {
        ValidateAssignment(
            assignment);

        await using var connection =
            await OpenConnectionAsync(
                cancellationToken);

        using var transaction =
            connection.BeginTransaction();

        await AssignUserRoleAsync(
            connection,
            transaction,
            assignment,
            cancellationToken);

        transaction.Commit();
    }

    private static async Task UpsertSecurityRoleAsync(
        SqliteConnection connection,
        SqliteTransaction transaction,
        SecurityRoleConfiguration role,
        CancellationToken cancellationToken)
    {
        await using (var command =
            connection.CreateCommand())
        {
            command.Transaction =
                transaction;
            command.CommandText =
                """
                INSERT INTO security_roles (
                    role_id,
                    name,
                    normalized_name,
                    built_in)
                VALUES (
                    $roleId,
                    $name,
                    $normalizedName,
                    $builtIn)
                ON CONFLICT(role_id) DO UPDATE SET
                    name = excluded.name,
                    normalized_name = excluded.normalized_name,
                    built_in = excluded.built_in;
                """;

            command.Parameters.AddWithValue(
                "$roleId",
                role.RoleId);
            command.Parameters.AddWithValue(
                "$name",
                role.Name);
            command.Parameters.AddWithValue(
                "$normalizedName",
                role.NormalizedName);
            command.Parameters.AddWithValue(
                "$builtIn",
                role.BuiltIn ? 1 : 0);

            await command.ExecuteNonQueryAsync(
                cancellationToken);
        }

        await using (var command =
            connection.CreateCommand())
        {
            command.Transaction =
                transaction;
            command.CommandText =
                """
                DELETE FROM security_role_permissions
                WHERE role_id = $roleId;
                """;
            command.Parameters.AddWithValue(
                "$roleId",
                role.RoleId);

            await command.ExecuteNonQueryAsync(
                cancellationToken);
        }

        foreach (var permission in role.Permissions)
        {
            await using var command =
                connection.CreateCommand();

            command.Transaction =
                transaction;
            command.CommandText =
                """
                INSERT INTO security_role_permissions (
                    role_id,
                    permission)
                VALUES (
                    $roleId,
                    $permission);
                """;
            command.Parameters.AddWithValue(
                "$roleId",
                role.RoleId);
            command.Parameters.AddWithValue(
                "$permission",
                permission);

            await command.ExecuteNonQueryAsync(
                cancellationToken);
        }
    }

    private static async Task AssignUserRoleAsync(
        SqliteConnection connection,
        SqliteTransaction transaction,
        UserRoleAssignment assignment,
        CancellationToken cancellationToken)
    {
        ValidateAssignment(
            assignment);

        await using var command =
            connection.CreateCommand();

        command.Transaction =
            transaction;
        command.CommandText =
            """
            INSERT OR IGNORE INTO security_user_roles (
                user_id,
                role_id)
            VALUES (
                $userId,
                $roleId);
            """;
        command.Parameters.AddWithValue(
            "$userId",
            assignment.UserId);
        command.Parameters.AddWithValue(
            "$roleId",
            assignment.RoleId);

        await command.ExecuteNonQueryAsync(
            cancellationToken);
    }

    private static void ValidateAssignment(
        UserRoleAssignment assignment)
    {
        ArgumentNullException.ThrowIfNull(
            assignment);
        ArgumentException.ThrowIfNullOrWhiteSpace(
            assignment.UserId);
        ArgumentException.ThrowIfNullOrWhiteSpace(
            assignment.RoleId);
    }

    private sealed record SecurityRoleRow(
        string RoleId,
        string Name,
        string NormalizedName,
        bool BuiltIn);
}
