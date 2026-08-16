namespace Dispatcher.Server.Templates;

public sealed record SnmpTagTemplateConfiguration(
    string TagIdSuffix,
    string Name,
    string Oid);
