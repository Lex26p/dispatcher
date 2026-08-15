using Dispatcher.Contracts.Historian;
using Dispatcher.Server.Configuration;

namespace Dispatcher.Server.Historian;

public sealed class HistorianPolicyService
{
    private readonly SqliteConfigurationStore _store;
    private readonly ConfigurationCatalog _configuration;
    private readonly HistorianPolicyCatalog _catalog;
    private readonly SemaphoreSlim _mutationLock =
        new(1, 1);

    public HistorianPolicyService(
        SqliteConfigurationStore store,
        ConfigurationCatalog configuration,
        HistorianPolicyCatalog catalog)
    {
        _store =
            store;
        _configuration =
            configuration;
        _catalog =
            catalog;
    }

    public IReadOnlyList<HistorianPolicyConfiguration> GetAll()
    {
        return _catalog.Policies;
    }

    public async Task<HistorianPolicyConfiguration> UpsertAsync(
        string tagId,
        HistorianPolicyUpsertRequest request,
        CancellationToken cancellationToken)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            tagId);
        ArgumentNullException.ThrowIfNull(
            request);

        var policy =
            HistorianContractMapper.ToConfiguration(
                tagId,
                request);

        HistorianPolicyValidator.Validate(
            policy);

        await _mutationLock.WaitAsync(
            cancellationToken);

        try
        {
            var existing =
                _catalog.Find(
                    tagId);

            if (existing is null
                && !_configuration.ContainsTagId(
                    tagId))
            {
                throw new KeyNotFoundException(
                    $"Тег '{tagId}' отсутствует в текущей конфигурации.");
            }

            await _store.UpsertHistorianPolicyAsync(
                policy,
                cancellationToken);

            var updated =
                _catalog.Policies
                    .Where(current =>
                        !string.Equals(
                            current.TagId,
                            tagId,
                            StringComparison.Ordinal))
                    .Append(
                        policy)
                    .ToArray();

            _catalog.ReplaceAll(
                updated);

            return policy;
        }
        finally
        {
            _mutationLock.Release();
        }
    }

    public async Task DeleteAsync(
        string tagId,
        CancellationToken cancellationToken)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            tagId);

        await _mutationLock.WaitAsync(
            cancellationToken);

        try
        {
            if (!_catalog.Contains(
                    tagId))
            {
                throw new KeyNotFoundException(
                    $"Historian policy for tag '{tagId}' was not found.");
            }

            var deleted =
                await _store.DeleteHistorianPolicyAsync(
                    tagId,
                    cancellationToken);

            if (!deleted)
            {
                throw new KeyNotFoundException(
                    $"Historian policy for tag '{tagId}' was not found.");
            }

            _catalog.ReplaceAll(
                _catalog.Policies
                    .Where(policy =>
                        !string.Equals(
                            policy.TagId,
                            tagId,
                            StringComparison.Ordinal))
                    .ToArray());
        }
        finally
        {
            _mutationLock.Release();
        }
    }
}
