using System.Security.Claims;
using Dispatcher.Contracts.Authentication;
using Microsoft.AspNetCore.Authentication;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Routing;

namespace Dispatcher.Server.Security;

public static class AuthenticationEndpoints
{
    public static IEndpointRouteBuilder MapAuthenticationEndpoints(
        this IEndpointRouteBuilder endpoints)
    {
        var group =
            endpoints.MapGroup(
                "/api/auth");

        group.MapPost(
            "/login",
            LoginAsync);

        group.MapPost(
            "/logout",
            (Func<HttpContext, Task<IResult>>)LogoutAsync);

        group.MapGet(
            "/current",
            GetCurrentUser);

        return endpoints;
    }

    private static async Task<IResult> LoginAsync(
        LoginRequest request,
        HttpContext httpContext,
        LocalAuthenticationService authenticationService,
        SecurityCatalog securityCatalog,
        CancellationToken cancellationToken)
    {
        SetNoStore(
            httpContext.Response);

        var user =
            await authenticationService.AuthenticateAsync(
                request.UserName,
                request.Password,
                cancellationToken);

        if (user is null)
        {
            return Results.Unauthorized();
        }

        var claims =
            new[]
            {
                new Claim(
                    ClaimTypes.NameIdentifier,
                    user.UserId),
                new Claim(
                    ClaimTypes.Name,
                    user.UserName),
                new Claim(
                    LocalAuthenticationDefaults.DisplayNameClaimType,
                    user.DisplayName)
            };

        var identity =
            new ClaimsIdentity(
                claims,
                LocalAuthenticationDefaults.CookieScheme);

        await httpContext.SignInAsync(
            LocalAuthenticationDefaults.CookieScheme,
            new ClaimsPrincipal(
                identity),
            new AuthenticationProperties
            {
                IsPersistent =
                    false,
                AllowRefresh =
                    true
            });

        return Results.Ok(
            ToDto(
                user,
                securityCatalog));
    }

    private static async Task<IResult> LogoutAsync(
        HttpContext httpContext)
    {
        SetNoStore(
            httpContext.Response);

        await httpContext.SignOutAsync(
            LocalAuthenticationDefaults.CookieScheme);

        return Results.NoContent();
    }

    private static IResult GetCurrentUser(
        HttpContext httpContext,
        SecurityCatalog securityCatalog)
    {
        SetNoStore(
            httpContext.Response);

        if (httpContext.User.Identity?.IsAuthenticated != true)
        {
            return Results.Ok(
                AnonymousUser());
        }

        var userId =
            httpContext.User.FindFirst(
                ClaimTypes.NameIdentifier)?.Value;

        var userName =
            httpContext.User.FindFirst(
                ClaimTypes.Name)?.Value;

        var displayName =
            httpContext.User.FindFirst(
                LocalAuthenticationDefaults.DisplayNameClaimType)?.Value;

        if (string.IsNullOrWhiteSpace(
                userId)
            || string.IsNullOrWhiteSpace(
                userName)
            || string.IsNullOrWhiteSpace(
                displayName))
        {
            return Results.Ok(
                AnonymousUser());
        }

        return Results.Ok(
            new CurrentUserDto(
                Authenticated:
                    true,
                UserId:
                    userId,
                UserName:
                    userName,
                DisplayName:
                    displayName,
                EffectivePermissions:
                    securityCatalog.GetEffectivePermissions(
                        userId)));
    }

    private static CurrentUserDto ToDto(
        LocalUserConfiguration user,
        SecurityCatalog securityCatalog)
    {
        return new CurrentUserDto(
            Authenticated:
                true,
            UserId:
                user.UserId,
            UserName:
                user.UserName,
            DisplayName:
                user.DisplayName,
            EffectivePermissions:
                securityCatalog.GetEffectivePermissions(
                    user.UserId));
    }

    private static CurrentUserDto AnonymousUser()
    {
        return new CurrentUserDto(
            Authenticated:
                false,
            UserId:
                null,
            UserName:
                null,
            DisplayName:
                null,
            EffectivePermissions:
                Array.Empty<string>());
    }

    private static void SetNoStore(
        HttpResponse response)
    {
        response.Headers[
            "Cache-Control"] =
            "no-store";

        response.Headers[
            "Pragma"] =
            "no-cache";
    }
}
