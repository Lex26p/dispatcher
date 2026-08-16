namespace Dispatcher.Contracts.Templates;

public sealed record SnmpTagTemplateDto(
    string TagIdSuffix,
    string Name,
    string Oid);
