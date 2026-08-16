namespace Dispatcher.Contracts.Templates;

public sealed record ModbusTagTemplateDto(
    string TagIdSuffix,
    string Name,
    int Address,
    bool Writable);
