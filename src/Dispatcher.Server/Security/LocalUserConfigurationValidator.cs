namespace Dispatcher.Server.Security;

public static class LocalUserConfigurationValidator
{
    public const int MaxUserIdLength = 64;
    public const int MaxUserNameLength = 128;
    public const int MaxDisplayNameLength = 200;
    public const int MaxPasswordHashLength = 2048;

    public static void Validate(
        LocalUserConfiguration user)
    {
        ArgumentNullException.ThrowIfNull(
            user);

        ValidateRequired(
            user.UserId,
            nameof(user.UserId),
            MaxUserIdLength);
        ValidateRequired(
            user.UserName,
            nameof(user.UserName),
            MaxUserNameLength);
        ValidateRequired(
            user.NormalizedUserName,
            nameof(user.NormalizedUserName),
            MaxUserNameLength);
        ValidateRequired(
            user.DisplayName,
            nameof(user.DisplayName),
            MaxDisplayNameLength);
        ValidateRequired(
            user.PasswordHash,
            nameof(user.PasswordHash),
            MaxPasswordHashLength);

        if (!string.Equals(
                user.UserName,
                user.UserName.Trim(),
                StringComparison.Ordinal))
        {
            throw new ArgumentException(
                "UserName must not contain leading or trailing whitespace.",
                nameof(user));
        }

        var expectedNormalizedUserName =
            LocalUserConfiguration.NormalizeUserName(
                user.UserName);

        if (!string.Equals(
                expectedNormalizedUserName,
                user.NormalizedUserName,
                StringComparison.Ordinal))
        {
            throw new ArgumentException(
                "NormalizedUserName does not match UserName.",
                nameof(user));
        }
    }

    private static void ValidateRequired(
        string value,
        string propertyName,
        int maximumLength)
    {
        if (string.IsNullOrWhiteSpace(
                value))
        {
            throw new ArgumentException(
                $"{propertyName} is required.",
                propertyName);
        }

        if (value.Length > maximumLength)
        {
            throw new ArgumentException(
                $"{propertyName} must not exceed {maximumLength} characters.",
                propertyName);
        }
    }
}
