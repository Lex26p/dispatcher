namespace Dispatcher.Server.Configuration;

public sealed class ConfigurationCatalog
{
    private Snapshot _snapshot = Snapshot.Empty;

    public IReadOnlyList<ModbusDeviceConfiguration> Devices =>
        Volatile.Read(ref _snapshot).Devices;

    public void Replace(
        IReadOnlyCollection<ModbusDeviceConfiguration> devices)
    {
        ModbusConfigurationValidator.Validate(devices);

        var copiedDevices = devices
            .Select(device => device with
            {
                Tags = device.Tags.ToArray()
            })
            .OrderBy(
                device => device.DeviceId,
                StringComparer.Ordinal)
            .ToArray();

        var tags = new Dictionary<string, ModbusTagBinding>(
            StringComparer.Ordinal);

        foreach (var device in copiedDevices)
        {
            foreach (var tag in device.Tags)
            {
                tags.Add(
                    tag.TagId,
                    new ModbusTagBinding(
                        device,
                        tag));
            }
        }

        Volatile.Write(
            ref _snapshot,
            new Snapshot(
                copiedDevices,
                tags));
    }

    public ModbusTagBinding? FindTag(
        string tagId)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(tagId);

        var snapshot = Volatile.Read(ref _snapshot);

        return snapshot.Tags.TryGetValue(
            tagId,
            out var binding)
            ? binding
            : null;
    }

    public bool IsTagWritable(
        string tagId)
    {
        var binding = FindTag(tagId);

        return binding is
        {
            Device.Enabled: true,
            Tag.Writable: true
        };
    }

    private sealed record Snapshot(
        IReadOnlyList<ModbusDeviceConfiguration> Devices,
        IReadOnlyDictionary<string, ModbusTagBinding> Tags)
    {
        public static Snapshot Empty { get; } =
            new(
                Array.Empty<ModbusDeviceConfiguration>(),
                new Dictionary<string, ModbusTagBinding>(
                    StringComparer.Ordinal));
    }
}
