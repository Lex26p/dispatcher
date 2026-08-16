using Dispatcher.Contracts.Alarms;
using Dispatcher.Contracts.Tags;

namespace Dispatcher.Server.Alarms;

internal static class AlarmRuntimeContractMapper
{
    public static AlarmRuntimeSnapshotDto ToDto(
        AlarmRuntimeSnapshot snapshot)
    {
        ArgumentNullException.ThrowIfNull(
            snapshot);

        return new AlarmRuntimeSnapshotDto(
            snapshot.AlarmId,
            snapshot.Name,
            snapshot.TagId,
            ToDto(
                snapshot.Severity),
            snapshot.Message,
            ToDto(
                snapshot.State),
            snapshot.RaisedAt,
            snapshot.AcknowledgedByUserId,
            snapshot.AcknowledgedByUserName,
            snapshot.AcknowledgedAt,
            snapshot.LastTransitionTimestamp,
            snapshot.CurrentValue is null
                ? null
                : new TagValueDto(
                    snapshot.CurrentValue.TagId,
                    snapshot.CurrentValue.Value,
                    snapshot.CurrentValue.Timestamp,
                    Writable:
                        false));
    }

    private static AlarmSeverityDto ToDto(
        AlarmSeverity severity)
    {
        return severity switch
        {
            AlarmSeverity.Information =>
                AlarmSeverityDto.Information,
            AlarmSeverity.Warning =>
                AlarmSeverityDto.Warning,
            AlarmSeverity.Error =>
                AlarmSeverityDto.Error,
            _ =>
                throw new ArgumentOutOfRangeException(
                    nameof(severity),
                    severity,
                    null)
        };
    }

    private static AlarmRuntimeStateDto ToDto(
        AlarmRuntimeState state)
    {
        return state switch
        {
            AlarmRuntimeState.Normal =>
                AlarmRuntimeStateDto.Normal,
            AlarmRuntimeState.ActiveUnacknowledged =>
                AlarmRuntimeStateDto.ActiveUnacknowledged,
            AlarmRuntimeState.ActiveAcknowledged =>
                AlarmRuntimeStateDto.ActiveAcknowledged,
            AlarmRuntimeState.ReturnedUnacknowledged =>
                AlarmRuntimeStateDto.ReturnedUnacknowledged,
            _ =>
                throw new ArgumentOutOfRangeException(
                    nameof(state),
                    state,
                    null)
        };
    }
}
