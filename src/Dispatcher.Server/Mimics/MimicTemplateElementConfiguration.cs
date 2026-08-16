namespace Dispatcher.Server.Mimics;

public sealed record MimicTemplateElementConfiguration(
    string ElementId,
    MimicElementType Type,
    int X,
    int Y,
    int Width,
    int Height,
    string? Text,
    string? TagId,
    string? TagParameterId,
    ushort? CommandValue);
