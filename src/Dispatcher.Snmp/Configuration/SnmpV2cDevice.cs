namespace Dispatcher.Snmp.Configuration;

public sealed record SnmpV2cDevice(
    string DeviceId,
    string Host,
    int Port,
    string Community);
