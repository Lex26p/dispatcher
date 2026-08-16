namespace Dispatcher.Contracts.Authentication;

public sealed record CurrentUserDto(
    bool Authenticated,
    string? UserId,
    string? UserName,
    string? DisplayName,
    IReadOnlyList<string> EffectivePermissions);
