namespace Dispatcher.Contracts.Security;

public sealed record UpdateSecurityUserRequest(
    string DisplayName,
    bool Enabled);
