namespace Dispatcher.Server.Templates;

public sealed class TemplateConflictException : Exception
{
    public TemplateConflictException(string message)
        : base(message)
    {
    }
}
