namespace Dispatcher.Server.Mimics;

public sealed record MimicConfiguration(
    string MimicId,
    string Name,
    int Width,
    int Height,
    IReadOnlyList<MimicElementConfiguration> Elements);
