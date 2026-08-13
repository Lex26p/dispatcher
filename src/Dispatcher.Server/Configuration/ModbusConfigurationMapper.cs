using Dispatcher.Modbus.Configuration;

namespace Dispatcher.Server.Configuration;

public static class ModbusConfigurationMapper
{
    public static ModbusPollingPlan? CreatePollingPlan(
        ModbusDeviceConfiguration device)
    {
        ArgumentNullException.ThrowIfNull(device);

        if (!device.Enabled
            || device.Tags.Count == 0)
        {
            return null;
        }

        return new ModbusPollingPlan(
            Device: CreateDevice(device),
            Points: device.Tags
                .Select(tag =>
                    new ModbusHoldingRegisterPoint(
                        tag.TagId,
                        checked((ushort)tag.Address)))
                .ToArray(),
            PollInterval: TimeSpan.FromMilliseconds(
                device.PollIntervalMilliseconds),
            RequestTimeout: TimeSpan.FromMilliseconds(
                device.RequestTimeoutMilliseconds));
    }

    public static ModbusWriteTarget CreateWriteTarget(
        ModbusTagBinding binding)
    {
        ArgumentNullException.ThrowIfNull(binding);

        if (!binding.Device.Enabled)
        {
            throw new InvalidOperationException(
                $"Device '{binding.Device.DeviceId}' is disabled.");
        }

        if (!binding.Tag.Writable)
        {
            throw new InvalidOperationException(
                $"Tag '{binding.Tag.TagId}' is read-only.");
        }

        return new ModbusWriteTarget(
            CreateDevice(binding.Device),
            new ModbusHoldingRegisterPoint(
                binding.Tag.TagId,
                checked((ushort)binding.Tag.Address)),
            TimeSpan.FromMilliseconds(
                binding.Device.RequestTimeoutMilliseconds));
    }

    private static ModbusTcpDevice CreateDevice(
        ModbusDeviceConfiguration device)
    {
        return new ModbusTcpDevice(
            device.DeviceId,
            device.Host,
            device.Port,
            checked((byte)device.UnitId));
    }
}
