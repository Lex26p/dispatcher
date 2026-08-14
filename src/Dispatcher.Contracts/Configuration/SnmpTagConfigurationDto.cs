namespace Dispatcher.Contracts.Configuration;

public sealed record SnmpTagConfigurationDto(
    string TagId,
    string Name,
    string Oid);
