namespace Dispatcher.Server.Templates;

public sealed class TemplateMutationGate
{
    public SemaphoreSlim Semaphore { get; } =
        new(1, 1);
}
