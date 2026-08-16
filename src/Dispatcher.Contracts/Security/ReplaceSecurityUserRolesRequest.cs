namespace Dispatcher.Contracts.Security;

public sealed record ReplaceSecurityUserRolesRequest(
    IReadOnlyList<string> RoleIds);
