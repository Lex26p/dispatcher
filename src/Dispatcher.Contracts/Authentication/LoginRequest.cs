namespace Dispatcher.Contracts.Authentication;

public sealed record LoginRequest(
    string UserName,
    string Password);
