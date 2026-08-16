namespace Dispatcher.Contracts.Mimics;

public sealed record InstantiateMimicTemplateRequest(
    int X,
    int Y,
    Dictionary<string, string> TagBindings);
