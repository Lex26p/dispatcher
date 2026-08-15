namespace Dispatcher.Server.Historian;

public sealed class HistorianPolicyCatalog
{
    private Snapshot _snapshot =
        Snapshot.Empty;

    public IReadOnlyList<HistorianPolicyConfiguration> Policies =>
        Volatile.Read(
            ref _snapshot)
            .Policies;

    public HistorianPolicyConfiguration? Find(
        string tagId)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            tagId);

        var snapshot =
            Volatile.Read(
                ref _snapshot);

        return snapshot.ByTagId.TryGetValue(
            tagId,
            out var policy)
            ? policy
            : null;
    }

    public bool Contains(
        string tagId)
    {
        return Find(
            tagId) is not null;
    }

    public void ReplaceAll(
        IReadOnlyCollection<HistorianPolicyConfiguration> policies)
    {
        HistorianPolicyValidator.Validate(
            policies);

        var copied =
            policies
                .OrderBy(
                    policy => policy.TagId,
                    StringComparer.Ordinal)
                .ToArray();

        var byTagId =
            copied.ToDictionary(
                policy => policy.TagId,
                StringComparer.Ordinal);

        Volatile.Write(
            ref _snapshot,
            new Snapshot(
                copied,
                byTagId));
    }

    private sealed record Snapshot(
        IReadOnlyList<HistorianPolicyConfiguration> Policies,
        IReadOnlyDictionary<string, HistorianPolicyConfiguration> ByTagId)
    {
        public static Snapshot Empty { get; } =
            new(
                Array.Empty<HistorianPolicyConfiguration>(),
                new Dictionary<string, HistorianPolicyConfiguration>(
                    StringComparer.Ordinal));
    }
}
