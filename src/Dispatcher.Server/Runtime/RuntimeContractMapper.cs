using Dispatcher.Contracts.Devices;
using Dispatcher.Contracts.Tags;
using Dispatcher.Core.Devices;
using Dispatcher.Core.Tags;

namespace Dispatcher.Server.Runtime;

internal static class RuntimeContractMapper
{
    public static TagValueDto ToDto(TagValue tag)
    {
        return new TagValueDto(
            tag.TagId,
            tag.Value,
            tag.Timestamp);
    }

    public static DeviceStateDto ToDto(DeviceRuntimeState state)
    {
        return new DeviceStateDto(
            state.DeviceId,
            ToDto(state.Status),
            state.UpdatedAt,
            state.LastSuccessfulPollAt,
            state.Error);
    }

    private static DeviceConnectionStatusDto ToDto(
        DeviceConnectionStatus status)
    {
        return status switch
        {
            DeviceConnectionStatus.Unknown =>
                DeviceConnectionStatusDto.Unknown,
            DeviceConnectionStatus.Online =>
                DeviceConnectionStatusDto.Online,
            DeviceConnectionStatus.Offline =>
                DeviceConnectionStatusDto.Offline,
            _ => throw new ArgumentOutOfRangeException(
                nameof(status),
                status,
                "Unknown device connection status.")
        };
    }
}
