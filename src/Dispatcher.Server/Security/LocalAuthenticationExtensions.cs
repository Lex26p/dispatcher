using Microsoft.AspNetCore.Authentication.Cookies;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.DependencyInjection;

namespace Dispatcher.Server.Security;

public static class LocalAuthenticationExtensions
{
    public static IServiceCollection AddLocalAuthentication(
        this IServiceCollection services)
    {
        services.AddSingleton<LocalAuthenticationService>();

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
                });

        services.AddAuthorization();

        return services;
    }
}
