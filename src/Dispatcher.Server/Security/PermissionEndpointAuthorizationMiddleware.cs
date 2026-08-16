using Dispatcher.Contracts.Authorization;
using Microsoft.AspNetCore.Authentication;
using Microsoft.AspNetCore.Authorization;

namespace Dispatcher.Server.Security;

public sealed class PermissionEndpointAuthorizationMiddleware
{
    public const string DenyPolicyName =
        "Dispatcher.DenyUnmappedApiMutation";

    private readonly RequestDelegate _next;

    public PermissionEndpointAuthorizationMiddleware(
        RequestDelegate next)
    {
        _next =
            next;
    }

    public async Task InvokeAsync(
        HttpContext context,
        IAuthorizationService authorizationService)
    {
        var policyName =
            ResolvePolicyName(
                context.Request);

        if (policyName is null)
        {
            await _next(
                context);
            return;
        }

        var authorizationResult =
            await authorizationService.AuthorizeAsync(
                context.User,
                context,
                policyName);

        if (authorizationResult.Succeeded)
        {
            await _next(
                context);
            return;
        }

        if (context.User.Identity?.IsAuthenticated == true)
        {
            await context.ForbidAsync(
                LocalAuthenticationDefaults.CookieScheme);
            return;
        }

        await context.ChallengeAsync(
            LocalAuthenticationDefaults.CookieScheme);
    }

    internal static string? ResolvePolicyName(
        HttpRequest request)
    {
        var path =
            request.Path;

        if (path.StartsWithSegments(
                "/api/auth"))
        {
            return null;
        }

        if (path.StartsWithSegments(
                "/hubs/runtime"))
        {
            return PermissionNames.RuntimeRead;
        }

        if (!path.StartsWithSegments(
                "/api"))
        {
            return null;
        }

        if (HttpMethods.IsPost(
                request.Method)
            && path.StartsWithSegments(
                "/api/tags")
            && path.Value?.EndsWith(
                "/write",
                StringComparison.OrdinalIgnoreCase) == true)
        {
            return PermissionNames.TagsWrite;
        }

        if (path.StartsWithSegments(
                "/api/security/users"))
        {
            if (path.Value?.EndsWith(
                    "/password",
                    StringComparison.OrdinalIgnoreCase) == true)
            {
                return SecurityManagementPolicyNames.CredentialReset;
            }

            if (path.Value?.EndsWith(
                    "/roles",
                    StringComparison.OrdinalIgnoreCase) == true)
            {
                return PermissionNames.RolesManage;
            }

            return PermissionNames.UsersManage;
        }

        if (path.StartsWithSegments(
                "/api/security/roles"))
        {
            return PermissionNames.RolesManage;
        }

        if (path.StartsWithSegments(
                "/api/configuration/modbus")
            || path.StartsWithSegments(
                "/api/configuration/snmp"))
        {
            return IsReadMethod(
                    request.Method)
                ? PermissionNames.RuntimeRead
                : PermissionNames.DevicesEdit;
        }

        if (path.StartsWithSegments(
                "/api/configuration/mimics"))
        {
            return IsReadMethod(
                    request.Method)
                ? PermissionNames.RuntimeRead
                : PermissionNames.MimicsEdit;
        }

        if (path.StartsWithSegments(
                "/api/configuration/historian"))
        {
            return IsReadMethod(
                    request.Method)
                ? PermissionNames.RuntimeRead
                : PermissionNames.HistorianConfigure;
        }

        if (IsReadMethod(
                request.Method))
        {
            return PermissionNames.RuntimeRead;
        }

        return DenyPolicyName;
    }

    private static bool IsReadMethod(
        string method)
    {
        return HttpMethods.IsGet(
                method)
            || HttpMethods.IsHead(
                method);
    }
}
