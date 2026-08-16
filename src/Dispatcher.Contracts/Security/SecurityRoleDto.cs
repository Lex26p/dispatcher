namespace Dispatcher.Contracts.Security;

public sealed record SecurityRoleDto(
    string RoleId,
    string Name,
    bool BuiltIn,
    IReadOnlyList<string> Permissions,
    int AssignedUserCount);
