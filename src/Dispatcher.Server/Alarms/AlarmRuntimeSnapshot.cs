namespace Dispatcher.Server.Alarms;

public sealed record AlarmRuntimeSnapshot(
    string AlarmId,
    string Name,
    string TagId,
    AlarmSeverity Severity,
    string Message,
    AlarmRuntimeState State,
    DateTimeOffset? LastTransitionTimestamp);
