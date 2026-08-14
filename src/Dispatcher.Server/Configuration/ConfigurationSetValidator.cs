namespace Dispatcher.Server.Configuration;

public static class ConfigurationSetValidator
{
    public static void Validate(
        IReadOnlyCollection<ModbusDeviceConfiguration> modbusDevices,
        IReadOnlyCollection<SnmpDeviceConfiguration> snmpDevices)
    {
        ModbusConfigurationValidator.Validate(modbusDevices);
        SnmpConfigurationValidator.Validate(snmpDevices);

        var deviceIds = new HashSet<string>(StringComparer.Ordinal);
        var tagIds = new HashSet<string>(StringComparer.Ordinal);

        foreach (var device in modbusDevices)
        {
            AddDeviceId(deviceIds, device.DeviceId);

            foreach (var tag in device.Tags)
            {
                AddTagId(tagIds, tag.TagId);
            }
        }

        foreach (var device in snmpDevices)
        {
            AddDeviceId(deviceIds, device.DeviceId);

            foreach (var tag in device.Tags)
            {
                AddTagId(tagIds, tag.TagId);
            }
        }
    }

    private static void AddDeviceId(HashSet<string> ids, string deviceId)
    {
        if (!ids.Add(deviceId))
        {
            throw new InvalidOperationException(
                $"DeviceId '{deviceId}' must be unique across all protocols.");
        }
    }

    private static void AddTagId(HashSet<string> ids, string tagId)
    {
        if (!ids.Add(tagId))
        {
            throw new InvalidOperationException(
                $"TagId '{tagId}' must be unique across all protocols.");
        }
    }
}
