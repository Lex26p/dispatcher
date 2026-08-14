namespace Dispatcher.Contracts.Mimics;

public sealed record MimicElementDto(
    string ElementId,
    MimicElementTypeDto Type,
    int X,
    int Y,
    int Width,
    int Height,
    string? Text,
    string? TagId,
    ushort? CommandValue);
