using Dispatcher.Contracts.Authorization;
using Microsoft.AspNetCore.Authorization;

namespace Dispatcher.Server.Security;

public sealed class PermissionRequirement : IAuthorizationRequirement
{
    public PermissionRequirement(string permission)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            permission);

        if (!PermissionNames.IsKnown(
                permission))
        {
            throw new ArgumentException(
                $"Unknown permission '{permission}'.",
                nameof(permission));
        }

        Permission =
            permission;
    }

    public string Permission { get; }
}
