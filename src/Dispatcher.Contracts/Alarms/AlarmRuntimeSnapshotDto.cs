using Dispatcher.Contracts.Tags;

namespace Dispatcher.Contracts.Alarms;

public sealed record AlarmRuntimeSnapshotDto(
    string AlarmId,
    string Name,
    string TagId,
    AlarmSeverityDto Severity,
    string Message,
    AlarmRuntimeStateDto State,
    DateTimeOffset? RaisedAt,
    string? AcknowledgedByUserId,
    string? AcknowledgedByUserName,
    DateTimeOffset? AcknowledgedAt,
    DateTimeOffset? LastTransitionTimestamp,
    TagValueDto? CurrentValue);
