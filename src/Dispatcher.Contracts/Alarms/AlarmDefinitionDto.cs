namespace Dispatcher.Contracts.Alarms;

public sealed record AlarmDefinitionDto(
    string AlarmId,
    string Name,
    bool Enabled,
    string TagId,
    AlarmConditionDto Condition,
    decimal? Threshold,
    AlarmSeverityDto Severity,
    string Message,
    int DelayMilliseconds,
    decimal? Hysteresis);
