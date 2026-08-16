using Dispatcher.Contracts.Alarms;
using Dispatcher.Server.Configuration;

namespace Dispatcher.Server.Alarms;

public sealed class AlarmDefinitionService
{
    private readonly SqliteConfigurationStore _store;
    private readonly ConfigurationCatalog _configuration;
    private readonly SemaphoreSlim _mutationLock =
        new(1, 1);

    public AlarmDefinitionService(
        SqliteConfigurationStore store,
        ConfigurationCatalog configuration)
    {
        _store =
            store;
        _configuration =
            configuration;
    }

    public Task<IReadOnlyList<AlarmDefinitionConfiguration>> GetAllAsync(
        CancellationToken cancellationToken = default)
    {
        return _store.LoadAlarmDefinitionsAsync(
            cancellationToken);
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
                await _store.LoadAlarmDefinitionsAsync(
                    cancellationToken);

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
