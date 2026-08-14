using Dispatcher.Snmp.Configuration;

namespace Dispatcher.Server.Configuration;

public static class SnmpConfigurationMapper
{
    public static SnmpPollingPlan? CreatePollingPlan(
        SnmpDeviceConfiguration device)
    {
        ArgumentNullException.ThrowIfNull(device);

        if (!device.Enabled || device.Tags.Count == 0)
        {
            return null;
        }

        return new SnmpPollingPlan(
            new SnmpV2cDevice(
                device.DeviceId,
                device.Host,
                device.Port,
                device.Community),
            device.Tags
                .Select(tag => new SnmpPoint(
                    tag.TagId,
                    tag.Oid))
                .ToArray(),
            TimeSpan.FromMilliseconds(
                device.PollIntervalMilliseconds),
            TimeSpan.FromMilliseconds(
                device.RequestTimeoutMilliseconds));
    }
}
