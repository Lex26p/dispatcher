using Dispatcher.Contracts.Alarms;
using Dispatcher.Server.Configuration;

namespace Dispatcher.Server.Alarms;

public sealed class AlarmDefinitionService
{
    private readonly SqliteConfigurationStore _store;
    private readonly ConfigurationCatalog _configuration;
    private readonly AlarmDefinitionCatalog _catalog;
    private readonly SemaphoreSlim _mutationLock =
        new(1, 1);

    public AlarmDefinitionService(
        SqliteConfigurationStore store,
        ConfigurationCatalog configuration,
        AlarmDefinitionCatalog catalog)
    {
        _store =
            store;
        _configuration =
            configuration;
        _catalog =
            catalog;
    }

    public Task<IReadOnlyList<AlarmDefinitionConfiguration>> GetAllAsync(
        CancellationToken cancellationToken = default)
    {
        return Task.FromResult<IReadOnlyList<AlarmDefinitionConfiguration>>(
            _catalog.Definitions);
    }

    public async Task<AlarmDefinitionConfiguration> CreateAsync(
        CreateAlarmDefinitionRequest request,
        CancellationToken cancellationToken = default)
    {
        var definition =
            AlarmDefinitionContractMapper.ToConfiguration(
                request);

        await _mutationLock.WaitAsync(
            cancellationToken);

        try
        {
            EnsureTagExists(
                definition.TagId);

            var existing =
                _catalog.Definitions;

            if (existing.Any(candidate =>
                    string.Equals(
                        candidate.AlarmId,
                        definition.AlarmId,
                        StringComparison.Ordinal)))
            {
                throw new AlarmDefinitionConflictException(
                    $"Alarm '{definition.AlarmId}' already exists.");
            }

            await _store.InsertAlarmDefinitionAsync(
                definition,
                cancellationToken);

            _catalog.ReplaceAll(
                existing
                    .Append(
                        definition)
                    .ToArray());

            return definition;
        }
        finally
        {
            _mutationLock.Release();
        }
    }

    public async Task<AlarmDefinitionConfiguration> UpdateAsync(
        string alarmId,
        UpdateAlarmDefinitionRequest request,
        CancellationToken cancellationToken = default)
    {
        var definition =
            AlarmDefinitionContractMapper.ToConfiguration(
                alarmId,
                request);

        await _mutationLock.WaitAsync(
            cancellationToken);

        try
        {
            EnsureTagExists(
                definition.TagId);

            var updated =
                await _store.UpdateAlarmDefinitionAsync(
                    definition,
                    cancellationToken);

            if (!updated)
            {
                throw new AlarmDefinitionNotFoundException(
                    $"Alarm '{alarmId}' was not found.");
            }

            _catalog.ReplaceAll(
                _catalog.Definitions
                    .Where(candidate =>
                        !string.Equals(
                            candidate.AlarmId,
                            alarmId,
                            StringComparison.Ordinal))
                    .Append(
                        definition)
                    .ToArray());

            return definition;
        }
        finally
        {
            _mutationLock.Release();
        }
    }

    public async Task DeleteAsync(
        string alarmId,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            alarmId);

        await _mutationLock.WaitAsync(
            cancellationToken);

        try
        {
            if (!await _store.DeleteAlarmDefinitionAsync(
                    alarmId,
                    cancellationToken))
            {
                throw new AlarmDefinitionNotFoundException(
                    $"Alarm '{alarmId}' was not found.");
            }

            _catalog.ReplaceAll(
                _catalog.Definitions
                    .Where(definition =>
                        !string.Equals(
                            definition.AlarmId,
                            alarmId,
                            StringComparison.Ordinal))
                    .ToArray());
        }
        finally
        {
            _mutationLock.Release();
        }
    }

    private void EnsureTagExists(
        string tagId)
    {
        if (!_configuration.ContainsTagId(
                tagId))
        {
            throw new KeyNotFoundException(
                $"Alarm target tag '{tagId}' does not exist in current configuration.");
        }
    }
}
