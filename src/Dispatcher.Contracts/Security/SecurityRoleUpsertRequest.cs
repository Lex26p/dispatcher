namespace Dispatcher.Contracts.Security;

public sealed record SecurityRoleUpsertRequest(
    string Name,
    IReadOnlyList<string> Permissions);
