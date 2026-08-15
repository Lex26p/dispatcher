namespace Dispatcher.Server.Historian;

public sealed record HistorianPolicyConfiguration(
    string TagId,
    bool Enabled,
    HistorianSamplingMode Mode,
    int? PeriodMilliseconds,
    int RetentionDays);
