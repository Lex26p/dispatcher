using Dispatcher.Contracts.Alarms;

namespace Dispatcher.Server.Alarms;

public static class AlarmDefinitionContractMapper
{
    public static AlarmDefinitionConfiguration ToConfiguration(
        CreateAlarmDefinitionRequest request)
    {
        ArgumentNullException.ThrowIfNull(
            request);

        var definition =
            new AlarmDefinitionConfiguration(
                AlarmId:
                    request.AlarmId,
                Name:
                    request.Name,
                Enabled:
                    request.Enabled,
                TagId:
                    request.TagId,
                Condition:
                    ToInternal(
                        request.Condition),
                Threshold:
                    request.Threshold,
                Severity:
                    ToInternal(
                        request.Severity),
                Message:
                    request.Message,
                DelayMilliseconds:
                    request.DelayMilliseconds,
                Hysteresis:
                    request.Hysteresis);

        AlarmDefinitionValidator.Validate(
            definition);

        return definition;
    }

    public static AlarmDefinitionConfiguration ToConfiguration(
        string alarmId,
        UpdateAlarmDefinitionRequest request)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            alarmId);
        ArgumentNullException.ThrowIfNull(
            request);

        var definition =
            new AlarmDefinitionConfiguration(
                AlarmId:
                    alarmId,
                Name:
                    request.Name,
                Enabled:
                    request.Enabled,
                TagId:
                    request.TagId,
                Condition:
                    ToInternal(
                        request.Condition),
                Threshold:
                    request.Threshold,
                Severity:
                    ToInternal(
                        request.Severity),
                Message:
                    request.Message,
                DelayMilliseconds:
                    request.DelayMilliseconds,
                Hysteresis:
                    request.Hysteresis);

        AlarmDefinitionValidator.Validate(
            definition);

        return definition;
    }

    public static AlarmDefinitionDto ToDto(
        AlarmDefinitionConfiguration definition)
    {
        ArgumentNullException.ThrowIfNull(
            definition);

        return new AlarmDefinitionDto(
            definition.AlarmId,
            definition.Name,
            definition.Enabled,
            definition.TagId,
            ToDto(
                definition.Condition),
            definition.Threshold,
            ToDto(
                definition.Severity),
            definition.Message,
            definition.DelayMilliseconds,
            definition.Hysteresis);
    }

    private static AlarmCondition ToInternal(
        AlarmConditionDto condition)
    {
        return condition switch
        {
            AlarmConditionDto.DigitalTrue =>
                AlarmCondition.DigitalTrue,
            AlarmConditionDto.DigitalFalse =>
                AlarmCondition.DigitalFalse,
            AlarmConditionDto.High =>
                AlarmCondition.High,
            AlarmConditionDto.Low =>
                AlarmCondition.Low,
            _ =>
                throw new ArgumentOutOfRangeException(
                    nameof(condition),
                    condition,
                    null)
        };
    }

    private static AlarmConditionDto ToDto(
        AlarmCondition condition)
    {
        return condition switch
        {
            AlarmCondition.DigitalTrue =>
                AlarmConditionDto.DigitalTrue,
            AlarmCondition.DigitalFalse =>
                AlarmConditionDto.DigitalFalse,
            AlarmCondition.High =>
                AlarmConditionDto.High,
            AlarmCondition.Low =>
                AlarmConditionDto.Low,
            _ =>
                throw new ArgumentOutOfRangeException(
                    nameof(condition),
                    condition,
                    null)
        };
    }

    private static AlarmSeverity ToInternal(
        AlarmSeverityDto severity)
    {
        return severity switch
        {
            AlarmSeverityDto.Information =>
                AlarmSeverity.Information,
            AlarmSeverityDto.Warning =>
                AlarmSeverity.Warning,
            AlarmSeverityDto.Error =>
                AlarmSeverity.Error,
            _ =>
                throw new ArgumentOutOfRangeException(
                    nameof(severity),
                    severity,
                    null)
        };
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
}
