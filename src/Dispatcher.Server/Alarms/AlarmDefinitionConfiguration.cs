namespace Dispatcher.Server.Alarms;

public sealed record AlarmDefinitionConfiguration(
    string AlarmId,
    string Name,
    bool Enabled,
    string TagId,
    AlarmCondition Condition,
    decimal? Threshold,
    AlarmSeverity Severity,
    string Message,
    int DelayMilliseconds,
    decimal? Hysteresis);
