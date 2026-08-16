namespace Dispatcher.Server.Security;

public static class LocalAuthenticationDefaults
{
    public const string CookieScheme =
        "Dispatcher.Local";

    public const string CookieName =
        "Dispatcher.Auth";

    public const string DisplayNameClaimType =
        "dispatcher:display_name";

    public static readonly TimeSpan SessionLifetime =
        TimeSpan.FromHours(8);
}
