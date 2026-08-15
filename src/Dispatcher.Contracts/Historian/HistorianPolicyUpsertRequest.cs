namespace Dispatcher.Contracts.Historian;

public sealed record HistorianPolicyUpsertRequest(
    bool Enabled,
    HistorianSamplingModeDto Mode,
    int? PeriodMilliseconds,
    int RetentionDays);
