using System.Net;
using System.Net.Sockets;
using Dispatcher.Snmp.Configuration;
using Lextm.SharpSnmpLib;
using Lextm.SharpSnmpLib.Messaging;

namespace Dispatcher.Snmp;

public sealed class SnmpGetClient
{
    public async Task<IReadOnlyDictionary<string, object?>> ReadAsync(
        SnmpV2cDevice device,
        IReadOnlyList<SnmpPoint> points,
        TimeSpan requestTimeout,
        CancellationToken cancellationToken = default)
    {
        ValidateDevice(device);
        ValidatePoints(points);

        if (requestTimeout <= TimeSpan.Zero)
        {
            throw new ArgumentOutOfRangeException(
                nameof(requestTimeout),
                requestTimeout,
                "Request timeout must be greater than zero.");
        }

        using var timeout = CancellationTokenSource.CreateLinkedTokenSource(
            cancellationToken);
        timeout.CancelAfter(requestTimeout);

        try
        {
            var addresses = await Dns.GetHostAddressesAsync(
                device.Host,
                timeout.Token);

            var address = addresses.FirstOrDefault(candidate =>
                    candidate.AddressFamily == AddressFamily.InterNetwork)
                ?? addresses.FirstOrDefault();

            if (address is null)
            {
                throw new InvalidOperationException(
                    $"Host '{device.Host}' did not resolve to an IP address.");
            }

            var requestVariables = points
                .Select(point => new Variable(
                    new ObjectIdentifier(point.Oid)))
                .ToArray();

            var responseVariables = await Messenger.GetAsync(
                VersionCode.V2,
                new IPEndPoint(address, device.Port),
                new OctetString(device.Community),
                requestVariables,
                timeout.Token);

            if (responseVariables.Count != requestVariables.Length)
            {
                throw new InvalidOperationException(
                    $"SNMP response returned {responseVariables.Count} varbind(s) for {requestVariables.Length} requested OID(s).");
            }

            var result = new Dictionary<string, object?>(
                StringComparer.Ordinal);

            for (var index = 0; index < points.Count; index++)
            {
                var response = responseVariables[index];
                var expected = requestVariables[index];

                if (!response.Id.Equals(expected.Id))
                {
                    throw new InvalidOperationException(
                        $"SNMP response OID '{response.Id}' does not match requested OID '{expected.Id}'.");
                }

                result.Add(
                    points[index].TagId,
                    SnmpValueConverter.ToRuntimeValue(response.Data));
            }

            return result;
        }
        catch (OperationCanceledException)
            when (!cancellationToken.IsCancellationRequested)
        {
            throw new System.TimeoutException(
                $"Timed out reading SNMP v2c device '{device.DeviceId}'.");
        }
    }

    private static void ValidateDevice(SnmpV2cDevice device)
    {
        ArgumentNullException.ThrowIfNull(device);
        ArgumentException.ThrowIfNullOrWhiteSpace(device.DeviceId);
        ArgumentException.ThrowIfNullOrWhiteSpace(device.Host);
        ArgumentException.ThrowIfNullOrWhiteSpace(device.Community);

        if (device.Port is < 1 or > 65535)
        {
            throw new ArgumentOutOfRangeException(
                nameof(device),
                device.Port,
                "SNMP UDP port must be between 1 and 65535.");
        }
    }

    private static void ValidatePoints(IReadOnlyList<SnmpPoint> points)
    {
        ArgumentNullException.ThrowIfNull(points);

        if (points.Count == 0)
        {
            throw new ArgumentException(
                "At least one SNMP point is required.",
                nameof(points));
        }

        foreach (var point in points)
        {
            ArgumentNullException.ThrowIfNull(point);
            ArgumentException.ThrowIfNullOrWhiteSpace(point.TagId);
            SnmpOidValidator.Validate(point.Oid);
        }
    }
}
