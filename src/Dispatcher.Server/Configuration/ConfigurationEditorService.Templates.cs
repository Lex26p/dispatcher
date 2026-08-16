namespace Dispatcher.Server.Configuration;

public sealed partial class ConfigurationEditorService
{
    public async Task<ModbusDeviceConfiguration> CreateDeviceConfigurationAsync(
        ModbusDeviceConfiguration device,
        CancellationToken cancellationToken)
    {
        ArgumentNullException.ThrowIfNull(
            device);

        return await MutateModbusAsync(
            devices =>
            {
                EnsureTemplateDeviceIdsAvailable(
                    device.DeviceId,
                    device.Tags.Select(tag => tag.TagId));

                devices.Add(
                    device with
                    {
                        Tags = device.Tags.ToArray()
                    });

                return device;
            },
            cancellationToken);
    }

    public async Task<SnmpDeviceConfiguration> CreateSnmpDeviceConfigurationAsync(
        SnmpDeviceConfiguration device,
        CancellationToken cancellationToken)
    {
        ArgumentNullException.ThrowIfNull(
            device);

        return await MutateSnmpAsync(
            devices =>
            {
                EnsureTemplateDeviceIdsAvailable(
                    device.DeviceId,
                    device.Tags.Select(tag => tag.TagId));

                devices.Add(
                    device with
                    {
                        Tags = device.Tags.ToArray()
                    });

                return device;
            },
            cancellationToken);
    }

    private void EnsureTemplateDeviceIdsAvailable(
        string deviceId,
        IEnumerable<string> tagIds)
    {
        if (_catalog.ContainsDeviceId(
                deviceId))
        {
            throw new ConfigurationConflictException(
                $"Устройство '{deviceId}' уже существует.");
        }

        foreach (var tagId in tagIds)
        {
            if (_catalog.ContainsTagId(
                    tagId))
            {
                throw new ConfigurationConflictException(
                    $"Тег '{tagId}' уже существует.");
            }
        }
    }
}
