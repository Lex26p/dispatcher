namespace Dispatcher.Server.Alarms;

public sealed class AlarmDefinitionCatalog
{
    private IReadOnlyList<AlarmDefinitionConfiguration> _definitions =
        Array.Empty<AlarmDefinitionConfiguration>();

    public IReadOnlyList<AlarmDefinitionConfiguration> Definitions =>
        Volatile.Read(
            ref _definitions);

    public event Action? Changed;

    public void ReplaceAll(
        IReadOnlyCollection<AlarmDefinitionConfiguration> definitions)
    {
        AlarmDefinitionValidator.Validate(
            definitions);

        Volatile.Write(
            ref _definitions,
            definitions
                .OrderBy(
                    definition => definition.AlarmId,
                    StringComparer.Ordinal)
                .ToArray());

        Changed?.Invoke();
    }
}
