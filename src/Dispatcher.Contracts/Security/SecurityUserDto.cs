namespace Dispatcher.Contracts.Security;

public sealed record SecurityUserDto(
    string UserId,
    string UserName,
    string DisplayName,
    bool Enabled,
    IReadOnlyList<string> RoleIds,
    IReadOnlyList<string> EffectivePermissions);
