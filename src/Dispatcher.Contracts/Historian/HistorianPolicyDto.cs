namespace Dispatcher.Contracts.Historian;

public sealed record HistorianPolicyDto(
    string TagId,
    bool Enabled,
    HistorianSamplingModeDto Mode,
    int? PeriodMilliseconds,
    int RetentionDays,
    bool TagExists);
