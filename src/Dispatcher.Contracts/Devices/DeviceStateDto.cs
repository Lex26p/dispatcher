namespace Dispatcher.Contracts.Devices;

public sealed record DeviceStateDto(
    string DeviceId,
    DeviceConnectionStatusDto Status,
    DateTimeOffset UpdatedAt,
    DateTimeOffset? LastSuccessfulPollAt,
    string? Error);
