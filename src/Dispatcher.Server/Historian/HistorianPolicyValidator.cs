namespace Dispatcher.Server.Historian;

public static class HistorianPolicyValidator
{
    public const int MinPeriodicIntervalMilliseconds = 100;
    public const int MaxPeriodicIntervalMilliseconds = 86_400_000;
    public const int MinRetentionDays = 1;
    public const int MaxRetentionDays = 36_500;

    public static void Validate(
        HistorianPolicyConfiguration policy)
    {
        ArgumentNullException.ThrowIfNull(
            policy);
        ArgumentException.ThrowIfNullOrWhiteSpace(
            policy.TagId);

        if (policy.RetentionDays is
            < MinRetentionDays
            or > MaxRetentionDays)
        {
            throw new InvalidOperationException(
                $"Historian policy '{policy.TagId}' RetentionDays must be between " +
                $"{MinRetentionDays} and {MaxRetentionDays}.");
        }

        switch (policy.Mode)
        {
            case HistorianSamplingMode.OnChange:
                if (policy.PeriodMilliseconds is not null)
                {
                    throw new InvalidOperationException(
                        $"Historian policy '{policy.TagId}' must not define PeriodMilliseconds in OnChange mode.");
                }

                break;

            case HistorianSamplingMode.Periodic:
                if (policy.PeriodMilliseconds is null
                    || policy.PeriodMilliseconds.Value < MinPeriodicIntervalMilliseconds
                    || policy.PeriodMilliseconds.Value > MaxPeriodicIntervalMilliseconds)
                {
                    throw new InvalidOperationException(
                        $"Historian policy '{policy.TagId}' PeriodMilliseconds must be between " +
                        $"{MinPeriodicIntervalMilliseconds} and {MaxPeriodicIntervalMilliseconds} in Periodic mode.");
                }

                break;

            default:
                throw new InvalidOperationException(
                    $"Historian policy '{policy.TagId}' has unsupported mode '{policy.Mode}'.");
        }
    }

    public static void Validate(
        IReadOnlyCollection<HistorianPolicyConfiguration> policies)
    {
        ArgumentNullException.ThrowIfNull(
            policies);

        var tagIds =
            new HashSet<string>(
                StringComparer.Ordinal);

        foreach (var policy in policies)
        {
            Validate(
                policy);

            if (!tagIds.Add(
                    policy.TagId))
            {
                throw new InvalidOperationException(
                    $"Duplicate historian policy TagId '{policy.TagId}'.");
            }
        }
    }
}
