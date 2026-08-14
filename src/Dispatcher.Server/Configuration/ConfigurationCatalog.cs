namespace Dispatcher.Server.Configuration;

public sealed class ConfigurationCatalog
{
    private Snapshot _snapshot = Snapshot.Empty;

    public IReadOnlyList<ModbusDeviceConfiguration> ModbusDevices =>
        Volatile.Read(ref _snapshot).ModbusDevices;

    public IReadOnlyList<SnmpDeviceConfiguration> SnmpDevices =>
        Volatile.Read(ref _snapshot).SnmpDevices;

    public void ReplaceAll(
        IReadOnlyCollection<ModbusDeviceConfiguration> modbusDevices,
        IReadOnlyCollection<SnmpDeviceConfiguration> snmpDevices)
    {
        Volatile.Write(
            ref _snapshot,
            CreateSnapshot(modbusDevices, snmpDevices));
    }

    public void ReplaceModbus(
        IReadOnlyCollection<ModbusDeviceConfiguration> devices)
    {
        var current = Volatile.Read(ref _snapshot);

        ReplaceAll(
            devices,
            current.SnmpDevices);
    }

    public void ReplaceSnmp(
        IReadOnlyCollection<SnmpDeviceConfiguration> devices)
    {
        var current = Volatile.Read(ref _snapshot);

        ReplaceAll(
            current.ModbusDevices,
            devices);
    }

    public ModbusTagBinding? FindTag(string tagId)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(tagId);

        var snapshot = Volatile.Read(ref _snapshot);

        return snapshot.ModbusTags.TryGetValue(
            tagId,
            out var binding)
            ? binding
            : null;
    }

    public bool IsTagWritable(string tagId)
    {
        var binding = FindTag(tagId);

        return binding is
        {
            Device.Enabled: true,
            Tag.Writable: true
        };
    }

    public bool ContainsDeviceId(string deviceId)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(deviceId);

        return Volatile.Read(ref _snapshot)
            .DeviceIds
            .Contains(deviceId);
    }

    public bool ContainsTagId(string tagId)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(tagId);

        return Volatile.Read(ref _snapshot)
            .TagIds
            .Contains(tagId);
    }

    private static Snapshot CreateSnapshot(
        IReadOnlyCollection<ModbusDeviceConfiguration> modbusDevices,
        IReadOnlyCollection<SnmpDeviceConfiguration> snmpDevices)
    {
        ConfigurationSetValidator.Validate(
            modbusDevices,
            snmpDevices);

        var copiedModbus = modbusDevices
            .Select(device => device with
            {
                Tags = device.Tags.ToArray()
            })
            .OrderBy(
                device => device.DeviceId,
                StringComparer.Ordinal)
            .ToArray();

        var copiedSnmp = snmpDevices
            .Select(device => device with
            {
                Tags = device.Tags.ToArray()
            })
            .OrderBy(
                device => device.DeviceId,
                StringComparer.Ordinal)
            .ToArray();

        var modbusTags = new Dictionary<string, ModbusTagBinding>(
            StringComparer.Ordinal);
        var deviceIds = new HashSet<string>(StringComparer.Ordinal);
        var tagIds = new HashSet<string>(StringComparer.Ordinal);

        foreach (var device in copiedModbus)
        {
            deviceIds.Add(device.DeviceId);

            foreach (var tag in device.Tags)
            {
                tagIds.Add(tag.TagId);
                modbusTags.Add(
                    tag.TagId,
                    new ModbusTagBinding(device, tag));
            }
        }

        foreach (var device in copiedSnmp)
        {
            deviceIds.Add(device.DeviceId);

            foreach (var tag in device.Tags)
            {
                tagIds.Add(tag.TagId);
            }
        }

        return new Snapshot(
            copiedModbus,
            copiedSnmp,
            modbusTags,
            deviceIds,
            tagIds);
    }

    private sealed record Snapshot(
        IReadOnlyList<ModbusDeviceConfiguration> ModbusDevices,
        IReadOnlyList<SnmpDeviceConfiguration> SnmpDevices,
        IReadOnlyDictionary<string, ModbusTagBinding> ModbusTags,
        IReadOnlySet<string> DeviceIds,
        IReadOnlySet<string> TagIds)
    {
        public static Snapshot Empty { get; } = new(
            Array.Empty<ModbusDeviceConfiguration>(),
            Array.Empty<SnmpDeviceConfiguration>(),
            new Dictionary<string, ModbusTagBinding>(
                StringComparer.Ordinal),
            new HashSet<string>(StringComparer.Ordinal),
            new HashSet<string>(StringComparer.Ordinal));
    }
}
