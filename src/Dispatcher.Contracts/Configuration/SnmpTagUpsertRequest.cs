namespace Dispatcher.Contracts.Configuration;

public sealed record SnmpTagUpsertRequest(
    string TagId,
    string Name,
    string Oid);
