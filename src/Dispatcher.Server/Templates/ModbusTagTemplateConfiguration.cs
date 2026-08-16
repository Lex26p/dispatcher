namespace Dispatcher.Server.Templates;

public sealed record ModbusTagTemplateConfiguration(
    string TagIdSuffix,
    string Name,
    int Address,
    bool Writable);
