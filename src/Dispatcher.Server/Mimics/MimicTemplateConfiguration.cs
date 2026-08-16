namespace Dispatcher.Server.Mimics;

public sealed record MimicTemplateConfiguration(
    string TemplateId,
    string Name,
    int Width,
    int Height,
    IReadOnlyList<MimicTemplateParameterConfiguration> Parameters,
    IReadOnlyList<MimicTemplateElementConfiguration> Elements,
    int Version = 1);
