using Dispatcher.Contracts.Authorization;
using Microsoft.AspNetCore.Authentication.Cookies;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.DependencyInjection;

namespace Dispatcher.Server.Security;

public static class LocalAuthenticationExtensions
{
    public static IServiceCollection AddLocalAuthentication(
        this IServiceCollection services)
    {
        services.AddSingleton<LocalAuthenticationService>();
        services.AddSingleton<IAuthorizationHandler, PermissionAuthorizationHandler>();

        services
            .AddAuthentication(
                LocalAuthenticationDefaults.CookieScheme)
            .AddCookie(
                LocalAuthenticationDefaults.CookieScheme,
                options =>
                {
                    options.Cookie.Name =
                        LocalAuthenticationDefaults.CookieName;
                    options.Cookie.HttpOnly =
                        true;
                    options.Cookie.SameSite =
                        SameSiteMode.Strict;
                    options.Cookie.SecurePolicy =
                        CookieSecurePolicy.SameAsRequest;
                    options.Cookie.IsEssential =
                        true;
                    options.ExpireTimeSpan =
                        LocalAuthenticationDefaults.SessionLifetime;
                    options.SlidingExpiration =
                        true;
                    options.Events.OnRedirectToLogin =
                        context =>
                        {
                            context.Response.StatusCode =
                                StatusCodes.Status401Unauthorized;
                            return Task.CompletedTask;
                        };
                    options.Events.OnRedirectToAccessDenied =
                        context =>
                        {
                            context.Response.StatusCode =
                                StatusCodes.Status403Forbidden;
                            return Task.CompletedTask;
                        };
                });

        services.AddAuthorization(
            options =>
            {
                foreach (var permission in PermissionNames.All)
                {
                    options.AddPolicy(
                        permission,
                        policy =>
                        {
                            policy.RequireAuthenticatedUser();
                            policy.AddRequirements(
                                new PermissionRequirement(
                                    permission));
                        });
                }

                options.AddPolicy(
                    PermissionEndpointAuthorizationMiddleware.DenyPolicyName,
                    policy =>
                        policy.RequireAssertion(
                            _ => false));
            });

        return services;
    }
}
