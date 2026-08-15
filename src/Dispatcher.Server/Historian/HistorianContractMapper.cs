using Dispatcher.Contracts.Historian;
using Dispatcher.Server.Configuration;

namespace Dispatcher.Server.Historian;

internal static class HistorianContractMapper
{
    public static HistorianPolicyDto ToDto(
        HistorianPolicyConfiguration policy,
        ConfigurationCatalog configuration)
    {
        return new HistorianPolicyDto(
            policy.TagId,
            policy.Enabled,
            MapMode(
                policy.Mode),
            policy.PeriodMilliseconds,
            policy.RetentionDays,
            configuration.ContainsTagId(
                policy.TagId));
    }

    public static HistorianPolicyConfiguration ToConfiguration(
        string tagId,
        HistorianPolicyUpsertRequest request)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            tagId);
        ArgumentNullException.ThrowIfNull(
            request);

        return new HistorianPolicyConfiguration(
            tagId,
            request.Enabled,
            MapMode(
                request.Mode),
            request.PeriodMilliseconds,
            request.RetentionDays);
    }

    private static HistorianSamplingModeDto MapMode(
        HistorianSamplingMode mode)
    {
        return mode switch
        {
            HistorianSamplingMode.OnChange =>
                HistorianSamplingModeDto.OnChange,
            HistorianSamplingMode.Periodic =>
                HistorianSamplingModeDto.Periodic,
            _ =>
                throw new ArgumentOutOfRangeException(
                    nameof(mode),
                    mode,
                    null)
        };
    }

    private static HistorianSamplingMode MapMode(
        HistorianSamplingModeDto mode)
    {
        return mode switch
        {
            HistorianSamplingModeDto.OnChange =>
                HistorianSamplingMode.OnChange,
            HistorianSamplingModeDto.Periodic =>
                HistorianSamplingMode.Periodic,
            _ =>
                throw new ArgumentOutOfRangeException(
                    nameof(mode),
                    mode,
                    null)
        };
    }
}
