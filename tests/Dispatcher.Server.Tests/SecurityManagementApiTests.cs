using System.Net;
using System.Net.Http.Json;
using Dispatcher.Contracts.Authentication;
using Dispatcher.Contracts.Authorization;
using Dispatcher.Contracts.Security;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Security;
using Microsoft.AspNetCore.Identity;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Options;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class SecurityManagementApiTests
{
    private const string InitialPassword =
        "Management-initial-password-42";
    private const string ResetPassword =
        "Management-reset-password-84";
    private const string RoleUserPassword =
        "Management-role-user-password-42";

    [TestMethod]
    public async Task SecurityManagementApi_RequiresUsersManageAndRolesManage()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        using var anonymousFactory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);
        using var anonymousClient =
            CreateCookieClient(
                anonymousFactory);

        await AssertStatusAsync(
            HttpStatusCode.Unauthorized,
            anonymousClient.GetAsync(
                "/api/security/users"));
        await AssertStatusAsync(
            HttpStatusCode.Unauthorized,
            anonymousClient.GetAsync(
                "/api/security/roles"));

        var engineer =
            await InsertUserWithRoleAsync(
                database.DatabasePath,
                "engineer.management",
                BuiltInSecurityRoles.EngineerRoleId,
                RoleUserPassword);
        using var engineerFactory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);
        using var engineerClient =
            CreateCookieClient(
                engineerFactory);

        await LoginAsync(
            engineerClient,
            engineer.UserName,
            RoleUserPassword,
            HttpStatusCode.OK);

        await AssertStatusAsync(
            HttpStatusCode.Forbidden,
            engineerClient.GetAsync(
                "/api/security/users"));
        await AssertStatusAsync(
            HttpStatusCode.Forbidden,
            engineerClient.GetAsync(
                "/api/security/roles"));
        await AssertStatusAsync(
            HttpStatusCode.Forbidden,
            engineerClient.PostAsJsonAsync(
                "/api/security/users",
                new CreateSecurityUserRequest(
                    UserName:
                        "blocked.user",
                    DisplayName:
                        "Blocked User",
                    Password:
                        InitialPassword)));

        var usersManager =
            await InsertUserWithPermissionsAsync(
                database.DatabasePath,
                "users.manager",
                RoleUserPassword,
                PermissionNames.UsersManage);
        using var usersManagerFactory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);
        using var usersManagerClient =
            CreateCookieClient(
                usersManagerFactory);

        await LoginAsync(
            usersManagerClient,
            usersManager.UserName,
            RoleUserPassword,
            HttpStatusCode.OK);

        await AssertStatusAsync(
            HttpStatusCode.OK,
            usersManagerClient.GetAsync(
                "/api/security/users"));
        await AssertStatusAsync(
            HttpStatusCode.Forbidden,
            usersManagerClient.GetAsync(
                "/api/security/roles"));
        await AssertStatusAsync(
            HttpStatusCode.Forbidden,
            usersManagerClient.PutAsJsonAsync(
                $"/api/security/users/{usersManager.UserId}/password",
                new ResetSecurityUserPasswordRequest(
                    Password:
                        ResetPassword)));
    }

    [TestMethod]
    public async Task SecurityManagementApi_AdministratorCanManageUserRoleAndPassword()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);
        using var client =
            factory.CreateClient();

        var createUserResponse =
            await client.PostAsJsonAsync(
                "/api/security/users",
                new CreateSecurityUserRequest(
                    UserName:
                        "operator.managed",
                    DisplayName:
                        "Managed Operator",
                    Password:
                        InitialPassword));

        Assert.AreEqual(
            HttpStatusCode.Created,
            createUserResponse.StatusCode);

        var createdUser =
            await createUserResponse.Content.ReadFromJsonAsync<SecurityUserDto>();

        Assert.IsNotNull(
            createdUser);
        Assert.AreEqual(
            "operator.managed",
            createdUser.UserName);
        Assert.AreEqual(
            "Managed Operator",
            createdUser.DisplayName);
        Assert.IsTrue(
            createdUser.Enabled);
        Assert.AreEqual(
            0,
            createdUser.RoleIds.Count);
        Assert.AreEqual(
            0,
            createdUser.EffectivePermissions.Count);

        var createRoleResponse =
            await client.PostAsJsonAsync(
                "/api/security/roles",
                new SecurityRoleUpsertRequest(
                    Name:
                        "Runtime Observer",
                    Permissions:
                    [
                        PermissionNames.RuntimeRead
                    ]));

        Assert.AreEqual(
            HttpStatusCode.Created,
            createRoleResponse.StatusCode);

        var role =
            await createRoleResponse.Content.ReadFromJsonAsync<SecurityRoleDto>();

        Assert.IsNotNull(
            role);
        Assert.IsFalse(
            role.BuiltIn);
        Assert.AreEqual(
            0,
            role.AssignedUserCount);
        CollectionAssert.AreEquivalent(
            new[]
            {
                PermissionNames.RuntimeRead
            },
            role.Permissions.ToArray());

        var assignResponse =
            await client.PutAsJsonAsync(
                $"/api/security/users/{createdUser.UserId}/roles",
                new ReplaceSecurityUserRolesRequest(
                    RoleIds:
                    [
                        role.RoleId
                    ]));

        Assert.AreEqual(
            HttpStatusCode.OK,
            assignResponse.StatusCode);

        var assignedUser =
            await assignResponse.Content.ReadFromJsonAsync<SecurityUserDto>();

        Assert.IsNotNull(
            assignedUser);
        CollectionAssert.AreEquivalent(
            new[]
            {
                role.RoleId
            },
            assignedUser.RoleIds.ToArray());
        CollectionAssert.AreEquivalent(
            new[]
            {
                PermissionNames.RuntimeRead
            },
            assignedUser.EffectivePermissions.ToArray());

        using var profileFactory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);
        using var profileClient =
            CreateCookieClient(
                profileFactory);

        await LoginAsync(
            profileClient,
            createdUser.UserName,
            InitialPassword,
            HttpStatusCode.OK);

        var updateResponse =
            await client.PutAsJsonAsync(
                $"/api/security/users/{createdUser.UserId}",
                new UpdateSecurityUserRequest(
                    DisplayName:
                        "Managed Operator Updated",
                    Enabled:
                        true));

        Assert.AreEqual(
            HttpStatusCode.OK,
            updateResponse.StatusCode);

        var updatedUser =
            await updateResponse.Content.ReadFromJsonAsync<SecurityUserDto>();

        Assert.IsNotNull(
            updatedUser);
        Assert.AreEqual(
            "Managed Operator Updated",
            updatedUser.DisplayName);

        var projectedCurrentUser =
            await profileClient.GetFromJsonAsync<CurrentUserDto>(
                "/api/auth/current");

        Assert.IsNotNull(
            projectedCurrentUser);
        Assert.IsTrue(
            projectedCurrentUser.Authenticated);
        Assert.AreEqual(
            "Managed Operator Updated",
            projectedCurrentUser.DisplayName);

        var resetResponse =
            await client.PutAsJsonAsync(
                $"/api/security/users/{createdUser.UserId}/password",
                new ResetSecurityUserPasswordRequest(
                    Password:
                        ResetPassword));

        Assert.AreEqual(
            HttpStatusCode.NoContent,
            resetResponse.StatusCode);

        using var loginFactory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);
        using var oldPasswordClient =
            CreateCookieClient(
                loginFactory);
        using var newPasswordClient =
            CreateCookieClient(
                loginFactory);

        await LoginAsync(
            oldPasswordClient,
            createdUser.UserName,
            InitialPassword,
            HttpStatusCode.Unauthorized);
        await LoginAsync(
            newPasswordClient,
            createdUser.UserName,
            ResetPassword,
            HttpStatusCode.OK);

        await AssertStatusAsync(
            HttpStatusCode.OK,
            newPasswordClient.GetAsync(
                "/api/tags"));
        await AssertStatusAsync(
            HttpStatusCode.Forbidden,
            newPasswordClient.PostAsJsonAsync(
                "/api/tags/missing/write",
                new
                {
                    value = 1
                }));

        var assignedRoleDelete =
            await client.DeleteAsync(
                $"/api/security/roles/{role.RoleId}");

        Assert.AreEqual(
            HttpStatusCode.Conflict,
            assignedRoleDelete.StatusCode);

        var unassignResponse =
            await client.PutAsJsonAsync(
                $"/api/security/users/{createdUser.UserId}/roles",
                new ReplaceSecurityUserRolesRequest(
                    RoleIds:
                        Array.Empty<string>()));

        Assert.AreEqual(
            HttpStatusCode.OK,
            unassignResponse.StatusCode);

        var deleteRoleResponse =
            await client.DeleteAsync(
                $"/api/security/roles/{role.RoleId}");

        Assert.AreEqual(
            HttpStatusCode.NoContent,
            deleteRoleResponse.StatusCode);

        var disableResponse =
            await client.PutAsJsonAsync(
                $"/api/security/users/{createdUser.UserId}",
                new UpdateSecurityUserRequest(
                    DisplayName:
                        "Managed Operator Updated",
                    Enabled:
                        false));

        Assert.AreEqual(
            HttpStatusCode.OK,
            disableResponse.StatusCode);

        var catalog =
            factory.Services.GetRequiredService<SecurityCatalog>();

        Assert.IsFalse(
            catalog.IsUserEnabled(
                createdUser.UserId));

        var persistedUsers =
            await new SqliteConfigurationStore(
                    database.DatabasePath)
                .LoadLocalUsersAsync();
        var persisted =
            persistedUsers.Single(user =>
                string.Equals(
                    user.UserId,
                    createdUser.UserId,
                    StringComparison.Ordinal));

        Assert.IsFalse(
            persisted.Enabled);
        Assert.AreEqual(
            "Managed Operator Updated",
            persisted.DisplayName);
        Assert.AreNotEqual(
            ResetPassword,
            persisted.PasswordHash);
    }

    [TestMethod]
    public async Task SecurityManagementApi_PreventsRemovingLastManagementAuthorityAndBuiltInRoleMutation()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();
        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath);
        using var client =
            factory.CreateClient();

        var users =
            await client.GetFromJsonAsync<SecurityUserDto[]>(
                "/api/security/users");

        Assert.IsNotNull(
            users);

        var administrator =
            users.Single(user =>
                string.Equals(
                    user.UserId,
                    TestDispatcherFactory.TestAdministratorUserId,
                    StringComparison.Ordinal));

        var disableResponse =
            await client.PutAsJsonAsync(
                $"/api/security/users/{administrator.UserId}",
                new UpdateSecurityUserRequest(
                    DisplayName:
                        administrator.DisplayName,
                    Enabled:
                        false));

        Assert.AreEqual(
            HttpStatusCode.Conflict,
            disableResponse.StatusCode);

        var replaceRolesResponse =
            await client.PutAsJsonAsync(
                $"/api/security/users/{administrator.UserId}/roles",
                new ReplaceSecurityUserRolesRequest(
                    RoleIds:
                    [
                        BuiltInSecurityRoles.ViewerRoleId
                    ]));

        Assert.AreEqual(
            HttpStatusCode.Conflict,
            replaceRolesResponse.StatusCode);

        var updateBuiltInResponse =
            await client.PutAsJsonAsync(
                $"/api/security/roles/{BuiltInSecurityRoles.AdministratorRoleId}",
                new SecurityRoleUpsertRequest(
                    Name:
                        "Administrator Changed",
                    Permissions:
                        PermissionNames.All));

        Assert.AreEqual(
            HttpStatusCode.Conflict,
            updateBuiltInResponse.StatusCode);

        var deleteBuiltInResponse =
            await client.DeleteAsync(
                $"/api/security/roles/{BuiltInSecurityRoles.AdministratorRoleId}");

        Assert.AreEqual(
            HttpStatusCode.Conflict,
            deleteBuiltInResponse.StatusCode);
    }

    private static HttpClient CreateCookieClient(
        WebApplicationFactory<Program> factory)
    {
        return factory.CreateClient(
            new WebApplicationFactoryClientOptions
            {
                AllowAutoRedirect =
                    false,
                HandleCookies =
                    true
            });
    }

    private static async Task LoginAsync(
        HttpClient client,
        string userName,
        string password,
        HttpStatusCode expectedStatus)
    {
        using var response =
            await client.PostAsJsonAsync(
                "/api/auth/login",
                new LoginRequest(
                    UserName:
                        userName,
                    Password:
                        password));

        Assert.AreEqual(
            expectedStatus,
            response.StatusCode);
    }

    private static async Task<LocalUserConfiguration> InsertUserWithPermissionsAsync(
        string databasePath,
        string userName,
        string password,
        params string[] permissions)
    {
        var roleName =
            $"Test role {Guid.NewGuid():N}";
        var role =
            new SecurityRoleConfiguration(
                RoleId:
                    Guid.NewGuid().ToString("N"),
                Name:
                    roleName,
                NormalizedName:
                    SecurityRoleConfiguration.NormalizeName(
                        roleName),
                BuiltIn:
                    false,
                Permissions:
                    permissions);
        var store =
            new SqliteConfigurationStore(
                databasePath);
        var user =
            CreateUserWithPassword(
                userName,
                password);

        await store.InsertLocalUserWithRoleAsync(
            user,
            role);

        return user;
    }

    private static async Task<LocalUserConfiguration> InsertUserWithRoleAsync(
        string databasePath,
        string userName,
        string roleId,
        string password)
    {
        var store =
            new SqliteConfigurationStore(
                databasePath);
        var role =
            BuiltInSecurityRoles.All.Single(candidate =>
                string.Equals(
                    candidate.RoleId,
                    roleId,
                    StringComparison.Ordinal));
        var user =
            CreateUserWithPassword(
                userName,
                password);

        await store.InsertLocalUserWithRoleAsync(
            user,
            role);

        return user;
    }

    private static LocalUserConfiguration CreateUserWithPassword(
        string userName,
        string password)
    {
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
                    userName,
                Enabled:
                    true,
                PasswordHash:
                    "pending");
        var hasher =
            new PasswordHasher<LocalUserConfiguration>(
                Options.Create(
                    new PasswordHasherOptions()));

        return user with
        {
            PasswordHash =
                hasher.HashPassword(
                    user,
                    password)
        };
    }

    private static async Task AssertStatusAsync(
        HttpStatusCode expected,
        Task<HttpResponseMessage> responseTask)
    {
        using var response =
            await responseTask;

        Assert.AreEqual(
            expected,
            response.StatusCode);
    }
}
