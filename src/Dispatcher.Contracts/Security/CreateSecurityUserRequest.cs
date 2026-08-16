namespace Dispatcher.Contracts.Security;

public sealed record CreateSecurityUserRequest(
    string UserName,
    string DisplayName,
    string Password,
    bool Enabled = true);
