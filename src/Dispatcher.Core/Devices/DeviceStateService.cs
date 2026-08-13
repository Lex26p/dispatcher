using System.Collections.Concurrent;

namespace Dispatcher.Core.Devices;

public sealed class DeviceStateService
{
    private readonly ConcurrentDictionary<string, DeviceRuntimeState> _states =
        new(StringComparer.Ordinal);

    public event Action<DeviceRuntimeState>? Changed;

    public DeviceRuntimeState SetOnline(
        string deviceId,
        DateTimeOffset timestamp)
    {
        ValidateDeviceId(deviceId);

        var state = new DeviceRuntimeState(
            deviceId,
            DeviceConnectionStatus.Online,
            timestamp,
            timestamp,
            Error: null);

        _states[deviceId] = state;
        Changed?.Invoke(state);

        return state;
    }

    public DeviceRuntimeState SetOffline(
        string deviceId,
        string? error,
        DateTimeOffset timestamp)
    {
        ValidateDeviceId(deviceId);

        var state = _states.AddOrUpdate(
            deviceId,
            _ => new DeviceRuntimeState(
                deviceId,
                DeviceConnectionStatus.Offline,
                timestamp,
                LastSuccessfulPollAt: null,
                Error: error),
            (_, current) => new DeviceRuntimeState(
                deviceId,
                DeviceConnectionStatus.Offline,
                timestamp,
                current.LastSuccessfulPollAt,
                error));

        Changed?.Invoke(state);

        return state;
    }

    public DeviceRuntimeState? Get(string deviceId)
    {
        ValidateDeviceId(deviceId);

        return _states.TryGetValue(deviceId, out var state)
            ? state
            : null;
    }

    public IReadOnlyList<DeviceRuntimeState> GetAll()
    {
        return _states.Values
            .OrderBy(state => state.DeviceId, StringComparer.Ordinal)
            .ToArray();
    }

    private static void ValidateDeviceId(string deviceId)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(deviceId);
    }
}
