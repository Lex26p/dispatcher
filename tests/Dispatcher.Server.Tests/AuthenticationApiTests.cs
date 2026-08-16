using System.Net;
using System.Net.Http.Json;
using Dispatcher.Contracts.Authentication;
using Dispatcher.Contracts.Authorization;
using Dispatcher.Server.Configuration;
using Dispatcher.Server.Security;
using Microsoft.AspNetCore.Identity;
using Microsoft.AspNetCore.Mvc.Testing;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Options;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Dispatcher.Server.Tests;

[TestClass]
public sealed class AuthenticationApiTests
{
    private const string Password =
        "Authentication-test-password-42";

    [TestMethod]
    public async Task AuthenticationApi_LoginCurrentLogout_UsesCookieSession()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        var user =
            await InsertUserAsync(
                database.DatabasePath,
                userName:
                    "operator.one",
                displayName:
                    "Operator One",
                enabled:
                    true,
                role:
                    BuiltInSecurityRoles.All.Single(role =>
                        role.RoleId
                        == BuiltInSecurityRoles.OperatorRoleId));

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);

        using var client =
            factory.CreateClient(
                new WebApplicationFactoryClientOptions
                {
                    AllowAutoRedirect =
                        false,
                    HandleCookies =
                        true
                });

        var beforeLogin =
            await client.GetFromJsonAsync<CurrentUserDto>(
                "/api/auth/current");

        Assert.IsNotNull(
            beforeLogin);
        Assert.IsFalse(
            beforeLogin.Authenticated);
        Assert.IsNull(
            beforeLogin.UserId);
        Assert.IsNull(
            beforeLogin.UserName);
        Assert.IsNull(
            beforeLogin.DisplayName);
        Assert.AreEqual(
            0,
            beforeLogin.EffectivePermissions.Count);

        var loginResponse =
            await client.PostAsJsonAsync(
                "/api/auth/login",
                new LoginRequest(
                    UserName:
                        "Operator.One",
                    Password:
                        Password));

        Assert.AreEqual(
            HttpStatusCode.OK,
            loginResponse.StatusCode);

        var setCookie =
            string.Join(
                ";",
                loginResponse.Headers.GetValues(
                    "Set-Cookie"));

        StringAssert.Contains(
            setCookie,
            LocalAuthenticationDefaults.CookieName
            + "=");
        StringAssert.Contains(
            setCookie.ToLowerInvariant(),
            "httponly");
        StringAssert.Contains(
            setCookie.ToLowerInvariant(),
            "samesite=strict");

        var loginUser =
            await loginResponse.Content.ReadFromJsonAsync<CurrentUserDto>();

        Assert.IsNotNull(
            loginUser);
        Assert.IsTrue(
            loginUser.Authenticated);
        Assert.AreEqual(
            user.UserId,
            loginUser.UserId);
        Assert.AreEqual(
            "operator.one",
            loginUser.UserName);
        Assert.AreEqual(
            "Operator One",
            loginUser.DisplayName);
        AssertEffectiveOperatorPermissions(
            loginUser.EffectivePermissions);

        var current =
            await client.GetFromJsonAsync<CurrentUserDto>(
                "/api/auth/current");

        Assert.IsNotNull(
            current);
        Assert.IsTrue(
            current.Authenticated);
        Assert.AreEqual(
            user.UserId,
            current.UserId);
        Assert.AreEqual(
            "operator.one",
            current.UserName);
        Assert.AreEqual(
            "Operator One",
            current.DisplayName);
        AssertEffectiveOperatorPermissions(
            current.EffectivePermissions);

        var logoutResponse =
            await client.PostAsync(
                "/api/auth/logout",
                content:
                    null);

        Assert.AreEqual(
            HttpStatusCode.NoContent,
            logoutResponse.StatusCode);

        var afterLogout =
            await client.GetFromJsonAsync<CurrentUserDto>(
                "/api/auth/current");

        Assert.IsNotNull(
            afterLogout);
        Assert.IsFalse(
            afterLogout.Authenticated);
        Assert.IsNull(
            afterLogout.UserId);
        Assert.IsNull(
            afterLogout.UserName);
        Assert.IsNull(
            afterLogout.DisplayName);
        Assert.AreEqual(
            0,
            afterLogout.EffectivePermissions.Count);
    }

    [TestMethod]
    public async Task AuthenticationApi_CurrentUserProjectsCurrentSecurityCatalogPermissions()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        var operatorRole =
            BuiltInSecurityRoles.All.Single(role =>
                role.RoleId
                == BuiltInSecurityRoles.OperatorRoleId);
        var user =
            await InsertUserAsync(
                database.DatabasePath,
                userName:
                    "operator.current",
                displayName:
                    "Operator Current",
                enabled:
                    true,
                role:
                    operatorRole);

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);

        using var client =
            factory.CreateClient(
                new WebApplicationFactoryClientOptions
                {
                    AllowAutoRedirect =
                        false,
                    HandleCookies =
                        true
                });

        var loginResponse =
            await client.PostAsJsonAsync(
                "/api/auth/login",
                new LoginRequest(
                    UserName:
                        user.UserName,
                    Password:
                        Password));

        Assert.AreEqual(
            HttpStatusCode.OK,
            loginResponse.StatusCode);

        var viewerRole =
            BuiltInSecurityRoles.All.Single(role =>
                role.RoleId
                == BuiltInSecurityRoles.ViewerRoleId);
        var securityCatalog =
            factory.Services.GetRequiredService<SecurityCatalog>();

        securityCatalog.ReplaceAll(
            [user],
            [viewerRole],
            [
                new UserRoleAssignment(
                    user.UserId,
                    viewerRole.RoleId)
            ]);

        var current =
            await client.GetFromJsonAsync<CurrentUserDto>(
                "/api/auth/current");

        Assert.IsNotNull(
            current);
        Assert.IsTrue(
            current.Authenticated);
        CollectionAssert.AreEqual(
            new[]
            {
                PermissionNames.RuntimeRead
            },
            current.EffectivePermissions.ToArray());
    }

    [TestMethod]
    public async Task AuthenticationApi_LoginWithWrongPassword_ReturnsUnauthorized()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        await InsertUserAsync(
            database.DatabasePath,
            userName:
                "operator.one",
            displayName:
                "Operator One",
            enabled:
                true);

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);

        using var client =
            factory.CreateClient(
                new WebApplicationFactoryClientOptions
                {
                    AllowAutoRedirect =
                        false,
                    HandleCookies =
                        true
                });

        var response =
            await client.PostAsJsonAsync(
                "/api/auth/login",
                new LoginRequest(
                    UserName:
                        "operator.one",
                    Password:
                        "Wrong-password-42"));

        Assert.AreEqual(
            HttpStatusCode.Unauthorized,
            response.StatusCode);

        var current =
            await client.GetFromJsonAsync<CurrentUserDto>(
                "/api/auth/current");

        Assert.IsNotNull(
            current);
        Assert.IsFalse(
            current.Authenticated);
    }

    [TestMethod]
    public async Task AuthenticationApi_DisabledUserCannotLogin()
    {
        using var database =
            await TestConfigurationDatabase.CreateAsync();

        await InsertUserAsync(
            database.DatabasePath,
            userName:
                "disabled.operator",
            displayName:
                "Disabled Operator",
            enabled:
                false);

        using var factory =
            TestDispatcherFactory.Create(
                database.DatabasePath,
                authenticateAsAdministrator:
                    false);

        using var client =
            factory.CreateClient(
                new WebApplicationFactoryClientOptions
                {
                    AllowAutoRedirect =
                        false,
                    HandleCookies =
                        true
                });

        var response =
            await client.PostAsJsonAsync(
                "/api/auth/login",
                new LoginRequest(
                    UserName:
                        "disabled.operator",
                    Password:
                        Password));

        Assert.AreEqual(
            HttpStatusCode.Unauthorized,
            response.StatusCode);

        var current =
            await client.GetFromJsonAsync<CurrentUserDto>(
                "/api/auth/current");

        Assert.IsNotNull(
            current);
        Assert.IsFalse(
            current.Authenticated);
    }

    private static async Task<LocalUserConfiguration> InsertUserAsync(
        string databasePath,
        string userName,
        string displayName,
        bool enabled,
        SecurityRoleConfiguration? role = null)
    {
        var store =
            new SqliteConfigurationStore(
                databasePath);

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
                    enabled,
                PasswordHash:
                    "pending");

        var hasher =
            new PasswordHasher<LocalUserConfiguration>(
                Options.Create(
                    new PasswordHasherOptions()));

        user =
            user with
            {
                PasswordHash =
                    hasher.HashPassword(
                        user,
                        Password)
            };

        if (role is null)
        {
            await store.InsertLocalUserAsync(
                user);
        }
        else
        {
            await store.InsertLocalUserWithRoleAsync(
                user,
                role);
        }

        return user;
    }

    private static void AssertEffectiveOperatorPermissions(
        IReadOnlyList<string> permissions)
    {
        CollectionAssert.AreEquivalent(
            new[]
            {
                PermissionNames.RuntimeRead,
                PermissionNames.TagsWrite,
                PermissionNames.AlarmsAcknowledge
            },
            permissions.ToArray());
    }
}
