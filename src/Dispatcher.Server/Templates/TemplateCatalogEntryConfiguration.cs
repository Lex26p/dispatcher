namespace Dispatcher.Server.Templates;

public sealed record TemplateCatalogEntryConfiguration(
    string TemplateId,
    string Name,
    TemplateKind Kind,
    int Version,
    IReadOnlyList<TemplateParameterConfiguration> Parameters);
