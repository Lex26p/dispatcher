using Dispatcher.Contracts.Configuration;

namespace Dispatcher.Server.Configuration;

internal static class ConfigurationContractMapper
{
    public static ModbusDeviceConfigurationDto ToDto(
        ModbusDeviceConfiguration device)
    {
        return new ModbusDeviceConfigurationDto(
            device.DeviceId,
            device.Name,
            device.Enabled,
            device.Host,
            device.Port,
            device.UnitId,
            device.PollIntervalMilliseconds,
            device.RequestTimeoutMilliseconds,
            device.Tags
                .OrderBy(
                    tag => tag.TagId,
                    StringComparer.Ordinal)
                .Select(ToDto)
                .ToArray());
    }

    public static ModbusTagConfigurationDto ToDto(
        ModbusTagConfiguration tag)
    {
        return new ModbusTagConfigurationDto(
            tag.TagId,
            tag.Name,
            tag.Address,
            tag.Writable);
    }

    public static SnmpDeviceConfigurationDto ToDto(
        SnmpDeviceConfiguration device)
    {
        return new SnmpDeviceConfigurationDto(
            device.DeviceId,
            device.Name,
            device.Enabled,
            device.Host,
            device.Port,
            device.Community,
            device.PollIntervalMilliseconds,
            device.RequestTimeoutMilliseconds,
            device.Tags
                .OrderBy(
                    tag => tag.TagId,
                    StringComparer.Ordinal)
                .Select(ToDto)
                .ToArray());
    }

    public static SnmpTagConfigurationDto ToDto(
        SnmpTagConfiguration tag)
    {
        return new SnmpTagConfigurationDto(
            tag.TagId,
            tag.Name,
            tag.Oid);
    }

    public static ModbusDeviceConfiguration ToConfiguration(
        ModbusDeviceUpsertRequest request,
        IReadOnlyList<ModbusTagConfiguration>? tags = null)
    {
        ArgumentNullException.ThrowIfNull(request);

        return new ModbusDeviceConfiguration(
            request.DeviceId,
            request.Name,
            request.Enabled,
            request.Host,
            request.Port,
            request.UnitId,
            request.PollIntervalMilliseconds,
            request.RequestTimeoutMilliseconds,
            tags ?? Array.Empty<ModbusTagConfiguration>());
    }

    public static ModbusTagConfiguration ToConfiguration(
        ModbusTagUpsertRequest request)
    {
        ArgumentNullException.ThrowIfNull(request);

        return new ModbusTagConfiguration(
            request.TagId,
            request.Name,
            request.Address,
            request.Writable);
    }

    public static SnmpDeviceConfiguration ToConfiguration(
        SnmpDeviceUpsertRequest request,
        IReadOnlyList<SnmpTagConfiguration>? tags = null)
    {
        ArgumentNullException.ThrowIfNull(request);

        return new SnmpDeviceConfiguration(
            request.DeviceId,
            request.Name,
            request.Enabled,
            request.Host,
            request.Port,
            request.Community,
            request.PollIntervalMilliseconds,
            request.RequestTimeoutMilliseconds,
            tags ?? Array.Empty<SnmpTagConfiguration>());
    }

    public static SnmpTagConfiguration ToConfiguration(
        SnmpTagUpsertRequest request)
    {
        ArgumentNullException.ThrowIfNull(request);

        return new SnmpTagConfiguration(
            request.TagId,
            request.Name,
            request.Oid);
    }
}
