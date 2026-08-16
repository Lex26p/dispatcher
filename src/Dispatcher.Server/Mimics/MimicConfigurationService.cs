using Dispatcher.Server.Configuration;

namespace Dispatcher.Server.Mimics;

public sealed class MimicConfigurationService
{
    private readonly SqliteConfigurationStore _store;
    private readonly SemaphoreSlim _mutationLock = new(1, 1);

    public MimicConfigurationService(
        SqliteConfigurationStore store)
    {
        _store = store;
    }

    public async Task<IReadOnlyList<MimicConfiguration>> GetAllAsync(
        CancellationToken cancellationToken)
    {
        return await _store.LoadMimicsAsync(
            cancellationToken);
    }

    public async Task<MimicConfiguration?> GetAsync(
        string mimicId,
        CancellationToken cancellationToken)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            mimicId);

        var mimics =
            await _store.LoadMimicsAsync(
                cancellationToken);

        return mimics.FirstOrDefault(mimic =>
            string.Equals(
                mimic.MimicId,
                mimicId,
                StringComparison.Ordinal));
    }

    public async Task<MimicConfiguration> UpsertAsync(
        string mimicId,
        MimicConfiguration mimic,
        CancellationToken cancellationToken)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            mimicId);
        ArgumentNullException.ThrowIfNull(
            mimic);

        if (!string.Equals(
                mimicId,
                mimic.MimicId,
                StringComparison.Ordinal))
        {
            throw new InvalidOperationException(
                "MimicId in URL must match MimicId in request body.");
        }

        MimicConfigurationValidator.Validate(
            mimic);

        await _mutationLock.WaitAsync(
            cancellationToken);

        try
        {
            await _store.UpsertMimicAsync(
                mimic,
                cancellationToken);

            return mimic;
        }
        finally
        {
            _mutationLock.Release();
        }
    }

    public async Task<MimicConfiguration?> AppendElementsAsync(
        string mimicId,
        IReadOnlyList<MimicElementConfiguration> elements,
        CancellationToken cancellationToken)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(
            mimicId);
        ArgumentNullException.ThrowIfNull(
            elements);

        await _mutationLock.WaitAsync(
            cancellationToken);

        try
        {
            var mimics =
                await _store.LoadMimicsAsync(
                    cancellationToken);
            var mimic =
                mimics.FirstOrDefault(candidate =>
                    string.Equals(
                        candidate.MimicId,
                        mimicId,
                        StringComparison.Ordinal));

            if (mimic is null)
            {
                return null;
            }

            var updated =
                mimic with
                {
                    Elements =
                        mimic.Elements
                            .Concat(
                                elements)
                            .ToArray()
                };

            MimicConfigurationValidator.Validate(
                updated);

            await _store.UpsertMimicAsync(
                updated,
                cancellationToken);

            return updated;
        }
        finally
        {
            _mutationLock.Release();
        }
    }

    public async Task<bool> DeleteAsync(
        string mimicId,
        CancellationToken cancellationToken)
    {
        await _mutationLock.WaitAsync(
            cancellationToken);

        try
        {
            return await _store.DeleteMimicAsync(
                mimicId,
                cancellationToken);
        }
        finally
        {
            _mutationLock.Release();
        }
    }
}
