namespace Dispatcher.Contracts.Templates;

public sealed record TemplateCatalogItemDto(
    string TemplateId,
    string Name,
    TemplateKindDto Kind,
    int Version,
    IReadOnlyList<TemplateParameterDto> Parameters);
