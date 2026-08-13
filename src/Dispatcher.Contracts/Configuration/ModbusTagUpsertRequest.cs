namespace Dispatcher.Contracts.Configuration;

public sealed record ModbusTagUpsertRequest(
    string TagId,
    string Name,
    int Address,
    bool Writable);
