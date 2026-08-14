namespace Dispatcher.Contracts.Mimics;

public sealed record MimicDefinitionDto(
    string MimicId,
    string Name,
    int Width,
    int Height,
    IReadOnlyList<MimicElementDto> Elements);
