using Dispatcher.Modbus.Configuration;

namespace Dispatcher.Server.Configuration;

public sealed class ModbusRuntimeOptions
{
    public const string SectionName = "Modbus";

    public bool Enabled { get; set; }

    public ModbusRuntimeDeviceOptions Device { get; set; } = new();

    public ModbusPollingPlan CreatePollingPlan()
    {
        if (!Enabled)
        {
            throw new InvalidOperationException(
                "Modbus runtime is disabled.");
        }

        ArgumentNullException.ThrowIfNull(Device);

        ValidateRequired(
            Device.DeviceId,
            "Modbus:Device:DeviceId");
        ValidateRequired(
            Device.Host,
            "Modbus:Device:Host");

        if (Device.Port is < 1 or > 65535)
        {
            throw new InvalidOperationException(
                "Modbus:Device:Port must be between 1 and 65535.");
        }

        if (Device.UnitId is < 0 or > byte.MaxValue)
        {
            throw new InvalidOperationException(
                "Modbus:Device:UnitId must be between 0 and 255.");
        }

        if (Device.PollIntervalMilliseconds <= 0)
        {
            throw new InvalidOperationException(
                "Modbus:Device:PollIntervalMilliseconds must be greater than zero.");
        }

        if (Device.RequestTimeoutMilliseconds <= 0)
        {
            throw new InvalidOperationException(
                "Modbus:Device:RequestTimeoutMilliseconds must be greater than zero.");
        }

        if (Device.Points is null || Device.Points.Count == 0)
        {
            throw new InvalidOperationException(
                "Modbus:Device:Points must contain at least one point.");
        }

        var points = Device.Points
            .Select((point, index) => CreatePoint(point, index))
            .ToArray();

        return new ModbusPollingPlan(
            Device: new ModbusTcpDevice(
                Device.DeviceId,
                Device.Host,
                Device.Port,
                (byte)Device.UnitId),
            Points: points,
            PollInterval: TimeSpan.FromMilliseconds(
                Device.PollIntervalMilliseconds),
            RequestTimeout: TimeSpan.FromMilliseconds(
                Device.RequestTimeoutMilliseconds));
    }

    private static ModbusHoldingRegisterPoint CreatePoint(
        ModbusRuntimePointOptions point,
        int index)
    {
        ArgumentNullException.ThrowIfNull(point);

        ValidateRequired(
            point.TagId,
            $"Modbus:Device:Points:{index}:TagId");

        if (point.Address is < 0 or > ushort.MaxValue)
        {
            throw new InvalidOperationException(
                $"Modbus:Device:Points:{index}:Address must be between 0 and 65535.");
        }

        return new ModbusHoldingRegisterPoint(
            point.TagId,
            (ushort)point.Address);
    }

    private static void ValidateRequired(
        string? value,
        string path)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            throw new InvalidOperationException(
                $"{path} is required.");
        }
    }
}

public sealed class ModbusRuntimeDeviceOptions
{
    public string DeviceId { get; set; } = string.Empty;

    public string Host { get; set; } = string.Empty;

    public int Port { get; set; } = 502;

    public int UnitId { get; set; } = 1;

    public int PollIntervalMilliseconds { get; set; } = 1000;

    public int RequestTimeoutMilliseconds { get; set; } = 1000;

    public List<ModbusRuntimePointOptions> Points { get; set; } = [];
}

public sealed class ModbusRuntimePointOptions
{
    public string TagId { get; set; } = string.Empty;

    public int Address { get; set; }
}
