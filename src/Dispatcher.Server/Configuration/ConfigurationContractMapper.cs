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
}
