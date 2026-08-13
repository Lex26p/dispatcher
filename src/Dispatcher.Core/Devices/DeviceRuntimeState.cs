namespace Dispatcher.Core.Devices;

public sealed record DeviceRuntimeState(
    string DeviceId,
    DeviceConnectionStatus Status,
    DateTimeOffset UpdatedAt,
    DateTimeOffset? LastSuccessfulPollAt,
    string? Error);
