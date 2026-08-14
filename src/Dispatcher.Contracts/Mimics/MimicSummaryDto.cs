namespace Dispatcher.Contracts.Mimics;

public sealed record MimicSummaryDto(
    string MimicId,
    string Name,
    int Width,
    int Height,
    int ElementCount);
