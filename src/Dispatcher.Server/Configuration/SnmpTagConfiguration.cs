namespace Dispatcher.Server.Configuration;

public sealed record SnmpTagConfiguration(
    string TagId,
    string Name,
    string Oid);
