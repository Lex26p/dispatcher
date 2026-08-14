using Dispatcher.Snmp.Configuration;

namespace Dispatcher.Server.Configuration;

public static class SnmpConfigurationValidator
{
    public static void Validate(
        IReadOnlyCollection<SnmpDeviceConfiguration> devices)
    {
        ArgumentNullException.ThrowIfNull(devices);

        var deviceIds = new HashSet<string>(StringComparer.Ordinal);
        var tagIds = new HashSet<string>(StringComparer.Ordinal);

        foreach (var device in devices)
        {
            ArgumentNullException.ThrowIfNull(device);
            ValidateRequired(device.DeviceId, "DeviceId");
            ValidateRequired(device.Name, $"Device '{device.DeviceId}' Name");
            ValidateRequired(device.Host, $"Device '{device.DeviceId}' Host");
            ValidateRequired(device.Community, $"Device '{device.DeviceId}' Community");

            if (!deviceIds.Add(device.DeviceId))
            {
                throw new InvalidOperationException(
                    $"Duplicate DeviceId '{device.DeviceId}'.");
            }

            if (device.Port is < 1 or > 65535)
            {
                throw new InvalidOperationException(
                    $"Device '{device.DeviceId}' Port must be between 1 and 65535.");
            }

            if (device.PollIntervalMilliseconds <= 0)
            {
                throw new InvalidOperationException(
                    $"Device '{device.DeviceId}' PollIntervalMilliseconds must be greater than zero.");
            }

            if (device.RequestTimeoutMilliseconds <= 0)
            {
                throw new InvalidOperationException(
                    $"Device '{device.DeviceId}' RequestTimeoutMilliseconds must be greater than zero.");
            }

            ArgumentNullException.ThrowIfNull(device.Tags);

            foreach (var tag in device.Tags)
            {
                ArgumentNullException.ThrowIfNull(tag);
                ValidateRequired(tag.TagId, $"Device '{device.DeviceId}' TagId");
                ValidateRequired(tag.Name, $"Tag '{tag.TagId}' Name");
                ValidateRequired(tag.Oid, $"Tag '{tag.TagId}' Oid");

                if (!tagIds.Add(tag.TagId))
                {
                    throw new InvalidOperationException(
                        $"Duplicate TagId '{tag.TagId}'.");
                }

                try
                {
                    SnmpOidValidator.Validate(tag.Oid);
                }
                catch (Exception exception)
                    when (exception is ArgumentException or FormatException or OverflowException)
                {
                    throw new InvalidOperationException(
                        $"Tag '{tag.TagId}' contains invalid OID '{tag.Oid}'.",
                        exception);
                }
            }
        }
    }

    private static void ValidateRequired(string? value, string path)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            throw new InvalidOperationException(
                $"{path} is required.");
        }
    }
}
