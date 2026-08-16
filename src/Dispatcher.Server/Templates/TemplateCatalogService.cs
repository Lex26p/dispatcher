using Dispatcher.Server.Configuration;

namespace Dispatcher.Server.Templates;

public sealed class TemplateCatalogService
{
    private readonly SqliteConfigurationStore _store;

    public TemplateCatalogService(
        SqliteConfigurationStore store)
    {
        _store = store;
    }

    public Task<IReadOnlyList<TemplateCatalogEntryConfiguration>> GetAllAsync(
        CancellationToken cancellationToken)
    {
        return _store.LoadTemplateCatalogAsync(
            cancellationToken);
    }
}
