using System.Security.Claims;
using Microsoft.AspNetCore.Authorization;

namespace Dispatcher.Server.Security;

public sealed class PermissionAuthorizationHandler
    : AuthorizationHandler<PermissionRequirement>
{
    private readonly SecurityCatalog _securityCatalog;

    public PermissionAuthorizationHandler(
        SecurityCatalog securityCatalog)
    {
        _securityCatalog =
            securityCatalog;
    }

    protected override Task HandleRequirementAsync(
        AuthorizationHandlerContext context,
        PermissionRequirement requirement)
    {
        var userId =
            context.User.FindFirstValue(
                ClaimTypes.NameIdentifier);

        if (!string.IsNullOrWhiteSpace(
                userId)
            && _securityCatalog.HasPermission(
                userId,
                requirement.Permission))
        {
            context.Succeed(
                requirement);
        }

        return Task.CompletedTask;
    }
}
