namespace Dispatcher.Server.Mimics;

public sealed record MimicElementConfiguration(
    string ElementId,
    MimicElementType Type,
    int X,
    int Y,
    int Width,
    int Height,
    string? Text,
    string? TagId,
    ushort? CommandValue);
