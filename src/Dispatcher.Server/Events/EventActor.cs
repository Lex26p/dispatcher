using System.Security.Claims;

namespace Dispatcher.Server.Events;

public sealed record EventActor(
    string UserId,
    string UserName)
{
    public static EventActor FromAuthenticatedPrincipal(
        ClaimsPrincipal principal)
    {
        ArgumentNullException.ThrowIfNull(
            principal);

        if (principal.Identity?.IsAuthenticated != true)
        {
            throw new InvalidOperationException(
                "Authenticated actor is required for this audit event.");
        }

        var userId =
            principal.FindFirst(
                ClaimTypes.NameIdentifier)?.Value;

        if (string.IsNullOrWhiteSpace(
                userId))
        {
            throw new InvalidOperationException(
                "Authenticated actor does not contain a user identifier.");
        }

        var userName =
            principal.FindFirst(
                ClaimTypes.Name)?.Value;

        if (string.IsNullOrWhiteSpace(
                userName))
        {
            userName =
                userId;
        }

        return new EventActor(
            userId,
            userName);
    }
}
