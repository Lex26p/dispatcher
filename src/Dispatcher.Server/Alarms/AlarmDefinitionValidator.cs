namespace Dispatcher.Server.Alarms;

public static class AlarmDefinitionValidator
{
    public const int MaxAlarmIdLength = 200;
    public const int MaxNameLength = 200;
    public const int MaxTagIdLength = 200;
    public const int MaxMessageLength = 1000;

    public static void Validate(
        AlarmDefinitionConfiguration definition)
    {
        ArgumentNullException.ThrowIfNull(
            definition);

        ValidateRequiredText(
            definition.AlarmId,
            nameof(definition.AlarmId),
            MaxAlarmIdLength);
        ValidateRequiredText(
            definition.Name,
            nameof(definition.Name),
            MaxNameLength);
        ValidateRequiredText(
            definition.TagId,
            nameof(definition.TagId),
            MaxTagIdLength);
        ValidateRequiredText(
            definition.Message,
            nameof(definition.Message),
            MaxMessageLength);

        if (definition.DelayMilliseconds < 0)
        {
            throw new InvalidOperationException(
                $"Alarm '{definition.AlarmId}' DelayMilliseconds must be zero or greater.");
        }

        switch (definition.Severity)
        {
            case AlarmSeverity.Information:
            case AlarmSeverity.Warning:
            case AlarmSeverity.Error:
                break;

            default:
                throw new InvalidOperationException(
                    $"Alarm '{definition.AlarmId}' has unsupported severity '{definition.Severity}'.");
        }

        switch (definition.Condition)
        {
            case AlarmCondition.DigitalTrue:
            case AlarmCondition.DigitalFalse:
                if (definition.Threshold is not null
                    || definition.Hysteresis is not null)
                {
                    throw new InvalidOperationException(
                        $"Alarm '{definition.AlarmId}' must not define Threshold or Hysteresis for digital conditions.");
                }

                break;

            case AlarmCondition.High:
            case AlarmCondition.Low:
                if (definition.Threshold is null)
                {
                    throw new InvalidOperationException(
                        $"Alarm '{definition.AlarmId}' must define Threshold for {definition.Condition} condition.");
                }

                if (definition.Hysteresis is null
                    || definition.Hysteresis.Value < 0)
                {
                    throw new InvalidOperationException(
                        $"Alarm '{definition.AlarmId}' Hysteresis must be zero or greater for {definition.Condition} condition.");
                }

                break;

            default:
                throw new InvalidOperationException(
                    $"Alarm '{definition.AlarmId}' has unsupported condition '{definition.Condition}'.");
        }
    }

    public static void Validate(
        IReadOnlyCollection<AlarmDefinitionConfiguration> definitions)
    {
        ArgumentNullException.ThrowIfNull(
            definitions);

        var alarmIds =
            new HashSet<string>(
                StringComparer.Ordinal);

        foreach (var definition in definitions)
        {
            Validate(
                definition);

            if (!alarmIds.Add(
                    definition.AlarmId))
            {
                throw new InvalidOperationException(
                    $"Duplicate AlarmId '{definition.AlarmId}'.");
            }
        }
    }

    private static void ValidateRequiredText(
        string value,
        string name,
        int maxLength)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            value,
            name);

        if (!string.Equals(
                value,
                value.Trim(),
                StringComparison.Ordinal))
        {
            throw new InvalidOperationException(
                $"{name} must not contain leading or trailing whitespace.");
        }

        if (value.Length > maxLength)
        {
            throw new InvalidOperationException(
                $"{name} must not exceed {maxLength} characters.");
        }
    }
}
