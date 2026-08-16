namespace Dispatcher.Contracts.Mimics;

public sealed record MimicTemplateDto(
    string TemplateId,
    string Name,
    int Width,
    int Height,
    IReadOnlyList<MimicTemplateParameterDto> Parameters,
    IReadOnlyList<MimicTemplateElementDto> Elements);
