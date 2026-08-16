namespace Dispatcher.Server.Security;

public sealed record UserRoleAssignment(
    string UserId,
    string RoleId);
